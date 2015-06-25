/**
 * Header for an implementation of an storage class for the first
 * n primes with their OnePrimeSieve
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
#ifndef __PRIME_SIEVE_SET_H__
#define __PRIME_SIEVE_SET_H__

#include "PoWCore/src/PoWUtils.h"
#include "PoWCore/src/Sieve.h"
#include "OnePrimeSieve.h"
#include "ChineseSet.h"

#define MAX_GREADY 1500
#define N_TEST 1000
#define LEVEL1_LAYERS 35

class BestChinese {
    
  private:

    /* the number of primes */
    sieve_t n_primes;
    
    /* the first n prime sieves */
    OnePrimeSieve **primes;

    /* the sieve size */
    sieve_t sievesize;
    sieve_t byte_size;

    /* the primorial */
    sieve_t primorial;

    /* the sieve of this */
    sieve_t *sieve;
    sieve_t *prev_layers;

    /* the target merit */
    double merit;

    /* the number of test to calculate the average number of candidates */
    ssieve_t avg_tests;

    /* the maximum number of greedy iterations */
    sieve_t max_gready;

    /* the calculates ChineseSet */
    ChineseSet *best_set;

    /* index */
    sieve_t i;

    /* verbose level */
    bool verbose;

  public:

    /* create a new BestChinese */
    BestChinese(sieve_t n, 
                double merit, 
                sieve_t min_bits = 0, 
                ssieve_t avg_tests = N_TEST,
                sieve_t max_gready = MAX_GREADY,
                sieve_t index = 0,
                bool verbose = true);

    /* deletes this */
    ~BestChinese();

    /* calculate the perfect residue classes */
    sieve_t calc_best_residues(bool save = true);

    /* returns the best_set */
    ChineseSet *get_best_set();
};


#endif /* __PRIME_SIEVE_SET_H__ */
