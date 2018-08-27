% neighborCount = number of contiguous neighbors that have to be 1 for a pixel to be
% set to 0
function skel = thinSkel(bwimg)
	
	skel = bwimg;
	
	[height, ~] = size(bwimg);
	surrounds = eightConnMat(height);
	
	
	changeCount = 1;
	while changeCount > 0
		changeCount = 0;
		
		for i = 1:numel(bwimg)
			thisSurrounds = surrounds + i;
			
			
			if bwimg(i)
				neighbors = bwimg(thisSurrounds( ...
					thisSurrounds >= 1 & thisSurrounds <= numel(bwimg)));
				
				% find greatest number of consecutive neighbors
				consecutive = 0;
				maxConsecutive = 0;
				firstCons = 0;
				for j = 1:numel(neighbors)
					
					if neighbors(j)
						consecutive = consecutive + 1;
					else
						if consecutive == j - 1
							firstCons = consecutive;
						elseif j == numel(neighbors)
							consecutive = consecutive + firstCons;
						end
						
						maxConsecutive = max([consecutive, maxConsecutive]);
						consecutive = 0;
					end
				end
				
				if maxConsecutive >= 3 && maxConsecutive <= 6
					skel(i) = 0;
					changeCount = changeCount + 1;
				end
			end
		end
		bwimg = skel;matchFeatures;
	end
end



function surrounds = eightConnMat(height)
	%[r, c] = ind2sub([3, 3], find([1,1,1; 1,0,1; 1,1,1]));
	%subSurrounds = [r, c] - 2;
	subSurrounds = [-1 -1; 0 -1; 1 -1; 1 0; 1 1; 0 1; -1 1; -1 0];
	surrounds = subSurrounds(:,1) + subSurrounds(:,2)*height;
end
