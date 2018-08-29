function angleMapCache()
	
	[coords, lengths] = circleLinePixels(7);
	coresDir = 'Working Files/Extracted characters/cores';
	list = dir(coresDir);
	
	filenames = {};
	maps = {};
	for i = 1:numel(list)
		filename = list(i).name;
		[~, ~, extension] = fileparts(filename);
		if strcmp(extension, '.png')
			character = imread([list(i).folder, '/', filename]);
			filenames{end+1} = filename;
			maps{end+1} = angleMap(character, coords, lengths);
		end
	end
	
	writeThing([coresDir '/ordered_filenames.dat'], filenames);
	writeThing([coresDir '/angle_maps.dat'], maps);
end

function writeThing(name, data)
	file = fopen(name, 'w');
	fwrite(file, hlp_serialize(data));
	fclose(file);
end