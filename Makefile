VERSION   = 0.0.1
SRC       = ./src
BIN       = ./bin
CC        = g++
DBFLAGS   = -g
CXXFLAGS  = -Wall -Wextra -c -Winline -Wformat -Wformat-security \
            -pthread --param max-inline-insns-single=1000 -lm \
						-Wno-write-strings -I/opt/AMDAPP/include -g
LDFLAGS   = -lm -lcrypto -lmpfr -lgmp -pthread -lcurl -ljansson -lboost_system \
					  -L/opts/AMDAPP/lib -lOpenCL
OTFLAGS   = -march=native -O3

.PHONY: clean test all install

# default target
all: link
	$(CC) $(ALL_OBJ) $(LDFLAGS) -o $(BIN)/gapminer

install: all
	cp $(BIN)/gapminer /usr/bin/


# development
#CXXFLAGS += $(DBFLAGS) 

# PoWCore debugging
#CXXFLAGS += -D DEBUG

# GPU-Miner enable fast debugging
#CXXFLAGS += -D DEBUG_BASIC -D DEBUG_FAST

# GPU-Miner enable slow debugging (more tests)
#CXXFLAGS += -D DEBUG_BASIC -D DEBUG_FAST -D DEBUG_SLOW


# optimization
CXXFLAGS  += $(OTFLAGS)
LDFLAGS   += $(OTFLAGS)

# disable GPU support
#CXXFLAGS += -DCPU_ONLY 
#LDFLAGS   = -lm -lcrypto -lmpfr -lgmp -pthread -lcurl -ljansson \
#						-lboost_system

ALL_SRC = $(shell find $(SRC) -type f -name '*.cpp')
ALL_OBJ = $(ALL_SRC:%.cpp=%.o)

$(SRC)/GPUFermat.o:
	$(CC) $(CXXFLAGS) -std=c++11  $(SRC)/GPUFermat.cpp -o  $(SRC)/GPUFermat.o

%.o: %.cpp
	$(CC) $(CXXFLAGS) $^ -o $@

compile: $(ALL_OBJ) $(ALL_OBJ)


prepare:
	@mkdir -p bin

link: prepare compile

clean:
	rm -rf $(BIN)
	rm -f $(ALL_OBJ) $(ALL_OBJ)

