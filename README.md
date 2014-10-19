# GapMiner - A standalone Gapcoin (GAP) CPU rpc miner
---
<br/>
## So whats the purpose of an standalone rpc miner? 


  * GapMiner is fully open source (GPLv3) and (hopefully)
    well documented, to give developper a good start into the
    pGap algorithm (Gapcoins prime gap based hashing algorithm).

  * simplicity and little dependencies (pure C++ code and only 
    pthread, openssl, curl, jansson, gmp and mpfr dependencies).

  * speed, at the moment GapMiner has no speed improvements over Gapcoin,
    but in future this houpfully changes.



## get it running
---

First of all keep in mind that GapMiner still has alpha qualities and 
doesn't claims to be the fastest CPU miner out there, the main focus
is read- and understandability of the Gapcoin mining algorithm!

Also it's currently Linux only, sorry.

### required libraries
  - pthread
  - openssl
  - curl
  - jansson
  - gmp 
  - mpfr

### installation
```sh
  git clone https://github.com/gapcoin/GapMiner.git
  cd GapMiner
  make all
  make install
```
## Usage
---

  `gapminer [--options]`

### basic

 - `-o  --host [ipv4]` host ip address

 - `-p  --port [port]` port to connect to

 - `-u  --user [user]` user for gapcoin rpc authentification

 - `-x  --pwd [pwd]` password for gapcoin rpc authentification

#### example:

`gapminer -o 127.0.0.1 -p  -u rpcuser -x rpcpassword`

### advanced

 - `-q  --quiet` be quiet (only prints shares)

 - `-i  --stats-interval [NUM]` interval (sec) to print mining informations

 - `-t  --threads [NUM]` number of mining threads

 - `-l  --pull-interval [NUM]` seconds to wait between getwork request

 - `-s  --sieve-size [NUM]` the prime sieve size

 - `-r  --sieve-primes [NUM]` number of primes to sieve

 - `-f  --shift [NUM]` the adder shift

 - `-h  --help` print this information

 - `-v  --license` show license of this program
