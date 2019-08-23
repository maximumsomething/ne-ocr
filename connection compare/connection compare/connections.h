#ifndef connections_h
#define connections_h

#include <vector>
#include <opencv2/core.hpp>

struct Connection {
	float length;
	float angle;
	// end points of connection
	// intersection numbers count from 1
	int intersect1, intersect2;
};

struct ConnectionList {
	// intersections and endpoints
	int numIntersections;
	std::vector<Connection> c;
};

// amount is how many good matches there are, and strength is how good those matches are.
struct CharPairScore {
	float amount, strength;
};

ConnectionList getConnections(cv::Mat skel);
CharPairScore compareConnections(ConnectionList inA, ConnectionList inB);

void writeConnectionLists(const char* filename, std::vector<ConnectionList> connections);
std::vector<ConnectionList> readConnectionLists(const char* filename);

#endif /* connections_h */
