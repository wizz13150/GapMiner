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
#include "Stratum.h"

using namespace std;

/* indicates if program should continue running */
static bool running = true;

/* indicates that we are waiting for gapcoind */
static bool waiting = false;

/* the miner */
static Miner *miner;

#ifndef WINDOWS
/**
 * signal handler to exit program softly
 */
void soft_shutdown(int signum) {

  (void) signum;
  
  static int shutdown = 0;
  running = false;
  waiting = true;

  if (shutdown == 0) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "shutdown.." << endl;
    pthread_mutex_unlock(&io_mutex);
  } else if (shutdown == 1) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "I'm on it, just wait! (press again to kill)" << endl;
    pthread_mutex_unlock(&io_mutex);
  }
  miner->stop();

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
#endif

/* periodically look if new work is available */
void *getwork_thread(void *arg) {
  
  Miner *miner = (Miner *) arg;
  Opts  *opts  = Opts::get_instance();

  Rpc *rpc            = Rpc::get_instance();
  BlockHeader *header = rpc->getwork();

  while (header == NULL) {
    waiting = true;
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "waiting for gapcoind ..." << endl;
    pthread_mutex_unlock(&io_mutex);
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
  time_t work_time = time(NULL);

  while (running) {
    sleep(sec);

    BlockHeader *new_header = rpc->getwork();

    while (new_header == NULL) {
      waiting = true;

      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "waiting for gapcoind ..." << endl;
      pthread_mutex_unlock(&io_mutex);

      sleep(sec);

      if (!running) return NULL;
      new_header = rpc->getwork();
      waiting = false;
    }
  
    new_header->shift       = shift;

    if (!header->equal_block_height(new_header) || time(NULL) >= work_time + 180) {
      miner->update_header(new_header);

      delete header;
      header = new_header;
      time(&work_time);

      if (!opts->has_quiet()) {
        pthread_mutex_lock(&io_mutex);
        cout << get_time() << "Got new target: ";
        cout << (((double) header->difficulty) / TWO_POW48) << endl;
        pthread_mutex_unlock(&io_mutex);
      }
    }
  }

  return NULL;
}

int main(int argc, char *argv[]) {

  Opts *opts = Opts::get_instance(argc, argv);

  if (opts->has_license()) {
    cout << "    GapMiner is a standalone Gapcoin (GAP) CPU rpc miner                 " << endl;
    cout << "                                                                         " << endl;
    cout << "    Copyright (C)  2014  The Gapcoin developers  <info@gapcoin.org>      " << endl;
    cout << "                                                                         " << endl;
    cout << "    This program is free software: you can redistribute it and/or modify " << endl;
    cout << "    it under the terms of the GNU General Public License as published by " << endl;
    cout << "    the Free Software Foundation, either version 3 of the License, or    " << endl;
    cout << "    (at your option) any later version.                                  " << endl;
    cout << "                                                                         " << endl;
    cout << "    This program is distributed in the hope that it will be useful,      " << endl;
    cout << "    but WITHOUT ANY WARRANTY; without even the implied warranty of       " << endl;
    cout << "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        " << endl;
    cout << "    GNU General Public License for more details.                         " << endl;
    cout << "                                                                         " << endl;
    cout << "    You should have received a copy of the GNU General Public License    " << endl;
    cout << "    along with this program.  If not, see <http://www.gnu.org/licenses/>." << endl;
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

#ifndef WINDOWS
  init_signal();
#endif


  /* 1 thread default */
  int n_threads = (opts->has_threads() ? atoi(opts->get_threads().c_str()) : 1);

  /* default 5 sec timout */
  int timeout = (opts->has_threads() ? atoi(opts->get_timeout().c_str()) : 5);

  /* default shift 20 */
  uint16_t shift = (opts->has_shift() ?  atoi(opts->get_shift().c_str()) : 20);

  /* 10 seconds default */
  unsigned int sec = (opts->has_stats() ? atoll(opts->get_stats().c_str()) : 10);

  string host = opts->get_host();
  string port = opts->get_port();
  string user = opts->get_user();
  string pass = opts->get_pass();

  string http = (opts->get_host().find("http://") == 0) ? string("") : 
                                                          string("http://");

  if (!opts->has_stratum()) {
    Rpc::init_curl(user + string(":") + pass, 
                  http + host + string(":") + port, 
                  timeout);
  }

  uint64_t sieve_size = (opts->has_sievesize() ? 
                         atoll(opts->get_sievesize().c_str()) :
                         1048576); 

  uint64_t primes     = (opts->has_primes() ? 
                         atoll(opts->get_primes().c_str()) :
                         500000);

  miner = new Miner(sieve_size, primes, n_threads, ((uint64_t) sec) * 1000000L);
  pthread_t thread;

  if (opts->has_stratum()) {
    Stratum::get_instance(&host, &port, &user, &pass, shift, miner);
  } else {
    pthread_create(&thread, NULL, getwork_thread, (void *) miner);
  }

  
  /* print status information while mining */
  while (running) {
    sleep(sec);

    if (!opts->has_quiet() && !waiting) {
      pthread_mutex_lock(&io_mutex);
      cout << get_time();
      cout << "pps: "    << (int) miner->primes_per_sec();
      cout << " / "      << (int) miner->avg_primes_per_sec();
      cout << "  10g/h " << miner->gaps10_per_hour();
      cout << " / "      << miner->avg_gaps10_per_hour();
      cout << "  15g/h " << miner->gaps15_per_hour();
      cout << " / "      << miner->avg_gaps15_per_hour() << endl;
      pthread_mutex_unlock(&io_mutex);
    }
  }

  if (!opts->has_stratum())
    pthread_join(thread, NULL);

  delete miner;

  return 0;
}
