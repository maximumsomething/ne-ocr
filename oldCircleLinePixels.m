
function crossings = oldCircleLinePixels()
	
	top = ([1, 4; 2, 4; 3, 4]);
	side = ([2, 2; 3, 3]);
	middle = ([1, 3; 2, 4; 3, 4]);
	
	crossings = cat(3, switchXY(allFlips(middle)),...
		padarray(allFlips(side), 1, 0, 'post'),...
		switchXY(top), switchXY([8, 8] - top));
end

function out = tol7x7Indexes(in)
	%out = zeros(7, 7, 'logical');
	out = sub2ind([7, 7], in(:, 1), in(:, 2));
end

function flipped = allFlips(in)
	flipped = cat(3, in,...
		[8, 0] + [-1, 1] .* in,...
		[0, 8] + [1, -1] .* in,...
		[8, 8] - in); 
end

function bothThings = switchXY(in)
	bothThings = cat(3, in, [in(:,2,:), in(:,1,:)]);
end
