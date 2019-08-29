#include "connections.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define ZERO(e) memset(e, 0, sizeof(e))
#define LOOPSET(e, val) { for (int iii = 0; iii < sizeof(e) / sizeof(e[0]); ++iii) e[iii] = val; }


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
		pixLength = 0;
		for (int i = 0; i <= currentDepth; ++i) {
			auto point1 = skel.isects[isectChain[i]].pos;
			auto point2 = skel.isects[isectChain[i + 1]].pos;
			
			combinedLength += sqrt(pow(point1.x - point2.x, 2) + pow(point1.y - point2.y, 2));
			//skel.isects[isectChain[i]].c[connsOn[i]].straightLength;
			pixLength += skel.isects[isectChain[i]].c[connsOn[i]].pixLength;
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
		
		
		straightLength = directLength;
		angle = atan2(xDiff, yDiff);
		return true;
	}
	
public:
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
	
	
	float straightLength, pixLength, angle;
	
};

CharPairScore compareSkeletons(AnalyzedSkeleton inA, AnalyzedSkeleton inB) {
	
	/*1. For each intersection and endpoint in in1, create a similarity score between it
	 and all intersections in in2.
	 2. For each connection in in1, match it with connections in in2 by
	 looking for the one with the most similar intersections.
	 */
	
	float isectScores[inA.isects.size()][inB.isects.size()];
	ZERO(isectScores);
	
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
			
			float totalScore = 0;
			float totalSignificance = 0;
			float totalCombos = 0;
			
			ConnectionFabricator connsA(inA, a);
			do {
				float bestScore = -INFINITY;
				float bestSignificance = 0;
				
				ConnectionFabricator connsB(inB, b);
				do {
					++totalCombos;
					
					float angleDiff = abs(connsA.angle - connsB.angle);
					while (angleDiff > M_PI) angleDiff -= M_PI;
					
					
					float anglePenalty = angleDiff / M_PI;
					float straightLengthPenalty = abs(connsA.straightLength - connsB.straightLength);
					float pixLengthPenalty = abs(connsA.pixLength - connsB.pixLength);
					
					float combinedScore = 2 - anglePenalty - straightLengthPenalty/2 - pixLengthPenalty/2;
					
					assert(combinedScore < 2);
					
					if (combinedScore > bestScore) {
						float significance = fmin(connsA.straightLength, connsB.straightLength);
						if (significance != 0) {
							bestScore = combinedScore;
							bestSignificance = significance;
						}
					}
					
				} while (connsB.next());
				
				if (bestScore != -INFINITY) {
					assert(bestScore > -2);
					totalScore += bestScore*bestSignificance;
					totalSignificance += bestSignificance;
				}
				
			} while (connsA.next());
			
			if (totalSignificance != 0) {
				isectScores[a][b] = totalScore / totalSignificance;
			}
			else {
				assert(totalScore == 0);
				isectScores[a][b] = -10;
			}
		}
	}
	
	/*float amount = 0, strength = 0;
	for (int a = 0; a < inA.isects.size(); ++a) {
		float bestScore = -INFINITY;
		for (int b = 0; b < inB.isects.size(); ++b) {
			
			if (isectScores[a][b] > bestScore) bestScore = isectScores[a][b];
		}
	}*/
	
	float connScores[inA.c.size()];
	ZERO(connScores);
	
	for (int a = 0; a < inA.c.size(); ++a) {
		float bestMatch = 0;
		
		for (int b = 0; b < inB.c.size(); ++b) {
			float endScores[4] = {
				isectScores[inA.c[a].intersect1][inB.c[b].intersect1],
				isectScores[inA.c[a].intersect1][inB.c[b].intersect2],
				isectScores[inA.c[a].intersect2][inB.c[b].intersect1],
				isectScores[inA.c[a].intersect2][inB.c[b].intersect2]
			};
			float betterScores[2] = { 0, 0 };
			
			// should have just used sort
			for (int k = 0; k < 4; ++k) {
				if (endScores[k] > betterScores[0]) {
					if (betterScores[0] < betterScores[1]) betterScores[1] = betterScores[0];
					betterScores[0] = endScores[k];
				}
				else if (endScores[k] > betterScores[1]) betterScores[1] = endScores[k];
			}
			float match = (betterScores[0] + betterScores[1])/2;
			assert(match < 10000);
			if (match > bestMatch) bestMatch = match;
		}
		connScores[a] = bestMatch;
	}
	
	constexpr float scoreThreshold = 1.1;
	
	float amount = 0, strength = 0;
	for (int i = 0; i < inA.c.size(); ++i) {
		
		if (connScores[i] > scoreThreshold) {
			++amount;
			strength += connScores[i];
		}
	}
	assert(strength < 10000);
	return { amount, strength / amount };
}




void visualizeConnections(cv::Mat skel) {
	AnalyzedSkeleton in = analyzeSkeleton(skel);
	
	float nomImgSize = (skel.rows + skel.cols)/2.0;
	
	for (int i = 0; i < in.isects.size(); ++i) {
		
		cv::Mat outImg(skel.rows, skel.cols, CV_8UC3);
		for (int y = 0; y < skel.rows; ++y) {
			for (int x = 0; x < skel.cols; ++x) {
				if (skel.at<uint8_t>(y, x) != 0) {
					outImg.at<cv::Vec3b>(y, x) = { 255, 255, 255 };
				}
				else outImg.at<cv::Vec3b>(y, x) = { 0, 0, 0 };
			}
		}
		
		auto isectPt = in.isects[i].pos * nomImgSize;
		
		ConnectionFabricator conns(in, i);
		do {
			
			cv::line(outImg, isectPt,
					 in.isects[conns.isectChain[conns.currentDepth + 1]].pos * nomImgSize,
					 CV_RGB(0, 0, 255));
			
		} while (conns.next());
		
		cv::circle(outImg, isectPt, 2, CV_RGB(255, 0, 0));
		
		cv::imshow("", outImg);
		cv::waitKey(0);
	}
}
