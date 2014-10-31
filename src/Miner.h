/**
 * Header file of a Gapcoin miner
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
#ifndef __MINER_H__
#define __MINER_H__
#include <pthread.h>
#include "BlockHeader.h"


class Miner {

  public:

    /* create a new miner */
    Miner(uint64_t sieve_size, 
          uint64_t sieve_primes, 
          int n_threads, 
          uint64_t interval = 60000000);

    /* delete a miner */
    ~Miner();

    /* start processing */
    void start(BlockHeader *header);

    /* stops all threads and waits until they are finished */
    void stop();

    /* updates the BlockHeader for all threads */
    bool update_header(BlockHeader *header);

    /**
     * returns the average primes per seconds
     */
    double avg_primes_per_sec();

    /**
     * returns the average 10 gaps per hour
     */
    double avg_gaps10_per_hour();

    /**
     * returns the average 15 gaps per hour
     */
    double avg_gaps15_per_hour();

    /**
     * returns the primes per seconds
     */
    double primes_per_sec();

    /**
     * returns the 10 gaps per hour
     */
    double gaps10_per_hour();

    /**
     * returns the 15 gaps per hour
     */
    double gaps15_per_hour();

    /**
     * returns wether this is running
     */
    bool started();

  private:

    /* sieve size */
    uint64_t sieve_size;

    /* sieve primes */
    uint64_t sieve_primes;

    /* number of threads */
    int n_threads;

    /* indicates if this should run */
    bool running;

    /* current primes per second */
    double *pps;

    /* current gaps > 10 per hour */
    double *gaps10ph;

    /* current gaps > 15 per hour */
    double *gaps15ph;

    /* overall primes per second */
    double *avg_pps;

    /* overall gaps > 10 per hour */
    double *avg_gaps10ph;

    /* overall gaps > 15 per hour */
    double *avg_gaps15ph;

    /* stats interval */
    uint64_t interval;

    /* synchronization mutex */
    static pthread_mutex_t mutex;       

    /* the threads of this */
    pthread_t *threads;
    
    /* thread arguments */
    class ThreadArgs {
      
      public:

        /* id of this thread */
        int id;

        /* number of threads */
        int n_threads;

        /* sieve size */
        uint64_t sieve_size;
       
        /* sieve primes */
        uint64_t sieve_primes;

        /* current primes per second */
        double *pps;
        
        /* current gaps > 10 per hour */
        double *gaps10ph;
        
        /* current gaps > 15 per hour */
        double *gaps15ph;
        
        /* overall primes per second */
        double *avg_pps;
        
        /* overall gaps > 10 per hour */
        double *avg_gaps10ph;
        
        /* overall gaps > 15 per hour */
        double *avg_gaps15ph;

        /* stats interval */
        uint64_t interval;

        /* indicates if this should run */
        bool *running;

        /* the Block header to mine for */
        BlockHeader *header;

        /* create a new ThreadArgs */
        ThreadArgs(int id, 
                   int n_threads,
                   uint64_t sieve,
                   uint64_t sieve_primes,
                   double *pps,
                   double *gaps10ph,
                   double *gaps15ph,
                   double *avg_pps,
                   double *avg_gaps10ph,
                   double *avg_gaps15ph,
                   uint64_t interval,
                   bool *running, 
                   BlockHeader *header);
    };

    /* the thread args of this */
    ThreadArgs **args;

    /* the actual miner thread */
    static void *miner(void *args);
};

#endif /* __MINER_H__ */
