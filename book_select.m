imgName = 'Working Files/Zuozhai book 1/11.jpeg';

%character = imcrop(imread(imgName));
bwchar = imbinarize(rgb2gray(character));

compare = imread(...
	'Working Files/Extracted characters/Page 30, object 310 (Obj4).jpg - 5.png');
bwcompare = ~imbinarize(rgb2gray(compare));

[dist, closest] = bwdist(bwcompare);

scale = 7.9;
translate = [0.0, 0.0]; % after scaling. 

meanTranslation = [0.0, 0.0]; % [y, x] or [r, c]

[r, c] = ind2sub(size(bwchar), find(~bwchar)); % black pixels
characterPixels = [r, c];

originalPixelDist = distFromCentroid(characterPixels);
for i = 1:10
	% transform pixels to the coordinate system of the
	% dictionary character
	transformedPixels = characterPixels * scale + translate ...
		+ ((size(bwcompare) - scale*size(bwchar)) / 2);
	
	closestPixels = zeros(size(transformedPixels));
	
	for j = 1:size(transformedPixels)
		
		pixel = transformedPixels(j, 1:2);
		% find closest white pixel of dictionary character
		roundPixel = round(pixel);
		[r, c] = size(bwcompare);
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
		
		[r, c] = ind2sub(size(bwcompare), closest(roundPixel(1), roundPixel(2)));
		nearestVect = [r, c] - pixel;
		
		closestPixels(j, 1:2) = [r, c];
		
		meanTranslation = (meanTranslation * ((j - 1)/j)) + ...
			(nearestVect * (1 / j));
	end
	scale = scale * (distFromCentroid(closestPixels) ...
		/ distFromCentroid(transformedPixels));
	
	translate = translate + meanTranslation;
end

function [dist] = distFromCentroid(points) % points = nx2 matrix
	centroid = mean(points);
	dist = mean(sqrt((points(1:end, 1) - centroid(1)).^2 + ...
	(points(1:end, 2) - centroid(2)).^2));
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