% image = grayscale or logical image
% coords, lengths = output of circleLinePixels function
% If image is grayscale, map is 3D, size(image) x length(circleLines)
% If image is logical, map is nxm matrix, where n = number of white pixels
% in image and m = length(circleLines)
function map = angleMap(image, coords, lengths)
	[height, ~] = size(image);
	
	isLogical = islogical(image);
	
	if isLogical
		pixels = find(image).';
		map = zeros([numel(pixels), numel(coords)], 'single');
		workImage = imdilate(image, strel('square', 2));
	else
		workImage = max(image(:)) - image;
		workImage = double(workImage) / double(max(workImage(:)));
		
		pixels = 1:1:numel(workImage);
		map = zeros([size(workImage), numel(coords)], 'single');
	end
	
	% one pixel on the outer circle at a time
	for i = 1:numel(lengths)
		numCoords = numel(lengths{i});
		
		scoreValues = (numCoords:-1:1) .* lengths{i};
		scoreValues = (scoreValues / sum(scoreValues));
		
		% nxm where n is number of coords and m is number of pixels
		indCoords = (coords{i}(:,1) + coords{i}(:,2) * height) + pixels;
		inBoundsCoords = indCoords > 0 & indCoords <= numel(workImage);
		
		angleVals = zeros(size(indCoords));
		angleVals(inBoundsCoords) = workImage(indCoords(inBoundsCoords)).' .* ...
			scoreValues(rem(find(inBoundsCoords) - 1, numCoords) + 1);
		
		angleVals = sum(angleVals, 1);
		
		%[r, c] = ind2sub(size(image), linearCoord);
		%coord = [r, c];
		
		%compareCoords = coord + coords{i};
		%inBoundsCoords = all(compareCoords > 0 & compareCoords < size(image), 2);
		
		%pixelVals = image(sub2ind(size(image), compareCoords(inBoundsCoords)));
		
		if isLogical
			map(:, i) = angleVals;
		else
			map(:, :, i) = reshape(angleVals, size(workImage)) .* workImage;
		end
	end
	if isLogical
		[~, depth] = size(map);
		dispImgs = zeros([size(image), depth], 'double');
		dispImgs(repmat(image, 1, 1, depth)) = map;
		montage(uint8(imresize(dispImgs,  5, 'nearest') * 255));
	else
		montage(uint8(imresize(cat(3, workImage, map),  10, 'nearest') * 255));
	end
end

%{
old shit with oversight
function map = angleMap(image, coords, lengths)
	[width, height] = size(image);
	
	isLogical = islogical(image);
	
	if isLogical
		pixels = find(image);
		map = zeros([numel(pixels), numel(coords)], 'float');
	else
		image = ~image;
		pixels = image;
		map = zeros([size(image), numel(coords)], 'float');
	end
	
	for i = 1:numel(pixels)
		
		scoreValues = (numel(lengths{i}):-1:1) .* lengths{i};
		scoreValues = (scoreValues / sum(scoreValues));
		
		if isLogical
			linearCoord = pixels(i);
		else
			linearCoord = i;
		end
		indCoords = (coords{i}(:,1,:) + coords{i}(:,2,:) * height) + linearCoord;
		inBoundsCoords = indCoords > 0 & indCoords <= numel(image);
		
		angleVals = sum(image(indCoords(inBoundsCoords)) .*...
			(scoreValues(inBoundsCoords)), 2);
		
		%[r, c] = ind2sub(size(image), linearCoord);
		%coord = [r, c];
		
		%compareCoords = coord + coords{i};
		%inBoundsCoords = all(compareCoords > 0 & compareCoords < size(image), 2);
		
		%pixelVals = image(sub2ind(size(image), compareCoords(inBoundsCoords)));
		
		if isLogical
			map(i, :) = angleVals(:);
		else
			[r, c] = ind2sub(size(image), i);
			map(r, c, :) = angleVals(:);
		end
	end
end
%}