% image = grayscale or logical image
% coords, lengths = output of circleLinePixels function
% If image is grayscale, map is 3D, size(image) x length(circleLines)
% If image is logical, map is nxm matrix, where n = number of white pixels
% in image and m = length(circleLines)
function map = angleMap(image, coords, lengths)
	
	isLogical = islogical(image);
	
	if isLogical
		
		comp = bwconncomp(image);
		map = zeros([sum(image(:)), numel(coords)], 'single');
		pixels = zeros([sum(image(:)), 1], 'uint32');
		
		pixelCount = 0;
		for i = 1:comp.NumObjects
			bwcomp = zeros(size(image), 'logical');
			bwcomp(comp.PixelIdxList{i}) = true;
			
			compDist = (3 - bwdist(bwcomp)) / 3;
			compDist(compDist < 0) = 0;
			
			newPixelCount = pixelCount + numel(comp.PixelIdxList{i});
			
			map((pixelCount + 1):newPixelCount, :) = doImage(coords, lengths, compDist, comp.PixelIdxList{i});
			 	 
			pixels((pixelCount + 1):newPixelCount) = comp.PixelIdxList{i};
			pixelCount = newPixelCount;
		end
		
		[~, indexes] = sort(pixels);
		map = map(indexes, :);
	else
		workImage = max(image(:)) - image;
		workImage = double(workImage) / double(max(workImage(:)));
		
		map = reshape(doImage(coords, lengths, workImage, 1:1:numel(workImage)), ...
			[size(workImage), numel(coords)]) .* workImage;
	end
	assert(all(all(all(map >= 0 & map <= 1))));

	
	if isLogical
		[~, depth] = size(map);
		dispImgs = zeros([size(image), depth], 'double');
		dispImgs(repmat(image, 1, 1, depth)) = map;
		montage(imresize(uint8(dispImgs * 255), 2, 'nearest'));
	else
		montage(uint8(imresize(cat(3, workImage, map),  10, 'nearest') * 255));
	end
	
end

function map = doImage(coords, lengths, image, pixels)
	map = zeros([numel(pixels), numel(lengths)], 'single');
	
	% one pixel on the outer circle at a time
	for i = 1:numel(lengths)
		numCoords = numel(lengths{i});
		
		scoreValues = (numCoords:-1:1) .* lengths{i};
		scoreValues = (scoreValues / sum(scoreValues));
		
		[r, c] = ind2sub(size(image), pixels);
		% nxmx2 where n is number of coords and m is number of pixels		
		transedCoords = cat(3, r(:).', c(:).') + permute(coords{i}, [1 3 2]);
		
		inBoundsCoords = all(transedCoords > 0 & ...
			transedCoords <= reshape(size(image), 1, 1, 2), 3);
		
		
		angleVals = zeros(size(coords{i}, 1), numel(pixels));
		angleVals(inBoundsCoords) = image(sub2ind(size(image),...
			transedCoords(inBoundsCoords),...
			transedCoords(padarray(inBoundsCoords, [0 0 1], 0, 'pre')))).' .* ...
			scoreValues(rem(find(inBoundsCoords) - 1, numCoords) + 1);
		
		map(:, i) = sum(angleVals, 1);
		
		
		%[r, c] = ind2sub(size(image), linearCoord);
		%coord = [r, c];
		
		%compareCoords = coord + coords{i};
		%inBoundsCoords = all(compareCoords > 0 & compareCoords < size(image), 2);
		
		%pixelVals = image(sub2ind(size(image), compareCoords(inBoundsCoords)));
		
		
	end
end

%