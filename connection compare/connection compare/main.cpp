#include "connections.h"
#include "createSkeleton.h"
#include <opencv2/highgui/highgui.hpp>
#include <string>

#include <thread>
#include <atomic>

using namespace cv;


// not a thread pool
template<class Function>
void threadHose(Function f) {
	std::vector<std::thread> workers(std::thread::hardware_concurrency() - 1);
	for (auto worker = workers.begin(); worker < workers.end(); ++worker) {
		*worker = std::thread(f);
	}
	f();
	for (auto worker = workers.begin(); worker < workers.end(); ++worker) {
		worker->join();
	}
}


AnalyzedSkeleton getConnectionsFromFile(const char* filename) {
	Mat skel = imread(filename, cv::IMREAD_GRAYSCALE);
	
	if (skel.empty()) {
		printf("Could not load image: %s\n", filename);
		return {{}, {}, -1};
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
	
	
	std::mutex outputLock;
	
	
	threadHose([&]() {
		
		char* filename = nullptr;
		size_t bufsize = 0;
	
		while (true) {
			ssize_t namelen = getline(&filename, &bufsize, names);
			if (namelen < 0) break;
			filename[namelen - 1] = 0;
			
			//printf("processing %s\n", filename);
			
			std::string fullname = path + std::string("/") + filename;
			AnalyzedSkeleton analyzedSkeleton = getConnectionsFromFile(fullname.c_str());
			
			
			outputLock.lock();
			
			if (analyzedSkeleton.nomImgSize != -1 && analyzedSkeleton.c.size() > 0 && analyzedSkeleton.isects.size() > 0) {
				analyzedSkeletons.push_back(analyzedSkeleton);
				filenames.push_back(filename);
			}
			
			outputLock.unlock();
		}
		free(filename);
		
	});
	return { analyzedSkeletons, filenames };
}


constexpr bool doParallel = true;

void doCompare(const char* coresDir, const char* skelFile, int visNum = 0) {
	auto compareChars = getAllCompareChars(coresDir);
	
	AnalyzedSkeleton thisChar = getConnectionsFromFile(skelFile);
	
	std::vector<CharPairScore> scores(compareChars.chars.size());
	std::vector<int> scoreIndices(compareChars.chars.size());
	
	if (doParallel) {
		// Thread pool for running compareSkeletons
		
		std::atomic<int> charNum(0);
		
		threadHose([&]() {
			while (true) {
				int i = ++charNum;
				if (i > compareChars.chars.size() - 1) break;
				
				scores[i] = compareSkeletons(thisChar, compareChars.chars[i]);
				assert(!isnan(scores[i].strength));
				scoreIndices[i] = i;
			}
		});
	}
	
	else {
		for (int i = 0; i < compareChars.chars.size(); ++i) {
			scores[i] = compareSkeletons(thisChar, compareChars.chars[i]);
			scoreIndices[i] = i;
		}
	}
	
	constexpr int outputCount = 100;
	
	
	std::sort(scoreIndices.begin(), scoreIndices.end(), [&scores](int a, int b) {
		return scores[a].strength > scores[b].strength;
	});
	
	for (int i = 0; i < outputCount; ++i) {
		fprintf(stderr, "strength: %f ", scores[scoreIndices[i]].strength);
		printf("%s\n", compareChars.names[scoreIndices[i]].c_str());
	}
	
	// visualize
	for (int i = 0; i < visNum; ++i) {
		visualizeIntersections(imread(skelFile, IMREAD_GRAYSCALE), imread(coresDir + std::string("/") + compareChars.names[scoreIndices[i]], IMREAD_GRAYSCALE));
	
	}
}

void pruneMatches(const char* matchesDir) {
	AllCompares matches = getAllCompareChars(matchesDir);
	
	
}

int main(int argc, const char * argv[]) {
	
	if (argc >= 2) {
		std::string verb = argv[1];
		
		if (verb == "compare") {
			if (argc < 4) {
				fprintf(stderr, "usage: compare coresDir skeletonImg\n");
				return 1;
			}
			else doCompare(argv[2], argv[3], 0);
		}
		else if (verb == "skeletonize") {
			if (argc < 5) {
				fprintf(stderr, "usage: skeletonize inputImage outputSkeleton diagonalOutputSize\n");
				return 1;
			}
			else {
				int size = atoi(argv[4]);
				if (size == 0) {
					fprintf(stderr, "invalid size\n");
					return 1;
				}
				else createSkeleton(argv[2], argv[3], size);
			}
		}
		else if (verb == "visc") {
			if (argc < 3) {
				fprintf(stderr, "usage: vis skeletonImg\n");
				return 1;
			}
			else visualizeConnections(imread(argv[2], cv::IMREAD_GRAYSCALE));
		}
		else if (verb == "visi") {
			if (argc < 4) {
				fprintf(stderr, "usage: visi skeleton1 skeleton2\n");
				return 1;
			}
			visualizeIntersections(imread(argv[2], IMREAD_GRAYSCALE), imread(argv[3], IMREAD_GRAYSCALE));
		}
		else if (verb == "compareAndVisi") {
			if (argc < 5) {
				fprintf(stderr, "usage: compareAndVisi numVis coresDir skeletonImg\n");
				return 1;
			}
			else doCompare(argv[3], argv[4], atoi(argv[2]));
		}
		else if (verb == "pruneMatches") {
			if (argc < 3) {
				fprintf(stderr, "usage: pruneMatches matchesDir");
				return 1;
			}
			else pruneMatches(argv[2]);
		}
		else fprintf(stderr, "unknown verb %s\n", argv[1]);
	}
	else fprintf(stderr, "no verb\n");
	
	
}
