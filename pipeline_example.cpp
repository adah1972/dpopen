// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

#include <iostream>
#include <stdexcept>
#include <string>
#include "pipeline.h"

int main()
{
    try {
        std::string result = pipeline("sort", "orange\n"
                                              "apple\n"
                                              "pear\n");
        std::cout << result;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Operation failed: " << e.what() << '\n';
    }
}
