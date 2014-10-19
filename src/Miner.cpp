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

/* synchronization mutex */
pthread_mutex_t Miner::mutex = PTHREAD_MUTEX_INITIALIZER;

/* create a new miner */
Miner::Miner(uint64_t sieve_size, 
             uint64_t sieve_primes, 
             int n_threads,
             uint64_t interval) {

  this->sieve_size   = sieve_size;
  this->sieve_primes = sieve_primes;
  this->n_threads    = n_threads;
  this->interval     = interval;
  this->running      = false;

  pps          = (double *)      calloc(n_threads, sizeof(double));
  gaps10ph     = (double *)      calloc(n_threads, sizeof(double));
  gaps15ph     = (double *)      calloc(n_threads, sizeof(double));
  avg_pps      = (double *)      calloc(n_threads, sizeof(double));
  avg_gaps10ph = (double *)      calloc(n_threads, sizeof(double));
  avg_gaps15ph = (double *)      calloc(n_threads, sizeof(double));
  threads      = (pthread_t *)   calloc(n_threads, sizeof(pthread_t));
  args         = (ThreadArgs **) calloc(n_threads, sizeof(ThreadArgs *));
}              

/* start processing */
void Miner::start(BlockHeader *header) {

  running = true;
  ShareProcessor::get_processor()->update_header(header);

  for (int i = 0; i < n_threads; i++) {
    
    args[i] = new ThreadArgs(i, 
                             n_threads, 
                             sieve_size, 
                             sieve_primes, 
                             pps,
                             gaps10ph,
                             gaps15ph,
                             avg_pps,
                             avg_gaps10ph,
                             avg_gaps15ph,
                             interval,
                             &running, 
                             header);
    
    pthread_create(&threads[i], NULL, miner, (void *) args[i]);
  }
}

/* delete a miner */
Miner::~Miner() {
  stop();

  free(pps);
  free(gaps10ph);
  free(gaps15ph);
  free(avg_pps);
  free(avg_gaps10ph);
  free(avg_gaps15ph);
  free(threads);
  free(args);
}

/* stops all threads and waits until they are finished */
void Miner::stop() {
  if (running) {
    running = false;
    
    for (int i = 0; i < n_threads; i++) {
      pthread_join(threads[i], NULL);
      delete args[i]->header;
    }
  }
}

/* updates the BlockHeader for all threads */
bool Miner::update_header(BlockHeader *header) {
  
  if (!running)
    return false;

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
                              uint64_t sieve_size,
                              uint64_t sieve_primes,
                              double *pps,
                              double *gaps10ph,
                              double *gaps15ph,
                              double *avg_pps,
                              double *avg_gaps10ph,
                              double *avg_gaps15ph,
                              uint64_t interval,
                              bool *running, 
                              BlockHeader *header) {

  this->id            = id;
  this->n_threads     = n_threads;
  this->sieve_size    = sieve_size;
  this->sieve_primes  = sieve_primes;
  this->pps           = pps;
  this->gaps10ph      = gaps10ph;
  this->gaps15ph      = gaps15ph;
  this->avg_pps       = avg_pps;
  this->avg_gaps10ph  = avg_gaps10ph;
  this->avg_gaps15ph  = avg_gaps15ph;
  this->interval      = interval;
  this->running       = running;
  this->header        = header->clone();
  this->header->nonce = id;
}

/* a single mining thread */
void *Miner::miner(void *args) {

  /* use idle CPU cycles for mining */
  struct sched_param param;
  param.sched_priority = sched_get_priority_min(SCHED_IDLE);
  sched_setscheduler(0, SCHED_IDLE, &param);

  
  ThreadArgs *targs = (ThreadArgs *) args;

  mpz_t mpz_hash;
  mpz_init(mpz_hash);

  uint64_t time = PoWUtils::gettime_usec();

  ShareProcessor *share_processor = ShareProcessor::get_processor();
  Sieve sieve((PoWProcessor *) share_processor, 
              targs->sieve_primes, 
              targs->sieve_size);

  while (*targs->running) {
    
    pthread_mutex_lock(&mutex);
    targs->header->get_hash(mpz_hash);
    
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
            targs->header->difficulty, 
            targs->header->nonce);

    sieve.run_sieve(&pow, NULL);

    /* measure speed */
    if ((PoWUtils::gettime_usec() - time) > targs->interval) {

      targs->pps[targs->id]          = sieve.primes_per_sec();
      targs->gaps10ph[targs->id]     = sieve.gaps10_per_hour();
      targs->gaps15ph[targs->id]     = sieve.gaps15_per_hour();
      targs->avg_pps[targs->id]      = sieve.avg_primes_per_sec();
      targs->avg_gaps10ph[targs->id] = sieve.avg_gaps10_per_hour();
      targs->avg_gaps15ph[targs->id] = sieve.avg_gaps15_per_hour();
      time                           = PoWUtils::gettime_usec();
    }

    pthread_mutex_lock(&mutex);
    targs->header->nonce += targs->n_threads;
    pthread_mutex_unlock(&mutex);
  }
  
  mpz_clear(mpz_hash);

  return NULL;
}


/**
 * returns the average primes per seconds
 */
double Miner::avg_primes_per_sec() {
  
  double apps = 0;
  for (int i = 0; i < n_threads; i++)
    apps += avg_pps[i];

  return apps;
}


/**
 * returns the average 10 gaps per hour
 */
double Miner::avg_gaps10_per_hour() {
  
  double ag10 = 0;
  for (int i = 0; i < n_threads; i++)
    ag10 += avg_gaps10ph[i];

  return ag10;
}

/**
 * returns the average 15 gaps per hour
 */
double Miner::avg_gaps15_per_hour() {
  
  double ag15 = 0;
  for (int i = 0; i < n_threads; i++)
    ag15 += avg_gaps15ph[i];

  return ag15;
}

/**
 * returns the primes per seconds
 */
double Miner::primes_per_sec() {
  
  double all_pps = 0;
  for (int i = 0; i < n_threads; i++)
    all_pps += pps[i];

  return all_pps;
}


/**
 * returns the 10 gaps per hour
 */
double Miner::gaps10_per_hour() {
  
  double g10 = 0;
  for (int i = 0; i < n_threads; i++)
    g10 += gaps10ph[i];

  return g10;
}

/**
 * returns the 15 gaps per hour
 */
double Miner::gaps15_per_hour() {
  
  double g15 = 0;
  for (int i = 0; i < n_threads; i++)
    g15 += gaps15ph[i];

  return g15;
}
