#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <CL/opencl.h>
#include <fstream>
#include <iostream>
/***************************Image histogram algorithm abstract by myself*************************************
 * Calculate the statistic data of the (R, G, B) of image, 
 * where each color can be describe as
 * 0xRRGGBB and the R_stat[RR]++; G_stat[GG]++; B_stat[BB]++;, after the iterations
 * the result can be shown as an image histogram
 * **********************************************************************************************************/

/* OpenCL kernel function 
    __kernel void histogram(__global unsigned char* image_data, __global unsigned int* result_data, unsigned int size)
    {
        int idx = get_global_id(0);
        if(idx > size - 2) 
        {
            return;
        }
        result_data[idx] = image_data[idx];
        result_data[idx + 1] = image_data[idx + 1];
        result_data[idx + 2] = image_data[idx + 2];
    }
 *
 * */

unsigned int * histogram(unsigned char *image_data, unsigned int _size) {

    unsigned char *img = image_data;
    unsigned int *ref_histogram_results;
    unsigned int *ptr;

    ref_histogram_results = (unsigned int *)malloc(256 * 3 * sizeof(unsigned int));
    ptr = ref_histogram_results;
    memset (ref_histogram_results, 0x0, 256 * 3 * sizeof(unsigned int));
    // data encoded with R G B R G B R G B...
    // histogram of R
    for (unsigned int i = 0; i < _size; i += 3)
    {
        unsigned int index = img[i];
        ptr[index]++;
    }

    // histogram of G
    ptr += 256;
    for (unsigned int i = 1; i < _size; i += 3)
    {
        unsigned int index = img[i];
        ptr[index]++;
    }

    // histogram of B
    ptr += 256;
    for (unsigned int i = 2; i < _size; i += 3)
    {
        unsigned int index = img[i];
        ptr[index]++;
    }

    return ref_histogram_results;
}

int main(int argc, char const *argv[])
{    
    //--------------------input size and required variables----//
    unsigned char* image; // For input image 
    unsigned int* histogram_results; // For output result
    unsigned int i = 0, a, input_size;
    
    //--------------------file IO and allocate memory-----------//
    FILE* inFile = fopen("input", "r");
    FILE* outFile = fopen("0416324.out", "w");
    fscanf(inFile, "%u", &input_size);
    
    image = (unsigned char* ) malloc (sizeof(unsigned char) * input_size);// R, G, B ranging from 0x00 to 0xFF
    histogram_results = (unsigned int* ) malloc (sizeof(unsigned int) * 256 * 3);// R 256, G 256, B 256, total 768 statistical data
    memset(histogram_results, 0, sizeof(sizeof(unsigned int) * 256 * 3));

    while(fscanf(inFile, "%u", &a) != EOF)
    {
        image[i++] = a;
    }
    i = 0;

    //-----------------------OpenCL init-----------------------//
    cl_int cl_err, cl_err2, cl_err3, cl_err4; // return value of CL functions, check whether OpenCL platform errors
    cl_uint num_device, num_plat;
    cl_device_id device_id;
    cl_platform_id plat_id;
    cl_context ctx;
    cl_command_queue que;

    // num_entries = 1 for 1 GPU card
    cl_err = clGetPlatformIDs(1, &plat_id, &num_plat); // get platform id and number
    cl_err2 = clGetDeviceIDs(plat_id, CL_DEVICE_TYPE_GPU, 1, &device_id,  &num_device);
    ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &cl_err3);
    que = clCreateCommandQueue(ctx, device_id, 0, &cl_err4);

    if (cl_err == CL_SUCCESS && cl_err2 == CL_SUCCESS && cl_err3 == CL_SUCCESS && cl_err4 == CL_SUCCESS)
    {
        printf("CL platform init OK \n");
    }
    else
    {
        printf("CL platform init failed \n");
        return 1;
    }

    //----------------------OpenCL memory allocation-----------//
    cl_mem img_cl = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(unsigned char) * input_size, NULL, &cl_err); // allocate mem space for input image
    cl_mem his_cl = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 256 * 3, NULL, &cl_err2); // allocate mem space for input image
    if (cl_err == CL_SUCCESS && cl_err2 == CL_SUCCESS)
    {
        printf("CL memory allocation OK\n");
    }
    else
    {
        printf("CL memory allocation failed\n");
        return 1;
    }
    //--------------------memory from host to device--------------//
    cl_err = clEnqueueWriteBuffer(que, img_cl, CL_TRUE, 0, sizeof(unsigned char) * input_size, image, 0, NULL, NULL); // input image from host to device
    cl_err2 = clEnqueueWriteBuffer(que, his_cl, CL_TRUE, 0, sizeof(unsigned int) * 256 * 3, histogram_results, 0, NULL, NULL); // histogram result from host to device
    if (cl_err == CL_SUCCESS && cl_err2 == CL_SUCCESS)
    {
        printf("CL memory enqueueing OK\n");
    }
    else
    {
        printf("CL memory enqueueing failed\n");
        return 1;
    }
    
    //-------------------kernel program execution-----------------//
    histogram_results = histogram(image, input_size);
    for(unsigned int i = 0; i < 256 * 3; ++i) 
    {
        if (i % 256 == 0 && i != 0)
        {
            fprintf(outFile, "\n");
        }
        fprintf(outFile, "%u ", histogram_results[i]);
    }
    fclose(inFile);
    fclose(outFile);
    return 0;
}
