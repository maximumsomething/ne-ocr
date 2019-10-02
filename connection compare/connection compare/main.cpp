#include "connections.h"
#include <opencv2/highgui/highgui.hpp>
#include <string>

using namespace cv;

AnalyzedSkeleton getConnectionsFromFile(const char* filename) {
	Mat skel = imread(filename, cv::IMREAD_GRAYSCALE);
	
	if (skel.empty()) {
		printf("Could not load image: %s\n", filename);
	}
	return analyzeSkeleton(skel);
}

struct AllCompares {
	std::vector<AnalyzedSkeleton> chars;
	std::vector<std::string> names;
};
AllCompares getAllCompareChars(const char* path) {
	std::vector<AnalyzedSkeleton> analyzedSkeletons;
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
		
		analyzedSkeletons.push_back(getConnectionsFromFile(fullname.c_str()));
		filenames.push_back(filename);
	}
	free(filename);
	return { analyzedSkeletons, filenames };
}

void doCompare(const char* coresDir, const char* skelFile) {
	auto compareChars = getAllCompareChars(coresDir);
	
	AnalyzedSkeleton thisChar = getConnectionsFromFile(skelFile);
	
	std::vector<CharPairScore> scores(compareChars.chars.size());
	std::vector<int> scoreIndices(compareChars.chars.size());
	for (int i = 0; i < compareChars.chars.size(); ++i) {
		scores[i] = compareSkeletons(thisChar, compareChars.chars[i]);
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
	
	// take the amountHold characters with highest amount, as well as all characters with amount over 2/3 of total
	//then sort them by strength
	std::sort(scoreIndices.begin(), scoreIndices.end(), [&scores](int a, int b) {
		return scores[a].amount > scores[b].amount;
	});
	int numGood = 0;
	for (int i = 0; i < compareChars.chars.size(); ++i) {
		if (scores[i].amount >= thisChar.isects.size() * (2.0/3.0)) ++numGood;
	}
	
	std::sort(scoreIndices.begin(), min(scoreIndices.begin() + max(amountHold, numGood),
										scoreIndices.end()), [&scores](int a, int b) {
		return scores[a].strength > scores[b].strength;
	});
	
	// visualize top result
	visualizeIntersections(imread(skelFile, IMREAD_GRAYSCALE), imread(coresDir + std::string("/") + compareChars.names[scoreIndices[0]], IMREAD_GRAYSCALE));
	
	
	for (int i = 0; i < amountHold; ++i) {
		fprintf(stderr, "amount: %f strength: %f ", scores[scoreIndices[i]].amount, scores[scoreIndices[i]].strength);
		printf("%s\n", compareChars.names[scoreIndices[i]].c_str());
	}
	
}


int main(int argc, const char * argv[]) {
	
	if (argc >= 2) {
		std::string verb = argv[1];
		
		if (verb == "compare") {
			if (argc < 4) {
				fprintf(stderr, "usage: compare coresDir skeletonImg");
				return 1;
			}
			else doCompare(argv[2], argv[3]);
		}
		else if (verb == "visc") {
			if (argc < 3) {
				fprintf(stderr, "usage: vis skeletonImg");
				return 1;
			}
			else visualizeConnections(imread(argv[2], cv::IMREAD_GRAYSCALE));
		}
		else if (verb == "visi") {
			if (argc < 4) {
				fprintf(stderr, "usage: compare coresDir skeletonImg");
				return 1;
			}
			else visualizeIntersections(imread(argv[2], IMREAD_GRAYSCALE), imread(argv[3], IMREAD_GRAYSCALE));
		}
		else fprintf(stderr, "unknown verb %s", argv[1]);
	}
	else fprintf(stderr, "no verb");
	
	
}
