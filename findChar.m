function charNames = findChar(imgIn, coresDir)
	
	fprintf('type of input: %s\n', class(imgIn));
	if ischar(imgIn)
		imgIn = imread(imgIn);
	elseif isstring(imgIn)
		imgIn = imread(char(imgIn));
	end
	bwchar = ~imbinarize(rgb2gray(imgIn));
	
	list = dir(coresDir);
	
	imgNames = cell(size(list));
	scores = Inf(size(list));
	
	[charPixels, charSize] = cropPixelsToEdge(pixelsFromImg(bwchar));
	
	parfor i = 1:numel(list)
		filename = list(i).name;
		%filename = 'Page 27, object 277 (Obj4).jpg - 2.png';
		[~, ~, extension] = fileparts(filename);
		if strcmp(extension, '.png')
			compareCore = imread([list(i).folder, '/', filename]);
			
			score = compareChar(bwchar, charPixels, charSize, ...
				compareCore, pixelsFromImg(compareCore));
			
			% infinite score is error signal
			if isinf(score)
				% todo: change to stderr
				disp (['there was an error processing: ' filename])
			end
			
			imgNames{i} = filename;
			scores(i) = score;
		end
	end
	
	validIndexes = scores ~= Inf();
	
	imgNames = imgNames(validIndexes);
	[~, sortIndexes] = sort(scores(validIndexes));
	imgNames = imgNames(sortIndexes);
	
	if numel(imgNames) > 60
		imgNames = imgNames(1:60);
	end
	charNames = strjoin(imgNames, '\n');
end

function out = pixelsFromImg(img)
	[r, c] = ind2sub(size(img), find(img));
	out = [r, c];
end

function [cropped, imgSize] = cropPixelsToEdge(pixels)
	
	if isinteger(pixels); maxBound = intmax(class(pixels));
	else; maxBound = Inf();
	end
	% top, left
	topBound = [maxBound, maxBound];
	% bottom, right
	bottomBound = zeros([1 2], class(pixels));
	
	for i = 1:size(pixels)
		
		low = pixels(i, 1:2) < topBound;
		topBound(low) = pixels(i, low);
			
		high = pixels(i, 1:2) > bottomBound;
		bottomBound(high) = pixels(i, high);
	end
	cropped = pixels - (topBound - 1);
	imgSize = bottomBound - topBound;
	
	assert(all(cropped(:) >= 0));
end