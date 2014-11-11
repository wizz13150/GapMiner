/**
 * Implementation of Gapcoins Proof of Work calculation unit.
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
#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS 
#endif
#ifndef __STDC_LIMIT_MACROS  
#define __STDC_LIMIT_MACROS  
#endif
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <pthread.h>
#include <iostream>

#include "verbose.h"
#include "HybridSieve.h"
#include "Opts.h"

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

/* gpu group size */
#define gpu_groub_size 256

/* gpu operand size */
#define gpu_op_size 10

using namespace std;

/**
 * create a new HybridSieve
 */
HybridSieve::HybridSieve(PoWProcessor *pprocessor, 
                         uint64_t n_primes, 
                         uint64_t sievesize,
                         uint64_t max_primes,
                         uint64_t work_items,
                         uint64_t queue_size) : Sieve(pprocessor, 
                                                      max_primes, 
                                                      sievesize) { 

  this->n_primes   = n_primes;
  this->max_primes = max_primes;
  this->work_items = work_items;
  this->candidates_template = (uint64_t *) malloc(sizeof(uint64_t) * 
                                                  gpu_groub_size   * 
                                                  work_items       *
                                                  gpu_op_size / 2);

  this->input  = new GPUQueue((queue_size < 2) ? 2 : queue_size);
  this->output = new GPUQueue((queue_size < 2) ? 2 : queue_size);

  this->passed_time      = 1;
  this->cur_passed_time  = 1;

  targs.input            = input;
  targs.output           = output;
  targs.running          = true;
  targs.found_primes     = &found_primes;
  targs.gaps10           = &gaps10;
  targs.gaps15           = &gaps15;
  targs.passed_time      = &passed_time;
  targs.cur_found_primes = &cur_found_primes;
  targs.cur_gaps10       = &cur_gaps10;
  targs.cur_gaps15       = &cur_gaps15;
  targs.cur_passed_time  = &cur_passed_time;
  targs.reset_stats      = &reset_stats;
  targs.sievesize        = sievesize;
  targs.utils            = utils;
  targs.work_items       = work_items;
  targs.pprocessor       = pprocessor;
  targs.sieve            = this;

  pthread_create(&gpu_thread, NULL, gpu_work_thread, (void *) &targs);
  pthread_create(&results_thread, NULL, gpu_results_thread, (void *) &targs);
}


HybridSieve::~HybridSieve() { 
  
  targs.running = false;
  stop();
  
  pthread_join(gpu_thread, NULL);
  pthread_join(results_thread, NULL);

  delete input;
  delete output;

  free(candidates_template);
}

/** 
 * sieve for the given header hash 
 *
 * Sets the pow adder to a prime starting a gap greater than difficulty,
 * if found
 *
 * The HybridSieve works in two stages, first it checks every odd number
 * if it is divisible by one of the pre-calculated primes.
 * Then it uses the Fermat-test to test the remaining numbers.
 */
void HybridSieve::run_sieve(PoW *pow, 
                            vector<uint8_t> *offset, 
                            uint8_t hash[SHA256_DIGEST_LENGTH]) {
  
  running = true;
  
  /* just to be sure */
  pow->set_shift(64);

  mpz_t mpz_offset;
  mpz_init_set_ui64(mpz_offset, 0);

  if (offset != NULL)
    ary_to_mpz(mpz_offset, offset->data(), offset->size());

  /* make sure offset (and later start) is divisible by two */
  if (mpz_get_ui64(mpz_offset) & 0x1)
    mpz_add_ui(mpz_offset, mpz_offset, 1L);

  mpz_t mpz_adder, mpz_tmp;
  mpz_init(mpz_tmp);
  mpz_init(mpz_adder);

  pow->get_hash(mpz_start);
  mpz_mul_2exp(mpz_start, mpz_start, pow->get_shift());
  mpz_add(mpz_start, mpz_start, mpz_offset);

  /* init candidates_template */
  size_t exported_size;
  mpz_export((uint32_t *) candidates_template, 
             &exported_size, -1, 4, 0, 0, mpz_start);

  /* copy start to every candidate assumes shift is 64 */
  for (uint64_t i = 1; i < gpu_groub_size * work_items; i++) {
    candidates_template[i * gpu_op_size / 2 + 1] = candidates_template[1];
    candidates_template[i * gpu_op_size / 2 + 2] = candidates_template[2];
    candidates_template[i * gpu_op_size / 2 + 3] = candidates_template[3];
    candidates_template[i * gpu_op_size / 2 + 4] = candidates_template[4];
  }

  /* current prime candidate index */
  sieve_t p_index = 0;

  /* create and initialize the GPUWork */
  GPUWork *work = new GPUWork(gpu_groub_size * work_items);
  work->nonce   = pow->get_nonce();
  work->target  = pow->get_target();
  memcpy(work->candidates, 
         candidates_template, 
         work_items * gpu_groub_size * gpu_op_size * sizeof(uint32_t));


  /* calculates for each prime, the first index in the sieve
   * which is divisible by that prime */
  calc_muls();

  
  /* run the sieve till stop signal arrives */
  for (sieve_t sieve_round = 0; running && !should_stop(hash); sieve_round++) {
    


    /* clear the sieve */
    memset(sieve, 0, sievesize / 8);

    /* sieve all small primes (skip 2) */
    for (sieve_t i = 1; i < n_primes; i++) {

      /**
       * sieve all odd multiplies of the current prime
       */
      sieve_t p;
      for (p = starts[i]; p < sievesize; p += primes2[i])
        set_composite(sieve, p);

      starts[i] = p - sievesize;
    }


    /* collect the prime candidates */
    for (sieve_t i = 1; i < sievesize; i += 2) {
      
      if (is_prime(sieve, i)) {
        ((uint64_t *) work->candidates)[p_index * gpu_op_size / 2] = i + sieve_round * sievesize;
        p_index++;

        if (p_index >= work_items * gpu_groub_size) {

          /* decrease sieve primes, if input queue is empty */
          if (input->size() < 1 && n_primes > max_primes / 50) {
            n_primes -= max_primes / 100;

            if (Opts::get_instance()->has_extra_vb()) {
              pthread_mutex_lock(&io_mutex);
              cout << get_time() << "Sieve primes changed  <=  " << n_primes << endl;
              pthread_mutex_unlock(&io_mutex);
            }
          }

          input->push(work);
          p_index = 0;

          /* create new work */
          work = new GPUWork(gpu_groub_size * work_items);
          work->nonce  = pow->get_nonce();
          work->target = pow->get_target();
          memcpy(work->candidates, 
                 candidates_template, 
                 work_items * gpu_groub_size * gpu_op_size * sizeof(uint32_t));

          /* increase sieve primes if input queue is full */
          if (input->full() && n_primes < max_primes - max_primes / 100) {
            n_primes += max_primes / 100;
            running = false;

            if (Opts::get_instance()->has_extra_vb()) {
              pthread_mutex_lock(&io_mutex);
              cout << get_time() << "Sieve primes changed  =>  " << n_primes << endl;
              pthread_mutex_unlock(&io_mutex);
            } 
          }
        }
      }
    }
  }
  
  delete work;
  mpz_clear(mpz_offset);
  mpz_clear(mpz_adder);
  mpz_clear(mpz_tmp);
}

/* creates a new GPUQueue */
HybridSieve::GPUQueue::GPUQueue(unsigned capacity) {

  this->capacity = capacity;
  pthread_mutex_init(&access_mutex, NULL);
  pthread_cond_init(&empty_cond, NULL);
  pthread_cond_init(&full_cond, NULL);
}

/* destroys a GPUQueue */
HybridSieve::GPUQueue::~GPUQueue() {
  pthread_mutex_destroy(&access_mutex);
  pthread_cond_destroy(&empty_cond);
  pthread_cond_destroy(&full_cond);
}

/* remove the oldest gpu work */
HybridSieve::GPUWork *HybridSieve::GPUQueue::pull() {
 
  pthread_mutex_lock(&access_mutex);

  while (q.empty())
    pthread_cond_wait(&empty_cond, &access_mutex);
   
  GPUWork *work = q.front();
  q.pop();

  pthread_cond_signal(&full_cond);
  pthread_mutex_unlock(&access_mutex);

  return work;
}

/* add an new GPUWork */
void HybridSieve::GPUQueue::push(HybridSieve::GPUWork *work) {
 
  pthread_mutex_lock(&access_mutex);

  while (q.size() >= capacity)
    pthread_cond_wait(&full_cond, &access_mutex);

  q.push(work);

  pthread_cond_signal(&empty_cond);
  pthread_mutex_unlock(&access_mutex);
}

/* clear this */
void HybridSieve::GPUQueue::clear() {
 
  pthread_mutex_lock(&access_mutex);

  while (!q.empty()) {
    GPUWork *work = q.front();
    q.pop();
    delete work;
  }

  pthread_mutex_unlock(&access_mutex);
}

/* get the size of this */
size_t HybridSieve::GPUQueue::size() {
 
  return q.size();
}

/* indicates that this queue is full */
bool HybridSieve::GPUQueue::full() {
  return (q.size() >= capacity);
}

/* the gpu thread */
void *HybridSieve::gpu_work_thread(void *args) {

  if (Opts::get_instance()->has_extra_vb()) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "GPU work thread started\n";
    pthread_mutex_unlock(&io_mutex);
  }

#ifndef WINDOWS
  /* use idle CPU cycles for mining */
  struct sched_param param;
  param.sched_priority = sched_get_priority_min(SCHED_IDLE);
  sched_setscheduler(0, SCHED_IDLE, &param);
#endif

  ThreadArgs *targs = (ThreadArgs *) args;
  GPUFermat *fermat = GPUFermat::get_instance();
  
  while (targs->running) {

    GPUWork *work = targs->input->pull();
    fermat->fermat_gpu(work->candidates, work->results);
    targs->output->push(work);
  }

  return NULL;
}


/* the gpu results processing thread */
void *HybridSieve::gpu_results_thread(void *args) {

  if (Opts::get_instance()->has_extra_vb()) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "GPU result processing thread started\n";
    pthread_mutex_unlock(&io_mutex);
  }

#ifndef WINDOWS
  /* use idle CPU cycles for mining */
  struct sched_param param;
  param.sched_priority = sched_get_priority_min(SCHED_IDLE);
  sched_setscheduler(0, SCHED_IDLE, &param);
#endif

  ThreadArgs *targs = (ThreadArgs *) args;

  mpz_t mpz_hash, mpz_start, mpz_adder;
  mpz_init(mpz_hash);
  mpz_init(mpz_start);
  mpz_init(mpz_adder);


  while (targs->running) {

    /* speed measurement */
    uint64_t start_time = PoWUtils::gettime_usec();
    
    GPUWork *work = targs->output->pull();

    if (*targs->reset_stats) {
      *targs->reset_stats = false;
      *targs->cur_found_primes = 0;
      *targs->cur_gaps10       = 0;
      *targs->cur_gaps15       = 0;
      *targs->cur_passed_time  = 1;
    }

    mpz_import(mpz_hash, gpu_op_size, -1, 4, 0, 0, work->candidates);
    mpz_div_2exp(mpz_hash, mpz_hash, 64);

    PoW pow(mpz_hash, 64, NULL, work->target, work->nonce);

    pow.get_hash(mpz_start);
    mpz_mul_2exp(mpz_start, mpz_start, 64);
    

    ssieve_t last_prime = 1 << 31;
    ssieve_t min_len    = pow.target_size(mpz_start);
    ssieve_t min10_len  = targs->utils->target_size(mpz_start, 10 * TWO_POW48);
    ssieve_t min15_len  = targs->utils->target_size(mpz_start, 15 * TWO_POW48);
    ssieve_t cur_prime  = 0;
 
    /* scan gpu results for a large prime gap
     */
    for (sieve_t i = 0; i < gpu_groub_size * targs->work_items; i++) {
      
      if (work->results[i]) {
        
        (*targs->found_primes)++;
        (*targs->cur_found_primes)++;
 
        cur_prime = work->candidates[i*10];
 
        if (cur_prime - last_prime > min10_len) {
          (*targs->gaps10)++;
          (*targs->cur_gaps10)++;
        }
 
        if (cur_prime - last_prime > min15_len) {
          (*targs->gaps15)++;
          (*targs->cur_gaps15)++;
        }
 
        if (cur_prime - last_prime >= min_len) {
 
          mpz_set_ui64(mpz_adder, (uint64_t) last_prime);
          pow.set_adder(mpz_adder);
 
          if (pow.valid()) {
 
            /* stop calculating if processor said so */
            if (!targs->pprocessor->process(&pow)) {
              targs->sieve->stop();
              break;
            }
          }
        }
      
        last_prime = cur_prime;
      }
    }

    *targs->passed_time     += PoWUtils::gettime_usec() - start_time;
    *targs->cur_passed_time += PoWUtils::gettime_usec() - start_time;

    delete work;
  }

  return NULL;
}

/* stop the current running sieve */
void HybridSieve::stop() {
  
  running = false;
  input->clear();
  output->clear();
}

/* check if we should stop sieving */
bool HybridSieve::should_stop(uint8_t hash[SHA256_DIGEST_LENGTH]) {

  bool result = false;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    if (hash_prev_block[i] != hash[i]) {
      result = true;
      break;
    }
  }

  if (result) stop();
  return result;
}
#endif /* CPU_ONLY */
