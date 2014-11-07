/**
 * Header file of Gapcoins Proof of Work calculation unit.
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
#ifndef CPU_ONLY
#ifndef __HYBRID_SIEVE_H__
#define __HYBRID_SIEVE_H__
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <queue>

#include "PoWCore/src/PoW.h"
#include "PoWCore/src/PoWUtils.h"
#include "PoWCore/src/PoWProcessor.h"
#include "PoWCore/src/Sieve.h"
#include "GPUFermat.h"

using namespace std;

class HybridSieve : public Sieve {

  public :

    /* sha256 hash of the previous block */
    uint8_t hash_prev_block[SHA256_DIGEST_LENGTH];

    /* stop the current running sieve */
    void stop();

    /**
     * create a new HybridSieve
     */
    HybridSieve(PoWProcessor *pprocessor, 
                uint64_t n_primes, 
                uint64_t sievesize,
                uint64_t max_primes = 30000000,
                uint64_t work_items = 2048,
                uint64_t queue_size = 10);

    ~HybridSieve();

    /** 
     * sieve for the given header hash 
     *
     * returns an adder (within pow)  starting a gap greater than difficulty
     *         or NULL if no such prime was found
     */
   void run_sieve(PoW *pow, 
                  vector<uint8_t> *offset,
                  uint8_t hash[SHA256_DIGEST_LENGTH]);
 
  protected :

    /* check if we should stop sieving */
    bool should_stop(uint8_t hash[SHA256_DIGEST_LENGTH]);

    /* indicates that the sieve should stop calculating */
    bool running;

    /* max sieve primes */
    uint64_t max_primes;

    /* the number of work items pushed to the gpu at once */
    uint64_t work_items;

    /* template array for the Fermat candidates */
    uint64_t *candidates_template;

    /* one set of work items for the GPU */
    class GPUWork {

      public :

        /* the prime candidates to test */
        uint32_t *candidates;

        /* the Fermat results */
        bool *results;

        /* candidate size */
        unsigned size;

        /* the pow nonce */
        uint32_t nonce;

        /* the pow target difficulty */
        uint64_t target;
       
        /* create a new GPUWork */
        GPUWork(unsigned size) {
       
          this->size = size;
       
          candidates = (uint32_t *) malloc(sizeof(uint32_t) * size * 10);
          results    = (bool *) malloc(sizeof(bool) * size);
        }
       
        /* destroys a GPUWork */
        ~GPUWork() {
          free(candidates);
          free(results);
        }
    };


    /**
     * a class to store prime chain candidates
     */
    class GPUQueue {

      public :

        GPUQueue(unsigned capacity = 5);
        ~GPUQueue();

        /* remove the oldest gpu work */
        GPUWork *pull();

        /* add an new GPUWork */
        void push(GPUWork *work);

        /* clear this */
        void clear();

        /* get the size of this */
        size_t size();

        /* indicates that this queue is full */
        bool full();

      private :

        /* the capacity of this */
        unsigned capacity;

        /* the GPUWork queue */
        queue<GPUWork *> q;

        /* synchronization */
        pthread_mutex_t access_mutex;
        pthread_cond_t  empty_cond;
        pthread_cond_t  full_cond;
    };

    /* work input for the gpu */
    GPUQueue *input;

    /* gpu output queue */
    GPUQueue *output;

    /* thread args for the calculation threads */
    typedef struct {
      GPUQueue *input;
      GPUQueue *output;
      bool running;
      uint64_t *found_primes;
      uint64_t *gaps10;
      uint64_t *gaps15;
      uint64_t *passed_time;
      uint64_t *cur_found_primes;
      uint64_t *cur_gaps10;
      uint64_t *cur_gaps15;
      uint64_t *cur_passed_time;
      bool *reset_stats;
      sieve_t sievesize;
      PoWUtils *utils;
      uint64_t work_items;
      PoWProcessor *pprocessor;
      HybridSieve *sieve;
    } ThreadArgs;

    /* thread args of this */
    ThreadArgs targs;

    /* the gpu thread */
    static void *gpu_work_thread(void *args);

    /* the gpu results processing thread */
    static void *gpu_results_thread(void *args);

    /* thread objects */
    pthread_t gpu_thread;
    pthread_t results_thread;

};
#endif /* __HYBRID_SIEVE_H__ */
#endif /* CPU_ONLY */
