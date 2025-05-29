CXX=mpic++

TARGETS=main

CURRENT_DIR=$(shell pwd)

INCLUDES_PATH=-I$(XILINX_XRT)/include -I$(CURRENT_DIR)/ompcnet/include -I.
LIBRARIES_PATH=-L$(XILINX_XRT)/lib -L$(CURRENT_DIR)/ompcnet/lib
LIBRARIES=-lxrt_coreutil -luuid -lompcnet -lvnxsh -pthread
STANDARD=-std=c++17

all: $(TARGETS) 
.PHONY: clean

main: main.cpp helper.hpp
	$(CXX) -g $< $(INCLUDES_PATH) $(LIBRARIES_PATH) $(STANDARD) -o $@ $(LIBRARIES)

eth1: eth1.cpp 
	$(CXX) $< $(INCLUDES_PATH) $(LIBRARIES_PATH) $(STANDARD) -o $@ $(LIBRARIES)

eth0: eth0.cpp
	$(CXX) $< $(INCLUDES_PATH) $(LIBRARIES_PATH) $(STANDARD) -o $@ $(LIBRARIES)

clean:
	rm -rf .run
	rm -rf $(wildcard *.json)
	rm -rf $(wildcard *.log)
	rm -rf $(wildcard *.xml)
	rm -rf $(TARGETS)