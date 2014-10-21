/**
 * Implementation of the Gapcoin block header format
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
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>
#include "BlockHeader.h"
#include "PoWCore/src/PoWUtils.h"

using namespace std;

/* creates an empty block header */
BlockHeader::BlockHeader() {
  set_null();
}

/**
 * creates a new Block header out of the given hex encoded string 
 * (little endian format)
 */
BlockHeader::BlockHeader(string *hex) {

  /* clear block header */
  set_null();
 
  uint32_t i, j;
  for (i = 0, j = 0; i < 8 && i < hex->length(); i += 2)
    set_from_hex(version, i / 2, hex, i);

  j = i;
  for (; i < j + 2 * SHA256_DIGEST_LENGTH && i < hex->length(); i += 2) 
    hash_prev_block[(i - j) / 2] = hex_to_byte(hex, i);
    
  j = i;
  for (; i < j + 2 * SHA256_DIGEST_LENGTH && i < hex->length(); i += 2) 
    hash_merkle_root[(i - j) / 2] = hex_to_byte(hex, i);
    
  j = i;
  for (; i < j + 8 && i < hex->length(); i += 2) 
    set_from_hex(time, (i - j) / 2, hex, i);

  j = i;
  for (; i < j + 16 && i < hex->length(); i += 2) 
    set_from_hex(difficulty, (i - j) / 2, hex, i);

  j = i;
  for (; i < j + 8 && i < hex->length(); i += 2) 
    set_from_hex(nonce, (i - j) / 2, hex, i);

  j = i;
  for (; i < j + 4 && i < hex->length(); i += 2) 
    set_from_hex(shift, (i - j) / 2, hex, i);

  j = i;
  for (; i < hex->length(); i += 2) {
    uint8_t byte;
    set_from_hex(byte, 0, hex, i);

    adder.push_back(byte);
  }
  
  /* if we are big endian swap byte order */
  if (!have_little_endian())
    switch_byte_oder(this);
}

/**
 * returns the block header as an hex encoded string
 * (in little endian byte order)
 */
string BlockHeader::get_hex() {
  BlockHeader *header = clone();
  
  if (!have_little_endian())
    switch_byte_oder(header);
  
  string hex;

  for (uint32_t i = 0; i < 4; i++)
    push_hex(hex, header->version, i);

  for (uint32_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
    ary_push_hex(hex, header->hash_prev_block, i);

  for (uint32_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
    ary_push_hex(hex, header->hash_merkle_root, i);

  for (uint32_t i = 0; i < 4; i++)
    push_hex(hex, header->time, i);

  for (uint32_t i = 0; i < 8; i++)
    push_hex(hex, header->difficulty, i);

  for (uint32_t i = 0; i < 4; i++)
    push_hex(hex, header->nonce, i);

  for (uint32_t i = 0; i < 2; i++)
    push_hex(hex, header->shift, i);

  for (uint32_t i = 0; i < header->adder.size(); i++)
    ary_push_hex(hex, header->adder, i);

  /* set adder = 0 if no adder exists */
  if (header->adder.size() <= 0) {
    hex.push_back('0');
    hex.push_back('0');
  }

  delete header;
  return hex;
}

/* clones this */
BlockHeader *BlockHeader::clone() {
  BlockHeader *header = new BlockHeader;

  header->version    = version;
  header->time       = time;
  header->difficulty = difficulty;
  header->nonce      = nonce;
  header->shift      = shift;

  memcpy(header->hash_prev_block, hash_prev_block, SHA256_DIGEST_LENGTH);
  memcpy(header->hash_merkle_root, hash_merkle_root, SHA256_DIGEST_LENGTH);
  header->adder.assign(adder.begin(), adder.end());

  return header;
}

/* returns the header of this as a mpz value */
void BlockHeader::get_hash(mpz_t mpz_hash) {
  
  uint8_t hash[SHA256_DIGEST_LENGTH];
  uint8_t tmp[SHA256_DIGEST_LENGTH];                                   

  SHA256_CTX sha256;                                                          
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, &version, 4);                                   
  SHA256_Update(&sha256, hash_prev_block, SHA256_DIGEST_LENGTH);                                   
  SHA256_Update(&sha256, hash_merkle_root, SHA256_DIGEST_LENGTH);                                   
  SHA256_Update(&sha256, &time, 4);                                   
  SHA256_Update(&sha256, &difficulty, 8);                                   
  SHA256_Update(&sha256, &nonce, 4);                                   
  SHA256_Final(tmp, &sha256); 

  /* hash the result again */
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, tmp, SHA256_DIGEST_LENGTH);  
  SHA256_Final(hash, &sha256);
  
  ary_to_mpz(mpz_hash, hash, SHA256_DIGEST_LENGTH);
}

/* returns whether byte order is little endian */
bool BlockHeader::have_little_endian() {
  int val = 1;
  return *(char *) &val == 1 ;
}

/* clears the block header */
void BlockHeader::set_null() {
  memset(hash_prev_block,  0, SHA256_DIGEST_LENGTH);
  memset(hash_merkle_root, 0, SHA256_DIGEST_LENGTH);

  version    = 0;
  time       = 0;
  difficulty = 0;
  nonce      = 0;
  shift      = 0;

  adder.clear();
}

/**
 * switches the byte order
 */
void BlockHeader::switch_byte_oder(BlockHeader *header) {

  header->version    = byte_swap(header->version);
  header->time       = byte_swap(header->time);
  header->difficulty = byte_swap(header->difficulty);
  header->nonce      = byte_swap(header->nonce);
  header->shift      = byte_swap(header->shift);

  /* hashes are stored as arrays of uint32_t in gapcoin */
  uint32_t *hprev = (uint32_t *) header->hash_prev_block;
  uint32_t *hroot = (uint32_t *) header->hash_merkle_root;

  for (int k = 0; k < SHA256_DIGEST_LENGTH / 4; k++) {
    hprev[k] = byte_swap(hprev[k]);
    hroot[k] = byte_swap(hroot[k]);
  }

  byte_swap(&header->adder);
}

/* swap byte order of a 16 bit byte */
inline uint16_t BlockHeader::byte_swap(uint16_t val) {
  
  return (val<<8) | (val>>8);
}

/* swap byte order of a 32 bit byte */
inline uint32_t BlockHeader::byte_swap(uint32_t val) {
  
  val = ((val & 0xFF00FF00) >> 8) | ((val & 0x00FF00FF) << 8);
  return (val<<16) | (val>>16);
}

/* swap byte order of a 64 bit byte */
inline uint64_t BlockHeader::byte_swap(uint64_t val) {
  
  return byte_swap((uint32_t) (val >> 32)) | 
         (((uint64_t) byte_swap((uint32_t) val)) << 32);
}

/* swap byte order of a byte vector */
inline void BlockHeader::byte_swap(vector<uint8_t> *val) {
  
  int size = val->size() - 1; 
  for (int i = 0; i < size / 2; i++) {
    uint8_t tmp           = val->data()[i];
    val->data()[i]        = val->data()[size - i];
    val->data()[size - i] = tmp;
  }
}

/* returns this as a string representation */
string BlockHeader::to_s() {

  BlockHeader *header = clone();
  string str;

  mpz_t mpz_hash;
  mpz_init(mpz_hash);
  get_hash(mpz_hash);

  stringstream ss;
  ss << "BlockHeader:   " << mpz_to_hex(mpz_hash) << "\n";
  ss << "  version:     " << version << "\n";
  ss << "  hash prev:   ";

  /* swap order if we have little endian format */
  if (have_little_endian())
    for (uint32_t i = SHA256_DIGEST_LENGTH; i > 0; i--)
      ary_push_hex(str, header->hash_prev_block, i - 1);
  
  ss << str << "\n";
  ss << "  hash merkle: ";

  str.clear();
  /* swap order if we have little endian format */
  if (have_little_endian())
    for (uint32_t i = SHA256_DIGEST_LENGTH; i > 0; i--)
      ary_push_hex(str, header->hash_merkle_root, i - 1);

  ss << str << "\n";
  ss << "  time:        " << time << "\n";
  ss << "  difficulty:  " << difficulty << "\n";
  ss << "  nonce:       " << nonce << "\n";
  ss << "  shift:       " << shift << "\n";
  ss << "  adder:       ";

  str.clear();
  /* swap order if we have little endian format */
  if (have_little_endian())
    for (uint32_t i = header->adder.size(); i > 0; i--)
      ary_push_hex(str, header->adder, i - 1);

  ss << str << "\n";
  mpz_clear(mpz_hash);
  
  return ss.str();
}

/* returns whether this and other are equal */
bool BlockHeader::equal(BlockHeader *other) {
  
  if (this->version != other->version) 
    return false;
 
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    if (this->hash_prev_block[i] != other->hash_prev_block[i])
      return false;

    if (this->hash_merkle_root[i] != other->hash_merkle_root[i])
      return false;
  }

  if (this->time != other->time)
    return false;

  if (this->difficulty != other->difficulty)
    return false;

  if (this->nonce != other->nonce) 
    return false;

  if (this->shift != other->shift)
    return false;

  if (this->adder.size() != other->adder.size())
    return false;

  for (unsigned int i = 0; i < this->adder.size(); i++)
    if (this->adder[i] != other->adder[i])
      return false;

  return true;
}

/* returns whether this and other having the same block height */
bool BlockHeader::equal_block_height(BlockHeader *other) {

  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    if (this->hash_prev_block[i] != other->hash_prev_block[i])
      return false;

  return true;
}

/* creates a PoW from this */
PoW BlockHeader::get_pow() {
  
  mpz_t mpz_hash, mpz_adder;
  mpz_init(mpz_hash);
  mpz_init(mpz_adder);

  get_hash(mpz_hash);
  ary_to_mpz(mpz_adder, adder.data(), adder.size());

  PoW pow(mpz_hash, shift, mpz_adder, difficulty, nonce);

  mpz_clear(mpz_hash);
  mpz_clear(mpz_adder);

  return pow;
}
