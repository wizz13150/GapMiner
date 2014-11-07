/**
 * Header file of GapMiners (simple) option parsing 
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
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iomanip> 
#include <iostream>

using namespace std;

/**
 * Singleton class to parse and store command line parameters
 */
class Opts {

  private:

    class SingleOpt {
      
      public:

      /* short opt */
      const char *short_opt;

      /* long opt */
      const char *long_opt;

      /* description */
      const char *description;

      /* has an argument */
      const bool _has_arg;

      /* was given */
      bool active;

      /* argument if given */
      string arg;

      SingleOpt(const char *short_opt,
                const char *long_opt,
                const char *description,
                const bool _has_arg) :
                short_opt(short_opt),
                long_opt(long_opt),
                description(description),
                _has_arg(_has_arg) {

        this->active = false;
        this->arg    = "";
      }
    };

    SingleOpt host;
    SingleOpt port;
    SingleOpt user;
    SingleOpt pass;
    SingleOpt quiet;
    SingleOpt extra_vb;
    SingleOpt stats;
    SingleOpt threads;
    SingleOpt pull;
    SingleOpt timeout;
    SingleOpt stratum;
    SingleOpt sievesize;
    SingleOpt primes;
    SingleOpt shift;
#ifndef CPU_ONLY
    SingleOpt benchmark;
    SingleOpt use_gpu;
    SingleOpt gpu_dev;
    SingleOpt work_items;
    SingleOpt max_primes;
    SingleOpt queue_size;
    SingleOpt platform;
#endif    
    SingleOpt help;
    SingleOpt license;

    Opts(int argc, char *argv[]);
    ~Opts() { }

    /* synchronization mutexes */
    static pthread_mutex_t creation_mutex;

  public:

    /* the only instance of this */
    static Opts *only_instance;

    /* access or create the only instance of this */
    static Opts *get_instance(int argc = 0, char *argv[] = NULL);

    /* read functions */
    bool has_host()         { return host.active;       }
    string get_host()       { return host.arg;          }
                                                        
    bool has_port()         { return port.active;       }
    string get_port()       { return port.arg;          }
                                                        
    bool has_user()         { return user.active;       }
    string get_user()       { return user.arg;          }
                                                        
    bool has_pass()         { return pass.active;       }
    string get_pass()       { return pass.arg;          }
                                                        
    bool has_quiet()        { return quiet.active;      }
                                                   
    bool has_extra_vb()     { return extra_vb.active;   }
                                                        
    bool has_stats()        { return stats.active;      }
    string get_stats()      { return stats.arg;         }
                                                        
    bool has_threads()      { return threads.active;    }
    string get_threads()    { return threads.arg;       }
                                                        
    bool has_pull()         { return pull.active;       }
    string get_pull()       { return pull.arg;          }
                                                        
    bool has_timeout()      { return timeout.active;    }
    string get_timeout()    { return timeout.arg;       }
                                                        
    bool has_stratum()      { return stratum.active;    }
                                                        
    bool has_sievesize()    { return sievesize.active;  }
    string get_sievesize()  { return sievesize.arg;     }
                                                        
    bool has_primes()       { return primes.active;     }
    string get_primes()     { return primes.arg;        }
                                                        
    bool has_shift()        { return shift.active;      }
    string get_shift()      { return shift.arg;         }
                                                        
#ifndef CPU_ONLY                                                        
    bool has_benchmark()    { return benchmark.active;  }
                                                        
    bool has_use_gpu()      { return use_gpu.active;    }
                                                        
    bool has_gpu_dev()      { return gpu_dev.active;    }
    string get_gpu_dev()    { return gpu_dev.arg;       }
                                                   
    bool has_work_items()   { return work_items.active; }
    string get_work_items() { return work_items.arg;    }
                                                  
    bool has_max_primes()   { return max_primes.active; }
    string get_max_primes() { return max_primes.arg;    }
                                                  
    bool has_queue_size()   { return queue_size.active; }
    string get_queue_size() { return queue_size.arg;    }
                                                  
    bool has_platform()     { return platform.active;   }
    string get_platform()   { return platform.arg;      }
#endif    
                                                  
    bool has_help()         { return help.active;       }
                                                        
    bool has_license()      { return license.active;    }
                                                    
    /* get help */                                
    string get_help();                            
};                                              
                                                
                                                
                                                
                                                
                                                
                                                
                                                
                                                
                                                
                                                
                                                
