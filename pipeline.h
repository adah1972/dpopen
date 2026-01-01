// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:filetype=cpp:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file  pipeline.h
 *
 * A C++17 wrapper for dpopen with a simple interface.
 *
 * @date  2026-01-01
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>
#include <string_view>

std::string pipeline(const char* command, std::string_view input);

#endif // PIPELINE_H
