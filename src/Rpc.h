/**
 * Header  of GapMiners rpc interface to gapcoind
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
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <sstream>
#include "BlockHeader.h"

#define USER_AGENT "GapMiner"

class Rpc {

  public:

    /* return the only instance of this */
    static Rpc *get_instance();

    /* run clean up */
    static void cleanup();

    /**
     * Request new Work from the gapcoin daemon
     */
    BlockHeader *getwork();

    /**
     * send processed work to the gapcoin daemon
     * returns true if share was accepted
     */
    bool sendwork(BlockHeader *header);

    /**
     * initialize curl with the given 
     * username, password, url, and port
     */
    static bool init_curl(string userpass, string url);

  private:

    /* curl session handle */
    static CURL *curl;

    /* private constructor to allow only one instance */
    Rpc();

    /* private destructor */
    ~Rpc();

    /* synchronization mutexes */
    static pthread_mutex_t send_mutex;       
    static pthread_mutex_t creation_mutex;

    /* this will be a singleton */
    static Rpc *only_instance;

    /* indicates if this was initialized */
    static bool initialized;

    /* indicates if curl was initialized */
    static bool curl_initialized;

    /* get work rpc command */
    static string getwork_rpccmd;

    /* rpc timeout */
    static int timeout;

    /* string stream for receiving */
    static stringstream *recv_ss;
    
    /* curl read function */
    static size_t getwork(void *ptr, 
                          size_t size, 
                          size_t nmemb, 
                          void *user_data);
    

};
