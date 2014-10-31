/**
 * Header file of the Gapcoin block header format
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
#ifndef __BLOCK_HEADER_H__
#define __BLOCK_HEADER_H__
#include <string.h>
#include <openssl/sha.h>
#include <inttypes.h>
#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include "PoWCore/src/PoW.h"

using namespace std;

class BlockHeader {
  
  public: 
  
    /* the block version */
    uint32_t version;

    /* sha256 hash of the previous block */
    uint8_t hash_prev_block[SHA256_DIGEST_LENGTH];

    /* merkle root hash */
    uint8_t hash_merkle_root[SHA256_DIGEST_LENGTH];

    /* the blocks creation time */
    uint32_t time;

    /* the current block difficulty */
    uint64_t difficulty;

    /* the target for pool mining */
    uint64_t target;

    /* the block nonce (value to change the block hash) */
    uint32_t nonce;

    /* the shift amount */
    uint16_t shift;

    /* the adder */
    vector<uint8_t> adder;

    /* creates an empty block header */
    BlockHeader();

    /**
     * creates a new Block header out of the given hex encoded string 
     * (little endian format)
     */
    BlockHeader(string *hex);

    /**
     * returns the block header as an hex encoded string
     * (in little endian byte order)
     */
    string get_hex();

    /* clones this */
    BlockHeader *clone();

    /* returns the header hash of this as a mpz value */
    void get_hash(mpz_t mpz_hash);

    /* returns a string representation of this*/
    string to_s();

    /* returns whether this and other are equal */
    bool equal(BlockHeader *other);

    /* returns whether this and other has the same block height */
    bool equal_block_height(BlockHeader *other);

    /* creates a PoW from this */
    PoW get_pow();

  private:

    /* returns whether byte order is little endian */
    bool have_little_endian();

    /* clears the block header */
    void set_null();

    /**
     * switches the byte order
     */
    void switch_byte_oder(BlockHeader *header);

    /* swap byte order of a 16 bit value */
    inline uint16_t byte_swap(uint16_t var);
    
    /* swap byte order of a 32 bit value */
    inline uint32_t byte_swap(uint32_t var);

    /* swap byte order of a 64 bit value */
    inline uint64_t byte_swap(uint64_t var);

    /* swap byte order of a byte vector */
    inline void byte_swap(vector<uint8_t> *var);

};

#endif /* __BLOCK_HEADER_H__ */
