coresDir = 'Working Files/Extracted characters/cores';

imgName = 'Working Files/Zuozhai book 1/11.jpeg';
%character = imcrop(imread(imgName));
bwchar = ~imbinarize(rgb2gray(character));

list = dir(coresDir);

imgNames = {};
scores = [];

for i = 1:size(list)
	filename = list(i).name;
	%filename = 'Page 27, object 277 (Obj4).jpg - 2.png';
	[~, ~, extension] = fileparts(filename);
	if strcmp(extension, '.png')
		compareCore = imread([list(i).folder, '/', filename]);
		score = compareChar(bwchar, compareCore);
		
		imgNames{end+1} = filename;
		scores(end+1) = score;
	end
end

[scores, indexes] = sort(scores);
imgNames = imgNames(indexes);

for i = 1:numel(imgNames)
	fprintf('%s\n', imgNames{i});
end


