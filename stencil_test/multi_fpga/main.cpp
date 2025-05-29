#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "mpi.h"

#include "helper.hpp"

#include "ompcnet.h"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

// VNx Objects
std::unique_ptr<vnx::CMAC> cmac;
std::unique_ptr<vnx::Networklayer> network_layer;

/**==========================================================================**/
/**                              GLOBAL CONFIGS                              **/

#define PACKET_INTS 8192
#define N_PACKETS 32 // 32 == 1 MB

/**==========================================================================**/
/**                                   FPGA                                   **/
void FPGA(std::string xclbin_file, int mpi_rank, int mpi_size, int num_fpgas,
          int REPLAYS, int size_mb, int instances) {
  // Global Configs
  int len = PACKET_INTS * N_PACKETS * size_mb;

  MPI_Barrier(MPI_COMM_WORLD);

  // Device start up
  int device_id = mpi_rank % num_fpgas;
  xrt::device dev(device_id); // Second Device
  xrt::uuid uuid = dev.load_xclbin(xclbin_file);
  MPI_Barrier(MPI_COMM_WORLD);

  // VNx
  ConfigureVNX(dev, mpi_rank, uuid, mpi_size, cmac, network_layer);
  MPI_Barrier(MPI_COMM_WORLD);

  int o = 0;
  while (o < 10) {
    std::this_thread::sleep_for(100ms);
    ConfigureARP(network_layer);
    std::this_thread::sleep_for(100ms);
    ShowARPTable(mpi_rank, mpi_size, network_layer);
    o++;
  }
  MPI_Barrier(MPI_COMM_WORLD);

  ompcnet::OMPCNet *ompc = new ompcnet::OMPCNet(dev);

  // Testing buffers
  float *input;
  float *output;

  long time_spent = 0;

  MPI_Barrier(MPI_COMM_WORLD);

  fprintf(stdout, "Executing for %d Replays\n", REPLAYS);
  for (int i = 0; i < REPLAYS; i++) {
    MPI_Barrier(MPI_COMM_WORLD);
    input = create_buffer(1, len);
    output = create_buffer(3, len);

    xrt::bo in =
        xrt::bo(dev, input, len * sizeof(float), xrt::bo::flags::normal, 0);
    xrt::bo out =
        xrt::bo(dev, output, len * sizeof(float), xrt::bo::flags::normal, 3);

    in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    out.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Stencil for 1024 iterations - In total
    fprintf(stdout, "Executing for %d Iterations\n",
            1024 / (instances * mpi_size));
    MPI_Barrier(MPI_COMM_WORLD);
    auto start = high_resolution_clock::now();
    for (int j = 0; j < 1024 / (instances * mpi_size); j++) {
      MPI_Barrier(MPI_COMM_WORLD);
      if (j % 2 == 0) {
        if (mpi_rank == 0) {
          ompc->MemToStream(0, in, len * sizeof(float));
          ompc->StreamTo(mpi_rank, mpi_rank + 1, 1, len * sizeof(float));
        } else if (mpi_rank == (mpi_size - 1)) {
          ompc->StreamFrom(mpi_rank - 1, mpi_rank, 0, len * sizeof(float));
          ompc->StreamToMem(1, out, len * sizeof(float));
        } else {
          ompc->StreamTo(mpi_rank, mpi_rank + 1, 0, len * sizeof(float));
          ompc->StreamFrom(mpi_rank - 1, mpi_rank, 1, len * sizeof(float));
        }
      } else {
        if (mpi_rank == 0) {
          ompc->StreamFrom(mpi_rank + 1, mpi_rank, 0, len * sizeof(float));
          ompc->StreamToMem(1, in, len * sizeof(float));
        } else if (mpi_rank == (mpi_size - 1)) {
          ompc->MemToStream(0, out, len * sizeof(float));
          ompc->StreamTo(mpi_rank, mpi_rank - 1, 1, len * sizeof(float));
        } else {
          ompc->StreamTo(mpi_rank, mpi_rank - 1, 0, len * sizeof(float));
          ompc->StreamFrom(mpi_rank + 1, mpi_rank, 1, len * sizeof(float));
        }
      }
      while (!(ompc->isOperationComplete(0) && ompc->isOperationComplete(1)))
        ;
    }
    MPI_Barrier(MPI_COMM_WORLD);
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
  if (argc < 3) {
    fprintf(stderr, "Program should receive 4 arguments\n"
                    "\t1: Path to xclbin file\n"
                    "\t2: Num FPGAs per node\n"
                    "\t3: Num REPLAY for operations\n"
                    "\t4: Size of buffer (in MB)\n"
                    "\t5: Stencil Instances per FPGA\n");
    return 1;
  }

  int provided;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  FPGA(std::string(argv[1]), rank, size, std::stoi(argv[2]), std::stoi(argv[3]),
       std::stoi(argv[4]), std::stoi(argv[5]));

  MPI_Finalize();

  return 0;
}