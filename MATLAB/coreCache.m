inDir = 'Working Files/Extracted characters';
outDir = 'Working Files/Extracted characters/cores';

mkdir(outDir);
list = dir(inDir);

for i = 1:size(list)
	filename = list(i).name;
	[~, ~, extension] = fileparts(filename);
	if strcmp(extension, '.png')
		img = imread([inDir, '/', filename]);
		
		core = scaleSkel(img, 2, 1, 16);
		if size(core) > 0
			imwrite(core, [outDir, '/', filename], 'png');
		else
			fprintf('rejected: %s\n', filename);
		end
	end
end