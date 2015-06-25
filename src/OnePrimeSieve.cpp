/**
 * Implementation of a helper class for the CTR greedy algorithm,
 * A sieve which can sieve on prime at a time
 *
 * Copyright (C)  2014  Jonny Frey  <j0nn9.fr39@gmail.com>
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
#include "OnePrimeSieve.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;


/* generate a new OnePrimeSieve */
OnePrimeSieve::OnePrimeSieve(sieve_t prime, sieve_t size, bool init) {

  this->prime     = prime;
  this->size      = size;
  this->byte_size = bound(size, sizeof(sieve_t) * 8) / 8;
                  
  sieves = (sieve_t **) malloc(sizeof(sieve_t *) * prime);
  memset(sieves, 0, sizeof(sieve_t *) * prime);

  indices_bytes = bound(prime, sizeof(sieve_t) * 8) / 8;
  indices = (sieve_t *) malloc(indices_bytes);
  memset(indices, 0, indices_bytes);

  if (init) {
  
    /* safe modulo 0 */
    sieves[0] = (sieve_t *) malloc(byte_size);
    memset(sieves[0], 0, this->size / 8);

    for (sieve_t p = 0; p < this->size; p += prime)
      set_composite(sieves[0], p);
 
    /* setting i % prime == 0 equals to base number % prime == prime - i */
    for (sieve_t i = 1; i < prime; i++) {
      
      sieves[prime - i] = (sieve_t *) malloc(byte_size);
      memset(sieves[prime - i], 0, byte_size);
 
      for (sieve_t p = i; p < this->size; p += prime)
        set_composite(sieves[prime - i], p);
      
    }
  } 
}

/* deletes this */
OnePrimeSieve::~OnePrimeSieve() {
  
  for (sieve_t i = 0; i < prime; i++)
    if (sieves[i])
      free(sieves[i]);

  free(sieves);
  free(indices);
}

/* clears all sieves */
void OnePrimeSieve::clear() {

  for (sieve_t i = 0; i < prime; i++) {
    if (sieves[i]) {
      free(sieves[i]);
      sieves[i] = (sieve_t *) 0;
    }
  }

  setted.clear();
  memset(indices, 0, indices_bytes);
}

void OnePrimeSieve::set(sieve_t index, bool set) {
  
  /* checks if index is already setted */
  for (sieve_t i = 0; i < setted.size(); i++)
    if (setted[i] == index) 
      return;

  setted.push_back(index);

  if (set) {
    sieves[index] = (sieve_t *) malloc(byte_size);
    memset(sieves[index], 0, byte_size);
  
    for (sieve_t p = prime - index; p < this->size; p += prime)
      set_composite(sieves[index], p);
  }
}

void OnePrimeSieve::set2(sieve_t index) {
  set_composite(indices, index);
}

void OnePrimeSieve::set2_finish() {
  for (sieve_t i = 0; i < prime; i++) {
    if (!is_prime(indices, i))
      setted.push_back(i);
  }
}

/* returns the number of setted indices */
sieve_t OnePrimeSieve::n_setted() {
  return setted.size();
}
