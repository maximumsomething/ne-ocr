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
	
	FILE* names = popen(("cd '" + std::string(path) + "'; printf \"%s\n\" *.png Alternates/*.png").c_str(), "r");
	
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
	
	if (thisChar.nomImgSize == -1) {
		// unloadable image. getConnectionsFromFile should have already printed an error
		exit(1);
	}
	
	std::vector<CharPairScore> scores(compareChars.chars.size());
	std::vector<int> scoreIndices(compareChars.chars.size());
	
	if (doParallel) {
		// Thread pool for running compareSkeletons
		
		std::atomic<int> charNum(0);
		
		threadHose([&]() {
			while (true) {
				int i = charNum++;
				if (i >= compareChars.chars.size()) break;
				
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
	
	int outputCount = std::min(100, (int) scores.size());
	
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

const char* isArgOption(const char* arg, const char* option) {
	int optLen = strlen(option);
	if (strncmp(arg, option, optLen) == 0) {
		// arg starts with option
		return arg + optLen;
	}
	else return nullptr;
}
bool setOptIfMatch(const char* arg, const char* optionName, float* option){
	const char* optionStr = isArgOption(arg, optionName);
	if (optionStr != nullptr) {
		try {
			*option = std::stof(optionStr);
		}
		catch(...) {
			fprintf(stderr, "Invalid argument %s", arg);
			return false;
		}
		return true;
	}
	else return false;
}
// args don't include executable or 'skeletonize' verb
int parseSkeletonizeArgs(const char** argv, int argc) {
	constexpr char usageString[] = "usage: skeletonize -outSize=<expected diagonal size> [-dotSize=X] [-holeSize=X] inputImage outputSkeleton\n";
	if (argc < 5) {
		fprintf(stderr, usageString);
		return 1;
	}
	else {
		const char* inPath = nullptr, *outPath = nullptr;
		float outSize = 0, dotSize = 0.03, holeSize = 0.015;
		
		for (int i = 0; i < argc; ++i) {
			
			bool isOpt = setOptIfMatch(argv[i], "-outSize=", &outSize)
				      || setOptIfMatch(argv[i], "-dotSize=", &dotSize)
			          || setOptIfMatch(argv[i], "-holeSize=", &holeSize);
	
			if (!isOpt) {
				if (inPath == nullptr) inPath = argv[i];
				else if (outPath == nullptr) outPath = argv[i];
				else fprintf(stderr, "Invalid argument %s", argv[i]);
			}
		}
		
		if (inPath == nullptr || outPath == nullptr || outSize == 0) {
			fprintf(stderr, usageString);
			return 1;
		}
		createSkeleton(inPath, outPath, outSize, dotSize, holeSize);
		
		return 0;
	}
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
			return parseSkeletonizeArgs(&(argv[2]), argc-2);
			
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
