// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file  pipeline.cpp
 *
 * A C++17 wrapper for dpopen with a simple interface.
 *
 * @date  2025-11-14
 */

#include "pipeline.h"
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "dpopen.h"

namespace {

std::string wait_status_string(int status)
{
    if (WIFEXITED(status)) {
        return std::string("command exited with status ") +
               std::to_string(WEXITSTATUS(status));
    }
    if (WIFSIGNALED(status)) {
        return std::string("command terminated by signal ") +
               std::to_string(WTERMSIG(status));
    }
    return std::string("command stopped by signal ") +
           std::to_string(WSTOPSIG(status));
}

} // unnamed namespace

std::string pipeline(const char* command, std::string_view input)
{
    int fd = dpopen_raw(command);
    if (fd < 0) {
        throw std::system_error(errno, std::system_category(),
                                "dpopen_raw failed");
    }

    std::string output;
    std::thread reader{[fd, &output]() {
        char    buffer[4096];
        ssize_t n;
        while ((n = read(fd, buffer, sizeof buffer)) > 0) {
            output.append(buffer, n);
        }
    }};

    size_t total_written = 0;
    while (total_written < input.size()) {
#ifdef MSG_NOSIGNAL
        /* Prevent SIGPIPE on write to closed socket */
        ssize_t n = send(fd, input.data() + total_written,
                         input.size() - total_written, MSG_NOSIGNAL);
#else
        ssize_t n = write(fd, input.data() + total_written,
                          input.size() - total_written);
#endif
        if (n <= 0) {
            break;
        }
        total_written += n;
    }
    dphalfclose_raw(fd);

    reader.join();

    int status = dpclose_raw(fd);
    if (status < 0) {
        throw std::system_error(errno, std::system_category(),
                                "dpclose_raw failed");
    }
    if (status != 0) {
        throw std::runtime_error(wait_status_string(status));
    }

    return output;
}
