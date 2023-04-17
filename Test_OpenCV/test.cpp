/*
 * Command to compile the Code 
 * g++ test.cpp -o test `pkg-config opencv4 --libs --cflags`
 * 
 * Command to run the application
 * ./test <full-image-path>
 * 
 */

#include <opencv2/opencv.hpp>

int main(int argc, char* argv[])
{
    cv::Mat image = cv::imread(argv[1]);
    cv::Mat binaryImage, grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    cv::threshold(grayImage, binaryImage, 200, 255, cv::THRESH_BINARY);
    cv::namedWindow("Original_Image", cv::WINDOW_NORMAL);
    cv::resizeWindow("Original_Image", 350,350);
    cv::imshow("Original_Image", image);
    cv::namedWindow("Gray_Image", cv::WINDOW_NORMAL);
    cv::resizeWindow("Gray_Image", 350,350);
    cv::imshow("Gray_Image", grayImage);
    cv::namedWindow("Binary_Image", cv::WINDOW_NORMAL);
    cv::resizeWindow("Binary_Image", 350,350);
    cv::imshow("Binary_Image", binaryImage);
    cv::waitKey(0);
    return 0;
}
