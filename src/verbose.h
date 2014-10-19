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
# ifndef __VERBOSE_H__
# define __VERBOSE_H__
  
# include <gmp.h>
# include <inttypes.h>

/**
 * print an errno message 
 */
void errno_msg(char *msg);

/**
 * print an error message 
 */
void error_msg(char *format, ...);

/**
 * print a formated string 
 */
void info_msg(char *format, ...);

#endif /* __VERBOSE_H__ */
