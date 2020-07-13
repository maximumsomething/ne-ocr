#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

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
Point2i getANeighborLocation(Mat& img, Point center, int neighborsEqual, Point2i notThis) {
	
	Rect bounds = Rect(center.x-1, center.y-1, 3, 3) & Rect(0, 0, img.cols, img.rows);
	
	//std::cout << "neighEq: " << neighborsEqual << " neighbors: ";
	
	for (int i = 0; i < bounds.width; ++i) {
		for (int j = 0; j < bounds.height; ++j) {
			
			Point2i neigh(i + bounds.x, j + bounds.y);
			//std::cout << img.at<uint16_t>(neigh) << " ";
			
			if (neigh != center && neigh != notThis
				&& img.at<uint16_t>(neigh) == neighborsEqual) {
				
				return neigh;
			}
		}
	}
	//std::cout << std::endl;
	return { -1, -1 };
}

typedef std::vector<Point2i>::const_iterator PointVecIt;

// Find the spot to split the curve that minimizes the length difference
// which is eqivalent to maximizing the sum of the straight lengths of the "legs"
PointVecIt splitCurveInTwo(PointVecIt begin, PointVecIt last) {
	
	assert(last - begin > 4);
	double bestLength = 0;
	std::vector<Point2i>::const_iterator bestSpot;
	
	for (auto j = begin; j <= last; ++j) {
		
		double lengthSum = sqrt(pow(j->x - begin->x, 2) + pow(j->y - begin->y, 2)) +
						  sqrt(pow(j->x - last->x, 2) + pow(j->y - last->y, 2));
		
		if (lengthSum > bestLength) {
			bestLength = lengthSum;
			bestSpot = j;
		}
	}
	return bestSpot;
}

// Splits curve of pixels into multiple straight-ish segments
// Returns spots that it's split at
std::vector<PointVecIt> splitCurve(PointVecIt begin, PointVecIt end) {
	constexpr int sidePix = 2;
	constexpr double maxLengthRatio = 0.85;

	
	double cumuLength = 0;
	for (auto i = begin + sidePix; i < end - sidePix; ++i) {
		cumuLength += sqrt(pow((i - sidePix)->x - (i + sidePix)->x, 2) +
						   pow((i - sidePix)->y - (i + sidePix)->y, 2))
		/ (sidePix * 2);
		
		double straightLength = sqrt(pow(i->x - begin->x, 2) + pow(i->y - begin->y, 2));
		
		if (straightLength > sidePix*2+1 && cumuLength * maxLengthRatio > straightLength) {
			
			
			// if more than 3/4 the way to the end
			if ((i - begin) * 4 >= (end - begin) * 3) {
				return std::vector<PointVecIt>(1, splitCurveInTwo(begin, end - 1));
			}
			else {
				PointVecIt spot = splitCurveInTwo(begin, i);
				
				// Run again, starting at the found spot, to see if the curve needs to be split again
				std::vector<PointVecIt> splitSpots = splitCurve(spot, end);
				
				if (splitSpots.size() == 0) {
					// No more splits, so split the whole thing more evenly
					return std::vector<PointVecIt>(1, splitCurveInTwo(begin, end - 1));
				}
				else {
					splitSpots.push_back(spot);
					return splitSpots;
				}
			}
		}
	}
	
	return std::vector<PointVecIt>();
}


struct labels {
	enum l : uint8_t {
		blank = 0, // Not part of the skeleton.
		normal = 1, // In the middle of a line of pixels.
		endpoint = 2, // The end of a line.
		intersection = 3, // The intersection between 3 or more lines.
		
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
	
	// Labels the "on" pixels of the skeleton based on the number of neighbors it has.
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
					// This skeleton has a single-pixel dot, which should not happen.
					fprintf(stderr, "dot\n");
				}
			}
		}
	}
//	imshow("", skel);
//	waitKey(0);
	//imwrite("/Users/max/Downloads/labeledSkel.png", labeledSkel * 75);
	
	// Calculates the "nominal image size", which all dimensions are divided by, necessary to compare differently sized skeletons to each other.
	/*double nomImgSize = 0;
	for (int i = 0; i < skel.total(); ++i) {
		if (skel.data[i] != 0) ++nomImgSize;
	}*/
	double nomImgSize = (skel.rows + skel.cols)/2.0;
	
	// the skeleton with the intersections removed, making each connection a seperate group of pixels
	Mat noIntersections(skel.size(), CV_8U);
	for (int i = 0; i < noIntersections.total(); ++i) {
		noIntersections.data[i] = (labeledSkel.data[i] != labels::blank
								   && labeledSkel.data[i] != labels::intersection);
	}
	
	Mat labeledNoIntersections;
	// the Mat has 0 as background, and components (connections) labeled counting up from 1
	int numConns = cv::connectedComponents(noIntersections, labeledNoIntersections, 8, CV_16U);
	numConns -= 1;
	
	//imwrite("/Users/max/Downloads/noi.png", labeledNoIntersections * 5000);
	
	// The skeleton with only the intersections
	Mat intersectionImg(skel.size(), CV_8U);
	for (int i = 0; i < intersectionImg.total(); ++i) {
		intersectionImg.data[i] = (labeledSkel.data[i] == labels::endpoint
								   || labeledSkel.data[i] == labels::intersection);
	}
	
	Mat labeledIntersections;
	int numIntersections = cv::connectedComponents(intersectionImg, labeledIntersections, 8, CV_16U) - 1;
	
	std::vector<Connection> connections(numConns, {-1, -1});
	std::vector<Intersection> intersections(numIntersections);
	std::vector<Point> foundConnectionEnds(numConns, Point(-1, -1));
	
	// This code goes through the labeledNoIntersections and labeledIntersections Mats and puts the connections and intersections it's found into the arrays above.
	for (int y = 0; y < skel.rows; ++y) {
		for (int x = 0; x < skel.cols; ++x) {
			
			uint16_t connectionNum = labeledNoIntersections.at<uint16_t>(y, x);
			if (connectionNum != 0
				&& numNeighbors<uint16_t>(labeledNoIntersections, x, y, connectionNum) == 2) {
				// end point of connection
				
				uint16_t intersectionNum = getANeighbor(labeledIntersections, x, y);
				assert(intersectionNum != 0);
				if (connections[connectionNum - 1].intersect1 < 0) {
					
					foundConnectionEnds[connectionNum - 1] = Point(x, y);
					connections[connectionNum - 1].intersect1 = intersectionNum - 1;
					
				}
				else {
					connections[connectionNum - 1].intersect2 = intersectionNum - 1;
					
					// find corners in the connections
					
					// Get the connection as a list of pixel coords
					std::vector<Point2i> connPoints;
					connPoints.push_back({x, y});
					while (true) {
						Point2i prevPix;
						if (connPoints.size() > 1) prevPix = connPoints[connPoints.size() - 2];
						else prevPix = { -1, -1 };
						
						Point2i newPix = getANeighborLocation(labeledNoIntersections, connPoints.back(), connectionNum, prevPix);
						
						if (newPix.x != -1) connPoints.push_back(newPix);
						else break;
					}
					
					auto splitSpots = splitCurve(connPoints.begin(), connPoints.end());
					
					// Add the intersections found, if any
					
					/*for (auto i = splitSpots.begin(); i < splitSpots.end(); ++i) {
						intersections.push_back({});
						Intersection& thisI = intersections.back();
						thisI.pos = ((Point2f) **i) / nomImgSize;
						
						Connection c1, c2;
						c1.intersect2 = intersections.size() - 1;
						c2.intersect1 = intersections.size() - 1;
						
						if (i == splitSpots.begin()) c1.intersect1 = connections[connectionNum - 1].intersect2;
						else c1.intersect1 = intersections.size() - 2;
						
						if (i == splitSpots.end() - 1) c2.intersect2 = connections[connectionNum - 1].intersect1;
						else c2.intersect2 = intersections.size();
						
						thisI.c.push_back(c1);
						thisI.c.push_back(c2);
					}*/
					// Don't add the connections in the intersections, just add to the connection array.
					// The last connection will replace the connection being split. The rest will be appended to the end of the connection array.
					if (splitSpots.size() > 0) {
						for (int i = 0; i < splitSpots.size(); ++i) {
							intersections.push_back({ std::vector<Connection>(), ((Point2f) *splitSpots[i]) / nomImgSize });
							
							Connection c;
							if (i == 0) c.intersect1 = intersectionNum - 1;
							else c.intersect1 = intersections.size() - 2;
							c.intersect2 = intersections.size() - 1;
							
							connections.push_back(c);
						}
						connections[connectionNum - 1].intersect2 =  intersections.size() - 1;
					}
				}
			}
			
			uint16_t thisIntersectionNum = labeledIntersections.at<uint16_t>(y, x);
			if (thisIntersectionNum != 0) {
				intersections[thisIntersectionNum - 1].pos = Point2f(x / nomImgSize, y / nomImgSize);
			}
		}
	}
	for (int i = 0; i < connections.size(); ++i) {
		if (connections[i].intersect1 == -1 || connections[i].intersect2 == -1
			|| connections[i].intersect1 == connections[i].intersect2) {
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
