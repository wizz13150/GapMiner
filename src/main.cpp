/**
 * GapMiners main 
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
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "BlockHeader.h"
#include "Miner.h"
#include "PoWCore/src/PoWUtils.h"
#include "Opts.h"
#include "Rpc.h"
#include "verbose.h"

/* indicates if program should continue running */
static bool running = true;

/* indicates that we are waiting for gapcoind */
static bool waiting = false;


/**
 * signal handler to exit program softly
 */
void soft_shutdown(int signum) {

  (void) signum;
  
  static int shutdown = 0;
  running = false;
  waiting = true;

  if (shutdown == 0)
    info_msg("shutdown..\n");
  else if (shutdown == 1)
    info_msg("I'm on it, just wait! (press again to kill)\n");

  if (shutdown >= 2)
    kill(0, SIGKILL);

  shutdown++;
}

/* init signal handler */
void init_signal() {

  /* set signal handler for soft shutdown */
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));

  action.sa_handler = soft_shutdown;
  sigaction(SIGINT,  &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGALRM, &action, NULL);

  /* continue mining if terminal lost connection */
  action.sa_handler = SIG_IGN;
  sigaction(SIGHUP,  &action, NULL);
  sigaction(SIGPIPE, &action, NULL); 
  sigaction(SIGQUIT, &action, NULL);
}

/* periodically look if new work is available */
void *getwork_thread(void *arg) {
  
  Miner *miner = (Miner *) arg;
  Opts  *opts  = Opts::get_instance();

  Rpc *rpc            = Rpc::get_instance();
  BlockHeader *header = rpc->getwork();

  while (header == NULL) {
    waiting = true;
    error_msg("waiting for gapcoind ...\n");
    sleep(5);

    if (!running) return NULL;
    header = rpc->getwork();
    waiting = false;
  }

  uint16_t shift = (opts->has_shift() ?  atoi(opts->get_shift().c_str()) : 20);
  header->shift  = shift;
  

  /* start mining */
  miner->start(header);

  /* 5 seconds default */
  unsigned int sec = (opts->has_pull() ? atoll(opts->get_pull().c_str()) : 5);

  while (running) {
    sleep(sec);

    BlockHeader *new_header = rpc->getwork();

    while (new_header == NULL) {
      waiting = true;
      error_msg("waiting for gapcoind ...\n");
      sleep(5);

      if (!running) return NULL;
      new_header = rpc->getwork();
      waiting = false;
    }
  
    new_header->shift       = shift;

    if (!header->equal_block_height(new_header)) {
      miner->update_header(new_header);

      delete header;
      header = new_header;

      if (!opts->has_quiet())
        info_msg("Got new target: %0.10F\n", 
                 ((double) header->difficulty) / TWO_POW48);
    }
  }

  return NULL;
}

int main(int argc, char *argv[]) {

  Opts *opts = Opts::get_instance(argc, argv);

  if (opts->has_license()) {
    cout << "    GapMiner is a standalone Gapcoin (GAP) CPU rpc miner                 \n";
    cout << "                                                                         \n";
    cout << "    Copyright (C)  2014  The Gapcoin developers  <info@gapcoin.org>      \n";
    cout << "                                                                         \n";
    cout << "    This program is free software: you can redistribute it and/or modify \n";
    cout << "    it under the terms of the GNU General Public License as published by \n";
    cout << "    the Free Software Foundation, either version 3 of the License, or    \n";
    cout << "    (at your option) any later version.                                  \n";
    cout << "                                                                         \n";
    cout << "    This program is distributed in the hope that it will be useful,      \n";
    cout << "    but WITHOUT ANY WARRANTY; without even the implied warranty of       \n";
    cout << "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        \n";
    cout << "    GNU General Public License for more details.                         \n";
    cout << "                                                                         \n";
    cout << "    You should have received a copy of the GNU General Public License    \n";
    cout << "    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n";
    exit(EXIT_SUCCESS);
  }

  if (opts->has_help()  ||
      !opts->has_host() ||
      !opts->has_port() ||
      !opts->has_user() ||
      !opts->has_pass()) {
    
    cout << opts->get_help();
    exit(EXIT_FAILURE);
  }

  /* exit without failure */
  printf("Congratulations, you are well prepared for the launch.\n");
  exit(EXIT_SUCCESS);

  init_signal();


  /* 1 thread default */
  int n_threads = (opts->has_threads() ? atoi(opts->get_threads().c_str()) : 1);

  /* 10 seconds default */
  unsigned int sec = (opts->has_stats() ? atoll(opts->get_stats().c_str()) : 10);


  string http = (opts->get_host().find("http://") == 0) ? string("") : string("http://");
  Rpc::init_curl(opts->get_user() + string(":") + opts->get_pass(),
                 http + opts->get_host() + string(":") + opts->get_port());

  uint64_t sieve_size = (opts->has_sievesize() ? 
                         atoll(opts->get_sievesize().c_str()) :
                         1048576); 

  uint64_t primes     = (opts->has_primes() ? 
                         atoll(opts->get_primes().c_str()) :
                         500000);

  Miner miner(sieve_size, primes, n_threads, ((uint64_t) sec) * 1000000L);

  pthread_t thread;
  pthread_create(&thread, NULL, getwork_thread, (void *) &miner);

  
  /* print status information while mining */
  while (running) {
    sleep(sec);

    if (!opts->has_quiet() && !waiting) {
      info_msg("pps: %d / %d  10g/h %.4F / %.4F  15g/h %.4F / %.4F\n",
               (int) miner.primes_per_sec(), 
               (int) miner.avg_primes_per_sec(), 
               miner.gaps10_per_hour(), 
               miner.avg_gaps10_per_hour(), 
               miner.gaps15_per_hour(), 
               miner.avg_gaps15_per_hour());
    }
  }

  pthread_join(thread, NULL);
  miner.stop();

  return 0;
}
