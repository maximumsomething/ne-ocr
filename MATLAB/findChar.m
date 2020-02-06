function charNames = findChar(imgIn, coresDir, inIsSkel)
	
	%fprintf('type of input: %s\n', class(imgIn));
	if ischar(imgIn)
		imgIn = imread(imgIn);
	elseif isstring(imgIn)
		imgIn = imread(char(imgIn));
	end
	if inIsSkel
		charSkel = imgIn;
	else
		charSkel = scaleSkel(imgIn, 12, 1/3, 0);
	end
	
	[coords, lengths] = circleLinePixels(7);
	charMap = angleMap(charSkel, coords, lengths);
	
	charIndMap = zeros(size(charSkel), 'uint32');
	charIndMap(charSkel) = 1:sum(charSkel(:));
	
	% cell arrays
	%filenames = loadFile(strcat(coresDir, '/ordered_filenames.dat'));
	%angleMaps = loadFile(strcat(coresDir, '/angle_maps.dat'));
	
	load(char(strcat(coresDir, '/coresCache.mat')), 'filenames', 'angleMaps', 'allSkels');
	
	imgNames = cell(size(filenames));
	scores = zeros(size(filenames));

	for i = 1:numel(filenames)
		filename = filenames{i};
		%compareCore = imread(char(strcat(coresDir, '/', filename)));
		
		score = compareChar(charSkel, charMap, charIndMap, allSkels{i}, angleMaps{i});
		
		% zero score is error signal
		if score <= 0
			% todo: change to stderr
			disp (strcat('there was an error processing: ', filename))
		end
		
		imgNames{i} = filename;
		scores(i) = score;
		
	end
	
	validIndexes = scores ~= Inf();
	
	imgNames = imgNames(validIndexes);
	[~, sortIndexes] = sort(scores(validIndexes));
	imgNames = imgNames(sortIndexes);
	
	if numel(imgNames) > 100
		imgNames = imgNames(end-100:end);
	end
	imgNames = flip(imgNames);
	charNames = strjoin(imgNames, '\n');
end
%{
% [top, left; height, width]
function bounds = getBounds(bwimg)
	[r, c] = find(bwimg);
	bounds = [min(r), min(c); ...
		max(r) - min(r), max(c) - min(c)];
end
%}
function deserialized = loadFile(name)
	file = fopen(name);
	deserialized = hlp_deserialize(fread(file, 'uint8=>uint8'));
	fclose(file);
end