% returns empty matrix if the core is too small
function core = scaleSkel(img, up, down, openPixels)
	
	core = distSkel(bwareaopen(imbinarize(rgb2gray(imresize(img, up))), openPixels));
	
	%core = imresize(double(core),...
	%	0.5, 'bilinear', 'Antialiasing', false, 'Dither', false) > 0;
	core = bwskel(imresize(double(core), down, 'bilinear') > 0);
	
	% minimum width and height of core, in pixels
	minCharSize = 10;
	% this code used before 'bwskel' function discovered
	%{
	% number of adjacent pixels that have to have a lower distance for a
	% pixel to be considered part of the core
	adjacencyConst = 7;
	% minimum distance from other pixels (effectively radius of line) to
	% be part of the core
	minDist = 1.5;
	
	dist = bwdist(bwimg);
	
	[height, ~] = size(bwimg);
	
	[r, c] = ind2sub([3, 3], find([1,1,1; 1,0,1; 1,1,1]));
	subSurrounds = [r, c] - 2;
	surrounds = subSurrounds(:,1) + subSurrounds(:,2)*height;
	
	core = zeros(size(dist), 'logical');
	%}
	maxBound = intmax('int32');
	% top, left
	topBound = [maxBound, maxBound];
	% bottom, right
	bottomBound = int32([0, 0]);
	
	for i = 1:numel(core)
		%{
		thisSurrounds = surrounds + i;
		
		core(i) = dist(i) > minDist && sum(dist(thisSurrounds( ...
			thisSurrounds >= 1 & thisSurrounds <= numel(dist))) ...
			<= dist(i)) >= adjacencyConst;
		%}
		if core(i)
			[r, c] = ind2sub(size(core), i);
			pixel = [r, c];
			
			low = pixel < topBound;
			topBound(low) = pixel(low);
			
			high = pixel > bottomBound;
			bottomBound(high) = pixel(high);
		end
	end
	
	
	
	cropRect = [topBound(2) topBound(1), ...
		bottomBound(2) - topBound(2), bottomBound(1) - topBound(1)];
	if cropRect(3) >= minCharSize && cropRect(4) >= minCharSize
		
		core = imcrop(core, cropRect);
	else 
		core = [];
	end
end
