function charNames = findChar(imgIn, coresDir)
	
	%fprintf('type of input: %s\n', class(imgIn));
	if ischar(imgIn)
		imgIn = imread(imgIn);
	elseif isstring(imgIn)
		imgIn = imread(char(imgIn));
	end
	grayChar = imgIn(:, :, 2);
	bounds = getBounds(~imbinarize(grayChar));
	
	[coords, lengths] = circleLinePixels(3);
	charMap = angleMap(grayChar, coords, lengths);
	
	% cell arrays
	filenames = loadFile(strcat(coresDir, '/ordered_filenames.dat'));
	compareMaps = loadFile(strcat(coresDir, '/angle_maps.dat'));
	
	imgNames = cell(size(filenames));
	scores = zeros(size(filenames));
	
	distMap = zeros(17, 'logical');
	distMap(9, 9) = true;
	distMap = 8 - bwdist(distMap);
	distMap(distMap < 0) = 0;
	[x, y] = meshgrid(-8:8);
	x = int32(x); y = int32(y);
	distMapInds = y + x*size(grayChar, 2);
	
	for i = 1:numel(filenames)
		filename = filenames{i};
		compareCore = imread(char(strcat(coresDir, '/', filename)));
		
		score = compareChar(grayChar, charMap, bounds, ...
			compareCore, compareMaps{i}, distMap, distMapInds);
		
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

% [top, left; height, width]
function bounds = getBounds(bwimg)
	[r, c] = find(bwimg);
	bounds = [min(r), min(c); ...
		max(r) - min(r), max(c) - min(c)];
end

function deserialized = loadFile(name)
	file = fopen(name);
	deserialized = hlp_deserialize(fread(file, 'uint8=>uint8'));
	fclose(file);
end