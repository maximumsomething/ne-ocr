skel = scaleSkel(imread(...
'/Users/max/Downloads/imageIn.png'...
), 36, 1/3, 0);

% all this code is hideously inefficent, but it doesn't matter

%bwmorph branchpoints doesn't work
intersections = getIntersections(skel);
noIntersections = skel;
noIntersections(intersections) = 0;
components = bwconncomp(noIntersections);

%imshow(cat(3, skel, intersections, ...
%	imdilate(intersections, strel('square', 3)) * 255));

avgSize = sqrt(numel(skel));

% each line between intersections is represented as two connections, one
% pointing each way. 
% col 1 is length, col 2 is angle, col 3 is which intersection
connectionList = zeros(components.NumObjects*2, 2);
% contains indices of items in connectionList
connectionImg = zeros(size(skel), 'uint32');

for i = 1:components.NumObjects 
	
	component = zeros(components.ImageSize, 'logical');
	component(components.PixelIdxList{i}) = 1;
	[endR, endC] = find(bwmorph(component, 'endpoints'));
	
	assert(numel(endR) == 0 || numel(endR) == 2);
	
	if numel(endR) == 2
		length = sqrt((endR(2) - endR(1))^2 + (endC(2) - endC(1))^2) / avgSize;
		angle = atan2(endR(2) - endR(1), endC(2) - endC(1));
		
		connectionList(i*2 - 1, :) = [length, angle];
		connectionList(i*2, 1:2) = [length, angle + pi];
		
		connectionImg(endR(1), endC(1)) = i*2 - 1;
		connectionImg(endR(2), endC(2)) = i*2;
	end
end
imshow(cat(3, skel, (connectionImg > 0) * 255, ...
	imdilate(intersections, strel('square', 3)) * 255));

% put intersection numbers of connectionList. Endpoints are included as
% intersections here.
wholeIntersections = bwconncomp(intersections | bwmorph(skel, 'endpoints'));
for i = 1:wholeIntersections.NumObjects 
	
	intersection = zeros(size(skel), 'logical');
	intersection(wholeIntersections.PixelIdxList{i}) = 1;
	
	connections = connectionImg(connectionImg > 0 & ...
		imdilate(intersection, strel('square', 3)));
	
	connectionList(connections, 3) = i;
end


function intersections = getIntersections(skel) 
	
	img = padarray(skel,[1 1], 0,'both');

	skelInds = find(img);

	intersections = zeros(size(skel), 'logical');
	for i = 1:numel(skelInds)
		[r,c] = ind2sub(size(img), skelInds(i));

		neighbors = img(r-1:r+1,c-1:c+1);
		numNeighbors = sum(neighbors, 'All') - 1;

		if (numNeighbors > 2) 
			intersections(r-1, c-1) = 1;
		end
	end
end


%{
img = padarray(skel,[1 1], 0,'both');

skelInds = find(img);

intersectionPts = [];
endPts = [];
for i = 1:numel(skelInds)
	[r,c] = ind2sub(size(img), skelInds(i));
	
	neighbors = img(r-1:r+1,c-1:c+1);
	numNeighbors = sum(neighbors, 'All') - 1;
	
	if (numNeighbors > 2) 
		intersectionPts(end+1) = skelInds(i);
	end
	if (numNeighbors == 1) 
		endPts(end+1) = skelInds(i);
	end
end


intersectionDots = zeros(size(img), 'logical');
intersectionDots(intersectionPts) = true;

endpointDots = zeros(size(img), 'logical');
endpointDots(endPts) = true;

toShow = zeros([size(img) 3],'uint8');
toShow(:,:,2) = img*255;
toShow(:,:,3) = branchPts*255;
toShow(:,:,1) = endpointDots*255;

%imshow(toShow);

function walkSkel(skel, startPoint, prevPoint)
	
	[r,c] = ind2sub(size(skel), startPoint);
	
	neighbors = img(r-1:r+1,c-1:c+1);
	[neighborR, neighborC] = find(neighbors);
	neighborInds = sub2ind(size(skel), neighborR + r - 2, neighborC + c - 2);
	
	
end
%}

