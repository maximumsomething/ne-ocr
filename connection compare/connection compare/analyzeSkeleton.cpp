#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "connections.h"


// idea: add (virtual?) connections between non adjacent intersections
// weight first step of compareConnections like second step, so a few non-matches don't matter

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


AnalyzedSkeleton analyzeSkeleton(Mat skel) {
	
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
	
	
	/*float nomImgSize = 0;
	for (int i = 0; i < skel.total(); ++i) {
		if (skel.data[i] != 0) ++nomImgSize;
	}*/
	float nomImgSize = (skel.rows + skel.cols)/2.0;
	
	// the skeleton with the intersections removed
	Mat noIntersections(skel.size(), CV_8U);
	for (int i = 0; i < noIntersections.total(); ++i) {
		noIntersections.data[i] = (labeledSkel.data[i] != labels::blank
								   && labeledSkel.data[i] != labels::intersection);
	}
	
	Mat labeledNoIntersections;
	// the Mat has 0 as background, and components labeled counting up from 1
	int numConns = connectedComponents(noIntersections, labeledNoIntersections, 8, CV_16U);
	numConns -= 1;
	
	//imwrite("/Users/max/Downloads/noi.png", labeledNoIntersections * 5000);
	
	// find corners in the connections
	// TODO
	
	
	Mat intersectionImg(skel.size(), CV_8U);
	for (int i = 0; i < intersectionImg.total(); ++i) {
		intersectionImg.data[i] = (labeledSkel.data[i] == labels::endpoint
								   || labeledSkel.data[i] == labels::intersection);
	}
	
	Mat labeledIntersections;
	int numIntersections = connectedComponents(intersectionImg, labeledIntersections, 8, CV_16U);
	numIntersections -= 1;
	
	std::vector<Connection> connections(numConns, { 0, -1, -1});
	std::vector<Intersection> intersections(numIntersections);
	std::vector<Point> foundConnectionEnds(numConns, Point(-1, -1));
	
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			uint16_t connectionNum = labeledNoIntersections.at<uint16_t>(y, x);
			if (connectionNum != 0) {
				connections[connectionNum - 1].pixLength += 1/nomImgSize;
				if (numNeighbors<uint16_t>(labeledNoIntersections, x, y, connectionNum) == 2) {
					// end point of connection
					
					uint16_t intersectionNum = getANeighbor(labeledIntersections, x, y);
					assert(intersectionNum != 0);
					if (connections[connectionNum - 1].intersect1 < 0) {
						
						foundConnectionEnds[connectionNum - 1] = Point(x, y);
						connections[connectionNum - 1].intersect1 = intersectionNum - 1;
					}
					else {
						connections[connectionNum - 1].intersect2 = intersectionNum - 1;
						double xSize = x - foundConnectionEnds[connectionNum - 1].x;
						double ySize = y - foundConnectionEnds[connectionNum - 1].y;
						
						//connections[connectionNum - 1].straightLength = sqrt(pow(xSize, 2) + pow(ySize, 2)) / nomImgSize;
						
						//connections[connectionNum - 1].angle = atan2(xSize, ySize);
					}
				}
			}
			uint16_t thisIntersectionNum = labeledIntersections.at<uint16_t>(y, x);
			if (thisIntersectionNum != 0) {
				intersections[thisIntersectionNum - 1].pos = Point2f(x / nomImgSize, y / nomImgSize);
			}
		}
	}
	for (int i = 0; i < numConns; ++i) {
		if (connections[i].intersect1 == -1 || connections[i].intersect2 == -1) {
			// bad connection
			//fprintf(stderr, "bad bone\n");
			connections.erase(connections.begin() + i);
			--i;
			--numConns;
			continue;
		}
		intersections[connections[i].intersect1].c.push_back(connections[i]);
		intersections[connections[i].intersect2].c.push_back(connections[i]);
	}
	
	return { intersections, connections, nomImgSize };
}
