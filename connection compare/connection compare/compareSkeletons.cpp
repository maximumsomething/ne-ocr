#include "connections.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <numeric>


#define ZERO(e) memset(e, 0, sizeof(e))
#define LOOPSET(e, val) { for (int iii = 0; iii < sizeof(e) / sizeof(e[0]); ++iii) e[iii] = val; }


struct ConnProps {
	float straightLength, pixLength, angle;
	int endIsectNum;
};

// Connections from, not only adjacent intersections, but down a chain!
class ConnectionFabricator {
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
		
		float combinedLength = 0;
		props.pixLength = 0;
		for (int i = 0; i <= currentDepth; ++i) {
			auto point1 = skel.isects[isectChain[i]].pos;
			auto point2 = skel.isects[isectChain[i + 1]].pos;
			
			combinedLength += sqrt(pow(point1.x - point2.x, 2) + pow(point1.y - point2.y, 2));
			//skel.isects[isectChain[i]].c[connsOn[i]].straightLength;
			props.pixLength += skel.isects[isectChain[i]].c[connsOn[i]].pixLength;
		}
		
		auto point1 = skel.isects[isectChain[0]].pos;
		auto point2 = skel.isects[isectChain[currentDepth + 1]].pos;
		
		float xDiff = point1.x - point2.x;
		float yDiff = point1.y - point2.y;
		
		float directLength = sqrt(xDiff*xDiff + yDiff*yDiff);
		
		// make it so multiple-connection chains are limited to mostly straight lines
		float lengthRatio = directLength / combinedLength;
		
		constexpr float minLengthRatio = 0.9;
		
		if ((lengthRatio < minLengthRatio || lengthRatio > 1.0/minLengthRatio)) return next();
		
		
		props.straightLength = directLength;
		props.angle = atan2(xDiff, yDiff);
		
		props.endIsectNum = isectChain[currentDepth + 1];
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
};

#define GET2D(arr, x, y, sizY) (arr[y + x*sizY])

struct ScoreSig { float score, significance; };

void compareConnectionsOfIntersections(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
									float* isectScores,
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
			
			if (a == 9 && b == 1) {
				printf("");
			}
			
			// For each connection in A, get the best match in B
			float totalScoreAB = 0;
			float totalSignificanceAB = 0;
			
			
			// Because connsB will be iterated through a bunch of times, it's best to cache it
			std::vector<ConnProps> connsB;
			connsB.reserve(10);
			ConnectionFabricator connFabB(inB, b);
			do {
				connsB.push_back(connFabB.props);
			} while (connFabB.next());
			
			// For each connection in B, get the best match in A
			// Contains an entry for each connection in B, containing the best scoring connection from A
			std::vector<ScoreSig> bestScoresAB(connsB.size(), { -INFINITY, 0 });
			
			ConnectionFabricator connsA(inA, a);
			do {
				float bestScoreAB = -INFINITY;
				float bestSignificanceAB = 0;
				
				
				for (int connB = 0; connB < connsB.size(); ++connB) {
					
					auto connRating = getScore(connsA.props, connsB[connB]);
					
					if (connRating.significance != 0) {
						if (connRating.score > bestScoreAB) {
							bestScoreAB = connRating.score;
							bestSignificanceAB = connRating.significance;
							
						}
						if (connRating.score > bestScoresAB[connB].score) {
							
							bestScoresAB[connB] = connRating;
						}
					}
				}
				
				if (bestScoreAB != -INFINITY) {
					//assert(bestScore > -2);
					totalScoreAB += bestScoreAB*bestSignificanceAB;
					totalSignificanceAB += bestSignificanceAB;
				}
				
			} while (connsA.next());
			
			if (totalSignificanceAB != 0) {
				float scoreAB = totalScoreAB / totalSignificanceAB;
				
				float totalScoreBA = 0, totalSignificanceBA = 0;
				for (int connB = 0; connB < connsB.size(); ++connB) {
					totalScoreBA += bestScoresAB[connB].score * bestScoresAB[connB].significance;
					totalSignificanceBA += bestScoresAB[connB].significance;
				}
				float scoreBA = totalScoreBA / totalSignificanceBA;
				
				GET2D(isectScores, a, b, inB.isects.size()) = (scoreAB + scoreBA) / 2;
			}
			else {
				assert(totalScoreAB == 0);
				GET2D(isectScores, a, b, inB.isects.size()) = -10;
			}
		}
	}
}


/*ScoreSig compareConnectionPair(ConnectionFabricator& connsA, ConnectionFabricator& connsB) {
	
}*/

CharPairScore compareSkeletons(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
							   std::function<void (float *)> visHook) {
	
	/*1. For each intersection and endpoint in in1, create a similarity score between it
	 and all intersections in in2.
	 2. For each connection in in1, match it with connections in in2 by
	 looking for the one with the most similar intersections.
	 */
	
	float isectScores[inA.isects.size()][inB.isects.size()];
	ZERO(isectScores);
	
	compareConnectionsOfIntersections(inA, inB, (float* ) isectScores,
									  [](ConnProps connA, ConnProps connB) -> struct ScoreSig {
		float angleDiff = abs(connA.angle - connB.angle);
		while (angleDiff > M_PI*2) angleDiff -= M_PI*2;
		
		
		float anglePenalty = angleDiff / M_PI;
		float straightLengthPenalty = abs(connA.straightLength - connB.straightLength);
		float pixLengthPenalty = abs(connA.pixLength - connB.pixLength);
		
		float combinedScore = 1 - anglePenalty - straightLengthPenalty/2 - pixLengthPenalty/2;
		
		assert(combinedScore <= 1);
		
		float significance = fmin(connA.straightLength, connB.straightLength);
								
								return { combinedScore, significance };
		
	});
	
	visHook((float *) isectScores);
	
	// Let's do that again!
	float newIsectScores[inA.isects.size()][inB.isects.size()];
	ZERO(newIsectScores);
	
	compareConnectionsOfIntersections(inA, inB, (float *) newIsectScores,
	[&](ConnProps connsA, ConnProps connsB) -> struct ScoreSig {
		
		
		float score = isectScores[connsA.endIsectNum][connsB.endIsectNum];
		
		return  { score, 1 };
	});
	
	visHook((float *) newIsectScores);
	
	
	std::vector<float> aScores(inA.isects.size()), bScores(inB.isects.size());
	
	
	for (int a = 0; a < inA.isects.size(); ++a) {
		for (int b = 0; b < inB.isects.size(); ++b) {
			
			if (isectScores[a][b] > aScores[a]) aScores[a] = isectScores[a][b];
			if (isectScores[a][b] > bScores[b]) bScores[b] = isectScores[a][b];
		}
	}
	
	constexpr int SCORES_TO_REMOVE = 2;
	
	std::sort(aScores.begin(), aScores.end());
	std::sort(bScores.begin(), bScores.end());
	
	float totalScore = 0;
	for (int i = SCORES_TO_REMOVE; i < aScores.size(); ++i) {
		totalScore += aScores[i];
	}
	for (int i = SCORES_TO_REMOVE; i < bScores.size(); ++i) {
		totalScore += bScores[i];
	}
	return { totalScore / (inA.isects.size() + inB.isects.size() - SCORES_TO_REMOVE*2) };
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
	
	float score = compareSkeletons(inA, inB).strength;
	
	compareSkeletons(inA, inB, [&](float* isectScores) {
		
		cv::Mat outImg((std::max(skelA.rows, skelB.rows) + 40) * scaleFactor,
					   (skelA.cols + skelB.cols + 60) * scaleFactor,
					   CV_8UC3, { 0, 0, 0 });
		
		
		float compThreshold = score * score;
		
		// draw worse lines first
		std::vector<int> sortIdxes(inA.isects.size() * inB.isects.size());
		std::iota(sortIdxes.begin(), sortIdxes.end(), 0);
		
		std::sort(sortIdxes.begin(), sortIdxes.end(), [&](int i1, int i2) {
			return isectScores[i1] < isectScores[i2];
		});
		
		for (int i = 0; i < inA.isects.size() * inB.isects.size(); ++i) {
			float score = isectScores[sortIdxes[i]];
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
		
		transferSkel(skelA, outImg, {20, 20}, scaleFactor);
		transferSkel(skelB, outImg, {skelA.cols + 40, 20}, scaleFactor);
		
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
		
		cv::imshow("Intersections", outImg);
		cv::waitKey(0);
	});
}


void visualizeConnections(cv::Mat skel) {
	AnalyzedSkeleton in = analyzeSkeleton(skel);
	
	float nomImgSize = (skel.rows + skel.cols)/2.0;
	
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
