#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "connections.h"


using namespace cv;

// number of neighbors including self
template<typename T>
int numNeighbors(Mat& img, int x, int y, int neighborsEqual) {
	Rect bounds = Rect(x-1, y-1, 3, 3) & Rect(0, 0, img.cols, img.rows);
	Mat neighbors = img(bounds);
	int num = 0;
	for (int i = 0; i < neighbors.rows; ++i) {
		for (int j = 0; j < neighbors.cols; ++j) {
			
			if (neighbors.at<T>(i, j) == neighborsEqual) ++num;
			//printf("i: %i j: %i neigh: %i\n", i, j, neighbors.at<T>(i, j));
		}
	}
	return num;
}

uint16_t getANeighbor(Mat& img, int x, int y) {
	
	Rect bounds = Rect(x-1, y-1, 3, 3) & Rect(0, 0, img.cols, img.rows);
	Mat neighbors = img(bounds);
	for (int i = 0; i < neighbors.rows; ++i) {
		for (int j = 0; j < neighbors.cols; ++j) {
			uint16_t pix = neighbors.at<uint16_t>(i, j);
			if (pix != 0) return pix;
		}
	}
	//return 0;
	assert(false);
}

struct labels {
	enum l : uint8_t {
		blank = 0,
		normal = 1,
		endpoint = 2,
		intersection = 3,
		
	};
};

/*Point getAnEndpoint(Mat& skel) {
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			int neigh = numNeighbors(skel, x, y);
			if (neigh == 2) return Point(x, y);
		}
	}
	
	assert(false);
}*/


ConnectionList getConnections(Mat skel) {
	
	Mat labeledSkel(skel.size(), CV_8U);
	
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			if (skel.at<uint8_t>(y, x) == 0) labeledSkel.at<uint8_t>(y,x) = labels::blank;
			else {
				int neigh = numNeighbors<uint8_t>(skel, x, y, 255);
				if (neigh == 3) labeledSkel.at<uint8_t>(y,x) = labels::normal;
				else if (neigh == 2) labeledSkel.at<uint8_t>(y,x) = labels::endpoint;
				else if (neigh > 3) labeledSkel.at<uint8_t>(y,x) = labels::intersection;
				else {
					fprintf(stderr, "dot\n");
				}
			}
		}
	}
//	imshow("", skel);
//	waitKey(0);
	//imwrite("/Users/max/Downloads/labeledSkel.png", labeledSkel * 75);
	
	
	double nomImgSize = 0;
	for (int i = 0; i < skel.total(); ++i) {
		if (skel.data[i] != 0) ++nomImgSize;
	}
	
	Mat noIntersections(skel.size(), CV_8U);
	for (int i = 0; i < noIntersections.total(); ++i) {
		noIntersections.data[i] = (labeledSkel.data[i] != labels::blank
								   && labeledSkel.data[i] != labels::intersection);
	}
	Mat labeledNoIntersections;
	int numConns = connectedComponents(noIntersections, labeledNoIntersections, 8, CV_16U);
	
	//imwrite("/Users/max/Downloads/noi.png", labeledNoIntersections * 5000);
	
	Mat intersections(skel.size(), CV_8U);
	for (int i = 0; i < intersections.total(); ++i) {
		intersections.data[i] = (labeledSkel.data[i] == labels::endpoint
								   || labeledSkel.data[i] == labels::intersection);
	}
	Mat labeledIntersections;
	int numIntersections = connectedComponents(intersections, labeledIntersections, 8, CV_16U);
	
	std::vector<Connection> connections(numConns - 1);
	std::vector<Point> foundConnectionEnds(numConns - 1, Point(-1, -1));
	
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			uint16_t val = labeledNoIntersections.at<uint16_t>(y, x);
			if (val != 0 && numNeighbors<uint16_t>(labeledNoIntersections, x, y, val) == 2) {
				// end point of connection
				
				uint16_t intersectionNum = getANeighbor(labeledIntersections, x, y);
				assert(intersectionNum != 0);
				if (foundConnectionEnds[val - 1].x < 0) {
					
					foundConnectionEnds[val - 1] = Point(x, y);
					connections[val - 1].intersect1 = intersectionNum - 1;
				}
				else {
					connections[val - 1].intersect2 = intersectionNum - 1;
					double xSize = x - foundConnectionEnds[val - 1].x;
					double ySize = y - foundConnectionEnds[val - 1].y;
					
					connections[val - 1].length = sqrt(pow(xSize, 2) + pow(ySize, 2)) / nomImgSize;
					
					connections[val - 1].angle = atan2(xSize, ySize);
				}
			}
		}
	}
	return { numIntersections - 1, connections };
}
