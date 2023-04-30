#include <opencv2/opencv.hpp>
#include <stdint.h>

int main(int argc, char *argv[]){
	cv::Mat inputImage = cv::imread(argv[1]);
	cv::Mat grayImage, binaryImage;
	std::vector<cv::Vec4i> lines;
	cv::cvtColor(inputImage, grayImage, cv::COLOR_BGR2GRAY);
	cv::threshold(grayImage, binaryImage, 150, 255, cv::THRESH_BINARY);
	cv::Canny(binaryImage,binaryImage,10,350);
	cv::HoughLinesP(binaryImage, lines, 1, CV_PI/180, std::atoi(argv[2]),0, 0 );
	std::cout << "LINES SIZE - " << lines.size() << std::endl;
   for( size_t i = 0; i < lines.size(); i++ )
    {
        //cv::line( binaryImage, cv::Point(),
        //cv::Point( lines[i][2], lines[i][3]), cv::Scalar::all(255), 3, 8 );
		cv::circle(inputImage, cv::Point(lines[i][0], lines[i][1]), 5, cv::Scalar(125,0,0), -1);
		std::cout << ""
		//cv::circle(inputImage, cv::Point(lines[i][2], lines[i][3]), 5, cv::Scalar(125,0,0), -1);
    }
	cv::line(binaryImage, cv::Point(binaryImage.cols/2, 0), cv::Point(binaryImage.cols/2, binaryImage.rows), cv::Scalar::all(125), 2);
	cv::namedWindow("InputImage", cv::WINDOW_NORMAL);
	cv::resizeWindow("InputImage", 700,700);
	cv::imshow("InputImage", inputImage);
	cv::namedWindow("GrayImage", cv::WINDOW_NORMAL);
	cv::resizeWindow("GrayImage", 700,700);
	cv::imshow("GrayImage", grayImage);
	cv::namedWindow("BinaryImage", cv::WINDOW_NORMAL);
	cv::resizeWindow("BinaryImage", 700,700);
	cv::imshow("BinaryImage", binaryImage);
	cv::waitKey(0);
	return 0;
	
}
