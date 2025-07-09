#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <math.h>
#include <string>

// XRT和PL内核相关头文件
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "adf/adf_api/XRTConfig.h"

using namespace std;

// 矩阵参数
const int MAT_SIZE = 64;       // 64x64矩阵
const int MAT_ELEMENTS = MAT_SIZE * MAT_SIZE;

// 生成随机矩阵
void generate_random_matrix(float* matrix, int size) {
    for (int i = 0; i < size; ++i) {
        matrix[i] = static_cast<float>(rand() % 5); // 0-4的随机整数
    }
}

// 打印矩阵部分内容
void print_matrix_partial(const string& name, float* matrix) {
    cout << "\nMatrix " << name << " (first 2 rows):" << endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 5; ++j) {
            cout << matrix[i * MAT_SIZE + j] << " ";
        }
        cout << "... [truncated]" << endl;
    }
}

// CPU端矩阵乘法参考计算
void matrix_mult_ref(float* a, float* b, float* result) {
    for (int i = 0; i < MAT_SIZE; ++i) {
        for (int j = 0; j < MAT_SIZE; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < MAT_SIZE; ++k) {
                sum += a[i * MAT_SIZE + k] * b[k * MAT_SIZE + j];
            }
            result[i * MAT_SIZE + j] = sum;
        }
    }
}

// 验证结果正确性
bool verify_results(float* fpga_result, float* cpu_result) {
    const float tolerance = 1e-4;
    int error_count = 0;
    
    for (int i = 0; i < MAT_ELEMENTS; ++i) {
        float diff = fabs(fpga_result[i] - cpu_result[i]);
        if (diff > tolerance) {
            if (error_count < 10) { // 仅显示前10个错误
                cout << "Mismatch at index " << i 
                     << ": FPGA=" << fpga_result[i] 
                     << " CPU=" << cpu_result[i] 
                     << " Diff=" << diff << endl;
            }
            error_count++;
        }
    }

    if (error_count == 0) {
        cout << "\nVerification PASSED: All results match within tolerance" << endl;
        return true;
    } else {
        cout << "\nVerification FAILED: Total errors = " << error_count << endl;
        return false;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <xclbin_path>" << endl;
        return EXIT_FAILURE;
    }

    // 初始化随机种子
    srand(time(nullptr));

    // 加载XCLBIN文件
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(argv[1]);

    // 初始化PL内核
    auto top_0 = xrt::kernel(device, uuid, "top:{top_0}");

    // 创建并初始化输入缓冲区
    auto in_bohdl0 = xrt::bo(device, MAT_ELEMENTS * sizeof(float), top_0.group_id(0));
    auto in_bomapped0 = in_bohdl0.map<float*>();
    generate_random_matrix(in_bomapped0, MAT_ELEMENTS);

    auto in_bohdl1 = xrt::bo(device, MAT_ELEMENTS * sizeof(float), top_0.group_id(0));
    auto in_bomapped1 = in_bohdl1.map<float*>();
    generate_random_matrix(in_bomapped1, MAT_ELEMENTS);

    // 创建输出缓冲区
    auto out_bohdl0 = xrt::bo(device, MAT_ELEMENTS * sizeof(float), top_0.group_id(0));
    auto out_bomapped0 = out_bohdl0.map<float*>();

    // 打印输入矩阵
    print_matrix_partial("A", in_bomapped0);
    print_matrix_partial("B", in_bomapped1);

    // 同步数据到设备
    in_bohdl0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    in_bohdl1.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // 启动AIE图
    auto adf_cell0_gr0 = xrt::graph(device, uuid, "adf_cell0_gr0");
    adf_cell0_gr0.run(-1);

    // 设置内核参数并运行
    xrt::run top_0_run = xrt::run(top_0);
    top_0_run.set_arg(0, in_bohdl0);
    top_0_run.set_arg(1, in_bohdl1);
    top_0_run.set_arg(2, out_bohdl0);

    // 执行并计时
    auto start = chrono::high_resolution_clock::now();
    top_0_run.start();
    top_0_run.wait();
    auto end = chrono::high_resolution_clock::now();

    // 同步结果回主机
    out_bohdl0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // 计算执行时间
    chrono::duration<double> duration = end - start;
    cout << "\nKernel execution time: " << duration.count() << " seconds" << endl;

    // 打印部分输出矩阵
    print_matrix_partial("FPGA Result", out_bomapped0);

    // CPU端参考计算
    vector<float> cpu_results(MAT_ELEMENTS);
    matrix_mult_ref(in_bomapped0, in_bomapped1, cpu_results.data());
    print_matrix_partial("CPU Reference", cpu_results.data());

    // 验证结果
    bool verification_status = verify_results(out_bomapped0, cpu_results.data());

    // 资源清理
    adf_cell0_gr0.end();

    return verification_status ? EXIT_SUCCESS : EXIT_FAILURE;
}