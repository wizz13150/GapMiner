/**
 * Header of a helper class for the CTR greedy algorithm,
 * A sieve which can sieve on prime at a time
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
#ifndef __ONE_PRIME_SIEVE_H__
#define __ONE_PRIME_SIEVE_H__
#include "PoWCore/src/PoWUtils.h"
#include "PoWCore/src/Sieve.h"

class OnePrimeSieve {
  
  public:

    /* the base prime of this */
    sieve_t prime;

    /* the sieve size of this */
    sieve_t size;
    sieve_t byte_size;

    /* the different sieves of this */
    sieve_t **sieves;

    /* the setted indices of this */
    sieve_t *indices;
    sieve_t indices_bytes;

    void set2(sieve_t index);
    void set2_finish();

    /* generate a new OnePrimeSieve */
    OnePrimeSieve(sieve_t prime, sieve_t size, bool init = true);

    /* deletes this */
    ~OnePrimeSieve();

    /* contains the setted sieves of this */
    vector<sieve_t> setted;

    /* clears all sieves */
    void clear();

    /* sets a given index */
    void set(sieve_t index, bool set = true);

    /* returns the number of setted indices */
    sieve_t n_setted();
        
};
#endif /* __ONE_PRIME_SIEVE_H__ */
