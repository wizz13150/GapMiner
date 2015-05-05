/**
 * Header file for a storage class of an chinese reminder theorem resut
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
#ifndef __CHINESE_SET_H__
#define __CHINESE_SET_H__
#include <gmp.h>
#include "PoWCore/src/PoWUtils.h"
#include "PoWCore/src/Sieve.h"
#include "utils.h"

class ChineseSet {

  public:

    /* the number of primes used for this */
    sieve_t n_primes;
    
    /* the intervall size used for this */
    sieve_t size;
    
    /* the byte size of the sieve */
    sieve_t byte_size;
    
    /* the primorial */
    mpz_t mpz_primorial;
    
    /* the primorial offset */
    mpz_t mpz_offset;
    
    /* the number of prime candidates still in this */
    sieve_t n_candidates;
    
    /* the min shift amount needed */
    sieve_t bit_size;
    
    /* the pre sieved numer range */
    sieve_t *sieve;

    /* the nubre of average candidates in the sieve */
    double avg_candidates;

    /* the max merit of this */
    double max_merit;
     
    /* random */
    rand128_t *rand; 

    /* creats a new ChineseSet */
    ChineseSet(sieve_t n_primes, 
               sieve_t size, 
               sieve_t n_candidates, 
               const char *offset);

    /* creats a new ChineseSet */
    ChineseSet(sieve_t n_primes, 
               sieve_t size, 
               sieve_t n_candidates, 
               mpz_t mpz_offset);

    /* saves this to a file */
    void save(FILE *file);
    void save(const char *fname);

    /* creats a new ChineseSet */
    ChineseSet(FILE *file);
    ChineseSet(const char *fname);

    ~ChineseSet();

    /* returns the theoreticaly speed increas factor for a given merit */
    double get_speed_factor(double merit);

  private:

    /* init this */
    void init();

}; 
#endif /* __CHINESE_SET_H__ */
