#include "connections.h"
#include <opencv2/highgui/highgui.hpp>
#include <string>

using namespace cv;

ConnectionList getConnectionsFromFile(const char* filename) {
	Mat skel = imread(filename, cv::IMREAD_GRAYSCALE);
	
	if (skel.empty()) {
		printf("Could not load image: %s\n", filename);
	}
	return getConnections(skel);
}

struct AllCompares {
	std::vector<ConnectionList> chars;
	std::vector<std::string> names;
};
AllCompares getAllCompareChars(const char* path) {
	std::vector<ConnectionList> connectionLists;
	std::vector<std::string> filenames;
	
	FILE* names = popen(("ls '"+ std::string(path) + "' | grep .png").c_str(), "r");
	
	char* filename = nullptr;
	size_t bufsize = 0;
	
	while (true) {
		ssize_t namelen = getline(&filename, &bufsize, names);
		if (namelen < 0) break;
		filename[namelen - 1] = 0;
		
		//printf("processing %s\n", filename);
		
		std::string fullname = path + std::string("/") + filename;
		
		connectionLists.push_back(getConnectionsFromFile(fullname.c_str()));
		filenames.push_back(filename);
	}
	free(filename);
	return { connectionLists, filenames };
}

int main(int argc, const char * argv[]) {
	
	/*if (argc >= 2) {
		
		if (std::string("build-cache") == argv[1]) {
			
			if (argc < 3) {
				printf("%s build-cache /path/to/skeleton/directory\n", argv[0]);
			}
			else {
				auto connectionLists = getAllConnectionLists(argv[2]);
				writeConnectionLists((argv[2] + std::string("/connectionLists.dat")).c_str(), connectionLists);
			}
		}
	}*/
	
	if (argc >= 3) {
		
		auto compareChars = getAllCompareChars(argv[1]);
		
		ConnectionList thisChar = getConnectionsFromFile(argv[2]);
		
		std::vector<CharPairScore> scores(compareChars.chars.size());
		std::vector<int> scoreIndices(compareChars.chars.size());
		for (int i = 0; i < compareChars.chars.size(); ++i) {
			scores[i] = compareConnections(thisChar, compareChars.chars[i]);
			scoreIndices[i] = i;
		}
		// Sort by strength the characters with amount 2/3 or greater of total
		/*std::sort(scoreIndices.begin(), scoreIndices.end(), [&](int a, int b) {
			return scores[a].amount >= thisChar.numIntersections * (2.0/3.0) &&
			scores[a].strength > scores[b].strength;
		});
		for (int i = 0; i < 100; ++i) {
			fprintf(stderr, "amount: %f strength: %f ", scores[scoreIndices[i]].amount, scores[scoreIndices[i]].strength);
			printf("%s\n", compareChars.names[scoreIndices[i]].c_str());
		}*/
		
		constexpr int amountHold = 40;
		// take the amountHold characters with highest amount, then sort them by strength
		std::sort(scoreIndices.begin(), scoreIndices.end(), [&scores](int a, int b) {
			return scores[a].amount > scores[b].amount;
		});
		std::sort(scoreIndices.begin(), min(scoreIndices.begin() + amountHold, scoreIndices.end()), [&scores](int a, int b) {
			return scores[a].strength > scores[b].strength;
		});
		
		for (int i = 0; i < amountHold; ++i) {
			fprintf(stderr, "amount: %f strength: %f ", scores[scoreIndices[i]].amount, scores[scoreIndices[i]].strength);
			printf("%s\n", compareChars.names[scoreIndices[i]].c_str());
		}
		
	}
	
	
	/*
	Mat skel1 = imread("/Users/max/Downloads/bwimage.png", cv::IMREAD_GRAYSCALE);
	ConnectionList conns1 = getConnections(skel1);
	Mat skel2 = imread("/Users/max/Downloads/bwimage2.png", cv::IMREAD_GRAYSCALE);
	ConnectionList conns2 = getConnections(skel2);
	
	compareConnections(conns1, conns2);*/
}
