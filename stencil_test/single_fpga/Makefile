CXX=mpic++

TARGETS=main

CURRENT_DIR=$(shell pwd)

INCLUDES_PATH=-I$(XILINX_XRT)/include -I$(CURRENT_DIR)/../ompcnet/include -I.. -I.
LIBRARIES_PATH=-L$(XILINX_XRT)/lib -L$(CURRENT_DIR)/../ompcnet/lib
LIBRARIES=-lxrt_coreutil -luuid -lompcnet -lvnxsh -pthread
STANDARD=-std=c++17

all: $(TARGETS) 
.PHONY: clean

main: main.cpp $(CURRENT_DIR)/../helper.hpp
	$(CXX) $< $(INCLUDES_PATH) $(LIBRARIES_PATH) $(STANDARD) -o $@ $(LIBRARIES)

clean:
	rm -rf $(TARGETS)