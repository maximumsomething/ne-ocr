inDir = 'Extracted characters';
outDir = 'Extracted characters/cores';

%doDir('Extracted characters', 'Extracted characters/cores', 1);
doDir('Extracted characters/Alternates', 'Extracted characters/cores/Alternates/', 4);

function doDir(inDir, outDir, scale) 
	mkdir(outDir);
	list = dir(inDir);

	for i = 1:size(list)
		filename = list(i).name;
		[~, ~, extension] = fileparts(filename);
		if strcmp(extension, '.png')
			img = imread([inDir, '/', filename]);

			core = scaleSkel(img, scale, 1, 16);
			if size(core) > 0
				imwrite(core, [outDir, '/', filename], 'png');
			else
				fprintf('rejected: %s\n', filename);
			end
		end
	end
end