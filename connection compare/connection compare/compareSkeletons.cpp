#include "connections.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <numeric>


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
	
	int getEndIsect() { return isectChain[currentDepth + 1]; }
	int getStartIsect() { return isectChain[0]; }
	
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

#define GET2D(arr, x, y, sizY) (arr[y + x*sizY])

struct scoreSig { float score, significance; };

void compareConnectionsOfIntersections(AnalyzedSkeleton& inA, AnalyzedSkeleton& inB,
									float* isectScores,
									std::function<struct scoreSig (ConnectionFabricator&, ConnectionFabricator&)> getScore) {
	
	
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
			
			if (a == 3 && b == 8) {
				printf("here\n");
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
					
					auto connRating = getScore(connsA, connsB);
					
					if (connRating.score > bestScore) {
						if (connRating.significance != 0) {
							bestScore = connRating.score;
							bestSignificance = connRating.significance;
						}
					}
					
				} while (connsB.next());
				
				if (bestScore != -INFINITY) {
					//assert(bestScore > -2);
					totalScore += bestScore*bestSignificance;
					totalSignificance += bestSignificance;
				}
				
			} while (connsA.next());
			
			if (totalSignificance != 0) {
				GET2D(isectScores, a, b, inB.isects.size()) = totalScore / totalSignificance;
			}
			else {
				assert(totalScore == 0);
				GET2D(isectScores, a, b, inB.isects.size()) = -10;
			}
		}
	}
}



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
							[](ConnectionFabricator& connsA, ConnectionFabricator& connsB) -> struct scoreSig {
		float angleDiff = abs(connsA.angle - connsB.angle);
		while (angleDiff > M_PI) angleDiff -= M_PI;
		
		
		float anglePenalty = angleDiff / M_PI;
		float straightLengthPenalty = abs(connsA.straightLength - connsB.straightLength);
		float pixLengthPenalty = abs(connsA.pixLength - connsB.pixLength);
		
		float combinedScore = 2 - anglePenalty - straightLengthPenalty/2 - pixLengthPenalty/2;
		
		assert(combinedScore < 2);
		
		float significance = fmin(connsA.straightLength, connsB.straightLength);
								
								return { combinedScore, significance };
		
	});
	
	visHook((float *) isectScores);
	
	// Let's do that again!
	/*float newIsectScores[inA.isects.size()][inB.isects.size()];
	ZERO(newIsectScores);
	
	compareConnectionsOfIntersections(inA, inB, (float *) newIsectScores,
	[&](ConnectionFabricator& connsA, ConnectionFabricator& connsB) -> struct scoreSig {
		
		
		float score = isectScores[connsA.getEndIsect()][connsB.getEndIsect()];
		
		return  { score, 1 };
	});*/
	
					
	
	
	float amount = 0, strength = 0;
	for (int a = 0; a < inA.isects.size(); ++a) {
		float bestScore = -INFINITY;
		for (int b = 0; b < inB.isects.size(); ++b) {
			
			if (isectScores[a][b] > bestScore) bestScore = isectScores[a][b];
		}
		if (bestScore > 1.8) {
			++amount;
			strength += bestScore;
		}
	}
	
	return { amount, strength / amount };
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
	
	compareSkeletons(inA, inB, [&](float* isectScores) {
		
		cv::Mat outImg(std::max(skelA.rows, skelB.rows) * scaleFactor,
					   (skelA.cols + skelB.cols + 10) * scaleFactor,
					   CV_8UC3, { 0, 0, 0 });
		
		
		constexpr float compThreshold = 1.9;
		
		// draw worse lines first
		std::vector<int> sortIdxes(inA.isects.size() * inB.isects.size());
		std::iota(sortIdxes.begin(), sortIdxes.end(), 0);
		
		std::sort(sortIdxes.begin(), sortIdxes.end(), [&](int i1, int i2) {
			return isectScores[i1] < isectScores[i2];
		});
		
		for (int i = 0; i < inA.isects.size() * inB.isects.size(); ++i) {
			float score = isectScores[sortIdxes[i]];
			if (score < compThreshold) continue;
			assert(score <= 2);
			
			
			Intersection isectA = inA.isects[sortIdxes[i] / inB.isects.size()];
			Intersection isectB = inB.isects[sortIdxes[i] % inB.isects.size()];
			
			printf("i: %d a: %d b: %d score: %f\n", sortIdxes[i], sortIdxes[i] / inB.isects.size(), sortIdxes[i] % inB.isects.size(), score);
			
			// determine color, red=bad, green=good
			cv::Mat3b bgrColor;
			cv::Vec3b hsvColor = { (unsigned char) ((score - compThreshold) / (2 - compThreshold) * 100), 255, 255 };
			
			cv::cvtColor(cv::Mat3b(hsvColor), bgrColor, cv::COLOR_HSV2BGR);
			
			cv::line(outImg, isectA.pos * inA.nomImgSize * scaleFactor,
					 cv::Point2f(skelA.cols + 10 + isectB.pos.x * inB.nomImgSize, isectB.pos.y * inB.nomImgSize) * scaleFactor, bgrColor.at<cv::Vec3b>(0));
		}
		
		transferSkel(skelA, outImg, {0, 0}, scaleFactor);
		transferSkel(skelB, outImg, {skelA.cols + 10, 0}, scaleFactor);
		
		for (int i = 0; i < inA.isects.size(); ++i) {
			cv::putText(outImg, std::to_string(i),
						inA.isects[i].pos * inA.nomImgSize * scaleFactor,
						cv::FONT_HERSHEY_PLAIN, 2, { 255, 200, 200 });
		}
		for (int i = 0; i < inB.isects.size(); ++i) {
			cv::putText(outImg, std::to_string(i),
						cv::Point2f(skelA.cols + 10 + inB.isects[i].pos.x * inB.nomImgSize,
									inB.isects[i].pos.y * inB.nomImgSize)
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
