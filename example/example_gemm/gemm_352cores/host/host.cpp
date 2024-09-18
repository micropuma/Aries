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

void sw_mm(std::vector<int32_t> DataInput0,std::vector<int32_t> DataInput1, std::vector<int32_t>& golden, const int I, const int K, const int J){
    float sum = 0;
    for (int i = 0; i < I; i++) {
        for (int j = 0; j < J; j++) {
            sum =0;
            for (int k = 0; k < K; k++) {
                sum=sum+DataInput0[k + i * K]*DataInput1[j + k * J];
            }
            golden[j + i * J]=sum;
        } 
    }
}

const int I=2816;
const int J=3072;
const int K=8192;
const int sizeIn0 = I * K;
const int sizeIn1 = K * J;
const int sizeOut = I * J;
int main(int argc, char **argv) {
  char* xclbinFilename;
  int iter=100,verify=0;
  if(argc == 4) {
    xclbinFilename = argv[1];
    if (sscanf (argv[2], "%i", &iter) != 1) {
        fprintf(stderr, "error - not an integer");
    }
    if (sscanf (argv[3], "%i", &verify) != 1) {
        fprintf(stderr, "error - not an integer");
    }
  }
  // Open xclbin
  auto device = xrt::device(0); //device index=0
	auto uuid = device.load_xclbin(xclbinFilename);
	auto dhdl = xrtDeviceOpenFromXcl(device);
  
  std::vector<int32_t> DataInput0(sizeIn0,1);
  std::vector<int32_t> DataInput1(sizeIn1,1);
  std::vector<int32_t> final_result_sw(sizeOut,1);
  for (int i = 0; i < I; i++) {
    for (int k = 0; k < K; k++) {
      int pos = k + i * K;
      DataInput0[pos]= (int)(rand()%10); 
    }
  }
  for (int k = 0; k < K; k++) {
    for (int j = 0; j < J; j++) {
      int pos = j + k * J;
      DataInput1[pos]= (int)(rand()%10);
    }
  }

  // PL control
  auto dma_pl_1= xrt::kernel(device, uuid, "dma_pl:{dma_pl_1}");
  auto out_bohdl0 = xrt::bo(device, sizeOut * sizeof(int), dma_pl_1.group_id(0));
  auto out_bomapped0 = out_bohdl0.map<int32_t*>();
  auto in_bohdl0 = xrt::bo(device, sizeIn0 * sizeof(int), dma_pl_1.group_id(0));
  auto in_bomapped0 = in_bohdl0.map<int32_t*>();
  ////////Initialize in_bomapped0 here/////////
  for (int k = 0; k < sizeIn0; k++) {
    in_bomapped0 [k]= DataInput0[k];
  }
  in_bohdl0.sync(XCL_BO_SYNC_BO_TO_DEVICE, sizeIn0 * sizeof(int), 0);
  auto in_bohdl1 = xrt::bo(device, sizeIn1 * sizeof(int), dma_pl_1.group_id(0));
  auto in_bomapped1 = in_bohdl1.map<int32_t*>();
  ////////Initialize in_bomapped1 here/////////
  for (int k = 0; k < sizeIn1; k++) {
    in_bomapped1 [k]= DataInput1[k];
  }
  in_bohdl1.sync(XCL_BO_SYNC_BO_TO_DEVICE, sizeIn1 * sizeof(int), 0);
  std::cout << "Kernel run\n";

  // AI Engine Graph Control
  std::cout << "Graph run\n";
  auto gr0= xrt::graph(device, uuid, "gr0");
  gr0.run(-1);

  //profile performance
  double kernel_time_in_sec = 0;
  std::chrono::duration<double> kernel_time(0);
  auto kernel_start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i<iter; i++){
    auto dma_pl_1_run = dma_pl_1(out_bohdl0, in_bohdl0, in_bohdl1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr); 
    // Wait for PL kernel to finish
    dma_pl_1_run.wait();
  }

  std::cout << "Kernel run finished!\n";

  auto kernel_end = std::chrono::high_resolution_clock::now();
  kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);
  kernel_time_in_sec = kernel_time.count();
  double TOPS = (double)(I * K) * (double) (J * 2 * iter * 1e-9) / kernel_time_in_sec;
  std::cout << "MM Size:"<< I << " * " << K << " * " << J << std::endl;
  std::cout << "Total time is: "<< kernel_time_in_sec <<"s, TOPS = " << TOPS << " GOPS/s" << std::endl;

  // Sync output buffer back to host
  out_bohdl0.sync(XCL_BO_SYNC_BO_FROM_DEVICE , sizeOut * sizeof(int), 0);

  //////// Add post processes here/////////
  if(verify){
    sw_mm(DataInput0,DataInput1,final_result_sw,I,K,J);
    int errorCount = 0;  
    for (int len = 0; len < sizeOut; len++) {
      // if(abs((float)(out_bomapped0[len])-final_result_sw[len])>=1e-4){
      //   printf("Error found out_bomapped0[%d]!=final_result_sw[%d], %f!=%f \n", len,len,out_bomapped0[len],final_result_sw[len]);
      //   errorCount++;
      // }
      if(out_bomapped0[len]!=final_result_sw[len]){
        printf("Error found out_bomapped0[%d]!=final_result_sw[%d], %d!=%d \n", len,len,out_bomapped0[len],final_result_sw[len]);
        errorCount++;
      }
    }
    if (errorCount)
      printf("Test failed with %d errors\n", errorCount);
    else
      printf("TEST PASSED\n");
  }

  std::cout << "New kernel\n";
  std::cout << "Host Run Finished!\n";
  return 0;
}