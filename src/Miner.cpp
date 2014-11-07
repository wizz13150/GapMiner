/**
 * Implementation of a Gapcoin miner
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
#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS 
#endif
#ifndef __STDC_LIMIT_MACROS  
#define __STDC_LIMIT_MACROS  
#endif
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>
#include "Miner.h"
#include "BlockHeader.h"
#include "PoWCore/src/PoWUtils.h"
#include "PoWCore/src/PoW.h"
#include "PoWCore/src/PoWProcessor.h"
#include "ShareProcessor.h"
#include "PoWCore/src/Sieve.h"
#include "Opts.h"
#include "verbose.h"
#include "HybridSieve.h"

/* synchronization mutex */
pthread_mutex_t Miner::mutex = PTHREAD_MUTEX_INITIALIZER;

/* create a new miner */
Miner::Miner(uint64_t sieve_size, 
             uint64_t sieve_primes, 
             int n_threads) {

  this->sieve_size   = sieve_size;
  this->sieve_primes = sieve_primes;
  this->n_threads    = n_threads;
  this->running      = false;
  this->is_started   = false;
#ifndef CPU_ONLY  
  this->use_gpu      = Opts::get_instance()->has_use_gpu(); 
#endif  

  threads      = (pthread_t *)   calloc(n_threads, sizeof(pthread_t));
  args         = (ThreadArgs **) calloc(n_threads, sizeof(ThreadArgs *));

#ifndef CPU_ONLY  
  if (use_gpu)
    this->n_threads = 1;
#endif    
}              

/* start processing */
void Miner::start(BlockHeader *header) {

  running = true;
#ifndef CPU_ONLY
  Opts *opts = Opts::get_instance();
  bool use_gpu = opts->has_use_gpu(); 

  uint64_t max_primes = (opts->has_max_primes() ? atoi(opts->get_max_primes().c_str()) : 30000000);
  uint64_t work_items = (opts->has_work_items() ? atoi(opts->get_work_items().c_str()) : 2048);
  uint64_t queue_size = (opts->has_queue_size() ? atoi(opts->get_queue_size().c_str()) : 10);
#endif

  ShareProcessor *share_processor = ShareProcessor::get_processor();
  share_processor->update_header(header);

  for (int i = 0; i < n_threads; i++) {

    args[i] = new ThreadArgs(i, 
                             n_threads, 
                             &running, 
                             header);
    
#ifndef CPU_ONLY
    if (use_gpu) {
      args[i]->hsieve = new HybridSieve((PoWProcessor *) share_processor, 
                                        sieve_primes, 
                                        sieve_size,
                                        max_primes,
                                        work_items,
                                        queue_size);

      memcpy(args[i]->hsieve->hash_prev_block, 
             header->hash_prev_block,
             SHA256_DIGEST_LENGTH);
             
    } else {
#endif
      args[i]->sieve = new Sieve((PoWProcessor *) share_processor, 
                                 sieve_primes, 
                                 sieve_size);
#ifndef CPU_ONLY
    }
#endif
    
    pthread_create(&threads[i], NULL, miner, (void *) args[i]);

    if (Opts::get_instance()->has_extra_vb()) {
      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "Worker " << i << " started\n";
      pthread_mutex_unlock(&io_mutex);
    }
  }

  is_started = true;
}

/* delete a miner */
Miner::~Miner() {
  stop();
}

/* stops all threads and waits until they are finished */
void Miner::stop() {

#ifndef CPU_ONLY
  bool use_gpu = Opts::get_instance()->has_use_gpu(); 
#endif

  if (running) {
    running = false;
    
    for (int i = 0; i < n_threads; i++) {

#ifndef CPU_ONLY
      if (use_gpu)
        args[i]->hsieve->stop();
#endif

      pthread_join(threads[i], NULL);
      delete args[i]->header;

#ifndef CPU_ONLY
      if (use_gpu)
        delete args[i]->hsieve;
      else
#endif
        delete args[i]->sieve;
    }
  }
}

/* updates the BlockHeader for all threads */
bool Miner::update_header(BlockHeader *header) {
  
  if (!is_started) return false;

#ifndef CPU_ONLY
  bool use_gpu = Opts::get_instance()->has_use_gpu(); 
#endif
  
  if (!running)
    return false;

#ifndef CPU_ONLY
  /* restart sieve  with new header */
  for (int i = 0; use_gpu && i < n_threads; i++) {
      memcpy(args[i]->hsieve->hash_prev_block, 
             header->hash_prev_block,
             SHA256_DIGEST_LENGTH);

      args[i]->hsieve->stop();
  }
#endif

  pthread_mutex_lock(&mutex);
  for (int i = 0; i < n_threads; i++) {

    delete args[i]->header; 
    args[i]->header = header->clone();
    args[i]->header->nonce = i;
  }
  pthread_mutex_unlock(&mutex);

  /* update header of ShareProcessor */
  ShareProcessor::get_processor()->update_header(header);

  return true;
}


/* create a new ThreadArgs */
Miner::ThreadArgs::ThreadArgs(int id, 
                              int n_threads, 
                              bool *running, 
                              BlockHeader *header) {

  this->id            = id;
  this->n_threads     = n_threads;
  this->running       = running;
  this->header        = header->clone();
  this->header->nonce = id;
}

/* a single mining thread */
void *Miner::miner(void *args) {
  
#ifndef WINDOWS
  /* use idle CPU cycles for mining */
  struct sched_param param;
  param.sched_priority = sched_get_priority_min(SCHED_IDLE);
  sched_setscheduler(0, SCHED_IDLE, &param);
#endif

  
  ThreadArgs *targs = (ThreadArgs *) args;
#ifndef CPU_ONLY
  bool use_gpu = Opts::get_instance()->has_use_gpu(); 
#endif

  mpz_t mpz_hash;
  mpz_init(mpz_hash);
#ifndef CPU_ONLY
  uint8_t hash_prev_block[SHA256_DIGEST_LENGTH];
#endif

  while (*targs->running) {
    
    pthread_mutex_lock(&mutex);
    targs->header->get_hash(mpz_hash);
#ifndef CPU_ONLY
    memcpy(hash_prev_block,
           targs->header->hash_prev_block,
           SHA256_DIGEST_LENGTH);
#endif           
    
    /* hash has to be in range (2^255, 2^256) */
    while (mpz_sizeinbase(mpz_hash, 2) != 256) {
      targs->header->nonce += targs->n_threads;
      targs->header->get_hash(mpz_hash);
    }

    pthread_mutex_unlock(&mutex);

    /* run the sieve */
    PoW pow(mpz_hash, 
            targs->header->shift, 
            NULL, 
            targs->header->target, 
            targs->header->nonce);

#ifndef CPU_ONLY
    if (use_gpu)
      targs->hsieve->run_sieve(&pow, NULL, hash_prev_block);
    else
#endif
      targs->sieve->run_sieve(&pow, NULL);

    pthread_mutex_lock(&mutex);
    targs->header->nonce += targs->n_threads;
    pthread_mutex_unlock(&mutex);
  }
  
  mpz_clear(mpz_hash);

#ifndef CPU_ONLY
  if (use_gpu)
    delete targs->hsieve;
  else
#endif
    delete targs->sieve;

  return NULL;
}

/**
 * returns the average primes per seconds
 */
double Miner::avg_primes_per_sec() {
  
  if (!is_started) return 0;
  
  double apps = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      apps += args[i]->hsieve->avg_primes_per_sec();
    else
#endif
      apps += args[i]->sieve->avg_primes_per_sec();

  return apps;
}


/**
 * returns the average 10 gaps per hour
 */
double Miner::avg_gaps10_per_hour() {
  
  if (!is_started) return 0;
  
  double ag10 = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      ag10 += args[i]->hsieve->avg_gaps10_per_hour();
    else
#endif    
      ag10 += args[i]->sieve->avg_gaps10_per_hour();

  return ag10;
}

/**
 * returns the average 15 gaps per hour
 */
double Miner::avg_gaps15_per_hour() {
  
  if (!is_started) return 0;
  
  double ag15 = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      ag15 += args[i]->hsieve->avg_gaps15_per_hour();
    else
#endif
      ag15 += args[i]->sieve->avg_gaps15_per_hour();

  return ag15;
}

/**
 * returns the primes per seconds
 */
double Miner::primes_per_sec() {
  
  if (!is_started) return 0;
  
  double pps = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      pps += args[i]->hsieve->primes_per_sec();
    else
#endif    
      pps += args[i]->sieve->primes_per_sec();

  return pps;
}


/**
 * returns the 10 gaps per hour
 */
double Miner::gaps10_per_hour() {
  
  if (!is_started) return 0;
  
  double g10 = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      g10 += args[i]->hsieve->gaps10_per_hour();
    else
#endif    
      g10 += args[i]->sieve->gaps10_per_hour();

  return g10;
}

/**
 * returns the 15 gaps per hour
 */
double Miner::gaps15_per_hour() {
  
  if (!is_started) return 0;
  
  double g15 = 0;
  for (int i = 0; i < n_threads; i++)
#ifndef CPU_ONLY
    if (use_gpu)
      g15 += args[i]->hsieve->gaps15_per_hour();
    else
#endif    
      g15 += args[i]->sieve->gaps15_per_hour();

  return g15;
}

/**
 * returns whether this is running
 */
bool Miner::started() { return is_started && running; }
