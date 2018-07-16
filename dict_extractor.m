clear;

inputFolder = 'Working Files/first pages without ToC';
outputFolder = 'Working Files/Extracted characters';

list = dir(inputFolder);

[numImages, null] = size(list);


Page = {};
Rect = {};
ImgName = {};

for i = 1:numImages
	filename = list(i).name;
	if filename(1) ~= '.'
		[imgs, rects] = processPage([inputFolder,'/',filename]);
		
		for j = 1:numel(rects)
			Page{end+1, 1} = filename;
			Rect{end+1, 1} = rects{j};
			
			imageFilename = [filename,' - ',num2str(j),'.png'];
			ImgName{end+1, 1} = imageFilename;
			imwrite(imgs{j}, [outputFolder,'/',imageFilename]);
		end
	end
end

fprintf(fopen([outputFolder, '/info.json'], 'w'), ...
	'%s', jsonencode(table(Page, Rect, ImgName)));


function [characterImgs, characterBounds] = processPage(filePath)
	boldnessConstant = 6; % pixels. Minimum radius of character stroke.
	
	% pixels, of big character being extracted. From any point to any point
	maxCharacterSize = 300;
	minCharacterSize = 10;
	characterBorder = 50; % added on all sides
	
	inputImage = imread(filePath);
	info = imfinfo(filePath);
	
	cropRect = [(0.045 * info.Width), (info.Height * 0.08), ...
		((1 - (0.045 + 0.03)) * info.Width), (info.Height * 0.84)];
	
	image = imcrop(inputImage, cropRect);
	
	bwimage = imbinarize(rgb2gray(image));
	dist = bwdist(bwimage);
	maxima = imregionalmax(dist);
	
	[height, width] = size(dist);
	
	pointRegions = {}; % array of (# of points) x 2 matrices
	boundingRects = {}; % array of vectors [xLeft, yTop, xRight, yBottom]
	
	validMaxima = find(dist(maxima) >= boldnessConstant);
	
	for i = 1:numel(validMaxima)
		[y, x] = ind2sub(size(dist), validMaxima(i));
		
		foundRegion = false;
		for j = 1:numel(pointRegions)
			if sqrt((pointRegions{j}(1, 1) - x) .^ 2 + ...
					(pointRegions{j}(1, 2) - y) .^ 2) < maxCharacterSize
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
		
		if rect(3) >= minCharacterSize && rect(4) >= minCharacterSize
			
			rect = rect + [-characterBorder, -characterBorder, 2*characterBorder, 2*characterBorder];
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




