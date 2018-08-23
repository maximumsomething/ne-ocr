function score = compareChar(bwchar, charPixels, charSize, compareCore, comparePixels)
	
	[compareDist, compareClosest] = bwdist(compareCore);
	[charDist, charClosest] = bwdist(bwchar);
	
	originalCharDist = distFromCentroid(charPixels);
	originalCompareDist = distFromCentroid(comparePixels);
	
	translate = [0.0, 0.0]; % after scaling.
	scale = min(size(compareCore) / charSize);
	
	for i = 1:5
		
		[scale1, trans1] = compareImgs(transformPixels(...
			charPixels, scale, translate, charSize, size(compareCore)),...
			originalCharDist * scale, compareClosest);
		
		
		[scale2, trans2] = compareImgs(transformPixels(...
			comparePixels, 1/scale, -translate / scale,...
			size(compareCore), charSize),...
			originalCompareDist / scale, charClosest);
		
		scale = scale * (scale1 / scale2);
		if scale == 0 || isinf(scale) || isnan(scale)
			score = Inf();
			return;
		end
		
		translate = trans1 - trans2*scale;
	end
	% score is divided by these to prevent more "cluttered" characters from
	% getting higher scores.
	[charPixLen, ~] = size(charPixels);
	[comparePixLen, ~] = size(comparePixels);
	
	score1 = scoreImg(transformPixels(comparePixels,...
		1/scale, -translate / scale, size(compareCore), charSize),...
		 charDist) / charPixLen;
	
	finalPixels = transformPixels(...
		charPixels, scale, translate, charSize, size(compareCore));
	score = score1 + scoreImg(finalPixels, compareDist) / comparePixLen;
	
	%visualizeResult(finalPixels, compareCore);
end

function out = transformPixels(pixels, scale, translate, imgSize, compareSize)
	out = pixels * scale + translate ...
		+ ((compareSize - scale*imgSize) / 2);
end

function [scale, translation] = compareImgs(...
		transformedPixels, centroidDist, compareImgClosest)
	[height, width] = size(compareImgClosest);
	
	translation = [0.0, 0.0];
	closestPixels = zeros(size(transformedPixels));	
	
	for j = 1:size(transformedPixels)
		coord = transformedPixels(j, 1:2);
		
		% find closest white pixel of dictionary character
		[r, c] = ind2sub(size(compareImgClosest), ...
			roundAndGetPixel(coord, compareImgClosest));
		
		%closestIndex = roundAndGetPixel(coord, compareImgClosest) - 1;
		%r = rem(closestIndex, height) + 1;
		%c = (closestIndex - r) ./ height + 1;
			
		nearestVect = [r, c] - coord;
		
		closestPixels(j, 1:2) = [r, c];
		
		translation = (translation * ((j - 1)/j)) + ...
			(nearestVect * (1 / j));
	end
	scale = (distFromCentroid(closestPixels) ...
		/ centroidDist);
end

%lower = better
function score = scoreImg(transformedPixels, compareImgDist)
	score = 0;
	
	for j = 1:size(transformedPixels)
		coord = transformedPixels(j, 1:2);
		
		[dist, extraDist] = roundAndGetPixel(coord, compareImgDist);
		score = score + dist + extraDist;
	end
	[numPix, ~] = size(transformedPixels);
	score = score / numPix;
end

function [pixelVal, extraDist] = roundAndGetPixel(coord, compareImg)
	
	[roundCoord, extraDist] = ...
		clamp(round(coord), [1, 1], size(compareImg));
	extraDist = sum(extraDist);
	
	pixelVal = compareImg(roundCoord(1), roundCoord(2));
end

% all three inputs must be same length
function [clamped, distance] = clamp(in, minBound, maxBound)
	clamped = in;
	distance = zeros(size(in));
	
	lows = in < minBound;
	distance(lows) = minBound(lows) - in(lows);
	clamped(lows) = minBound(lows);
	
	highs = in > maxBound;
	distance(highs) = in(highs) - maxBound(highs);
	clamped(highs) = maxBound(highs);
end

function [dist] = distFromCentroid(points) % points = nx2 matrix
	centroid = mean(points);
	dist = mean(sqrt((points(1:end, 1) - centroid(1)).^2 + ...
		(points(1:end, 2) - centroid(2)).^2));
end

function visualizeResult(transedPixels, bwcompare)
	[r, c] = size(bwcompare);
	%img = zeros(r, c, 3, 'uint8');
	
	img = uint8((~bwcompare) * 255);
	
	for k = 1:size(transedPixels)
		%ind = sub2ind([r, c, 3], round(transedPixels(k, 1)), round(transedPixels(k, 2)), 2);
		%img(ind) = 0;
		
		img = insertShape(img, 'FilledCircle', ...
			[round(transedPixels(k, 2)), round(transedPixels(k, 1)), 6],...
			'Color', 'blue', 'Opacity', 0.3);
	end
	imshow(imresize(img, 3, 'nearest'));
	pause(0.5);
end

