function skel = distSkel(bwimg)
	skel = zeros(size(bwimg), 'logical');
	
	[comp, numComp] = bwlabel(~bwimg, 4);
	dist = bwdist(bwimg);
	
	for i = 1:numComp
		thisDist = dist;
		thisDist(comp ~= i) = 0;
		skel = skel | connDistSkel(thisDist);
	end
end

function skel = connDistSkel(dist)
	
	[allR, allC] = ind2sub(size(dist), 1:numel(dist));
	allR = allR(:); allC = allC(:);
	
	% seeds of skeleton
	%{
	skel = (compareNeighbors(dist, [allR-1, allR+1], [allC, allC]) + ...
		compareNeighbors(dist, [allR-1, allR+1], [allC+1, allC-1]) + ...
		compareNeighbors(dist, [allR-1, allR+1], [allC-1, allC+1]) + ...
		compareNeighbors(dist, [allR, allR], [allC+1, allC-1])) ...
		>= 2;
	%}
	skel = opposingNeighbors(dist, allR, allC, false);
	
	% numel(dist) by 8 matrix
	
	[neighbor8Inds, valid8Neighbor] = neighbors8(size(dist), allR, allC);
	
	%imshow(skel);
	%pause on;
	%pause(2);
	
	% grow skeleton
	while true
		endpoints = zeros(size(dist), 'logical');
	endpoints(valid8Neighbor) = skel(valid8Neighbor) ...
		& sum(skel(neighbor8Inds), 2) <= 1;
	endpointList = find(endpoints);
		
	%assert(sum(endpoints(valid8Neighbor)) == numel(endpoints(:)));
		growth = growSkel(skel, dist, endpoints(valid8Neighbor), endpointList,...
			neighbor8Inds, false, 0);
		
		if all(all(skel(growth)))
			break
		else
			skel(growth) = true;
		end
	end
	
	validSkel = opposingNeighbors(dist, allR, allC, true);
	
	oldGrowth = [];
	% connects disconnected components of skeleton
	% improve performance later
	while true
		skel = skel & validSkel;
		
		[comp, numComp] = bwlabel(skel);
		if numComp <= 1
			break
		end
		
		endpoints = zeros(size(dist), 'logical');
		endpoints(valid8Neighbor) = skel(valid8Neighbor) ...
			& sum(skel(neighbor8Inds), 2) <= 1;
		endpointList = find(endpoints);
		
		if numel(endpointList) == 0
			% disp("error growing skeleton")
			break
		end
		
		targets = zeros([numel(endpointList), 1]);
		orderedEndpoints = targets;
		
		lastTarget = 0;
		for i = 1:numComp
			compPoints = find(endpoints & comp == i);
			
			otherComps = false(size(skel));
			otherComps(comp ~= 0 & comp ~= i) = true;
			[~, indMap] = bwdist(otherComps);
			
			pointCount = numel(compPoints);
			targets(lastTarget+1:lastTarget+pointCount) = indMap(compPoints);
			orderedEndpoints(lastTarget+1:lastTarget+pointCount) = compPoints;
			
			lastTarget = lastTarget + pointCount;
		end
		
		[~, sortIndexes] = sort(orderedEndpoints);
		growth = growSkel(skel, dist, endpoints(valid8Neighbor), endpointList,...
			neighbor8Inds, true, 0, targets(sortIndexes));
		
		if numel(growth) == 0 || ...
				(numel(growth) == numel(oldGrowth) && all(growth == oldGrowth))
			break
		else
			skel(growth) = true;
		end
		oldGrowth = growth;
	end
end

% targets = list of closest pixel indices for each endpoint
function growth = growSkel(skel, dist, endpoints, endpointList, neighbor8Inds,...
		doTargeting, repetition, targets, forbiddenGrowth)
	
	% n by 8
	endNeighbors = neighbor8Inds(endpoints, :);
	neighborVals = dist(endNeighbors);
	
	if doTargeting
		neighborVals(skel(endNeighbors)) = 0;
	end
	if repetition > 0
		neighborVals(forbiddenGrowth) = 0;
	end
	
	[maxes, growthPlaces] = max(neighborVals, [], 2);
	growthInds = sub2ind(size(neighborVals), (1:size(neighborVals, 1)).', growthPlaces);
	
	growth = endNeighbors(growthInds);
	if doTargeting
		[pointR, pointC] = ind2sub(size(skel), endpointList);%find(endpoints);
		endpointCoords = [pointR(:), pointC(:)];
		
		[targetR, targetC] = ind2sub(size(skel), targets);
		targetCoords = [targetR(:), targetC(:)] - endpointCoords;
		
		[growthR, growthC] = ind2sub(size(skel), growth);
		
		growth = growth(maxes > 0 & ...
			all(abs(([growthR(:), growthC(:)] - endpointCoords) - ...
			targetCoords ./ max(abs(targetCoords), [], 2)) < 0.75, 2));
		
	else
		growth = growth(maxes > 0);
	end
	
	if doTargeting && numel(growth) == 0
		if repetition >= 4
			return
		elseif repetition > 0
			growthInds = [forbiddenGrowth, growthInds];
		end
		growth = growSkel(skel, dist, endpoints, endpointList, neighbor8Inds,...
		doTargeting, repetition + 1, targets, growthInds);
	end
end

% rsubs and csubs are nx2
function dir = compareNeighbors(dist, rSubs, cSubs, useVals) 
	[inds, valid] = getNeighbors(size(dist), rSubs, cSubs);
	%assert(numel(inds) == numel(dist(valid)) * 2);
	
	if useVals
		dir = zeros(size(dist), 'single');
		dir(valid) = dist(valid)*2 - sum(dist(inds), 2);
	else
		dir = zeros(size(dist), 'logical');
		dir(valid) = all(dist(valid) > dist(inds), 2);
	end
end

function map = opposingNeighbors(dist, allR, allC, useVals)
	neighbors = cat(3, compareNeighbors(dist, [allR-1, allR+1], [allC, allC], useVals), ...
		compareNeighbors(dist, [allR-1, allR+1], [allC+1, allC-1], useVals), ...
		compareNeighbors(dist, [allR-1, allR+1], [allC-1, allC+1], useVals), ...
		compareNeighbors(dist, [allR, allR], [allC+1, allC-1], useVals));
	
	if useVals
		map = max(neighbors, [], 3) >= 0.5;
	else
		map = sum(neighbors, 3) >= 2;
	end
end

% valid is imgSize, but linear
% rSubs, cSubs have n columns that size
% inds is sum(valid) by n
function [inds, valid] = getNeighbors(imgSize, rSubs, cSubs)
	valid = all(rSubs <= imgSize(1) & cSubs <= imgSize(2) & ...
				rSubs >= 1 & cSubs >= 1, 2);
	
	inds = sub2ind(imgSize, rSubs(valid, :), cSubs(valid, :));
end

function [inds, valid] = neighbors8(imgSize, r, c)
	[inds, valid] = getNeighbors(imgSize, ...
		[r-1, r-1, r-1, r,   r,   r+1, r+1, r+1], ...
	    [c-1, c,   c+1, c-1, c+1, c-1, c,   c+1]);
end