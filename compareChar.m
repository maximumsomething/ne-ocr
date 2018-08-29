% higher score is better
% bwcharBounds = [top, left; height, width]
function score = compareChar(grayChar, charMap, bwCharBounds, bwCompare, compareMap, distMap, distMapInds)
	
	score = 0;
	
	comparePixels = find(bwCompare);
	for i = 1:numel(comparePixels)
		
		[r, c] = ind2sub(size(bwCompare), comparePixels(i));
		pixelSub = num2cell(ceil([r, c] ./ size(bwCompare) .* bwCharBounds(2, :)) + bwCharBounds(1, :));		
		pixelInds = sub2ind(size(grayChar), pixelSub{:}) + distMapInds;
		
		validInds = pixelInds > 0 & pixelInds < numel(grayChar);
		thisDistMap = zeros(size(grayChar), 'single');
		thisDistMap(pixelInds(validInds)) = distMap(validInds);
		
		%{
		to implement: find largest-scoring matches, regardless of distance
		match them somehow
		%}
		
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
