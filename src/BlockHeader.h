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
  
    uint32_t version;
    uint8_t hash_prev_block[SHA256_DIGEST_LENGTH];
    uint8_t hash_merkle_root[SHA256_DIGEST_LENGTH];
    uint32_t time;
    uint64_t difficulty;
    uint32_t nonce;
    uint16_t shift;
    vector<uint8_t> adder;

    BlockHeader();
    BlockHeader(string *hex);
    string get_hex();
    BlockHeader *clone();
    void get_hash(mpz_t mpz_hash);
    string to_s();
    bool equal(BlockHeader *other);
    bool equal_block_height(BlockHeader *other);
    PoW get_pow();
};

#endif /* __BLOCK_HEADER_H__ */
