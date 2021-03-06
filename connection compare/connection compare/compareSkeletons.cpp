#include "connections.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <numeric>


#define ZERO(e) memset(e, 0, sizeof(e))
#define LOOPSET(e, val) { for (int iii = 0; iii < sizeof(e) / sizeof(e[0]); ++iii) e[iii] = val; }



// This simple 2D array class allows you to address it like arr[x][y], like a plain C array
template<class innerT, class containerT = std::vector<innerT> >
class array2D : public containerT {
	
	
public:
	size_t sizX, sizY;
	
	array2D(size_t x, size_t y): sizX(x), sizY(y), containerT(x*y) {}
	array2D(size_t x, size_t y, containerT&& super): sizX(x), sizY(y), containerT(super) {}
	
	class single_row {
	public:
		size_t row;
		array2D<innerT, containerT>* theArr;
		
		innerT& operator[](size_t i) { return theArr->get(row, i); }
	};
	single_row operator[](size_t i) {
		return { i, this };
	}
	innerT& get(size_t ix, size_t iy) {
		assert(iy < sizY && ix < sizX);
		return containerT::operator[](iy + ix*sizY);
	}
	size_t total() {
		return sizX * sizY;
	}
	containerT& linear() {
		return *this;
	}
};


struct ConnProps {
	double straightLength, /*pixLength,*/ angle;
	 // number between 0 and 1 that is smaller if this connection is similar to others in the same intersection
	double uniqueness = 1;
	int startIsectNum, endIsectNum;
};

struct ScoreSig {
	double score, significance;
};
bool operator<(ScoreSig a, ScoreSig b) {
	return a.score < b.score;
}

// Connections from, not only adjacent intersections, but down a chain!
class ConnectionFabricator {
	// ConnectionFabricator is a state machine that, each time you call next(), props will contain a new connection. This connection is between the intersection isectChain[0] and some other intersection. It may be composed of multiple connections in the original AnalyzedSkeleton, but it approximates a straight line.
	
	// getAll() will call next() as much as needed and return the complete list of connections.
	// TODO: remove the state machine business and just use getAll()
	
	
	friend void visualizeConnections(cv::Mat skel);
	
	static constexpr int maxDepth = 3;
	
	AnalyzedSkeleton& skel;
	// the index of the current intersections on each level of the chain.
	int isectChain[maxDepth + 1] = {0};
	
	// the index of the current connection on each level of the chain.
	int connsOn[maxDepth] = {0};
	
	int currentDepth = 0;
	
	
	int getOtherIntersection(Connection connection, int isect) {
		if (connection.intersect1 == isect) return connection.intersect2;
		else {
			assert(connection.intersect2 == isect);
			return connection.intersect1;
		}
	}
	
	// Makes sure this connection chain meets certian criteria and sets length, angle, etc.
	bool setVals() {
		
		// in all cases, isectChain[currentDepth] will be valid
		isectChain[currentDepth + 1] = getOtherIntersection(
															skel.isects[isectChain[currentDepth]].c[connsOn[currentDepth]], isectChain[currentDepth]);
		
		double combinedLength = 0;
		//props.pixLength = 0;
		for (int i = 0; i <= currentDepth; ++i) {
			auto point1 = skel.isects[isectChain[i]].pos;
			auto point2 = skel.isects[isectChain[i + 1]].pos;
			
			combinedLength += sqrt(pow(point1.x - point2.x, 2) + pow(point1.y - point2.y, 2));
			//skel.isects[isectChain[i]].c[connsOn[i]].straightLength;
			//props.pixLength += skel.isects[isectChain[i]].c[connsOn[i]].pixLength;
		}
		
		auto point1 = skel.isects[isectChain[0]].pos;
		auto point2 = skel.isects[isectChain[currentDepth + 1]].pos;
		
		double xDiff = point1.x - point2.x;
		double yDiff = point1.y - point2.y;
		
		double directLength = sqrt(xDiff*xDiff + yDiff*yDiff);
		
		// make it so multiple-connection chains are limited to mostly straight lines
		double lengthRatio = directLength / combinedLength;
		
		constexpr double minLengthRatio = 0.9;
		
		if ((lengthRatio < minLengthRatio || lengthRatio > 1.0/minLengthRatio)) return next();
		
		
		props.straightLength = directLength;
		props.angle = atan2(xDiff, yDiff);
		
		props.endIsectNum = isectChain[currentDepth + 1];
		props.startIsectNum = isectChain[0];
		return true;
	}
	
public:
	ConnProps props;
	
	ConnectionFabricator(AnalyzedSkeleton& skel, int isectNum) : skel(skel) {
		isectChain[0] = isectNum;
		setVals();
	};
	
	bool next() {
		if (currentDepth < maxDepth - 1 && skel.isects[isectChain[currentDepth + 1]].c.size() > 1) {
			++currentDepth;
			connsOn[currentDepth] = 0;
		}
		else {
			while (true) {
				// if there's more connections to go in the current intersection
				if (connsOn[currentDepth] < skel.isects[isectChain[currentDepth]].c.size() - 1) {
					++connsOn[currentDepth];
					break;
				}
				else if (currentDepth == 0) {
					return false;
				}
				else {
					--currentDepth;
					// continue;
				}
			}
		}
		return setVals();
	}
	
	// Gets all the connections and sets their uniqueness
	std::vector<ConnProps> getAll() {
		std::vector<ConnProps> conns;
		conns.reserve(10);
		
		do {
			// Make sure there's no exact match
			for (int i = 0; i < conns.size(); ++i) {
				if (conns[i].endIsectNum == this->props.endIsectNum) continue;
			}
			conns.push_back(props);
		} while (next());
		
		std::vector<double> inverseUniquenesses(conns.size(), 1);
		
		for (int i = 0; i < conns.size(); ++i) {
			for (int j = i + 1; j < conns.size(); ++j) {
				
				double angleDiff = abs(conns[i].angle - conns[j].angle);
				angleDiff = fmod(angleDiff, M_PI*2);
				if (angleDiff > M_PI) angleDiff = M_PI*2 - angleDiff;
				
				double angleCloseness = 1 - angleDiff / M_PI;
				
				double lengthCloseness = ((conns[i].straightLength < conns[j].straightLength) ?
											   conns[i].straightLength / conns[j].straightLength :
											   conns[j].straightLength / conns[i].straightLength);
							
				// Value between 0 and 1, with 1 being exactly equal.
				double combinedCloseness = angleCloseness * lengthCloseness;
				
				inverseUniquenesses[i] += combinedCloseness;
				inverseUniquenesses[j] += combinedCloseness;
			}
		}
		for (int i = 0; i < conns.size(); ++i) {
			conns[i].uniqueness = 1 / inverseUniquenesses[i];
		}
		
		return conns;
	}
};

// Compares every intersection in inA with every intersection in inB. For every pair of connections in the pair of intersections, the getScore function assigns a similarity score and as a "significance" value. Every connection in the intersection from inA is matched with the most similar connection in inB, and a weighted average of these scores based on the sigificance value is taken.
// The average score of each pair of intersections is returned in the isectScores array.
void compareConnectionsOfIntersections(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
									array2D<ScoreSig>& isectScores,
									   std::function<struct ScoreSig (ConnProps,ConnProps)> getScore) {
	
	
	for (int a = 0; a < inA.isects.size(); ++a) {
		if (inA.isects[a].c.size() == 0) {
			//fprintf(stderr, "empty intersection!\n");
			continue;
		}
		for (int b = 0; b < inB.isects.size(); ++b) {
			if (inB.isects[b].c.size() == 0) {
				//fprintf(stderr, "empty intersection!\n");
				continue;
			}
			
			// For each connection in A, get the best match in B
			double totalScoreAB = 0;
			double totalSignificanceAB = 0;
			
			
			std::vector<ConnProps> connsA = ConnectionFabricator(inA, a).getAll();
			std::vector<ConnProps> connsB = ConnectionFabricator(inB, b).getAll();
			
			// For each connection in B, get the best match in A
			// Contains an entry for each connection in B, containing the best scoring connection from A
			std::vector<ScoreSig> bestScoresBA(connsB.size(), { -INFINITY, 0 });
			
			// Loop through all pairs of connections
			for (int connA = 0; connA < connsA.size(); ++connA) {
	
				double bestScoreAB = -INFINITY;
				double bestSignificanceAB = 0;
				
				for (int connB = 0; connB < connsB.size(); ++connB) {
					
					auto connRating = getScore(connsA[connA], connsB[connB]);
					
					if (connRating.significance != 0) {
						if (connRating.score > bestScoreAB) {
							bestScoreAB = connRating.score;
							bestSignificanceAB = connRating.significance * connsA[connA].uniqueness;
							
						}
						if (connRating.score > bestScoresBA[connB].score) {
							
							bestScoresBA[connB] = connRating;
							bestScoresBA[connB].significance *= connsB[connB].uniqueness;
						}
					}
				}
				
				if (bestScoreAB != -INFINITY) {
					//assert(bestScore > -2);
					totalScoreAB += bestScoreAB*bestSignificanceAB;
					totalSignificanceAB += bestSignificanceAB;
				}
				
			}
			
			// TODO: check for totalSignificanceBA
			if (totalSignificanceAB != 0) {
				double scoreAB = totalScoreAB / totalSignificanceAB;
				
				double totalScoreBA = 0, totalSignificanceBA = 0;
				for (int connB = 0; connB < connsB.size(); ++connB) {
					
					if (bestScoresBA[connB].score != -INFINITY) {
						totalScoreBA += bestScoresBA[connB].score * bestScoresBA[connB].significance;
						totalSignificanceBA += bestScoresBA[connB].significance;
					}
				}
				double scoreBA = totalScoreBA / totalSignificanceBA;
				
				isectScores[a][b] = { (scoreAB + scoreBA) / 2,
					(totalSignificanceAB + totalSignificanceBA) / 2 };
				
				assert(!isnan(isectScores[a][b].score));
			}
			else {
				assert(totalScoreAB == 0);
				isectScores[a][b] = { -10, 0 };
			}
		}
	}
}

// Compares two connections based on their length and angle.
ScoreSig compareConnPair(ConnProps connA, ConnProps connB) {
	double angleDiff = abs(connA.angle - connB.angle);
	angleDiff = fmod(angleDiff, M_PI*2);
	if (angleDiff > M_PI) angleDiff = M_PI*2 - angleDiff;
	
	double anglePenalty = angleDiff / M_PI;
	double linearLengthPenalty = abs(connA.straightLength - connB.straightLength);
	double propLengthPenalty = 1 - ((connA.straightLength < connB.straightLength) ?
								   connA.straightLength / connB.straightLength :
								   connB.straightLength / connA.straightLength);
	
	double combinedScore = 1 - anglePenalty - linearLengthPenalty/2;// - propLengthPenalty/4;
	
	assert(combinedScore <= 1);
	assert(combinedScore > -10);
	assert(!isnan(combinedScore));
	
	double significance = fmin(connA.straightLength, connB.straightLength);
	
	return { std::max(0.0, combinedScore), significance };
}

// By comparing intersections based on the amount of similar connecting intersections, the score better reflects larger groups of intersections.
array2D<ScoreSig> reiterateScores(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
							array2D<ScoreSig>& isectScores) {
	
	array2D<ScoreSig> newIsectScores(inA.isects.size(), inB.isects.size());
	
	compareConnectionsOfIntersections(inA, inB, newIsectScores,
									  [&](ConnProps connA, ConnProps connB) -> struct ScoreSig {
										  
		
		ScoreSig neighbor = isectScores[connA.endIsectNum][connB.endIsectNum];
		ScoreSig connScore = compareConnPair(connA, connB);
		
		return { neighbor.score * connScore.score, neighbor.significance * connScore.significance };
	});
	/*
	constexpr double oldWeight = 1;
	for (int i = 0; i < newIsectScores.total(); ++i) {
		newIsectScores.linear()[i].score =
		(newIsectScores.linear()[i].score + isectScores.linear()[i].score*oldWeight) / (1 + oldWeight);
		
		newIsectScores.linear()[i].significance =
		(newIsectScores.linear()[i].significance + isectScores.linear()[i].significance*oldWeight) / (1 + oldWeight);
	}*/
	
	return newIsectScores;
}

// Uses the best match of each intersection to average a final score.
CharPairScore totalIsectScores(array2D<ScoreSig> isectScores) {
	std::vector<ScoreSig> aScores(isectScores.sizX, {-INFINITY, -INFINITY}),
	bScores(isectScores.sizY, {-INFINITY, -INFINITY});
	
	for (int a = 0; a < isectScores.sizX; ++a) {
		for (int b = 0; b < isectScores.sizY; ++b) {
			
			if (isectScores[a][b].score > aScores[a].score) aScores[a] = isectScores[a][b];
			if (isectScores[a][b].score > bScores[b].score) bScores[b] = isectScores[a][b];
		}
	}
	
	std::sort(aScores.begin(), aScores.end());
	std::sort(bScores.begin(), bScores.end());
	
	//constexpr int SCORES_TO_REMOVE = 2;
	constexpr int SCORES_TO_REMOVE = 0;
	
	double totalScore = 0;
	double totalSig = 0;
	for (int i = SCORES_TO_REMOVE; i < aScores.size(); ++i) {
		totalScore += aScores[i].score * aScores[i].significance;
		totalSig += aScores[i].significance;
	}
	for (int i = SCORES_TO_REMOVE; i < bScores.size(); ++i) {
		totalScore += bScores[i].score * bScores[i].significance;
		totalSig += bScores[i].significance;
	}
	double finalStrength = totalScore / totalSig;
	if (totalSig == 0) finalStrength = -10;
	assert(!isnan(finalStrength));
	assert(finalStrength <= 1);
	return { finalStrength };
}

CharPairScore compareSkeletons(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
							   std::function<void (array2D<ScoreSig>)> visHook) {
	
	/*1. For each intersection and endpoint in in1, create a similarity score between it
	 and all intersections in in2.
	 2. For each connection in in1, match it with connections in in2 by
	 looking for the one with the most similar intersections.
	 */
	
	array2D<ScoreSig> isectScores(inA.isects.size(), inB.isects.size());
	
	compareConnectionsOfIntersections(inA, inB, isectScores, &compareConnPair);
	
	// Take position of intersections into account
	constexpr double isectDistanceWeight = 0.3;
	for (int a = 0; a < inA.isects.size(); ++a)
	for (int b = 0; b < inB.isects.size(); ++b) {
		auto pointDiff = inA.isects[a].pos - inB.isects[b].pos;
		auto pointDist = sqrt(pointDiff.x*pointDiff.x + pointDiff.y*pointDiff.y);
		
		isectScores[a][b].score = isectScores[a][b].score * (1 - isectDistanceWeight) + isectDistanceWeight * (1 - pointDist); 
	}
	
	
	if (visHook) visHook(isectScores);
	
	//constexpr int reiterations = 10;
	int reiterations = visHook ? 10 : 2;
	for (int i = 0; i < reiterations; ++i) {
		isectScores = reiterateScores(inA, inB, isectScores);
		if (visHook) visHook(isectScores);
	}
	
	return totalIsectScores(isectScores);
}

CharPairScore compareSkeletons(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB) {
	return compareSkeletons(inA, inB, nullptr);
}


void transferSkel(cv::Mat from, cv::Mat to, cv::Point2i where, int scaleFactor = 1) {
	
	/*for (int y = 0; y < from.rows; ++y) {
		for (int x = 0; x < from.cols; ++x) {
			if (from.at<uint8_t>(y, x) != 0) {
				to.at<cv::Vec3b>(y + where.y, x + where.x) = { 255, 255, 255 };
			}
			//else to.at<cv::Vec3b>(y + where.y, x + where.x) = { 0, 0, 0 };
		}
	}*/
	
	for (int y = where.y * scaleFactor; y < std::min((where.y + from.rows) * scaleFactor, to.rows); ++y) {
		for (int x = where.x * scaleFactor; x < std::min((where.x + from.cols) * scaleFactor, to.cols); ++x) {
			
			if (from.at<uint8_t>(y / scaleFactor - where.y, x / scaleFactor - where.x) != 0) {
				to.at<cv::Vec3b>(y, x) = { 255, 255, 255 };
			}
		}
	}
}
void visualizeIntersections(cv::Mat skelA, cv::Mat skelB) {
	constexpr int scaleFactor = 3;
	
	AnalyzedSkeleton inA = analyzeSkeleton(skelA), inB = analyzeSkeleton(skelB);
	
	compareSkeletons(inA, inB, [&](array2D<ScoreSig> isectScores) {
		
		cv::Mat outImg((std::max(skelA.rows, skelB.rows) + 40) * scaleFactor,
					   (skelA.cols + skelB.cols + 60) * scaleFactor,
					   CV_8UC3, { 0, 0, 0 });
		

		transferSkel(skelA, outImg, {20, 20}, scaleFactor);
		transferSkel(skelB, outImg, {skelA.cols + 40, 20}, scaleFactor);
		
		
		double compThreshold = totalIsectScores(isectScores).strength * 0.8 - 0.05;
		
		// draw worse lines first
		std::vector<int> sortIdxes(inA.isects.size() * inB.isects.size());
		std::iota(sortIdxes.begin(), sortIdxes.end(), 0);
		
		std::sort(sortIdxes.begin(), sortIdxes.end(), [&](int i1, int i2) {
			return isectScores.linear()[i1] < isectScores.linear()[i2];
		});
		
		for (int i = 0; i < inA.isects.size() * inB.isects.size(); ++i) {
			double score = isectScores.linear()[sortIdxes[i]].score;
			if (score < compThreshold) continue;
			assert(score <= 1);
			
			
			Intersection isectA = inA.isects[sortIdxes[i] / inB.isects.size()];
			Intersection isectB = inB.isects[sortIdxes[i] % inB.isects.size()];
			
			//printf("i: %d a: %d b: %d score: %f\n", sortIdxes[i], sortIdxes[i] / inB.isects.size(), sortIdxes[i] % inB.isects.size(), score);
			
			// determine color, red=bad, green=good
			cv::Mat3b bgrColor;
			cv::Vec3b hsvColor = { (unsigned char) ((score - compThreshold) / (1 - compThreshold) * 100), 255, 255 };
			
			cv::cvtColor(cv::Mat3b(hsvColor), bgrColor, cv::COLOR_HSV2BGR);
			
			cv::line(outImg, cv::Point2f(20 + isectA.pos.x * inA.nomImgSize, 20 + isectA.pos.y * inA.nomImgSize) * scaleFactor,
					 cv::Point2f(skelA.cols + 40 + isectB.pos.x * inB.nomImgSize, 20 + isectB.pos.y * inB.nomImgSize) * scaleFactor, bgrColor.at<cv::Vec3b>(0));
		}
		
		//transferSkel(skelA, outImg, {20, 20}, scaleFactor);
		//transferSkel(skelB, outImg, {skelA.cols + 40, 20}, scaleFactor);
		
		for (int i = 0; i < inA.isects.size(); ++i) {
			cv::putText(outImg, std::to_string(i),
						cv::Point2f(20 + inA.isects[i].pos.x * inA.nomImgSize, 20 + inA.isects[i].pos.y * inA.nomImgSize) * scaleFactor,
						cv::FONT_HERSHEY_PLAIN, 2, { 255, 200, 200 });
		}
		for (int i = 0; i < inB.isects.size(); ++i) {
			cv::putText(outImg, std::to_string(i),
						 cv::Point2f(skelA.cols + 40 + inB.isects[i].pos.x * inB.nomImgSize,
									 20 + inB.isects[i].pos.y * inB.nomImgSize)
						* scaleFactor,
						cv::FONT_HERSHEY_PLAIN, 2, { 255, 200, 200 });
		}
		
		cv::imshow("Intersections - cutoff: " + std::to_string(compThreshold), outImg);
		cv::waitKey(0);
	});
}


void visualizeConnections(cv::Mat skel) {
	AnalyzedSkeleton in = analyzeSkeleton(skel);
	
	double nomImgSize = (skel.rows + skel.cols)/2.0;
	
	for (int i = 0; i < in.isects.size(); ++i) {
		
		cv::Mat outImg(skel.rows, skel.cols, CV_8UC3, { 0, 0, 0 });
		transferSkel(skel, outImg, {0, 0});
		
		auto isectPt = in.isects[i].pos * nomImgSize;
		
		ConnectionFabricator conns(in, i);
		do {
			
			cv::line(outImg, isectPt,
					 in.isects[conns.isectChain[conns.currentDepth + 1]].pos * nomImgSize,
					 CV_RGB(0, 0, 255));
			
		} while (conns.next());
		
		cv::circle(outImg, isectPt, 2, CV_RGB(255, 0, 0));
		
		cv::imshow("Connections", outImg);
		cv::waitKey(0);
	}
}
