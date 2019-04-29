#pragma warning(disable : 4996)
#include "bmpReader.h"
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <string>
#include "platform.h"

using namespace std;

#define uint32 unsigned int

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0
int img_width, img_height;

int FILTER_SIZE;
uint32 FILTER_SCALE;
uint32 *filter_G;
float *filter_F; // the same as filter_G

unsigned char *pic_in, *pic_out;
float *pic_x;

int clamp(float rgb) {
    if (rgb != rgb) return 0;
    if (rgb > 255) return 255;
    if (rgb < 0) return 0;
    return (int)rgb;
}

float gaussian_filter_x(int w, int h,int shift)
{
    int ws = FILTER_SIZE;
    int target = 0;
    uint32 tmp = 0;
    int a;

    for (int i = 0; i  <  ws; i++)
    {
        a = w + i - (ws / 2);

        // detect for borders of the image
        if (a < 0 || a >= img_width)
        {
            continue;
        } 
        tmp += filter_G[i] * pic_in[3 * (h * img_width + a) + shift];
    }

    return (float) tmp / FILTER_SCALE;
}
unsigned char gaussian_filter_y(int w, int h,int shift)
{
    int ws = FILTER_SIZE;
    int target = 0;
    float tmp = 0;
    int a;

    for (int i = 0; i  <  ws; i++)
    {
        a = h + i - (ws / 2);

        // detect for borders of the image
        if (a < 0 || a >= img_height)
        {
            continue;
        } 
        tmp += filter_F[i] * pic_x[3 * (a * img_width + w) + shift];
    }

    tmp /= FILTER_SCALE;
    if (tmp < 0)
    {
        tmp = 0;
    } 
    if (tmp > 255)
    {
        tmp = 255;
    }
    return (unsigned char)tmp;
}
// show the progress of gaussian segment by segment
const float segment[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
void write_and_show(BmpReader* bmpReader, string outputblur_name)
{
    bmpReader->WriteBMP(outputblur_name.c_str(), img_width, img_height, pic_out);

    // show the output file
    show_image(outputblur_name.c_str());
}

int my_main(int argc, char* argv[])
{
    // read input filename
    string inputfile_name;
    string outputblur_name;

    if (argc < 2)
    {
        printf("Please provide filename for Gaussian Blur. usage ./gb_sep_omp.o <BMP image file>");
        return 1;
    }

    // read Gaussian mask file from system
    FILE* mask;
    mask = fopen("mask_Gaussian_1D.txt", "r");
    if (mask == NULL) {
        fprintf(stderr, "mask_Gaussian.txt not found. Use create_matrix.cpp to generate one.\n");
        exit(2);
    }
    fscanf(mask, "%d", &FILTER_SIZE);
    filter_G = new uint32 [FILTER_SIZE];
    filter_F = new float [FILTER_SIZE];

    for (int i = 0; i < FILTER_SIZE; i++)
    {
        fscanf(mask, "%u", &filter_G[i]);
        filter_F[i] = filter_G[i];
    }

    FILTER_SCALE = 0; //recalculate
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        FILTER_SCALE += filter_G[i];
        //printf("filter_G %d ", filter_G[i]);
    }
    fclose(mask);

    // main part of Gaussian blur
    BmpReader* bmpReader = new BmpReader();
    for (int k = 1; k < argc; k++)
    {
        // read input BMP file
        inputfile_name = argv[k];
        pic_in = bmpReader -> ReadBMP(inputfile_name.c_str(), &img_width, &img_height);
        printf("Filter scale = %u, filter size %d x %d and image size W = %d, H = %d\n", FILTER_SCALE, FILTER_SIZE, FILTER_SIZE, img_width, img_height);

        int resolution = 3 * (img_width * img_height);
        // allocate space for x axis blurred image
        pic_x = (float*)malloc(3 * img_width * img_height * sizeof(float));
        // allocate space for output image
        pic_out = (unsigned char*)malloc(3 * img_width * img_height * sizeof(unsigned char));

        //apply the Gaussian filter to the image, RGB respectively
        string tmp(inputfile_name);
        int segment_cnt = 1;
        outputblur_name = inputfile_name.substr(0, inputfile_name.size() - 4)+ "_blur_sep_omp.bmp";

        // blur in x axis
        #pragma omp parallel for num_threads(4)
        for (int j = 0; j < img_height; j++) 
        {
            for (int i = 0; i < img_width; i++)
            {
                pic_x[3 * (j * img_width + i) + MYRED] = gaussian_filter_x(i, j, MYRED);
                pic_x[3 * (j * img_width + i) + MYGREEN] = gaussian_filter_x(i, j, MYGREEN);
                pic_x[3 * (j * img_width + i) + MYBLUE] = gaussian_filter_x(i, j, MYBLUE);
                pic_out[3 * (j * img_width + i) + MYRED] = clamp(pic_x[3 * (j * img_width + i) + MYRED]);
                pic_out[3 * (j * img_width + i) + MYGREEN] = clamp(pic_x[3 * (j * img_width + i) + MYGREEN]);
                pic_out[3 * (j * img_width + i) + MYBLUE] = clamp(pic_x[3 * (j * img_width + i) + MYBLUE]);
            }
        }
        write_and_show(bmpReader, outputblur_name);
        // blur in y axis
        #pragma omp parallel for num_threads(4)
        for (int j = 0; j < img_height; j++) 
        {
            for (int i = 0; i < img_width; i++)
            {
                pic_out[3 * (j * img_width + i) + MYRED] = gaussian_filter_y(i, j, MYRED);
                pic_out[3 * (j * img_width + i) + MYGREEN] = gaussian_filter_y(i, j, MYGREEN);
                pic_out[3 * (j * img_width + i) + MYBLUE] = gaussian_filter_y(i, j, MYBLUE);
            }
        }
        // write output BMP file
        write_and_show(bmpReader, outputblur_name);
        // if demo, decomment this to show until ESC is pressed
        
        
        /*Mat img = imread(outputblur_name);
        while(1)
        {
            imshow("Current progress", img);
            if (waitKey(0) % 256 == 27)
            {
                break;
            }
        }*/
        
        // free memory space
        free(pic_in);
        free(pic_x);
        free(pic_out);
    }

    return 0;
}
