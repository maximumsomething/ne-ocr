
% higher score is better
function score = compareChar(charSkel, charMap, charIndMap, compareSkel, compareMap)
	maxDist = 12;
	
	score = 0;
	
	comparePixels = find(compareSkel);
	for i = 1:numel(comparePixels)
		
		[r, c] = ind2sub(size(compareSkel), comparePixels(i));
		pixelSub = ceil([r, c] .* size(charSkel) ./ size(compareSkel));
		
		low = max([1, 1], pixelSub - maxDist);
		high = min(size(charSkel), pixelSub + maxDist);
		
		nearMask = false(size(charSkel));
		nearMask(low(1):high(1), low(2):high(2)) = charSkel(low(1):high(1), low(2):high(2));
		
		[r, c] = find(nearMask);
		
		dists = (sqrt(sum(([r(:), c(:)] - pixelSub) .^ 2, 2))) / maxDist - 0.3;
		dists(dists < 0) = 0;
		
		thisScore = 1 - min(min(min(charMap(charIndMap(nearMask), :) .* compareMap(i, :) + dists), 1));
		if numel(thisScore) > 0
			score = score + thisScore;
		end
		
		assert(numel(score) == 1);
	end
	score = score / numel(comparePixels);
end


% higher score is better
% bwcharBounds = [top, left; height, width]
function score = compareCharOld(grayChar, charMap, bwCharBounds, bwCompare, compareMap, distMap)
	
	score = 0;
	
	comparePixels = find(bwCompare);
	for i = 1:numel(comparePixels)
		
		[r, c] = ind2sub(size(bwCompare), comparePixels(i));
		pixelSub = ceil([r, c] ./ size(bwCompare) .* bwCharBounds(2, :)) + bwCharBounds(1, :);	
		
		thisDistBounds = [max(pixelSub - floor(size(distMap) / 2), 1); ...
			min(pixelSub + ceil(size(distMap) / 2), size(grayChar) + 1) - 1];
		
		distMapArea = thisDistBounds(2,:) - thisDistBounds(1,:) + 1;
		
		distMapBounds = ceil(size(distMap) / 2) + ...
			((ceil(distMapArea / 2) - (pixelSub - (thisDistBounds(1,:) - 1)))) + ...
			[-ceil(distMapArea / 2) + 1; floor(distMapArea / 2)];
		
		
		thisDistMap = zeros(size(grayChar), 'single');
		thisDistMap(thisDistBounds(1):thisDistBounds(2), thisDistBounds(3):thisDistBounds(4)) =...
			distMap(distMapBounds(1):distMapBounds(2), distMapBounds(3):distMapBounds(4));
		
		scoreMap = zeros(size(grayChar));
		% for every direction in the map
		for j = 1:size(compareMap, 2)
			 scoreMap = max(scoreMap, thisDistMap .* (compareMap(i, j) .* ...
				charMap(:, :, ceil(j * size(charMap, 3) / size(compareMap, 2)))));
			
		end
		
		score = score + sum(scoreMap(:));
	end
	% so busier characters don't get artifically higher scores
	score = score / sum(compareMap(:));
end
%{
function bounds = centeredBounds(center, maxSize, areaLimit)
	bounds = [max(center - floor(maxSize / 2), 1); ...
			min(center + ceil(maxSize / 2), areaLimit + 1) - 1];
end
%}
