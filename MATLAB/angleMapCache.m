function angleMapCache()
	
	[coords, lengths] = circleLinePixels(7);
	coresDir = 'Working Files/Extracted characters/cores';
	list = dir(coresDir);
	
	filenames = {};
	angleMaps = {};
	allSkels = {};
	for i = 1:numel(list)
		filename = list(i).name;
		[~, ~, extension] = fileparts(filename);
		if strcmp(extension, '.png')
			character = imread([list(i).folder, '/', filename]);
			allSkels{end+1} = character;
			filenames{end+1} = filename;
			angleMaps{end+1} = angleMap(character, coords, lengths);
		end
	end
	
	save([coresDir '/coresCache.mat'], 'filenames', 'angleMaps', 'allSkels');
	
	%writeThing([coresDir '/ordered_filenames.dat'], filenames);
	%writeThing([coresDir '/angle_maps.dat'], angleMaps);
end

function writeThing(name, data)
	file = fopen(name, 'w');
	fwrite(file, hlp_serialize(data));
	fclose(file);
end