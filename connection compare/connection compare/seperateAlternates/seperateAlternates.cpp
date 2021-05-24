#include <stdio.h>
#include <vector>
#include <map>

#include <opencv2/opencv.hpp>


// Bad stupid O(n^waytoohigh) A* implementation
struct Point {
	int x, y;
};
bool operator<(Point l, Point r) {
	if (l.x != r.x) return l.x < r.x;
	else return l.y < r.y;
}

struct SearchNode {
	Point p;
	int score, heurScore;
	Point parent;
	
};
typedef std::map<Point, SearchNode> nodeList;
struct badAStarResult {
	int score;
	std::vector<Point> path;
};
badAStarResult badAStar(cv::Mat bwimage, int startX) {
	
	nodeList open, closed;
	
	open[Point{ startX, 0 }] = SearchNode{startX, 0, 0, bwimage.rows, -1, -1};
	
	while (!open.empty()) {
		
		nodeList::iterator inspected = open.begin();
		for (nodeList::iterator i = ++open.begin(); i != open.end(); ++i) {
			if (i->second.heurScore < inspected->second.heurScore) inspected = i;
		}
		SearchNode inspVal = inspected->second;
		//printf("visiting (%d, %d), score=%d, heur=%d\n", inspVal.p.x, inspVal.p.y, inspVal.score, inspVal.heurScore);
		open.erase(inspected);
		
		if (inspVal.p.y == bwimage.rows - 1) {
			// Done!
			std::vector<Point> path;
			SearchNode current = inspVal;
			do {
				path.push_back(current.p);
				nodeList::iterator parentIt = closed.find(current.parent);
				assert(parentIt != closed.end());
				current = parentIt->second;
				
			} while (current.parent.x > 0);
			
			//printf("found path. x=%d, score=%d, closed size=%lu, open size=%lu\n", startX, inspVal.score, closed.size(), open.size());
			return { inspVal.score, path };
		}
		
		Point successors[4] = {
			Point{inspVal.p.x, inspVal.p.y+1},
			Point{inspVal.p.x-1, inspVal.p.y},
			Point{inspVal.p.x+1, inspVal.p.y},
			Point{inspVal.p.x, inspVal.p.y-1}
		};
		for (Point successor : successors) {
			if (successor.x < 0 || successor.y < 0
				|| successor.x >= bwimage.cols || successor.y >= bwimage.rows
				|| bwimage.at<uint8_t>(successor.y, successor.x) == 255) continue;
			
			int score = inspVal.score + 1;
			int heurScore = score + (bwimage.rows - 1 - inspVal.p.y);
			
			bool skipped = false;
			auto openMatchIt = open.find(successor);
			if (openMatchIt != open.end() && openMatchIt->second.heurScore <= heurScore) skipped = true;
			
			auto closedMatchIt = closed.find(successor);
			if (closedMatchIt != closed.end() && closedMatchIt->second.heurScore <= heurScore) skipped = true;
			
			if (!skipped) {
				SearchNode successorNode{ successor, score, heurScore, inspVal.p };
				open[successor] = successorNode;
			}
		}
		closed[inspVal.p] = inspVal;
	}
	// no path
	//printf("No path. x=%d, closed size=%lu\n", startX, closed.size());
	return { INT_MAX };
}

int main(int argc, char** argv) {
	
	assert(argc >= 3);
	
	std::string pathless;
	size_t pathlessLoc = std::string(argv[1]).rfind('/');
	if (pathlessLoc == std::string::npos) pathless = argv[1];
	else pathless = std::string(argv[1]).substr(pathlessLoc+1, std::string::npos);
	
	int pageNum, imgX, imgY, imgWidth, imgHeight, imgRefX, imgRefY;
	sscanf(pathless.c_str(), "Page %d - loc%d,%d,%d,%d ref%d,%d.png", &pageNum, &imgX, &imgY, &imgWidth, &imgHeight, &imgRefX, &imgRefY);
	
	cv::Mat inImg = cv::imread(argv[1]);
	cv::Mat grayImg;
	cv::cvtColor(inImg, grayImg, cv::COLOR_BGR2GRAY);
	//cv::Mat big;
	//constexpr double scaleFactor = 2;
	//cv::resize(in, big, {0,0}, scaleFactor, scaleFactor, cv::INTER_CUBIC);
	
	cv::Mat bwimage;
	cv::threshold(grayImg, bwimage, 0, 255, cv::THRESH_OTSU+cv::THRESH_BINARY_INV);
	
	// For each pixel along the top of bwimage, find the shortest path to the bottom
	std::vector<badAStarResult> paths;
	for (int i = 0; i < bwimage.cols; ++i) {
		paths.push_back(badAStar(bwimage, i));
	}
	
	constexpr int minMinDist = 15;
	std::vector<int> minimasStart, minimasEnd;
	int minimaRangeStart = 0;
	for (int i = minMinDist; i < bwimage.cols - minMinDist; ++i) {
		if (paths[i].score < paths[i-1].score) minimaRangeStart = i;
		if (paths[i].score <= paths[i-1].score && paths[i].score < paths[i+1].score) {
			// This is a minimum
			//int minLoc = (i + minimaRangeStart) / 2;
			
			if (minimasEnd.size() > 0 && minimaRangeStart - minimasEnd.back() < minMinDist) {
				if (paths[i].score < paths[minimasEnd.back()].score
					|| (paths[i].score == paths[minimasEnd.back()].score
					&& i - minimaRangeStart > minimasEnd.back() - minimasStart.back())) {
							
					minimasStart.back() = minimaRangeStart;
					minimasEnd.back() = i;
				}
			}
			else {
				minimasStart.push_back(minimaRangeStart);
				minimasEnd.push_back(i);
			}
		}
	}
	std::vector<int> minimas(minimasStart.size());
	for (int i = 0; i < minimasStart.size(); ++i) {
		minimas[i] = (minimasEnd[i] + minimasStart[i]) / 2;
	}
	//printf("%lu minimas\n", minimas.size());
	
	for (int i = 0; i <= minimas.size(); ++i) {
		int leftCrop, rightCrop;
		if (i == 0) leftCrop = 0;
		else {
			leftCrop = INT_MAX;
			for (Point point : paths[minimas[i-1]].path) {
				if (point.x < leftCrop) leftCrop = point.x;
			}
		}
		if (i == minimas.size()) rightCrop = inImg.cols;
		else {
			rightCrop = 0;
			for (Point point : paths[minimas[i]].path) {
				if (point.x > rightCrop) rightCrop = point.x;
			}
		}
		cv::Mat cropImg = inImg(cv::Rect(leftCrop, 0, rightCrop - leftCrop, inImg.rows)).clone();
		
		// Blank out pixels outside the path
		if (i != 0) for (Point point : paths[minimas[i-1]].path) {
			for (int x = 0; x < point.x - leftCrop; ++x) cropImg.at<cv::Vec3b>(point.y, x) = cv::Vec3b{255, 255, 255};
		}
		if (i != minimas.size()) for (Point point : paths[minimas[i]].path) {
			for (int x = point.x + 1; x < rightCrop - leftCrop; ++x) cropImg.at<cv::Vec3b>(point.y, x) = cv::Vec3b{255, 255, 255};
		}
		
		
		char outFilename[512];
		int out = snprintf(outFilename, 512, "%s/Page %d - loc%d,%d,%d,%d ref%d,%d.png", argv[2], pageNum, imgX + leftCrop, imgY, rightCrop - leftCrop, imgHeight, imgRefX, imgRefY);
		assert(out <= 512);
		
		cv::imwrite(outFilename, cropImg);
	}
}
