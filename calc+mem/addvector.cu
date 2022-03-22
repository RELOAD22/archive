#include "common.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <algorithm>
#include <device_functions.h>
#include <pthread.h>
#include <sys/time.h>
#include <thread>
#include <iostream>

class TimeInterval
{
public:
    TimeInterval() : start_(std::chrono::steady_clock::now()) {}

    double Elapsed()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<Duration>(now - start_).count();
    }

private:
    using Duration = std::chrono::duration<double>;
    std::chrono::steady_clock::time_point start_;
};

void checkResult(float *hostRef, float *gpuRef, const int N)
{
    double epsilon = 1.0E-8;
    bool match = 1;

    for (int i = 0; i < N; i++)
    {
        if (abs(hostRef[i] - gpuRef[i]) > epsilon)
        {
            match = 0;
            printf("Arrays do not match!\n");
            printf("host %5.2f gpu %5.2f at current %d\n", hostRef[i],
                   gpuRef[i], i);
            break;
        }
    }

    if (match)
        printf("Arrays match.\n\n");

    return;
}

void initialData(float *ip, int size)
{
    // generate different seed for random number
    time_t t;
    srand((unsigned)time(&t));

    for (int i = 0; i < size; i++)
    {
        ip[i] = (float)(rand() & 0xFF) / 10.0f;
    }

    return;
}

void sumArraysOnHost(float *A, float *B, float *C, const int N)
{
    for (int idx = 0; idx < N; idx++)
    {
        C[idx] = A[idx] + B[idx];
    }
}
__global__ void sumArraysOnGPU(float *A, float *B, float *C, const int N)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    while (i < N)
    {
        C[i] = A[i] + 1;
        i += gridDim.x * blockDim.x;
    }
}

void print_current_affinity()
{
    CUexecAffinityParam affinity_new;
    cuCtxGetExecAffinity(&affinity_new, CU_EXEC_AFFINITY_TYPE_SM_COUNT);
    int numSms = affinity_new.param.smCount.val;
    printf("current affinity.param.smCount.val:%d\n", numSms);
}

int main(int argc, char **argv)
{
    printf("%s Starting...\n", argv[0]);

    // set up device
    int dev = 0;

    CUexecAffinityParam affinity[1];
    CUcontext contextPool;
    cudaDeviceProp prop;
    affinity[0].type = CU_EXEC_AFFINITY_TYPE_SM_COUNT;
    affinity[0].param.smCount.val = (unsigned int)32;

    cudaSetDevice(dev);
    printf("cuCtxCreate_v3\n");
    cuCtxCreate_v3(&contextPool, affinity, 1, 0, 0);

    print_current_affinity();

    cudaGetDeviceProperties(&prop, dev);
    printf("current prop.multiProcessorCount:%d\n", prop.multiProcessorCount);

    // set up data size of vectors
    int nElem = 1 << 28;
    printf("Vector size %d\n", nElem);

    // malloc host memory
    size_t nBytes = nElem * sizeof(float);

    float *h_A, *h_B, *hostRef, *gpuRef;
    h_A = (float *)malloc(nBytes);
    h_B = (float *)malloc(nBytes);
    hostRef = (float *)malloc(nBytes);
    gpuRef = (float *)malloc(nBytes);

    double iStart, iElaps;

    // initialize data at host side
    iStart = seconds();
    // initialData(h_A, nElem);
    // initialData(h_B, nElem);
    iElaps = seconds() - iStart;
    printf("initialData Time elapsed %f sec\n", iElaps);
    memset(hostRef, 0, nBytes);
    memset(gpuRef, 0, nBytes);

    // malloc device global memory
    float *d_A, *d_B, *d_C;
    CHECK(cudaMalloc((float **)&d_A, nBytes));
    CHECK(cudaMalloc((float **)&d_B, nBytes));
    CHECK(cudaMalloc((float **)&d_C, nBytes));

    // transfer data from host to device
    // CHECK(cudaMemcpy(d_A, h_A, nBytes, cudaMemcpyHostToDevice));
    // CHECK(cudaMemcpy(d_B, h_B, nBytes, cudaMemcpyHostToDevice));
    // CHECK(cudaMemcpy(d_C, gpuRef, nBytes, cudaMemcpyHostToDevice));

    // std::vector<int> blocks = {1024, 512 , 256, 128, 64};
    std::vector<int> blocks = {1024};

    printf("begin kernel:\n");
    TimeInterval T;
    for (int iter = 0; iter < 1; ++iter)
        for (auto blocksize : blocks)
        {
            // invoke kernel at host side
            int iLen = blocksize;
            dim3 block(iLen);
            // dim3 grid  ((nElem + block.x - 1) / (block.x ));
            dim3 grid(32);

            // iStart = seconds();
            sumArraysOnGPU<<<grid, block> > >(d_A, d_B, d_C, nElem);
            CHECK(cudaDeviceSynchronize());
            // iElaps = seconds() - iStart;
            // printf("sumArraysOnGPU <<<  %d, %d  >>>  Time elapsed %f sec\n", grid.x,
            // block.x, iElaps);
        }

    cudaDeviceSynchronize();
    auto stop = T.Elapsed();
    std::cout << "in" << stop << "seconds" << std::endl;

    // check kernel error
    CHECK(cudaGetLastError());

    // copy kernel result back to host side
    CHECK(cudaMemcpy(gpuRef, d_C, nBytes, cudaMemcpyDeviceToHost));

    // check device results
    // checkResult(hostRef, gpuRef, nElem);

    // free device global memory
    CHECK(cudaFree(d_A));
    CHECK(cudaFree(d_B));
    CHECK(cudaFree(d_C));

    // free host memory
    free(h_A);
    free(h_B);
    free(hostRef);
    free(gpuRef);

    return (0);
}
