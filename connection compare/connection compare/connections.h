#ifndef connections_h
#define connections_h


#include <vector>
#include <opencv2/core.hpp>

struct Connection {
	// distance from one end to the other
	//double straightLength;
	// number of pixels in the connection
	//double pixLength;
	
	// end points of connection
	// intersection numbers count from 0
	int intersect1, intersect2;
};

struct Intersection {
	std::vector<Connection> c;
	cv::Point2f pos;
};
struct AnalyzedSkeleton {
	std::vector<Intersection> isects;
	std::vector<Connection> c;
	// all isect positions are divided by this
	double nomImgSize;
};

/*struct ConnectionList {
	// intersections and endpoints
	int numIntersections;
	std::vector<Connection> c;
};*/

// Used to store multiple values.. now only stores one
struct CharPairScore {
	double strength;
};

// The input skeleton must be 1 pixel wide (via 8-connectivity).
AnalyzedSkeleton analyzeSkeleton(cv::Mat skel);
CharPairScore compareSkeletons(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB);

void visualizeConnections(cv::Mat skel);
void visualizeIntersections(cv::Mat skelA, cv::Mat skelB);

//void writeAnalyzedSkeletons(const char* filename, std::vector<AnalyzedSkeleton> connections);
//std::vector<AnalyzedSkeleton> readAnalyzedSkeletons(const char* filename);

#endif /* connections_h */
