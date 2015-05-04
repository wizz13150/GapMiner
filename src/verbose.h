/**
 * Header file of some thread safe verbose functions
 *
 * Copyright (C)  2014  The Gapcoin developers  <info@gapcoin.org>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __VERBOSE_H__
#define __VERBOSE_H__
 
#include <gmp.h>
#include <inttypes.h>
#include <string>

using namespace std;

extern pthread_mutex_t io_mutex;

/* returns the current time */
string get_time();

/**
 * converts a given integer to a string
 */
string itoa(uint64_t i);

/**
 * converts a given double to a string
 */
string dtoa(double d, unsigned precision = 3);

#define LOG_E 0
#define LOG_W 1
#define LOG_I 2
#define LOG_D 3

#ifdef NO_LOGGING
#define log_str(str, status)
#else
#define log_str(str, status) \
  log_string(string("[") + __FILE__ + ":" + itoa(__LINE__) + "] " + str, status)

/* logs the given string */
void log_string(string str, int status);
#endif

#endif /* __VERBOSE_H__ */
