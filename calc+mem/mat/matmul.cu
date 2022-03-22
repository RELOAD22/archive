#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <algorithm>
#include <device_functions.h>
using namespace std;
#define NELEM 32
#define WIDTH 32
#define ITEMS_PER_THREAD 8
#define M 1024
#define N 1024
#define K 1024

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

int main()
{

    auto mata_size = M * K * sizeof(float);
    auto matb_size = K * N * sizeof(float);
    auto matc_size = M * N * sizeof(float);

    auto elems_size = NELEM * NELEM;

    float *a = (float *)malloc(mata_size);
    float *b = (float *)malloc(matb_size);
    float *c = (float *)malloc(matc_size);
    float *cpu_c = (float *)malloc(matc_size);

    InitData(a, b, M, N, K);
    printf("init finish\n");

    // CpuMatmul(a, b, cpu_c, M, N, K);
    printf("cpu compute finish\n");

    float *d_a, *d_b, *d_c;
    cudaMalloc(&d_a, mata_size);
    cudaMalloc(&d_b, matb_size);
    cudaMalloc(&d_c, matc_size);
    cudaMemset(d_c, 0, matc_size);

    cudaMemcpy(d_a, a, mata_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, matb_size, cudaMemcpyHostToDevice);
    printf("begin kernel:\n");
    int gridsize = (max(M, N) + WIDTH - 1) / WIDTH;
    printf("grid:%d %d \n", gridsize, gridsize);
    dim3 grid(gridsize, gridsize);

    dim3 block(WIDTH, WIDTH / ITEMS_PER_THREAD);
    matmul<<<grid, block> > >(d_a, d_b, d_c, M, N, K);
    cudaDeviceSynchronize();

    cudaMemcpy(c, d_c, matc_size, cudaMemcpyDeviceToHost);
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
    printf("%f ", *(c));
    printf("%f ", *(c + M * N / 2 + N / 2));
    printf("%f ", *(c + M * N - 1));
    printf("\n");
}