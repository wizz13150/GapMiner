/**
 * Implementation of a Chinese Remainder Theorem computation class
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
#include "PoWCore/src/Sieve.h"
#include "ChineseRemainder.h"
#include "utils.h"
#include <vector>
#include <gmp.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

/* run the test */
ChineseRemainder::ChineseRemainder(const sieve_t *numbers, 
                                   const sieve_t *reminders, 
                                   const sieve_t len) {
  
  this->numbers   = (sieve_t *) malloc(sizeof(sieve_t) * len);
  this->reminders = (sieve_t *) malloc(sizeof(sieve_t) * len);
  this->len       = len;

  memcpy(this->numbers,   numbers,   sizeof(sieve_t) * len);
  memcpy(this->reminders, reminders, sizeof(sieve_t) * len);

  mpz_init_set_ui(mpz_primorial, 1);

  for (sieve_t i = 0; i < len; i++)
    mpz_mul_ui(mpz_primorial, mpz_primorial, numbers[i]);

  mpz_t mpz_prime, mpz_divisor, mpz_tmp, mpz_g, mpz_r, mpz_s;
  mpz_init(mpz_prime);
  mpz_init(mpz_divisor);
  mpz_init(mpz_tmp);
  mpz_init(mpz_g);
  mpz_init(mpz_r);
  mpz_init(mpz_s);

  mpz_init_set_ui(mpz_target, 0);

  /* calculate the modulo congruent system */
  for (sieve_t i = 0; i < len; i++) {
    mpz_set_ui(mpz_prime, numbers[i]);
    mpz_div_ui(mpz_divisor, mpz_primorial, numbers[i]);
    mpz_gcdext(mpz_g, mpz_r, mpz_s, mpz_prime, mpz_divisor);

    mpz_mul(mpz_tmp, mpz_s, mpz_divisor);
    mpz_mul_ui(mpz_tmp, mpz_tmp, reminders[i]);
    mpz_add(mpz_target, mpz_target, mpz_tmp);
  }
  mpz_fdiv_r(mpz_target, mpz_target, mpz_primorial);

  /* check the results */
  for (sieve_t i = 0; i < len; i++) {
    if (mpz_tdiv_ui(mpz_target, numbers[i]) != reminders[i]) {
      log_str("ChineseRemainder Failed!!!", LOG_W);
    }
  }
}

