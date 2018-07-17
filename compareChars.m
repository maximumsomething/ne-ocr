imgName = 'Working Files/Zuozhai book 1/11.jpeg';

%character = imcrop(imread(imgName));
bwchar = ~imbinarize(rgb2gray(character));

compare = imread(...
	'Working Files/Extracted characters/Page 30, object 310 (Obj4).jpg - 5.png');
bwcompare = imbinarize(rgb2gray(compare));
compareCore = findCore(bwcompare);

[compareDist, compareClosest] = bwdist(compareCore);
[charDist, charClosest] = bwdist(bwchar);

scale = 7.9;
translate = [0.0, 0.0]; % after scaling. 

charPixels = pixelsFromImg(bwchar);
comparePixels = pixelsFromImg(compareCore);

originalPixelDist = distFromCentroid(charPixels);
for i = 1:10
	% transform pixels to the coordinate system of the
	% dictionary character
	transedPixels = transformPixels(...
		charPixels, scale, translate, size(bwchar), size(bwcompare));
	
	[scale1, trans1] = compareImgs(transedPixels, compareClosest);
	
	transedPixels2 = transformPixels(...
		comparePixels, 1/scale, -translate / scale,...
		size(bwcompare), size(bwchar));
	
	[scale2, trans2] = compareImgs(transedPixels2, charClosest);
	
	scale = scale * (scale1 / scale2);
	
	translate = trans1 - trans2*scale;
end

function out = pixelsFromImg(img)
	[r, c] = ind2sub(size(img), find(img));
	out = [r, c];
end

function out = transformPixels(pixels, scale, translate, imgSize, compareSize)
	out = pixels * scale + translate ...
		+ ((compareSize - scale*imgSize) / 2);
end


function [scale, translation] = compareImgs(transformedPixels, compareImgClosest)
	
	translation = [0.0, 0.0];
	closestPixels = zeros(size(transformedPixels));
	
	for j = 1:size(transformedPixels)
		
		pixel = transformedPixels(j, 1:2);
		% find closest white pixel of dictionary character
		roundPixel = round(pixel);
		[r, c] = size(compareImgClosest);
		if roundPixel(1) > r
			roundPixel(1) = r;
		end
		if roundPixel(2) > c
			roundPixel(2) = c;
		end
		if roundPixel(1) < 1
			roundPixel(1) = 1;
		end
		if roundPixel(2) < 1
			roundPixel(2) = 1;
		end
		
		[r, c] = ind2sub(size(compareImgClosest), ...
			compareImgClosest(roundPixel(1), roundPixel(2)));
		nearestVect = [r, c] - pixel;
		
		closestPixels(j, 1:2) = [r, c];
		
		translation = (translation * ((j - 1)/j)) + ...
			(nearestVect * (1 / j));
	end
	scale = (distFromCentroid(closestPixels) ...
		/ distFromCentroid(transformedPixels));
end

function [dist] = distFromCentroid(points) % points = nx2 matrix
	centroid = mean(points);
	dist = mean(sqrt((points(1:end, 1) - centroid(1)).^2 + ...
	(points(1:end, 2) - centroid(2)).^2));
end

function core = findCore(bwimg)
	
	dist = bwdist(bwimg);
	
	[height, ~] = size(bwimg);
	
	[r, c] = ind2sub([3, 3], find([1,1,1; 1,0,1; 1,1,1]));
	subSurrounds = [r, c] - 2;
	surrounds = subSurrounds(:,1) + subSurrounds(:,2)*height;
	
	core = zeros(size(dist), 'logical');
	
	for i = 1:numel(dist)
		
		thisSurrounds = surrounds + i;
		
		core(i) = dist(i) > 3 && sum(dist(thisSurrounds( ...
			thisSurrounds >= 1 & thisSurrounds <= numel(dist))) ...
			<= dist(i)) >= 7;
	end
end

%{
[featuresDict,  validPtsDict] = extractFeatures(...
	compare,  detectSURFFeatures(compare));
[featuresDoc, validPtsDoc] = extractFeatures(...
	grayCharacter, detectSURFFeatures(grayCharacter));

indexPairs = matchFeatures(featuresDict, featuresDoc);

matchedDict = validPtsDict(indexPairs(:,1));
matchedDoc = validPtsDoc(indexPairs(:,2));

figure;
showMatchedFeatures(compare,grayCharacter,matchedDict,matchedDoc);
	
%}