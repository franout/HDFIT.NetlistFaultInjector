/*
 * Copyright (C) 2022 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <stdlib.h>

#ifdef VERBOSE_DEBUG
#define NFI_DEBUG 1
#else
#define NFI_DEBUG 0
#endif /*VERBOSE_DEBUG*/

extern size_t nfiErrorCnt;

#define nfiWarning(...)                                           \
	do                                                            \
	{                                                             \
		fprintf(stderr, "Warning (%s:%i): ", __FILE__, __LINE__); \
		if (errno)                                                \
		{                                                         \
			fprintf(stderr, "%s: ", strerror(errno));             \
		}                                                         \
		fprintf(stderr, __VA_ARGS__);                             \
		fflush(stderr);                                           \
	} while (0)

#define nfiError(...)                                           \
	do                                                          \
	{                                                           \
		fprintf(stderr, "Error (%s:%i): ", __FILE__, __LINE__); \
		if (errno)                                              \
		{                                                       \
			fprintf(stderr, "%s: ", strerror(errno));           \
		}                                                       \
		fprintf(stderr, __VA_ARGS__);                           \
		fflush(stderr);                                         \
		nfiErrorCnt++;                                          \
	} while (0)

#define nfiFatal(...)                                           \
	do                                                          \
	{                                                           \
		fprintf(stderr, "Fatal (%s:%i): ", __FILE__, __LINE__); \
		if (errno)                                              \
		{                                                       \
			fprintf(stderr, "%s: ", strerror(errno));           \
		}                                                       \
		fprintf(stderr, __VA_ARGS__);                           \
		fflush(stderr);                                         \
		exit(1);                                                \
	} while (0)

#if NFI_DEBUG
#define nfiDebug(...)        \
	do                       \
	{                        \
		printf(__VA_ARGS__); \
		fflush(stdout);      \
	} while (0)
#else // !NFI_DEBUG
#define nfiDebug(...)
#endif // !NFI_DEBUG

#define nfiInfo(...)         \
	do                       \
	{                        \
		printf(__VA_ARGS__); \
		fflush(stdout);      \
	} while (0)


namespace ParseUtils {

const char *strrstr(const char *haystack, const char *pos, const char *needle);
void backslashToDoubleBackslash(std::string *out, const std::string &in);
const char *firstNonSpaceGet(const char *in);
bool isSpace(char c);
const char *lastNonSpaceGet(const char *in);
const char *lastCharAfterGet(const char *in, const char *pNeedles, size_t nNeedles);

int signalArraySizeGet(const char ** endArray, const char * in);
int signalWidthGet(const char ** endWidth, const char * in);
int subSignalArraySizeGet(const char ** endArray, const char * in);
bool has_suffix(const std::string &str, const std::string &suffix);


}

#endif /* COMMON_H_ */
