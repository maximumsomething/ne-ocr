#include "createSkeleton.h"

#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>


using namespace cv;

void createSkeleton(const char* inPath, const char* outPath, int diagonalSize) {
	
	Mat in = cv::imread(inPath, IMREAD_GRAYSCALE);
	Mat big;
	
	double scaleFactor = diagonalSize / sqrt(pow(in.cols, 2) + pow(in.rows, 2));
	
	resize(in, big, {0,0}, scaleFactor, scaleFactor, INTER_CUBIC);
	
	Mat bwimage;
	//adaptiveThreshold(big, bwimage, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 149, 2);
	threshold(big, bwimage, 0, 255, THRESH_OTSU+THRESH_BINARY_INV);
	
	//imshow("Threshold", bwimage);
	
	// Open and close the image to eliminate dots
	
	
	// Remove small white dots
	constexpr float dotArea = 0.001;
	int maxDotPix = dotArea * bwimage.rows * bwimage.cols;
	Mat connCompStats, connComp, junkArray;
	connectedComponentsWithStats(bwimage, connComp, connCompStats, junkArray);
	
	for (int y = 0; y < bwimage.rows; ++y)
	for (int x = 0; x < bwimage.cols; ++x) {
		int32_t label = connComp.at<int32_t>(y, x);
		if (label > 0) bwimage.at<uint8_t>(y, x) = (connCompStats.at<int32_t>(label, CC_STAT_AREA) > maxDotPix) ? 255 : 0;
	}
	
	//imshow("opened", bwimage);
	
	constexpr float dotSize = 0.03;
	int strelSize = (int) (dotSize * diagonalSize);
	Mat strel = getStructuringElement(MORPH_ELLIPSE, Size2i(strelSize, strelSize));
	
	//morphologyEx(bwimage, bwimage, MORPH_OPEN, strel);
	//imshow("Opened", bwimage);
	//morphologyEx(bwimage, bwimage, MORPH_CLOSE, strel);
	//imshow("Closed", bwimage);
	// Remove black dots and connect erroneously disconnected components
	dilate(bwimage, bwimage, strel);
	//imshow("Dilated", bwimage);
	
	// Add a single black pixel to the sides so skeleton doesn't treat the side specially
	copyMakeBorder(bwimage, bwimage, 1, 1, 1, 1, BORDER_CONSTANT, 0);
	
	Mat skel;
	
	// Zhangsuen has better overall shape, but has places where a non-intersection pixel has more than 2 neighbors
	ximgproc::thinning(bwimage, skel, ximgproc::THINNING_ZHANGSUEN);
	ximgproc::thinning(skel, skel, ximgproc::THINNING_GUOHALL);
	
	//imshow("skeleton", skel);
	//waitKey();
	
	assert(skel.type() == CV_8U);
	// Crop the output to the boundaries of the skeleton
	int top = INT_MAX, left = INT_MAX, bottom = 0, right = 0;
	
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			if (skel.at<uint8_t>(y, x) > 0) {
				if (y < top) top = y;
				if (x < left) left = x;
				if (y > bottom) bottom = y;
				if (x > right) right = x;
			}
		}
	}
	Mat cropSkel = skel(cv::Rect(left, top, right - left + 1, bottom - top + 1));
	
	imwrite(outPath, cropSkel);
}
