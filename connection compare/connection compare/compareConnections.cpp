#include "connections.h"

#define ZERO(e) memset(e, 0, sizeof(e))
#define LOOPSET(e, val) { for (int iii = 0; iii < sizeof(e) / sizeof(e[0]); ++iii) e[iii] = val; }

// higher is better
// connections of an intersection are weighted averaged together based on sigificance.
struct ConnPairScore {
	float score, significance;
};
ConnPairScore connectionSimilarity(Connection conn1, int isect1, Connection conn2, int isect2) {

	if (isect1 == conn1.intersect2) conn1.angle += M_PI;
	else assert(isect1 == conn1.intersect1);
	
	if (isect2 == conn2.intersect2) conn2.angle += M_PI;
	else assert(isect2 == conn2.intersect1);
	
	// very simple, tweak later
	float angleDiff = abs(conn2.angle - conn1.angle);
	while (angleDiff > M_PI) angleDiff -= M_PI;
	
	// between 0 and 1
	float angleScore = (1 - (angleDiff / M_PI));
	float lengthScore = 1 - abs(conn1.length - conn2.length);
	
	float combinedScore = angleScore + lengthScore*0;
	float significance = fmin(conn1.length, conn2.length);
	
	
	return { combinedScore, significance };
}

std::vector<std::vector<Connection> > getIntersections(ConnectionList conns) {
	std::vector<std::vector<Connection> > toReturn(conns.numIntersections);
	
	for (int i = 0; i < conns.numIntersections; ++i) {
		for (auto conn = conns.c.begin(); conn < conns.c.end(); ++conn) {
			
			if (conn->intersect1 == i || conn->intersect2 == i) {
				toReturn[i].push_back(*conn);
			}
		}
	}
	return toReturn;
}


CharPairScore compareConnections(ConnectionList inA, ConnectionList inB) {
	
	/*1. For each intersection and endpoint in in1, create a similarity score between it
	 and all intersections in in2.
	 2. For each connection in in1, match it with connections in in2 by
	 looking for the one with the most similar intersections.
	 */
	
	auto isectsA = getIntersections(inA);
	
	float isectScores[inA.numIntersections][inB.numIntersections];
	ZERO(isectScores);
	
	for (int i = 0; i < inA.numIntersections; ++i) {
		float totalSignificance[inB.numIntersections];
		ZERO(totalSignificance);
		
		// compare this intersection's connections to all the connections in connB
		for (auto connB = inB.c.begin(); connB < inB.c.end(); ++connB) {
			// the connB will only match with one connection at this intersection (one connA)
			//float connScores[inB.numIntersections];
			//LOOPSET(connScores, INFINITY);
			// one score for each intersection of connB
			ConnPairScore pair1 = { 0, 0 }, pair2 = { 0, 0 };
			
			for (auto connA = isectsA[i].begin(); connA < isectsA[i].end(); ++connA) {
				
				
				/*float score1 = connectionSimilarity(*connA, i, *connB, connB->intersect1);
				if (score1 < isectScores[i][connB->intersect1]) isectScores[i][connB->intersect1] = score1;
				
				float score2 = connectionSimilarity(*connA, i, *connB, connB->intersect2);
				if (score2 < isectScores[i][connB->intersect2]) isectScores[i][connB->intersect2] = score2;*/
				/*float score1 = connectionSimilarity(*connA, i, *connB, connB->intersect1);
				if (score1 < connScores[connB->intersect1]) connScores[connB->intersect1] = score1;
				
				/float score2 = connectionSimilarity(*connA, i, *connB, connB->intersect2);
				if (score2 < connScores[connB->intersect2]) connScores[connB->intersect2] = score2;*/
				ConnPairScore s1 = connectionSimilarity(*connA, i, *connB, connB->intersect1);
				if (s1.score > pair1.score) pair1 = s1;
				ConnPairScore s2 = connectionSimilarity(*connA, i, *connB, connB->intersect2);
				if (s2.score > pair2.score) pair2 = s2;
			}
			/*for (int j = 0; j < inB.numIntersections; ++j) {
				assert(connScores[i] < INFINITY);
				isectScores[i][j] += connScores[j];
			}*/
			isectScores[i][connB->intersect1] += pair1.score * pair1.significance;
			isectScores[i][connB->intersect2] += pair1.score * pair1.significance;;
			totalSignificance[connB->intersect1] += pair1.significance;
			totalSignificance[connB->intersect2] += pair2.significance;
		}
		for (int j = 0; j < inB.numIntersections; ++j) {
			//if (numConns[j] == 0) printf("An intersection without connections; shouldn't happen\n");
			if (totalSignificance[j] > 0) isectScores[i][j] /= totalSignificance[j];
			//printf("%f ", isectScores[i][j]);
		}
		//printf("\n");
	}
	
	/*for (int i = 0; i < inA.numIntersections; ++i) {
		for (int j = 0; j < inB.numIntersections; ++j) {
			
			printf("%f ", isectScores[i][j]);
		}
		printf("\n");
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
	assert(strength < INFINITY);
	return { amount, strength / amount };
}
