/**
 * Implementation of some thread safe verbose functions
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
#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS 
#endif
#ifndef __STDC_LIMIT_MACROS  
#define __STDC_LIMIT_MACROS  
#endif
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <gmp.h>
#include <string>

/* rerurn the current time */
std::string get_time() {
  time_t timer;
  char buffer[25];
  struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 25, "\r[%Y-%m-%d %H:%M:%S] ", tm_info);
  return std::string(buffer);
}

/**
 * mutex to avoid mutual exclusion by writing output
 */
pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;

