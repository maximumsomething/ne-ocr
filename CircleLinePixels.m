classdef CircleLinePixels
	properties
		Coords % nx2 matrix of y, x pixel points.
		
		% cell array of doubles. Each corresponds to one coord. 
		% Is how far line travels through that pixel.
		Lengths
	end
	
	methods(Static)
		% returns array of CircleLinePixels objects
		function crossings = get(radius)
			imgSize = radius*2 + 1;
			center = [radius+1, radius+1];
			
			img = insertShape(zeros(imgSize, imgSize, 'uint8'),...
				'Circle', [center, radius],...
				'Color', 'white', 'SmoothEdges', false);
			img = img(:,:,1) > 100;
			%imshow(imresize(img, 10, 'nearest'))
			
			[r, c] = find(img);
			
			crossings = cell([1, numel(r)]);
			
			for i = 1:numel(r)
				target = [r(i), c(i)] - center;
				
				circlePart = CircleLinePixels;
				
				% in this calculation, center is 0, 0 instead of radius+1,
				% radius+1
				currentCoord = [0, 0];
				while true
					% whole numbers represent the centers of pixels.
					% therefore, 0.5 is the edge of a pixel.
					nextPixel = fix(currentCoord + 0.5*sign(target))...
						+ 0.5*sign(target);
					nextPixel = nextPixel + (nextPixel == currentCoord);
					
					nextDist = (nextPixel - currentCoord);
					
					% possibleDist(1) is the movement in x if we move
					% nextDist(1) in y, and vice versa.
					possibleDist = nextDist .* [target(2)/target(1), target(1)/target(2)];
					
					if abs(nextDist(1)) < abs(possibleDist(2))
						nextCoord = currentCoord + [nextDist(1), possibleDist(1)];
					else
						nextCoord = currentCoord + [possibleDist(2), nextDist(2)];
					end
					
					if all(abs(nextCoord) <= abs(target))
		
						circlePart.Coords(end+1,:) = roundCoord(nextCoord + center);
						circlePart.Lengths(end+1) = ...
							sqrt(sum((nextCoord - currentCoord) .^ 2));
					else
						break
					end
					currentCoord = nextCoord;
				end
				
				crossings{i} = circlePart;
			end
		end
	end
end

% rounds .5 towards zero
function rounded = roundCoord(coord)
	rounded = round(coord);
	rounded = rounded - sign(coord) .* (abs(rounded - coord) >= 0.5);
end
