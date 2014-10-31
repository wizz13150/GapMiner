/**
 * Implementation of GapMiners rpc interface to gapcoind
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
#include "Rpc.h"
#include <jansson.h>
#include "verbose.h"

using namespace std;

/* synchronization mutexes */
pthread_mutex_t Rpc::send_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Rpc::creation_mutex = PTHREAD_MUTEX_INITIALIZER;


/* this will be a singleton */
Rpc *Rpc::only_instance = NULL;

/* indicates if this was initialized */
bool Rpc::initialized = false;

/* indicates if curl was initialized */
bool Rpc::curl_initialized = false;

/* string stream for receiving */
stringstream *Rpc::recv_ss = new stringstream;

/* curl session handle */
CURL *Rpc::curl = NULL;

/* get work rpc command */
string Rpc::getwork_rpccmd = "{\"jsonrpc\": \"1.0\", "
                             "\"id\":\"" USER_AGENT "\", "
                             "\"method\": \"getwork\", "
                             "\"params\": [] }";

/**
 * curl callback function to send rpc commands to gapcoind
 */
size_t curl_read(void *ptr, size_t size, size_t nmemb, void *user_data) {

  static unsigned int pos = 0;
  string *rpccmd = (string *) user_data;
  unsigned int len = rpccmd->length();

  if (pos < len) {
    unsigned int bytes = (size * nmemb >= len - pos) ? len - pos : size * nmemb;
    memcpy(ptr, rpccmd->c_str() + pos, bytes);
    
    pos += bytes;
    return bytes;
  }
  
  pos = 0;
  return 0;
}

/**
 * curl callback function to receive rpc responses from gapcoind
 */
size_t curl_write(char *ptr, size_t size, size_t nmemb, void *user_data) {
  stringstream *ss = (stringstream *) user_data;
  
  if ((size * nmemb) > 0)
    ss->write(ptr, size * nmemb);

  return size * nmemb;
}

/**
 * initialize curl with the given 
 * username, password, url and port
 */
bool Rpc::init_curl(string userpass, string url, int timeout) {

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  	curl_easy_setopt(curl, CURLOPT_ENCODING, "");
	  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
	  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
	  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) recv_ss);
  	curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_read);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	  curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	  curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpass.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

    curl_initialized = true;
    return true;
  }
  
  return false;
}

/* return the only instance of this */
Rpc *Rpc::get_instance() {

  pthread_mutex_lock(&creation_mutex);
  if (!initialized) {
    only_instance = new Rpc();
    initialized   = true;
  }
  pthread_mutex_unlock(&creation_mutex);

  return (curl_initialized ? only_instance : NULL);
}

/* private constructor */
Rpc::Rpc() { }

/* destructor */
Rpc::~Rpc() { }


/**
 * Request new Work from gapcoin daemon
 */
BlockHeader *Rpc::getwork() {

  /* building http header */
	char content_len[64];
  struct curl_slist *header = NULL;

	sprintf(content_len, "Content-Length: %lu", getwork_rpccmd.length());
	header = curl_slist_append(header, "Content-Type: application/json");
	header = curl_slist_append(header, content_len);
	header = curl_slist_append(header, "User-Agent: " USER_AGENT);
	header = curl_slist_append(header, "Accept:"); /* disable Accept hdr*/
	header = curl_slist_append(header, "Expect:"); /* disable Expect hdr*/

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &getwork_rpccmd);
	  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
  } else
    return NULL;

  CURLcode res;

  /* Perform the request, res will get the return code */ 
  res = curl_easy_perform(curl);

  /* Check for errors */ 
  if(res != CURLE_OK) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "curl_easy_perform() failed: "; 
    cout << curl_easy_strerror(res) << endl;
    pthread_mutex_unlock(&io_mutex);
    return NULL;
  }


  /* parse response */
  json_t *root, *tdiff;
  json_error_t error;

  root = json_loads(recv_ss->str().c_str(), 0, &error);
  recv_ss->str("");

  if(!root) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "jansson error: on line " << error.line;
    cout << ": " << error.text << endl;
    pthread_mutex_unlock(&io_mutex);
    return NULL;
  }

  if (!json_is_object(root)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind response" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(root);
    return NULL;
  }
  root = json_object_get(root, "result");

  if (!json_is_object(root)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind response" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(root);
    return NULL;
  }
  tdiff = json_object_get(root, "difficulty");
  if (!json_is_integer(tdiff)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind difficulty" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(tdiff);
    return NULL;
  }

  uint64_t nDiff = json_number_value(tdiff);
  json_decref(tdiff);

  root = json_object_get(root, "data");
  if (!json_is_string(root)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind response" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(root);
    return NULL;
  }
  string data = json_string_value(root);
  json_decref(root);

  BlockHeader *head = new BlockHeader(&data);
  head->target = nDiff;
  return head;
}

/**
 * send processed work to the gapcoin daemon
 * returns true if share was accepted
 */
bool Rpc::sendwork(BlockHeader *header) {

  stringstream data;
  data << "{\"jsonrpc\": \"1.0\", ";
  data << "\"id\":\"" USER_AGENT "\", ";
  data << "\"method\": \"getwork\", ";
  data << "\"params\": [\"";
  data << header->get_hex();
  data << "\"] }";
  
  /* building http header */
	char content_len[64];
  struct curl_slist *httphead = NULL;
  string hex = data.str();

	sprintf(content_len, "Content-Length: %lu", hex.length());
	httphead = curl_slist_append(httphead, "Content-Type: application/json");
	httphead = curl_slist_append(httphead, content_len);
	httphead = curl_slist_append(httphead, "User-Agent: " USER_AGENT);
	httphead = curl_slist_append(httphead, "Accept:"); /* disable Accept hdr*/
	httphead = curl_slist_append(httphead, "Expect:"); /* disable Expect hdr*/

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &hex);
	  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httphead);
  } else
    return false;

  CURLcode res;

  /* Perform the request, res will get the return code */ 
  res = curl_easy_perform(curl);

  /* Check for errors */ 
  if(res != CURLE_OK) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "curl_easy_perform() failed: ";
    cout << curl_easy_strerror(res) << endl;
    pthread_mutex_unlock(&io_mutex);
    return false;
  }


  /* parse the response */
  json_t *root;
  json_error_t error;

  root = json_loads(recv_ss->str().c_str(), 0, &error);
  recv_ss->str("");

  if(!root) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "jansson error: on line " << error.line;
    cout << ": " << error.text << endl;
    pthread_mutex_unlock(&io_mutex);
    return NULL;
  }

  if (!json_is_object(root)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind response" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(root);
    return NULL;
  }
  root = json_object_get(root, "result");

  if (!json_is_boolean(root)) {
    pthread_mutex_lock(&io_mutex);
    cout << get_time() << "can not parse gapcoind response" << endl;
    pthread_mutex_unlock(&io_mutex);
    json_decref(root);
    return NULL;
  }
  bool result = json_is_true(root);
  json_decref(root);

  return result;
}
