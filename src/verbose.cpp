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

/* print current time */
static void print_time(FILE *out = stdout) {
  time_t timer;
  char buffer[25];
  struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 25, "\r[%Y-%m-%d %H:%M:%S]", tm_info);
  fprintf(out, "%s ", buffer);
}

/**
 * mutex to avoid mutual exclusion by writing output
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * print an errno message 
 */
void errno_msg(char *msg) {

  pthread_mutex_lock(&mutex);
  print_time(stderr);
  perror(msg);
  pthread_mutex_unlock(&mutex);
}

/**
 * print an error message 
 */
void error_msg(char *format, ...) {

  pthread_mutex_lock(&mutex);

  va_list args;
  va_start(args, format);
  print_time(stderr);
  vfprintf(stderr, format, args);
  va_end(args);

  pthread_mutex_unlock(&mutex);
}

/**
 * print a formated string 
 */
void info_msg(char *format, ...) {
  
  pthread_mutex_lock(&mutex);

  va_list args;
  va_start(args, format);
  print_time();
  vprintf(format, args);
  va_end(args);

  pthread_mutex_unlock(&mutex);

}

/**
 * prints an mpz value (debugging)
 */
void print_mpz(const mpz_t mpz) {
  
  pthread_mutex_lock(&mutex);
  mpz_out_str(stderr, 10, mpz);
  pthread_mutex_unlock(&mutex);
}
