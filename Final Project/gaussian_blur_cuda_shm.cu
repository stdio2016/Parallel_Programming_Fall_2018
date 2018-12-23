#include "bmpReader.h"
#include "bmpReader.cpp"
#include <cuda.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <string>

// openCV libraries for showing the images dont change
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define ull unsigned long long int

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0
int img_width, img_height;

int FILTER_SIZE;
ull FILTER_SCALE;
ull *filter_G;

unsigned char *input_image, *pic_blur, *output_image;

// variables for cuda parallel processing
int TILE_WIDTH;

__constant__ unsigned long long int* const_ptr;
__global__ void cuda_gaussian_filter(unsigned char* cuda_input_image, unsigned char* cuda_output_image,int img_width, int img_height, int shift, unsigned long long int* cuda_filter_G, int ws, unsigned long long int FILTER_SCALE, int block_width)
{
    // for CUDA parallelization
    int cuda_width = blockIdx.x * blockDim.x + threadIdx.x;
    int cuda_height = blockIdx.y * blockDim.y + threadIdx.y;

    // for shared memory
    // __shared__ unsigned char smem[block_width * block_width]; 
    __constant__ unsigned long long int* const_ptr = cuda_filter_G;
    unsigned long long int tmp = 0;
    int a, b;

    
    if (cuda_width >= img_width || cuda_height >= img_height)
    {
        return;
    }

    for (int j = 0; j  <  ws; j++)
    {
        for (int i = 0; i  <  ws; i++)
        {
            a = cuda_width + i - (ws / 2);
            b = cuda_height + j - (ws / 2);

            tmp += const_ptr[j * ws + i] * cuda_input_image[3 * (b * img_width + a) + shift]; 
        }
    }
    tmp /= FILTER_SCALE;

    if (tmp > 255)
    {
        tmp = 255;
    }
    cuda_output_image[3 * (cuda_height * img_width + cuda_width) + shift] = tmp;
}
// show the progress of gaussian segment by segment
// const float segment[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
void write_and_show(BmpReader* bmpReader, string outputblur_name, int k)
{
    bmpReader->WriteBMP(outputblur_name.c_str(), img_width, img_height, output_image);

    // show the output file
    Mat img = imread(outputblur_name);
    while(1)
    {
        imshow("Current progress", img);
        if (waitKey(0) % 256 == 27)
        {
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    TILE_WIDTH = 1024;

    // read input filename
    string inputfile_name;
    string outputblur_name;

    if (argc < 2)
    {
        printf("Please provide filename for Gaussian Blur. usage ./gb_std.o <BMP image file>");
        return 1;
    }
    else if (argc == 3)
    {
        sscanf(argv[2], "%d", &TILE_WIDTH);
        printf("Testing with %d threads in each CUDA block\n", TILE_WIDTH);
    }

    // read Gaussian mask file from system
    FILE* mask;
    mask = fopen("mask_Gaussian.txt", "r");
    fscanf(mask, "%d", &FILTER_SIZE);
    filter_G = new unsigned long long [FILTER_SIZE];

    for (int i = 0; i < FILTER_SIZE; i++)
    {
        fscanf(mask, "%llu", &filter_G[i]);
    }

    FILTER_SCALE = 0; //recalculate
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        FILTER_SCALE += filter_G[i];	
    }
    fclose(mask);

    // main part of Gaussian blur
    BmpReader* bmpReader = new BmpReader();

    // platform information
    int num = 0;
    cudaGetDeviceCount(&num);

    // get gpu properties
    cudaDeviceProp prop;
    if (num > 0)
    {
        cudaGetDeviceProperties(&prop, 0);
        // get device name
        cout << "Device: " <<prop.name << endl;
    }
    else
    {
        printf("No NVIDIA GPU detected! \n");
        return 1;
    }


    // read input BMP file
    inputfile_name = argv[1];
    input_image = bmpReader -> ReadBMP(inputfile_name.c_str(), &img_width, &img_height);
    printf("Filter scale = %llu, filter size %d x %d and image size W = %d, H = %d\n", FILTER_SCALE, (int)sqrt(FILTER_SIZE), (int)sqrt(FILTER_SIZE), img_width, img_height);

    // allocate space for output image
    int resolution = 3 * (img_width * img_height); //padding
    output_image = (unsigned char*)malloc(resolution * sizeof(unsigned char));
    memset(output_image, 0, sizeof(output_image));
    // apply the Gaussian filter to the image, RGB respectively
    string tmp(inputfile_name);

    //---------------------CUDA main part-------------------------//
    // allocate space
    cudaError_t cuda_err, cuda_err2, cuda_err3;

    unsigned char* cuda_input_image;
    unsigned char* cuda_output_image;
    unsigned long long int* cuda_filter_G;
    cuda_err = cudaMalloc((void**) &cuda_input_image, resolution * sizeof(unsigned char));
    cuda_err2 = cudaMalloc((void**) &cuda_output_image, resolution * sizeof(unsigned char));
    cuda_err3 = cudaMalloc((void**) &cuda_filter_G, FILTER_SIZE * sizeof(unsigned long long int)); //dont forget to allocate space for it
    if(cuda_err != cudaSuccess || cuda_err2 != cudaSuccess || cuda_err3 != cudaSuccess)
    {
        printf("Failed with error part1 %s \n", cudaGetErrorString(cuda_err));
        printf("Failed with error part1 err2: %s \n", cudaGetErrorString(cuda_err2));
        printf("Failed with error part1 err3: %s \n", cudaGetErrorString(cuda_err3));
        return 1;

    }

    // copy memory from host to GPU
    cuda_err = cudaMemcpy(cuda_input_image, input_image, resolution * sizeof(unsigned char), cudaMemcpyHostToDevice);
    cuda_err2 = cudaMemcpy(cuda_output_image, output_image, resolution * sizeof(unsigned char), cudaMemcpyHostToDevice);
    cuda_err3 = cudaMemcpy(cuda_filter_G, filter_G, FILTER_SIZE* sizeof(unsigned long long int), cudaMemcpyHostToDevice);
    if(cuda_err != cudaSuccess || cuda_err2 != cudaSuccess || cuda_err3 != cudaSuccess)
    {
        printf("Failed with error part2 err: %s \n", cudaGetErrorString(cuda_err));
        printf("Failed with error part2 err2: %s \n", cudaGetErrorString(cuda_err2));
        printf("Failed with error part2 err3: %s \n", cudaGetErrorString(cuda_err3));
        return 2;
    }

    // grid and block, divide the image into 1024 per block
    const dim3 block_size((int)sqrt(TILE_WIDTH), (int) sqrt(TILE_WIDTH), 1);
    const dim3 grid_size(img_width / block_size.x + 1, img_height / block_size.y + 1, 1);
    for (int i = 2; i >= 0; i--) //R G B channel respectively
    {
        cuda_gaussian_filter<<<grid_size, block_size>>>(cuda_input_image, cuda_output_image, img_width, img_height, i, cuda_filter_G, (int)sqrt((int)FILTER_SIZE), FILTER_SCALE, (int)sqrt(TILE_WIDTH));
        cuda_err = cudaDeviceSynchronize();

        if(cuda_err != cudaSuccess)
        {
            printf("Failed with error part3 %s \n", cudaGetErrorString(cuda_err));
            return 3;
        }
    }

    // copy memory from GPU to host
    cudaMemcpy(output_image, cuda_output_image, resolution * sizeof(unsigned char), cudaMemcpyDeviceToHost);
    //---------------------CUDA main part-------------------------//

    // write output BMP file
    outputblur_name = inputfile_name.substr(0, inputfile_name.size() - 4)+ "_blur_cuda.bmp";
    bmpReader->WriteBMP(outputblur_name.c_str(), img_width, img_height, output_image);
    // if demo, decomment this to show
    // write_and_show(bmpReader, outputblur_name, 0);
    // free memory space
    free(input_image);
    free(output_image);
    cudaFree(cuda_input_image);
    cudaFree(cuda_output_image);
    cudaFree(cuda_filter_G);

    // diff pic if needed
    /* 
       printf("diff img \n");

       string inputfile_name2 = inputfile_name.substr(0, inputfile_name.size() - 4)+ "_blur.bmp";
       unsigned char* input_image2 = bmpReader -> ReadBMP(inputfile_name2.c_str(), &img_width, &img_height);

       string inputfile_name3 = inputfile_name.substr(0, inputfile_name.size() - 4)+ "_blur_cuda.bmp";
       unsigned char* input_image3 = bmpReader -> ReadBMP(inputfile_name3.c_str(), &img_width, &img_height);
       cout << "name 2 3 " << inputfile_name2 << " , " << inputfile_name3 << endl;
       for (int j = 0; j < img_width * img_height * 3; j+=3)
       {
       if(input_image2[j] != input_image3[j] 
       || input_image2[j + 1] != input_image3[j + 1]
       || input_image2[j + 2] != input_image3[j + 3])
       {
       printf("Normal %d, %d, %d Dim %d, %d, %d \n", input_image2[j], input_image2[j + 1], input_image2[j +2], input_image3[j], input_image3[j + 1], input_image3[j +2]);
       }
       }
     */


    return 0;
}