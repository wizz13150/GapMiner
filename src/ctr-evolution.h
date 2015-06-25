/**
 * Header file of a Chinese Remainder Theorem optimizer
 * using an evolutionary algorithm
 *
 * Copyright (C)  2015  The Gapcoin developers  <info@gapcoin.org>
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
#include <inttypes.h>
#include "PoWCore/src/Sieve.h"
#include "Evolution/src/C-Utils/Rand/src/rand.h"

/**
 * An Chinese Reminder Theorem structure
 */
typedef struct {
  sieve_t n_primes;    /* the number of primes */
  sieve_t *primes;     /* the primes */
  sieve_t *offsets;    /* the prime offsets */
  sieve_t fixed_len;   /* the number of fixed starting prime offsets */
  sieve_t sievesize;   /* the sieve size */
  sieve_t *base;       /* pre sieved sieve */
  sieve_t *sieve;
  sieve_t byte_size;   /* the byte size of this */
  sieve_t word_size;   /* the word size of this */
  double  merit;       /* the merit we are sieving for */
  rand128_t *rand;     /* random value */
  mpz_t   mpz_primorial;
  sieve_t bits;        /* additional bits */
} Chinese;

/* creation opts */
typedef struct {
  sieve_t n_primes;
  double  merit;
  sieve_t fixed_len;
  sieve_t level;
  double  range;
  sieve_t bits;        /* additional bits */
} CreatOpts;

/**
 * create a new Chinese
 */
void *new_chinese(void *opts);

void start_chinese_evolution(sieve_t n_primes, 
                             double merit, 
                             sieve_t fixed_len,
                             sieve_t population,
                             sieve_t n_threads,
                             double range,
                             sieve_t bits);
