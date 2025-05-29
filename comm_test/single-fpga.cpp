#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>


#include "helper.hpp"
#include "mpi.h"
#include "ompcnet.h"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

/**==========================================================================**/
/**                              GLOBAL CONFIGS                              **/

#define PACKET_INTS 8192
#define N_PACKETS 32 * 1 // 32 == 1 MB


/**==========================================================================**/
/**                                   Test                                   **/
float TestOP(xrt::device dev, int32_t *input, int32_t *output, int32_t len,
             int32_t rank, int32_t size) {
  // create OMPC Net Object
  ompcnet::OMPCNet ompc(dev);

  // Create Buffers
  xrt::bo input_bo = xrt::bo(dev, input, len * 4, xrt::bo::flags::normal, 0);
  xrt::bo output_bo = xrt::bo(dev, output, len * 4, xrt::bo::flags::normal, 3);
  input_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

  MPI_Barrier(MPI_COMM_WORLD);
  // Dispatch OMPC Operations

  if (rank != (size - 1))
    ompc.Send(rank, rank + 1, 0, input_bo, len * 4);

  if (rank != 0)
    ompc.Recv(rank, rank - 1, 1, output_bo, len * 4);

  if (rank != (size - 1)) {
    while (!ompc.isOperationComplete(0))
      ;
  }

  if (rank != 0) {
    while (!ompc.isOperationComplete(1))
      ;
  }

  return ompc.getAverageTime(1); // Send Time
}

/**==========================================================================**/
/**                                   FPGA                                   **/
void FPGA(std::string xclbin_file, int mpi_rank, int mpi_size, int num_fpgas,
          int REPLAYS, int size_mb) {
  // Global Configs
  int len = PACKET_INTS * N_PACKETS * size_mb;

  // VNx Objects
  std::unique_ptr<vnx::CMAC> cmac;
  std::unique_ptr<vnx::Networklayer> network_layer;

  MPI_Barrier(MPI_COMM_WORLD);

  // Device start up
  int device_id = mpi_rank % num_fpgas;
  xrt::device dev(device_id); // Second Device
  xrt::uuid uuid = dev.load_xclbin(xclbin_file);
  MPI_Barrier(MPI_COMM_WORLD);

  // VNx
  ConfigureVNX(dev, mpi_rank, uuid, mpi_size, cmac, network_layer);
  MPI_Barrier(MPI_COMM_WORLD);

  int i = 0;
  while (i < 10) {
    std::this_thread::sleep_for(100ms);
    ConfigureARP(network_layer);
    std::this_thread::sleep_for(100ms);
    ShowARPTable(mpi_rank, mpi_size, network_layer);
    i++;
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Testing buffers
  int32_t *input_buffer;
  int32_t *output_buffer;

  float time_spent = 0;

  MPI_Barrier(MPI_COMM_WORLD);

  for (int i = 0; i < REPLAYS; i++) {
    std::this_thread::sleep_for(10ms);
    input_buffer = create_buffer(6, len);
    output_buffer = create_buffer(12, len);
    float time_spent_curr =
        TestOP(dev, input_buffer, output_buffer, len, mpi_rank, mpi_size);
    if (mpi_rank != (mpi_size -1)) fprintf(stderr, "%.2f\n", time_spent_curr);
    delete input_buffer;
    delete output_buffer;
  }
}

/**==========================================================================**/
/**                                   MAIN                                   **/
int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Program should receive 4 arguments\n"
                    "\t1: Path to xclbin file\n"
                    "\t2: Num FPGAs per node\n"
                    "\t3: Num REPLAY for operations\n"
                    "\t4: Size of buffer (in MB)\n");
    return 1;
  }

  int provided;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  FPGA(std::string(argv[1]), rank, size, std::stoi(argv[2]), std::stoi(argv[3]),
       std::stoi(argv[4]));

  return 0;
}