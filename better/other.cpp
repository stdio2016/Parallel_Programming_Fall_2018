// for any platform that has installed OpenCV
#include <stdio.h>
#include "platform.h"

// openCV libraries for showing the images dont change
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;

void show_image(const char *outputblur_name) {
    // show the output file
    Mat img = imread(outputblur_name);
    imshow("Current progress", img);
    waitKey(20);
}

int main(int argc, char *argv[]) {
    return my_main(argc, argv);
}
