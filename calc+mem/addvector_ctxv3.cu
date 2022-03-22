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
#include <unistd.h>
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
using namespace std;

#define ITER 1

#define NELEM 32
#define WIDTH 32
#define ITEMS_PER_THREAD 8
#define M 1024
#define N 1024
#define K 1024
int nElem = 1 << 28;
float *d_a, *d_b, *d_c;
float *d_A, *d_B, *d_C;
#define gridsize (max(M, N) + WIDTH - 1) / WIDTH
dim3 grid(gridsize, gridsize);
dim3 block(WIDTH, WIDTH / ITEMS_PER_THREAD);

#define CONTEXT_POOL_SIZE 6
CUcontext contextPool[CONTEXT_POOL_SIZE];
int smCounts[CONTEXT_POOL_SIZE];

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

void print_current_affinity()
{
    CUexecAffinityParam affinity_new;
    cuCtxGetExecAffinity(&affinity_new, CU_EXEC_AFFINITY_TYPE_SM_COUNT);
    int numSms = affinity_new.param.smCount.val;
    printf("current affinity.param.smCount.val:%d\n", numSms);
}

// A:M*K B:K*N C:M*N
void CpuMatmul(float *a, float *b, float *c, int m, int n, int k)
{
    float sum = 0;
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            sum = 0;
            for (int t = 0; t < k; ++t)
            {
                sum += a[i * k + t] * b[t * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

void InitData(float *a, float *b, int m, int n, int k)
{
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < k; ++j)
        {
            *(a + i * k + j) = 2;
        }
    }
    for (int i = 0; i < k; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            *(b + i * n + j) = 3;
        }
    }
}
__global__ void matmul(float *a, float *b, float *c, int m, int n, int k)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y * ITEMS_PER_THREAD + threadIdx.y;

    // int threadid = threadIdx.x + threadIdx.y * blockDim.x;
    int tid = y * n + x;
    if (x >= n || y >= m)
        return;
    __shared__ float sha[WIDTH][WIDTH];
    __shared__ float shb[WIDTH][WIDTH];

    // as[tid] = a[]
    float sum[ITEMS_PER_THREAD] = {0};
    int nIter = (k + WIDTH - 1) / WIDTH;

    for (int i = 0; i < nIter; i++)
    {
        for (int index = 0; index < ITEMS_PER_THREAD; ++index)
        {
            int offset = index * (WIDTH / ITEMS_PER_THREAD);
            sha[threadIdx.y + offset][threadIdx.x] = a[(y + offset) * k + i * WIDTH + threadIdx.x];
            shb[threadIdx.y + offset][threadIdx.x] = b[(i * WIDTH + threadIdx.y + offset) * n + x];
        }
        /*
        sha[threadIdx.y][threadIdx.x] = a[y * k + i * WIDTH + threadIdx.x];
        shb[threadIdx.y][threadIdx.x] = b[(i * WIDTH + threadIdx.y) * n + x];
        sha[threadIdx.y + 8][threadIdx.x] = a[(y + 8) * k + i * WIDTH + threadIdx.x];
        shb[threadIdx.y + 8][threadIdx.x] = b[(i * WIDTH + threadIdx.y + 8) * n + x];
        sha[threadIdx.y + 16][threadIdx.x] = a[(y + 16) * k + i * WIDTH + threadIdx.x];
        shb[threadIdx.y + 16][threadIdx.x] = b[(i * WIDTH + threadIdx.y + 16) * n + x];
        sha[threadIdx.y + 24][threadIdx.x] = a[(y + 24) * k + i * WIDTH + threadIdx.x];
        shb[threadIdx.y + 24][threadIdx.x] = b[(i * WIDTH + threadIdx.y + 24) * n + x];
        */
        __syncthreads();
        for (int index = 0; index < WIDTH; ++index)
        {
            for (int i = 0; i < ITEMS_PER_THREAD; ++i)
            {
                int offset = i * (WIDTH / ITEMS_PER_THREAD);
                sum[i] += sha[threadIdx.y + offset][index] * shb[index][threadIdx.x];
            }
            /*
            sum[0] += sha[threadIdx.y][index] * shb[index][threadIdx.x];
            sum[1] += sha[threadIdx.y + 8][index] * shb[index][threadIdx.x];
            sum[2] += sha[threadIdx.y + 16][index] * shb[index][threadIdx.x];
            sum[3] += sha[threadIdx.y + 24][index] * shb[index][threadIdx.x];
            */
            // printf("tid: %d read: a[%d],b[%d]\n", tid, y * size + index, index * size + x);
        }
        __syncthreads();
    }

    for (int index = 0; index < ITEMS_PER_THREAD; ++index)
    {
        int offset = index * (WIDTH / ITEMS_PER_THREAD);
        c[(y + offset) * n + x] = sum[index];
    }
    /*
    c[tid] = sum[0];
    c[(y + 8) * n + x] = sum[1];
    c[(y + 16) * n + x] = sum[2];
    c[(y + 24) * n + x] = sum[3];
    */
    // printf("c[%d]:%f size:%d\n", tid, sum, size);
}
cudaDeviceProp prop;
// CUcontext contextPool;

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

void sumArraysOnHost(float *A, float *B, float *C, const int Ns)
{
    for (int idx = 0; idx < Ns; idx++)
    {
        C[idx] = A[idx] + B[idx];
    }
}
__global__ void sumArraysOnGPU(float *A, float *B, float *C, const int Ns)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    while (i < Ns)
    {
        C[i] = A[i] + 1;
        i += gridDim.x * blockDim.x;
    }
}

void *thread1(void *t)
{
    printf("thread1:\n");
    cuCtxSetCurrent(contextPool[0]);

    auto mata_size = M * K * sizeof(float);
    auto matb_size = K * N * sizeof(float);
    auto matc_size = M * N * sizeof(float);

    auto elems_size = NELEM * NELEM;

    float *a = (float *)malloc(mata_size);
    float *b = (float *)malloc(matb_size);
    float *c = (float *)malloc(matc_size);
    float *cpu_c = (float *)malloc(matc_size);

    InitData(a, b, M, N, K);
    float *d_a, *d_b, *d_c;
    cudaMalloc(&d_a, mata_size);
    cudaMalloc(&d_b, matb_size);
    cudaMalloc(&d_c, matc_size);
    cudaMemset(d_c, 0, matc_size);

    cudaMemcpy(d_a, a, mata_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, matb_size, cudaMemcpyHostToDevice);
    dim3 grid(gridsize, gridsize);

    dim3 block(WIDTH, WIDTH / ITEMS_PER_THREAD);

    print_current_affinity();
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    for (int i = 0; i < ITER; ++i)
        matmul<<<grid, block> > >(d_a, d_b, d_c, M, N, K);
    cudaDeviceSynchronize();
    gettimeofday(&end_time, NULL);
    double timespend = (end_time.tv_sec * 1000000 + end_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec);
    printf("time:%f\n", timespend);

    cudaMemcpy(c, d_c, matc_size, cudaMemcpyDeviceToHost);

    printf("%f ", *(c));
    printf("%f ", *(c + M * N / 2 + N / 2));
    printf("%f ", *(c + M * N - 1));
    printf("\n");
}

void *mythread(int i)
{
    cuCtxSetCurrent(contextPool[i]);
    print_current_affinity();
    TimeInterval T;
    for (int i = 0; i < ITER; ++i)
        sumArraysOnGPU<<<grid, block> > >(d_A, d_B, d_C, nElem);
    cudaDeviceSynchronize();
    auto stop = T.Elapsed();
    std::cout << "in " << stop << " seconds" << std::endl;
}

int main()
{

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

    printf("init finish\n");

    // CpuMatmul(a, b, cpu_c, M, N, K);
    printf("cpu compute finish\n");

    // init device data
    int device = 0;

    CHECK(cudaMalloc((float **)&d_A, nBytes));
    CHECK(cudaMalloc((float **)&d_B, nBytes));
    CHECK(cudaMalloc((float **)&d_C, nBytes));

    printf("begin kernel:\n");
    printf("grid:%d %d \n", gridsize, gridsize);

    CUexecAffinityParam affinity[2];
    affinity[0].type = CU_EXEC_AFFINITY_TYPE_SM_COUNT;
    affinity[0].param.smCount.val = (unsigned int)2;
    affinity[1].type = CU_EXEC_AFFINITY_TYPE_MAX;
    affinity[1].param.smCount.val = (unsigned int)34;
    cudaSetDevice(device);
    printf("cuCtxCreate_v3\n");
    cuCtxCreate_v3(&contextPool[0], affinity, 1, 0, 0);

    print_current_affinity();

    cudaGetDeviceProperties(&prop, device);
    printf("current prop.multiProcessorCount:%d\n", prop.multiProcessorCount);

    // printf("cuCtxSetCurrent finish\n");
    // pthread_t pt;
    // pthread_create(&pt, NULL, thread1, NULL);
    // pthread_join(pt, NULL);

    /*
    smCounts[0] = (prop. multiProcessorCount - 5) / 5 * 1;
    smCounts[1] = (prop. multiProcessorCount - 5) / 5 * 2;
    smCounts[2] = (prop. multiProcessorCount - 5) / 5 * 3;
    smCounts[3] = (prop. multiProcessorCount - 5) / 5 * 4;
    smCounts[4] = (prop. multiProcessorCount);*/

    for (int i = 0; i < CONTEXT_POOL_SIZE; i++)
    {
        smCounts[i] = (prop.multiProcessorCount - 5) / CONTEXT_POOL_SIZE * (i + 1);
    }
    smCounts[CONTEXT_POOL_SIZE - 1] = (prop.multiProcessorCount);
    /*
    for (int i = 0; i < CONTEXT_POOL_SIZE; i++){
        smCounts[i] = (prop. multiProcessorCount);
    }*/

    for (int i = 0; i < CONTEXT_POOL_SIZE; i++)
    {
        CUexecAffinityParam affinity[2];
        affinity[0].type = CU_EXEC_AFFINITY_TYPE_SM_COUNT;
        affinity[0].param.smCount.val = smCounts[i];
        cuCtxCreate_v3(&contextPool[i], affinity, 1, 0, 0);
    }

    usleep(500000);
    printf("begin kernel:\n");
    TimeInterval T;
    pthread_t pts[CONTEXT_POOL_SIZE];
    for (int i = 0; i < CONTEXT_POOL_SIZE; i++)
    {
        /*
        std::thread([i]() {
            printf("thread%d:\n", i);
            CUexecAffinityParam affinity;
            cuCtxSetCurrent(contextPool[i]);
            cuCtxGetExecAffinity(&affinity, CU_EXEC_AFFINITY_TYPE_SM_COUNT);
            numSms = affinity.param.smCount.val;
            printf("numSms:%d  numBlocksPerSm:%d\n", numSms, numBlocksPerSm);
            void *kernelArgs[] = { };
            cudaLaunchCooperativeKernel((void*)matmul, dimGrid, dimBlock, kernelArgs);
        });*/
        std::thread t(mythread, i);
        // pthread_create(&pts[i], NULL, mythread, (void *)i);
        t.join();
    }

    print_current_affinity();

    for (int i = 0; i < ITER; ++i)
        sumArraysOnGPU<<<grid, block> > >(d_A, d_B, d_C, nElem);
    cudaDeviceSynchronize();
    auto stop = T.Elapsed();
    std::cout << "in" << stop << "seconds" << std::endl;

    /*
    for(int i = 0; i < 64; ++i){
        for(int j = 0; j < 64; ++j)
            printf("%f ", *(c + i * N + j));
        printf("\n");
    }*/
    /*
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (cpu_c[i * N + j] != c[i * N + j])
            {
                printf("ERROR AT: c[%d][%d]: %f(cpu) %f(gpu)", i, j, cpu_c[i * N + j], c[i * N + j]);
                return 0;
            }
        }
    }*/
    printf("\n");
}