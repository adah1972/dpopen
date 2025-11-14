// -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file  dpopen.h
 *
 * Header file of a duplex pipe stream.
 *
 * @date  2025-11-14
 */

#ifndef DPOPEN_H
#define DPOPEN_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int dpopen_raw(const char *command);
int dpclose_raw(int fd);
int dphalfclose_raw(int fd);

FILE *dpopen(const char *command);
int dpclose(FILE *stream);
int dphalfclose(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* DPOPEN_H */
