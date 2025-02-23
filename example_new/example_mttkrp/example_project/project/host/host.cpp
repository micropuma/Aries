
//===----------------------------------------------------------------------===//
//
// Automatically generated file for host.cpp
//
//===----------------------------------------------------------------------===//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <math.h>
#include <string>

// This is used for the PL Kernels
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"


// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"
//#include "../aie/adf_graph.h"

using namespace std;


int main(int argc, char **argv) {
  char* xclbinFilename = argv[1];
  
  // Open xclbin
  auto device = xrt::device(0); //device index=0
	auto uuid = device.load_xclbin(xclbinFilename);
	auto dhdl = xrtDeviceOpenFromXcl(device);

  // PL control
  auto top_0= xrt::kernel(device, uuid, "top:{top_0}");

  // Define arguments
  auto in_bohdl0 = xrt::bo(device, 100663296 * sizeof(float), top_0.group_id(0));
  auto in_bomapped0 = in_bohdl0.map<float*>();
  for (unsigned i=0; i < 100663296; i++){
    in_bomapped0[i] = (float)(rand()%5);
  }
  in_bohdl0.sync(XCL_BO_SYNC_BO_TO_DEVICE, 100663296 * sizeof(float), 0);

  auto in_bohdl1 = xrt::bo(device, 4718592 * sizeof(float), top_0.group_id(0));
  auto in_bomapped1 = in_bohdl1.map<float*>();
  for (unsigned i=0; i < 4718592; i++){
    in_bomapped1[i] = (float)(rand()%5);
  }
  in_bohdl1.sync(XCL_BO_SYNC_BO_TO_DEVICE, 4718592 * sizeof(float), 0);

  auto in_bohdl2 = xrt::bo(device, 12582912 * sizeof(float), top_0.group_id(0));
  auto in_bomapped2 = in_bohdl2.map<float*>();
  for (unsigned i=0; i < 12582912; i++){
    in_bomapped2[i] = (float)(rand()%5);
  }
  in_bohdl2.sync(XCL_BO_SYNC_BO_TO_DEVICE, 12582912 * sizeof(float), 0);

  auto out_bohdl0 = xrt::bo(device, 393216 * sizeof(float), top_0.group_id(0));
  auto out_bomapped0 = out_bohdl0.map<float*>();

  // AI Engine Graph Control
  std::cout << "Graph run\n";
  auto adf_cell0_gr0= xrt::graph(device, uuid, "adf_cell0_gr0");
  adf_cell0_gr0.run(-1);

  // Set arguments of the PL function
  xrt::run top_0_run= xrt::run(top_0);
  top_0_run.set_arg(0, in_bohdl0);
  top_0_run.set_arg(1, in_bohdl1);
  top_0_run.set_arg(2, in_bohdl2);
  top_0_run.set_arg(3, out_bohdl0);
  std::cout << "Kernel run\n";
  double kernel_time_in_sec = 0;
  std::chrono::duration<double> kernel_time(0);
  auto kernel_start = std::chrono::high_resolution_clock::now();
  top_0_run.start();
  top_0_run.wait();
  auto kernel_end = std::chrono::high_resolution_clock::now();
  kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);
  kernel_time_in_sec = kernel_time.count();
  std::cout << "Kernel run finished!\n";
  std::cout << "Total time is: "<< kernel_time_in_sec<< "s" << std::endl;
  // Sync output buffer back to host
  out_bohdl0.sync(XCL_BO_SYNC_BO_FROM_DEVICE , 393216 * sizeof(float), 0);
  std::cout << "Output buffer sync back finished\n";

  std::cout << "Host Run Finished!\n";
  return 0;
}


