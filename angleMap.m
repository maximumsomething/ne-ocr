% image = logical image
% coords, lengths = output of circleLinePixels function
% map is nxm matrix, where n = number of white pixels
% in image and m = length(circleLines)
function condensedMap = angleMap(image, coords, lengths)
	circleSize = numel(lengths);
	
	distImg = (3 - bwdist(image)) / 3;
	distImg(distImg < 0) = 0;
		
	pixels = find(image);
	
	map = zeros([numel(pixels), circleSize], 'single');
	
	% one pixel on the outer circle at a time
	for i = 1:circleSize
		numCoords = numel(lengths{i});
		
		scoreValues = (numCoords:-1:1) .* lengths{i};
		scoreValues = (scoreValues / sum(scoreValues));
		
		[r, c] = ind2sub(size(image), pixels);
		% nxmx2 where n is number of coords and m is number of pixels		
		transedCoords = cat(3, r(:).', c(:).') + permute(coords{i}, [1 3 2]);
		
		inBoundsCoords = all(transedCoords > 0 & ...
			transedCoords <= reshape(size(image), 1, 1, 2), 3);
		
		
		angleVals = zeros(size(coords{i}, 1), numel(pixels));
		angleVals(inBoundsCoords) = distImg(sub2ind(size(image),...
			transedCoords(inBoundsCoords),...
			transedCoords(padarray(inBoundsCoords, [0 0 1], 0, 'pre')))).' .* ...
			scoreValues(rem(find(inBoundsCoords) - 1, numCoords) + 1);
		
		map(:, i) = sum(angleVals, 1);
	end
	
	% halve number of angles and combine with neighbors
	condensedMap = (map(:, 1:2:circleSize) + map(:, 2:2:circleSize)*2 + map(:, [1, 3:2:circleSize])) / 4;

	%{
	[~, depth] = size(condensedMap);
	dispImgs = zeros([size(image), depth], 'double');
	dispImgs(repmat(image, 1, 1, depth)) = condensedMap;
	montage(imresize(uint8(dispImgs * 255), 2, 'nearest'));
	%}
end
