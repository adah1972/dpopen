// -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file  dpopen.c
 *
 * Implementation of a duplex pipe stream.
 *
 * @date  2025-11-14
 */

#include "dpopen.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef _REENTRANT
#include <pthread.h>
static pthread_mutex_t chain_mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

/** Struct to store duplex-pipe-specific information. */
struct dpipe_chain {
    int                fd;     ///< File descriptor of the duplex pipe
    pid_t              pid;    ///< Process ID of the command
    struct dpipe_chain *next;  ///< Pointer to the next one in chain
};

/** Typedef to make the struct easier to use. */
typedef struct dpipe_chain dpipe_t;

/** Header of the chain of opened duplex pipe streams. */
static dpipe_t *chain_hdr;

static void do_child(const char *command, int parent, int child)
{
    dpipe_t *chain;

    /* Close the other end */
    close(parent);

    /* Duplicate to stdin and stdout */
    if (child != STDIN_FILENO) {
        if (dup2(child, STDIN_FILENO) < 0) {
            _exit(126);
        }
    }
    if (child != STDOUT_FILENO) {
        if (dup2(child, STDOUT_FILENO) < 0) {
            _exit(126);
        }
    }

    /* Close this end too after it is duplicated to standard I/O */
    close(child);

    /* Close all previously opened pipe streams, as popen does */
    for (chain = chain_hdr; chain != NULL; chain = chain->next) {
        close(chain->fd);
    }

    /* Execute the command via sh */
    execl("/bin/sh", "sh", "-c", command, NULL);
}

/**
 * Initiates a duplex pipe from/to a process (raw fd version).
 *
 * Like \e popen, all previously opened pipe streams will be closed
 * in the child process.
 *
 * @param command  the command to execute on \c sh
 * @return         a file descriptor on successful completion; \c -1
 *                 otherwise
 */
int dpopen_raw(const char *command)
{
    int     fd[2];
    int     parent;
    int     child;
    pid_t   pid;
    dpipe_t *chain;

    /* Create a duplex pipe using the BSD socketpair call */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
        return -1;
    }
    parent = fd[0];
    child  = fd[1];

#ifdef SO_NOSIGPIPE
    /* Prevent SIGPIPE on write to closed socket */
    {
        int set = 1;
        setsockopt(parent, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
    }
#endif

    /* Fork the process and check whether it is successful */
    if ((pid = fork()) < 0) {
        close(parent);
        close(child);
        return -1;
    }

    if (pid == 0) {                         /* child */
        /* Do things in the child process; it should not return */
        do_child(command, parent, child);
        /* Exit the child process if execl fails */
        _exit(127);
    } else {                                /* parent */
        /* Close the other end */
        close(child);
        /* Allocate memory for the dpipe_t struct */
        chain = (dpipe_t *)malloc(sizeof(dpipe_t));
        if (chain == NULL) {
            close(parent);
            return -1;
        }
        /* Store necessary info for dpclose_raw, and adjust chain header */
        chain->fd  = parent;
        chain->pid = pid;
#ifdef _REENTRANT
        pthread_mutex_lock(&chain_mtx);
#endif
        chain->next = chain_hdr;
        chain_hdr   = chain;
#ifdef _REENTRANT
        pthread_mutex_unlock(&chain_mtx);
#endif
        /* Successfully return here */
        return parent;
    }
}

/**
 * Initiates a duplex pipe stream from/to a process.
 *
 * Like \e popen, all previously #dpopen'd pipe streams will be closed
 * in the child process.
 *
 * @param command  the command to execute on \c sh
 * @return         a pointer to an open stream on successful
 *                 completion; \c NULL otherwise
 */
FILE *dpopen(const char *command)
{
    int  fd;
    FILE *stream;

    fd = dpopen_raw(command);
    if (fd < 0) {
        return NULL;
    }
    stream = fdopen(fd, "r+");
    if (stream == NULL) {
        dpclose_raw(fd);
        return NULL;
    }
    return stream;
}

/**
 * Closes a duplex pipe from/to a process (raw fd version).
 *
 * @param fd  file descriptor returned from a previous #dpopen_raw call
 * @return    the wait status of the command if successful; \c -1 if an
 *            error occurs
 */
int dpclose_raw(int fd)
{
    int     status;
    pid_t   pid;
    pid_t   wait_res;
    dpipe_t *cur;
    dpipe_t **ptr;

    /* Search for the fd starting from chain header */
#ifdef _REENTRANT
    pthread_mutex_lock(&chain_mtx);
#endif
    ptr = &chain_hdr;
    while ((cur = *ptr) != NULL) {          /* Not end of chain */
        if (cur->fd == fd) {                /* fd found */
            pid = cur->pid;
            *ptr = cur->next;
#ifdef _REENTRANT
            pthread_mutex_unlock(&chain_mtx);
#endif
            free(cur);
            close(fd);
            do {
                wait_res = waitpid(pid, &status, 0);
            } while (wait_res == -1 && errno == EINTR);
            if (wait_res == -1) {
                return -1;
            }
            return status;
        }
        ptr = &cur->next;                   /* Check next */
    }
#ifdef _REENTRANT
    pthread_mutex_unlock(&chain_mtx);
#endif
    errno = EBADF;              /* If the given stream is not found */
    return -1;
}

/**
 * Closes a duplex pipe stream from/to a process.
 *
 * @param stream  pointer to a pipe stream returned from a previous
 *                #dpopen call
 * @return        the wait status of the command if successful; \c -1
 *                if an error occurs
 */
int dpclose(FILE *stream)
{
    int     fd;
    int     status;
    pid_t   pid;
    pid_t   wait_res;
    dpipe_t *cur;
    dpipe_t **ptr;

    fd = fileno(stream);

    /* Search for the fd starting from chain header */
#ifdef _REENTRANT
    pthread_mutex_lock(&chain_mtx);
#endif
    ptr = &chain_hdr;
    while ((cur = *ptr) != NULL) {          /* Not end of chain */
        if (cur->fd == fd) {                /* fd found */
            pid  = cur->pid;
            *ptr = cur->next;
#ifdef _REENTRANT
            pthread_mutex_unlock(&chain_mtx);
#endif
            free(cur);
            if (fclose(stream) != 0) {
                return -1;
            }
            do {
                wait_res = waitpid(pid, &status, 0);
            } while (wait_res == -1 && errno == EINTR);
            if (wait_res == -1) {
                return -1;
            }
            return status;
        }
        ptr = &cur->next;                   /* Check next */
    }
#ifdef _REENTRANT
    pthread_mutex_unlock(&chain_mtx);
#endif
    errno = EBADF;              /* If the given stream is not found */
    return -1;
}

/**
 * Sends \c EOF to the process at the other end of the duplex pipe (raw
 * fd version).
 *
 * @param fd  file descriptor returned from a previous #dpopen_raw call
 * @return    \c 0 if successful; \c -1 if an error occurs
 */
int dphalfclose_raw(int fd)
{
    return shutdown(fd, SHUT_WR);
}

/**
 * Flushes the buffer and sends \c EOF to the process at the other end
 * of the duplex pipe stream.
 *
 * @param stream  pointer to a pipe stream returned from a previous
 *                #dpopen call
 * @return        \c 0 if successful; \c -1 if an error occurs
 */
int dphalfclose(FILE *stream)
{
    /* Ensure all data are flushed */
    if (fflush(stream) == EOF) {
        return -1;
    }
    /* Close pipe for writing */
    return dphalfclose_raw(fileno(stream));
}
