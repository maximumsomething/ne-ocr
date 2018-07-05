imgName = 'Zuozhai book 1/10.jpeg';

%character = imcrop(imread(imgName));
%bwchar = imbinarize(rgb2gray(character));

compare = imread(...
	'Extracted characters/Page 30, object 310 (Obj4).jpg - 2.png');
%bwcompare = ~imbinarize(rgb2gray(compare));

[dist, closest] = bwdist(bwcompare);

scale = 7.9;
translate = [0, 0]; % after scaling. 

numPoints = 0;
meanTranslation = [0, 0];

for i = 1:numel(bwchar)
	if ~bwchar(i) % if pixel is black
		
		 % transform current pixel to the coordinate system of the 
		 % dictionary character
		[y, x] = ind2sub(size(bwchar), i);
		pixel = [y, x] * scale;
		% find closest white pixel of dictionary character
		roundPixel = round(pixel);
		if
		[y, x] = ind2sub(size(bwcompare), closest(roundPixel(1), roundPixel(2)));
		
		nearestVect = [y, x] - pixel;
	
		numPoints  = numPoints + 1;
		meanTranslation = (meanTranslation * ((numPoints - 1)/numPoints)) + ...
			(nearestVect * (1 / numPoints));
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