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

BlockHeader::BlockHeader() { }

BlockHeader::BlockHeader(string *hex) {
  (void) hex;
}

string BlockHeader::get_hex() {
  return string("");
}

BlockHeader *BlockHeader::clone() {
  return new BlockHeader();
}

void BlockHeader::get_hash(mpz_t mpz_hash) {
  (void) mpz_hash;
}

string BlockHeader::to_s() {
  return string("");
}

bool BlockHeader::equal(BlockHeader *other) {
  (void) other;
  return false;
}

bool BlockHeader::equal_block_height(BlockHeader *other) {
  (void) other;
  return false;
}

PoW BlockHeader::get_pow() {
  return PoW(new vector<uint8_t>, 0, new vector<uint8_t>, 0);
}
