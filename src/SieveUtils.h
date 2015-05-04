/**
 * Header file for an easy and accesible primalty sieve
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
#ifndef __SIEVE_UTILS_H__
#define __SIEVE_UTILS_H__
#include <inttypes.h>
#include <gmp.h>
#include <mpfr.h>
#include <vector>

using namespace std;
//#include "GapAnalyseTool.h"

/**
 * define the sieve array word size
 */
#if __WORDSIZE == 64
#define sieve_t uint64_t
#define ssieve_t int64_t
#define SIEVE_MAX UINT64_MAX
#define PRISIEVE PRIu64
#else
#define sieve_t uint32_t
#define ssieve_t int32_t
#define SIEVE_MAX UINT32_MAX
#define PRISIEVE PRIu32
#endif

/**
 * converts an byte array to an mpz value
 *
 * last significant word first
 * last significant byte first
 */
#define ary_to_mpz(mpz_res, ary, len) \
  mpz_import(mpz_res, len, -1, sizeof(uint8_t), -1, 0, ary)

/**
 * converts an mpz value to an byte array
 *
 * last significant word first
 * last significant byte first
 */
#define mpz_to_ary(mpz_src, ary, len) \
  mpz_export(ary, len, -1, sizeof(uint8_t), -1, 0, mpz_src)


/**
 * sets a mpz to an uint64_t value
 * by checking if unsigned long has 32 or 64 bits
 */
#if __WORDSIZE == 64
#define mpz_set_ui64(mpz, ui64) mpz_set_ui(mpz, ui64)
#else
inline void mpz_set_ui64(mpz_t mpz, uint64_t ui64) {

  mpz_set_ui(mpz, (uint32_t) (ui64 >> 32));      
  mpz_mul_2exp(mpz, mpz, 32);                     
  mpz_add_ui(mpz, mpz, (uint32_t) ui64);       
} 
#endif

/**
 * sets and init a mpz to an uint64_t value
 * by checking if unsigned long has 32 or 64 bits
 */
#if __WORDSIZE == 64
#define mpz_init_set_ui64(mpz, ui64) mpz_init_set_ui(mpz, ui64)
#else
inline void mpz_init_set_ui64(mpz_t mpz, uint64_t ui64) {

  mpz_init_set_ui(mpz, (uint32_t) (ui64 >> 32));  
  mpz_mul_2exp(mpz, mpz, 32);                      
  mpz_add_ui(mpz, mpz, (uint32_t) ui64);        
}
#endif

/**
 * returns the 64 lower bits of an mpz value
 */
#if __WORDSIZE == 64
#define mpz_get_ui64(mpz) mpz_get_ui(mpz)
#else
inline uint64_t mpz_get_ui64(mpz_t mpz) {
  mpz_t mpz_tmp;
  mpz_init_set(mpz_tmp, mpz);
  uint64_t ui64 = mpz_get_ui(mpz_tmp) & 0xffffffff;
  
  mpz_div_2exp(mpz_tmp, mpz_tmp, 32);
  ui64 |= (((uint64_t) mpz_get_ui(mpz_tmp)) & 0xffffffff) << 32;

  mpz_clear(mpz_tmp);
  return ui64;
}
#endif

/**
 * returns whether the given mpz fits an uint64_t
 */
#if __WORDSIZE == 64
#define mpz_fits_uint64_p(mpz) mpz_fits_ulong_p(mpz)
#else
#define mpz_fits_uint64_p(mpz) (mpz_sizeinbase(mpz, 2) <= 64)
#endif

/**
 * sets a mpfr to an uint64_t value
 * by checking if unsigned long has 32 or 64 bits
 */
#if __WORDSIZE == 64
#define mpfr_set_ui64(mpfr, ui64, rand) mpfr_set_ui(mpfr, ui64, rand)
#else
inline void mpfr_set_ui64(mpfr_t mpfr, uint64_t ui64, mp_rnd_t rand) {

  mpfr_set_ui(mpfr, (uint32_t) (ui64 >> 32), rand);        
  mpfr_mul_2exp(mpfr, mpfr, 32, rand);                      
  mpfr_add_ui(mpfr, mpfr, (uint32_t) ui64, rand);        
}
#endif

/**
 * sets a mpfr to an uint64_t value
 * by checking if unsigned long has 32 or 64 bits
 */
#if __WORDSIZE == 64
#define mpfr_init_set_ui64(mpfr, ui64, rand) \
  mpfr_init_set_ui(mpfr, ui64, rand)
#else
inline void mpfr_init_set_ui64(mpfr_t mpfr, uint64_t ui64, mp_rnd_t rand) {

  mpfr_init_set_ui(mpfr, (uint32_t) (ui64 >> 32), rand);        
  mpfr_mul_2exp(mpfr, mpfr, 32, rand);                      
  mpfr_add_ui(mpfr, mpfr, (uint32_t) ui64, rand);        
}
#endif

#if __WORDSIZE == 64
#define popcount(X) __builtin_popcountl(X)
#else
#define popcount(X) __builtin_popcountll(X)
#endif


#if __WORDSIZE == 64
/**
 * Sets the given bit-position in a 64-bit array
 */
#define set_bit(ary, i) (ary[(i) >> 6] |= (1L << ((i) & 0x3f)))
    
/**
 * Unset the given bit-position in a 64-bit array
 */
#define unset_bit(ary, i) (ary[(i) >> 6] &= ~(1L << ((i) & 0x3f)))

/**
 * returns whether the given bit-position in a 64-bit array is set or not
 */
#define bit_at(ary, i) (ary[(i) >> 6] & (1L << ((i) & 0x3f)))
#else
/**
 * Sets the given bit-position in a 32-bit array
 */
#define set_bit(ary, i) (ary[(i) >> 5] |= (1 << ((i) & 0x1f)))
    
/**
 * Unset the given bit-position in a 32-bit array
 */
#define unset_bit(ary, i) (ary[(i) >> 5] &= ~(1 << ((i) & 0x1f)))

/**
 * returns whether the given bit-position in a 32-bit array is set or not
 */
#define bit_at(ary, i) (ary[(i) >> 5] & (1 << ((i) & 0x1f)))
#endif

/**
 * returns whether the given index is a prime or not
 */
#define is_prime(ary, i) !bit_at(ary, i)

/**
 * marks the given index in the given array as composite
 */
#define set_composite(ary, i) set_bit(ary, i)

/**
 * sets x to the next greater number divisible by y
 */
#define bound(x, y) ((((x) + (y) - 1) / (y)) * (y))

/**
 * returns the sieve limit for an simple sieve of Eratosthenes
 */
#define sieve_limit(x) ((uint64_t) (sqrt((double) (x)) + 1))

/**
 * generate x^2
 */
#define POW(X) ((X) * (X))


class SieveUtils {
  
  private:

    /* sieve size in bits */
    sieve_t sievesize;

    /* the realy sieve byte size */
    sieve_t real_size;
 
    /* the sieve as an ary of 64 bit words */
    sieve_t *sieve;

    /* number of sieve filter primes */
    sieve_t n_primes;
    
    /* array of the first n primes */
    sieve_t *primes;

    /* array of the first n primes * 2 */
    sieve_t *primes2;
 
    /**
     * array of the start indexes for each prime.
     * while sieving the current gap
     */
    sieve_t *starts;

    /* the start of the sieve */
    mpz_t mpz_start;

    /* primality testing */
    mpz_t mpz_e, mpz_r, mpz_two;

    /**
     * Generates the first n primes using the sieve of Eratosthenes
     */
    void init_primes(uint64_t n);
 
    /**
     * calculate the sieve start indexes;
     */
    void calc_muls();

    /**
     * Fermat pseudo prime test
     */
    inline bool fermat_test(mpz_t mpz_p);

  public:

    /* returns the current time in microseconds */
    static uint64_t gettime_usec();
    
    /* creat a new SieveUtils object with given nuber of primes */
    SieveUtils(uint64_t n_primes);

    /* destroy this */
    ~SieveUtils();

    /* returns the nuber of primes used for sieveing */
    uint64_t get_n_sieve_primes();

    /* returns a copy of the primes used for sieveing */
    uint64_t *get_primes();

    /* returns a copy of the primes2 (primes * 2) used for sieveing */
    uint64_t *get_primes2();

    /* run a sieve of the given size with the given start prime */
    bool run_sieve(mpz_t mpz_start, uint64_t sievesize);

    /* returns a copy of the last runed sieve */
    sieve_t *get_sieve();

    /* returns tha last used sievesize */
    uint64_t get_sievesize();

    /* returns a copy of the last start indices */
    uint64_t *get_starts();

    /* retruns the nuber of the last calculated prime candidates */
    uint64_t get_n_candidates();

    /* retruns the nuber of the last calculated primes candidates */
    uint64_t get_n_primes();

    /* returns the last calulated prime candidate offsets from the sieve */
    vector<uint64_t> get_candidate_offset();

    /* returns the last calulated prime offsets from the sieve */
    vector<uint64_t> get_prime_offset();

    /* returns a copy of the last start */
    void get_start(mpz_t mpz_start);

    /* calculate a primorial up to n primes */
    bool primorial(mpz_t mpz_primorial, uint64_t n);

    /**
     * calculates the log from a mpz value
     * (double version for debugging)
     */
    static double mpz_log(mpz_t mpz);

    /**
     * runn an independent fermat test on the given mpz nuber
     */
    static bool mpz_fermat_test(mpz_t mpz_p);

    /* calculate n primes, beginning at start, and store thme in primes */
    uint64_t *get_primes_from(uint64_t start, uint64_t n);

};

/* the first 78498 primes all primes below 1'000'000 */
extern sieve_t first_primes[78498];


#endif /* __SIEVE_UTILS_H__ */
