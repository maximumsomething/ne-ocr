inputFolder = 'Working Files/Alternates Pages';
blobsFolder = 'Working Files/Alternates Blobs';
refsFolder = 'Extracted Characters/Alternates Refs';

list = dir(inputFolder);

[numImages, ~] = size(list);
for i = 1:numImages
	filename = list(i).name
	if numel(filename) >= 6 && strcmp(filename(1:5), 'Page ')
		pageNum = sscanf(filename, 'Page %d.jpg');
		%if pageNum < 751
		%	continue
		%end
		
		processPage(imread([inputFolder,'/',filename]),...
			sprintf('%s/Page %d', blobsFolder, pageNum), sprintf('%s/Page %d', refsFolder, pageNum));
	end
end

function processPage(fullPage, namePrefix, refNamePrefix)

	BWPage = ~imbinarize(rgb2gray(fullPage));

	rects = getColumns(BWPage);
	%{
	imshow(imcrop(fullPage, rects(1, :)))
	figure, imshow(imcrop(fullPage, rects(2, :)))
	figure, imshow(imcrop(fullPage, rects(3, :)))
%}
	altRects = [];
	refRects = [];
	for i = 1:3
		[newAltRects, newRefRects] = getAltRows(rects(i, :), BWPage);
		altRects = [altRects; newAltRects];
		refRects = [refRects; newRefRects];
	end

	[numRects, ~] = size(altRects);

	figure, imshow(fullPage), hold on
	for i = 1:numRects
		rectangle('Position', altRects(i, :));
	end
	
	margin = 1;
	marginBox = [-margin, -margin, margin*2, margin*2];
	prevRefRect = [-1, -1, -1, -1];
	for i = 1:numRects
		if refRects(i, 1) > 0
			prevRefRect = round(refRects(i, :)) + marginBox;
			imwrite(imcrop(fullPage, prevRefRect), ...
				sprintf('%s - ref%d,%d.png', refNamePrefix, ...
				prevRefRect(1), prevRefRect(2)));
		end
		cropRect = round(altRects(i, :)) + marginBox;
		imwrite(imcrop(fullPage, cropRect), ...
			sprintf('%s - loc%d,%d,%d,%d ref%d,%d.png', namePrefix, ...
			cropRect(1), cropRect(2), cropRect(3), cropRect(4), ...
			prevRefRect(1), prevRefRect(2)));
	end
	
end

function [altRects, refRects] = getAltRows(columnBox, BWPage) 
	
	column = imcrop(BWPage, columnBox);
	%{
	cropOutAmount = 200;
	columnMain = imcrop(BWPage, columnBox + [0, cropOutAmount, 0, -2*cropOutAmount]);
	verticalEmpties = sum(columnMain, 1) == 0;
	verticalEmpties = imerode(imdilate(verticalEmpties, ones(10)), ones(10));
	colRegions = regionprops(verticalEmpties);
	colBoxes1D = cat(1, colRegions.BoundingBox);
	
	dividingCols = colBoxes1D(colBoxes1D(:, 3) > 10, :);
	dividingCols = dividingCols(dividingCols(:, 1) >= 1 & ... 
		dividingCols(:, 1) + dividingCols(:, 3) < columnBox(3), :);
	[n, ~] = size(dividingCols);
	assert(n == 2);
	colDividers = dividingCols(:, 1) + dividingCols(:, 3) / 2;
	%}
	
	rowRegions = regionprops(sum(column, 2) > 0);
	rowBoxes1D = cat(1, rowRegions.BoundingBox);
	
	assert(issorted(rowBoxes1D(:, 2)));
	
	altRects = zeros(numel(rowRegions), 4);
	refRects = zeros(numel(rowRegions), 4) - 1;
	
	for i = 1:numel(rowRegions)
		
		rowImg = imcrop(column, [0, rowBoxes1D(i, 2), columnBox(3), rowBoxes1D(i, 4)]);
		blobImg = imdilate(rowImg, strel('square', 10));
		%imshow(blobImg);
		
		blobRegions = regionprops(blobImg);
		blobBoxes = cat(1, blobRegions.BoundingBox);
		blobBoxes = blobBoxes(blobBoxes(:, 3) > 15 & blobBoxes(:, 4) > 15, :);
		
		[numBoxes, ~] = size(blobBoxes);
		if numBoxes <= 1
			continue
		end
		
		[~, leftBlobs] = sort(blobBoxes(:, 1));
		if blobBoxes(leftBlobs(1), 1) < 30
			refRects(i, 1:4) = blobBoxes(leftBlobs(1), :)...
				+ [0, rowBoxes1D(i, 2), 0, 0];
			if numBoxes <= 2
				continue
			end
		end
		
		altRects(i, 1:4) = blobBoxes(leftBlobs(end), :)...
			+ [0, rowBoxes1D(i, 2), 0, 0];
	end
	goodAltFilter = altRects(:, 1) > 0 & altRects(:, 2) > 0;
	altRects = altRects(goodAltFilter, :);
	refRects = refRects(goodAltFilter, :);
	
	altRects(:, 1:2) = altRects(:, 1:2) + columnBox(1:2);
	
	goodRefsFilter = refRects(:, 1) > 0 & refRects(:, 2) > 0;
	refRects(goodRefsFilter, 1:2) = refRects(goodRefsFilter, 1:2) + columnBox(1:2);
	
end

function rects = getColumns(BWPage)

	[r, c, ~] = size(BWPage);
	cropRect = [200, 200, c - 400, r - 200];

	deEdged = imcrop(BWPage, cropRect);
	deEdged = imdilate(deEdged, strel('square', 2));
	%deEdged = BWPage;

	deedgedCcomps = regionprops(deEdged);
	boxes = cat(1, deedgedCcomps.BoundingBox);
	%{
	[numBoxes, ~] = size(boxes);
	figure, imshow(~deEdged), hold on
	for i = 1:numBoxes
		rectangle('Position', boxes(i, :));
	end
	%}
	boxes(:, 1:2) = boxes(:, 1:2) + cropRect(1:2);
	
	talls = boxes(boxes(:,4) > 700 & boxes(:, 3) < 20, :);

	dividers = talls(1:2, :);
	if (dividers(1, 1) > dividers(2, 1)) 
		dividers = [dividers(2, :); dividers(1, :)];
	end
	
	margin = 3;
	
	top = min(dividers(1:2, 2));
	height = max(dividers(1:2, 4)) + 10;
	
	%centerBox = [dividers(1, 1) + dividers(1, 3) + margin, top,...
	%	dividers(2, 1) - (dividers(1, 1) + dividers(1, 3) + margin*2), height];
	
	
	lesserCropRect = [100, 100, c - 200, r - 200];
	lesserDeedged = imcrop(BWPage, lesserCropRect);
	edgedComps = regionprops(lesserDeedged);
	edgedBoxes = cat(1, edgedComps.BoundingBox);
	edgedBoxes(:, 1:2) = edgedBoxes(:, 1:2) + lesserCropRect(1:2);
	
	smalls = edgedBoxes(edgedBoxes(:, 3) < 70 & edgedBoxes(:, 4) < 70 ...
	& edgedBoxes(:, 3) > 5 & edgedBoxes(:, 4) > 5, :);
	
	left = min(smalls(:, 1)) - margin;
	smallRights = smalls(:, 1) + smalls(:, 3);
	right = max(smallRights) + margin;
	
	%{
	[numSmalls, ~] = size(smalls);
	figure, imshow(~BWPage), hold on
	for i = 1:numSmalls
		rectangle('Position', smalls(i, :));
	end
	%}
	
	leftBox = [left, top, dividers(1, 1) - left - margin, height];
	rightBox = [dividers(2, 1) + dividers(2, 3) + margin, top, ...
		right - (dividers(2, 1) + dividers(2, 3) + margin), height];
	
	centerLeft = min(smalls(smalls(:, 1) > dividers(1, 1) + dividers(1, 3), 1));
	centerRight = max(smallRights(smallRights < dividers(2, 1)));
	centerBox = [centerLeft - margin, top, centerRight - centerLeft + margin*2, height];

	rects = [leftBox; centerBox; rightBox];

end

%{
[H,T,R] = hough(BW);
P  = houghpeaks(H,5,'threshold',ceil(0.3*max(H(:))));

lines = houghlines(BW,T,R,P, 'FillGap', 50, 'MinLength',700);

figure, imshow(BW), hold on
for k = 1:length(lines)
   xy = [lines(k).point1; lines(k).point2];
   plot(xy(:,1),xy(:,2),'LineWidth',2,'Color','green');

   % Plot beginnings and ends of lines
   plot(xy(1,1),xy(1,2),'x','LineWidth',2,'Color','yellow');
   plot(xy(2,1),xy(2,2),'x','LineWidth',2,'Color','red');

   % Determine the endpoints of the longest line segment
   len = norm(lines(k).point1 - lines(k).point2);
   if ( len > max_len)
      max_len = len;
      xy_long = xy;
   end
end
%}


%skel = bwskel(imerode(imbinarize(rgb2gray(fullPage)), strel('square', 10)));

%shower = fullPage * 0.5;
%shower(skel) = shower(skel) * 3;

%(shower);