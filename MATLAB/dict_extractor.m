inputFolder = 'Working Files/Pages';
outputFolder = 'Working Files/Extracted characters';

list = dir(inputFolder);

[numImages, ~] = size(list);


Page = {};
Rect = {};
ImgName = {};

for i = 1:numImages
	filename = list(i).name;
	if numel(filename) >= 6 && strcmp(filename(1:5), 'Page ') ...
			&& sscanf(filename, 'Page %d.jpg') >= 37
		
		fprintf('%s\n', filename);
		[imgs, rects] = processPage([inputFolder,'/',filename]);
		
		for j = 1:numel(rects)
			filenameParts = strsplit(filename, '.');
			pageName = filenameParts{1};
			
			%Page{end+1, 1} = pageName;
			%Rect{end+1, 1} = rects{j};
			
			imageFilename = [pageName,' - ',num2str(j),'.png'];
			%ImgName{end+1, 1} = imageFilename;
			imwrite(imgs{j}, [outputFolder,'/',imageFilename]);
		end
	end
end

%fprintf(fopen([outputFolder, '/info.json'], 'w'), ...
%	'%s', jsonencode(table(Page, Rect, ImgName)));


function [characterImgs, characterBounds] = processPage(filePath)
	boldnessConstant = 0.0018165; % fraction of page. Minimum radius of character stroke.
	
	% fraction of page size, of big character being extracted. From any point to any point
	maxCharacterSize = 0.075;
	minCharacterSize = 0.006;
	characterBorder = 0.01; % added on all sides
	
	inputImage = imread(filePath);
	info = imfinfo(filePath);
	
	minCharacterPix = minCharacterSize * info.Width;
	maxCharacterPix = maxCharacterSize * info.Width;
	characterBorderPix = characterBorder * info.Width;
	
	cropRect = [(0.045 * info.Width), (info.Height * 0.08), ...
		((1 - (0.045 + 0.03)) * info.Width), (info.Height * 0.84)];
	
	image = imcrop(inputImage, cropRect);
	
	bwimage = imbinarize(rgb2gray(image));
	dist = bwdist(bwimage);
	%maxima = imregionalmax(dist);
	
	%[height, width] = size(dist);
	
	pointRegions = {}; % array of (# of points) x 2 matrices
	% array of vectors [xLeft, yTop, xRight, yBottom]. 
	% Each corresponds to pointRegion of the same index.
	boundingRects = {}; 
	
	validPoints = find(dist >= boldnessConstant * info.Width);
	if (numel(validPoints) > 200000) 
		fprintf('Too many valid points (%d), using maxima instead\n', numel(validPoints));
		dist(dist < boldnessConstant * info.Width) = 0;
		validPoints = find(imregionalmax(dist));
	end
	
	for i = 1:numel(validPoints)
		[y, x] = ind2sub(size(dist), validPoints(i));
		
		foundRegion = false;
		for j = 1:numel(boundingRects)
			corners = rectCorners(boundingRects{j});
			nearby = false;
			for k = 1:4
				nearby = nearby || sqrt((corners(k, 1) - x) .^ 2 + ...
					(corners(k, 2) - y) .^ 2) < maxCharacterPix;
			end
			
			if nearby
				pointRegions{j}(end+1, 1:end) = [x, y];
				
				if x < boundingRects{j}(1)
					boundingRects{j}(1) = x;
				end
				if y < boundingRects{j}(2)
					boundingRects{j}(2) = y;
				end
				if x > boundingRects{j}(3)
					boundingRects{j}(3) = x;
				end
				if y > boundingRects{j}(4)
					boundingRects{j}(4) = y;
				end
				
				foundRegion = true;
			end
		end
		if ~foundRegion
			
			newElement = zeros(1, 2);
			newElement(1, 1:end) = [x, y];
			pointRegions{end+1} = newElement;
			boundingRects{end+1} = [x, y, x, y];
		end	
	end
	
	
	characterImgs = {};
	characterBounds = {};
	for j = 1:numel(boundingRects)
		
		rect = boundingRects{j};
		rect(3:4) = [rect(3) - rect(1), rect(4) - rect(2)];
		
		if rect(3) >= minCharacterPix && rect(4) >= minCharacterPix && ...
		   rect(3) <= maxCharacterPix && rect(4) <= maxCharacterPix
			
			rect = rect + [-characterBorderPix, -characterBorderPix, ...
				2*characterBorderPix, 2*characterBorderPix];
			rect = rect + [cropRect(1), cropRect(2), 0, 0];
			
			characterBounds{end+1} = rect;
			characterImgs{end+1} = imcrop(inputImage, rect);
			%subplot(1, size(boundingRects), j), imshow(outputImages{j});
		end
	end
	
	%imshow(montage(outputImages));
	%if size(outputImages) > 0
	%	imshow(outputImages{1});
	%end
	
	%imshow(uint8(dist * 30));
	%imshowpair(bwimage, maxima, 'montage');
end

% in: [xLeft, yTop, xRight, yBottom]. out: 4x2 matrix of corner points.
function corners = rectCorners(rect)

	corners = [rect(1) rect(2);...
		rect(3) rect(4);...
		rect(1) rect(4);...
		rect(3) rect(2)];
end

