% higher score is better
% bwcharBounds = [top, left; height, width]
function score = compareChar(grayChar, charMap, bwCharBounds, bwCompare, compareMap, distMap)
	
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
