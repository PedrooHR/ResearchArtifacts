#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "helper.hpp"

#include "ompcnet.h"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

/**==========================================================================**/
/**                              GLOBAL CONFIGS                              **/

#define PACKET_INTS 8192
#define N_PACKETS 32 // 32 == 1 MB

/**==========================================================================**/
/**                                   FPGA                                   **/
void FPGA(std::string xclbin_file, int REPLAYS, int size_mb, int instances) {
  // Global Configs
  int len = PACKET_INTS * N_PACKETS * size_mb;

  // Device start up
  xrt::device dev(0);
  xrt::uuid uuid = dev.load_xclbin(xclbin_file);

  ompcnet::OMPCNet *ompc = new ompcnet::OMPCNet(dev);

  // Testing buffers
  float *input;
  float *output;

  long time_spent = 0;

  for (int i = 0; i < REPLAYS; i++) {
    std::this_thread::sleep_for(1000ms);
    input = create_buffer(1, len);
    output = create_buffer(3, len);

    xrt::bo in =
        xrt::bo(dev, input, len * sizeof(float), xrt::bo::flags::normal, 0);
    xrt::bo out =
        xrt::bo(dev, output, len * sizeof(float), xrt::bo::flags::normal, 3);

    in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    out.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Stencil for 1024 iterations
    auto start = high_resolution_clock::now();
    for (int j = 0; j < 1024 / instances; j++) {
      std::this_thread::sleep_for(1us);
      ompc->StreamToMem(1, (j % 2 == 0) ? out : in, len * sizeof(float));
      ompc->MemToStream(0, (j % 2 == 0) ? in : out, len * sizeof(float));

      while (!(ompc->isOperationComplete(0) && ompc->isOperationComplete(1)))
        ;
    }
    auto stop = high_resolution_clock::now();
    time_spent = duration_cast<microseconds>(stop - start).count();

    fprintf(stderr, "%ld\n", time_spent);
    
    out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    in.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    delete input;
    delete output;
  }
}

/**==========================================================================**/
/**                                   MAIN                                   **/
int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Program should receive 4 arguments\n"
                    "\t1: Path to xclbin file\n"
                    "\t2: Num REPLAY for operations\n"
                    "\t3: Size of buffer (in MB)\n"
                    "\t4: Number of Stencil instances\n");
             
    return 1;
  }

  FPGA(std::string(argv[1]), std::stoi(argv[2]), std::stoi(argv[3]),
       std::stoi(argv[4]));

  return 0;
}