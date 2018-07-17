function core = findCore(bwimg)
	
	dist = bwdist(bwimg);
	
	[height, ~] = size(bwimg);
	
	[r, c] = ind2sub([3, 3], find([1,1,1; 1,0,1; 1,1,1]));
	subSurrounds = [r, c] - 2;
	surrounds = subSurrounds(:,1) + subSurrounds(:,2)*height;
	
	core = zeros(size(dist), 'logical');
	
	for i = 1:numel(dist)
		
		thisSurrounds = surrounds + i;
		
		core(i) = dist(i) > 3 && sum(dist(thisSurrounds( ...
			thisSurrounds >= 1 & thisSurrounds <= numel(dist))) ...
			<= dist(i)) >= 7;
	end
end