/**
 * Stratum protocol implementation for GapMiner
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
#include <pthread.h>
#include <iostream>
#include <boost/asio.hpp>
#include <jansson.h>
#include "Stratum.h"
#include "verbose.h"

using boost::asio::ip::tcp;
using namespace std;


/* synchronization mutexes */
pthread_mutex_t Stratum::creation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Stratum::send_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Stratum::shares_mutex   = PTHREAD_MUTEX_INITIALIZER;

/* the socket of this */
tcp::socket *Stratum::socket = NULL;

/* the server address */
string *Stratum::host = NULL;

/* the server port */
string *Stratum::port = NULL;

/* the user */
string *Stratum::user = NULL;

/* the users password */
string *Stratum::password = NULL;

/* the mining shift */
uint16_t Stratum::shift = 0;

/* the only instance of this */
Stratum *Stratum::only_instance;

/* access or create the only instance of this */
Stratum *Stratum::get_instance(string *host, 
                               string *port, 
                               string *user,
                               string *password,
                               uint16_t shift,
                               Miner *miner) {
  
  pthread_mutex_lock(&creation_mutex);

  /* allow only one creation */
  if (host  != NULL &&
      port  != NULL && 
      miner != NULL && 
      user  != NULL && 
      password != NULL && 
      shift >= 14 &&
      only_instance == NULL) {

    Stratum::host = host;
    Stratum::port = port;
    Stratum::user = user;
    Stratum::password = password;
    Stratum::shift = shift;
 
    init(host, port);

    only_instance = new Stratum(miner);
  }

  pthread_mutex_unlock(&creation_mutex);

  return only_instance;
}

/* initiates the connection to the given host and port */
void Stratum::init(string *host, string *port) {

  bool error;
  do {
    error = false;
    
    try {
      boost::asio::io_service io_service;
  
      tcp::resolver resolver(io_service);
      tcp::resolver::query query(tcp::v4(), *host, *port);
  
      socket = new tcp::socket(io_service);
      if (socket != NULL) {
        socket->open(boost::asio::ip::tcp::v4());
        boost::asio::socket_base::keep_alive option(true);
        socket->set_option(option);

        boost::asio::connect(*socket, resolver.resolve(query));
      } else {
        pthread_mutex_lock(&io_mutex);
        cout << get_time() << "Connection failed: could not create socket";
        cout << endl;
        pthread_mutex_unlock(&io_mutex);

        sleep(5);
        error = true;
      }
    } catch (exception &e) {
  
      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "Connection failed: " << e.what() << endl;
      pthread_mutex_unlock(&io_mutex);
      
      try {
        if (socket != NULL && socket->is_open())
          socket->close();

        if (socket != NULL)
          delete socket;

        socket = NULL;
      } catch (exception &e) {
        pthread_mutex_lock(&io_mutex);
        cout << get_time() << "Closing socket for reconnection failed: ";
        cout << e.what() << endl;
        pthread_mutex_unlock(&io_mutex);
      }

      sleep(5);
      error = true;
    }
  } while (error);
}

/* reinitialize the connection to the server */
void Stratum::reinit() {

  try {
    if (socket != NULL && socket->is_open()) 
      socket->close();

    if (socket != NULL)
      delete socket;

    socket = NULL;
  } catch (exception &e) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "Closing socket for reconnection failed: ";
    cout << e.what() << endl;
    pthread_mutex_unlock(&io_mutex);
  }

  init(host, port);
}

/* creates a new Stratum instance */
Stratum::Stratum(Miner *miner) {

  this->n_msgs = 0;
  this->targs = new ThreadArgs(miner, &shares);
  pthread_create(&thread, NULL, recv_thread, targs);

  getwork();
}

Stratum::~Stratum() {
  
  targs->running = false;
  pthread_join(thread, NULL);
  
  delete targs;
}

Stratum::ThreadArgs::ThreadArgs(Miner *miner, map<int, double> *shares) {
  this->miner   = miner;
  this->shares  = shares;
  this->running = true;
}

/**
 * Thread that listens for new messages from the server.
 * it updates miners, and prints share information
 *
 * Messages that can be received by this:
 *
 * Block notification:
 *   "{ "id": null, "method": "blockchain.block.new", "params": 
 *      { "data": <block hex data to solve>, 
 *      "difficulty": <target difficulty> } }"
 *
 *   This message informs the clients that a new block arrived and they should
 *   update their mining work.
 *
 * Getwork response:
 *   "{ "id": <id of the request>, "result": { "data": <block data to solve>,    
 *      "difficulty": <target difficulty> }, "error": <null or errors string> }"
 *
 *   This message send the current block data when the client asks for a
 *   work update.
 *
 * Sendwork response:
 *   "{ "id": <id of the share>, "result": <true/false>,
 *      "error": <null or errors string>}"
 *
 *   This message sends the result for a submitted share, which can be
 *   accepted (true) ore stale (false)
 * 
 * Note: "error" is currently ignored.
 */
void *Stratum::recv_thread(void *arg) {
  
  ThreadArgs *targs = (ThreadArgs *) arg;
  Miner *miner = targs->miner;
  map<int, double> *shares = targs->shares;

  boost::asio::streambuf buffer;

  while (targs->running) {
    
    /* receive message from server */
    try {
      boost::asio::read_until(*socket, buffer, '\n');
    } catch (exception &e) {

      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "Error receiving message form server: ";
      cout << e.what() << endl;
      pthread_mutex_unlock(&io_mutex);

      /* reset connection */
      reinit();
      get_instance()->getwork();
    }

    json_t *root;
    json_error_t error;
 
    /* parse message */
    istream is(&buffer);
    string msg;
    getline(is, msg);

    root = json_loads(msg.c_str(), 0, &error);
 
    if(!root) {
      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "jansson error: on line " << error.line;
      cout << ": " << error.text << endl;
      pthread_mutex_unlock(&io_mutex);
      continue;
    }
 
    if (!json_is_object(root)) {
      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "can not parse server response" << endl;
      pthread_mutex_unlock(&io_mutex);
      json_decref(root);
      continue;
    }
    json_t *j_id = json_object_get(root, "id");
 
    /* parse response */
    if (json_is_integer(j_id)) {
      int id = json_number_value(j_id);

      json_t *result = json_object_get(root, "result");

      /* share response */
      if (json_is_boolean(result)) {
        process_share(shares, id, json_is_true(result));

      /* getwork response */
      } else if (json_is_object(result)) {
        parse_block_work(miner, result);

      } else {
        pthread_mutex_lock(&io_mutex);
        cout << get_time() << "can not parse server response" << endl;
        pthread_mutex_unlock(&io_mutex);
      }
      json_decref(root);

    /* block notify message */
    } else {
      json_t *params = json_object_get(root, "params");
      
      if (json_is_object(params)) {
        parse_block_work(miner, params);

      } else {
        pthread_mutex_lock(&io_mutex);
        cout << get_time() << "can not parse server response" << endl;
        pthread_mutex_unlock(&io_mutex);
      }
      json_decref(root);
    }
  }

  return NULL;
}

/* helper function which processes an response share */
void Stratum::process_share(map<int, double> *shares, int id, bool accepted) {
  
  pthread_mutex_lock(&shares_mutex);
  bool share_not_found = (shares->find(id) == shares->end());
  pthread_mutex_unlock(&shares_mutex);

  if (share_not_found) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "Received invalid server response" << endl;
    pthread_mutex_unlock(&io_mutex);
    return;
  }
  

  pthread_mutex_lock(&io_mutex);
  cout.precision(4);
  cout << get_time();
  cout << "Found Share: " << fixed << shares->at(id);
  cout << "  =>  " <<  (accepted ? "accepted" : "stale!");
  cout << endl;
  pthread_mutex_unlock(&io_mutex);

  pthread_mutex_lock(&shares_mutex);
  shares->erase(id);
  pthread_mutex_unlock(&shares_mutex);
}

/* helper function to parse a json block work in the form of:
 * "{ "data": <block data to solve>, "difficulty": <target difficulty> }"
 */
void Stratum::parse_block_work(Miner *miner, json_t *result) {

  json_t *tdiff;

  tdiff  = json_object_get(result, "difficulty");
  result = json_object_get(result, "data");

  /* parse difficulty */
  if (!json_is_integer(tdiff)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse server difficulty" << endl;
    pthread_mutex_unlock(&io_mutex);
    return;
  }

  uint64_t nDiff = json_number_value(tdiff);

  /* parse block data */
  if (!json_is_string(result)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse server block data" << endl;
    pthread_mutex_unlock(&io_mutex);
    return;
  }
  string data = json_string_value(result);
  
  BlockHeader head(&data);
  head.target = nDiff;
  head.shift  = shift;

  /* update work */
  if (miner->started())
    miner->update_header(&head);
  else
    miner->start(&head);

  pthread_mutex_lock(&io_mutex);
  cout.precision(7);
  cout << get_time() << "Got new target: ";
  cout << fixed << (((double) head.difficulty) / TWO_POW48) << endl;
  pthread_mutex_unlock(&io_mutex);
}

/**
 * sends a given BlockHeader to the server 
 * with a stratum request, the response should
 * tell if the share was accepted or not.
 *
 * The format should be:
 *   "{ "id": <id of the share>, "result": <true/false>,
 *      "error": <null or errors string> }"
 */
bool Stratum::sendwork(BlockHeader *header) {

  stringstream ss;
  ss << "{\"id\": " << n_msgs;
  ss << ", \"method\": \"blockchain.block.submit\", \"params\": ";
  ss << "{ \"user\": \"" << *user;

  /* not optimal password should be hashed */
  ss << "\", \"password\": \"" << *password  << "\", ";
  ss << "\"data\": \"" << header->get_hex() << "\" } }\n";

  bool error;
  do {
    error = false;

    try {
      pthread_mutex_lock(&send_mutex);
      socket->send(boost::asio::buffer((ss.str()))); 
      pthread_mutex_unlock(&send_mutex);

    } catch (exception &e) {

      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "Submitting share failed" << e.what() << endl;
      pthread_mutex_unlock(&io_mutex);
      error = true;
    }

    /* force reconnect */
    if (error) {
      reinit();
    }
  } while (error);

  pthread_mutex_lock(&shares_mutex);
  shares[n_msgs] = ((double) header->difficulty) / TWO_POW48;
  n_msgs++;
  pthread_mutex_unlock(&shares_mutex);

  return true;
}

/**
 * request new work, return is always NULL, 
 * because it handles response internally .
 *
 * The stratum response for this request should be: 
 *   "{ "id": <id of the request>, "result": 
 *      { "data": <block hex data to solve>,    
 *        "difficulty": <target difficulty> }, 
 *        "error": <null or errors string> }"
 */
BlockHeader *Stratum::getwork() {
  
  stringstream ss;
  ss << "{\"id\": " << n_msgs;
  ss << ", \"method\": \"blockchain.block.request\", \"params\": ";
  ss << "{ \"user\": \"" << *user;

  /* not optimal password should be hashed */
  ss << "\", \"password\": \"" << *password  << "\" } }\n";

  bool error;
  do {
    error = false;

    try {
      pthread_mutex_lock(&send_mutex);
      socket->send(boost::asio::buffer((ss.str()))); 
      pthread_mutex_unlock(&send_mutex);

    } catch (exception &e) {

      pthread_mutex_lock(&io_mutex);
      cout << get_time() << "Requesting work failed" << e.what() << endl;
      pthread_mutex_unlock(&io_mutex);
      error = true;
    }

    /* force reconnect */
    if (error) {
      reinit();
    }
  } while (error);

  pthread_mutex_lock(&shares_mutex);
  n_msgs++;
  pthread_mutex_unlock(&shares_mutex);
  return NULL;
}
