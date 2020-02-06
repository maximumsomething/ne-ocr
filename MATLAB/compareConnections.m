
function score = compareConnections(in1, in2)
	
	%{ 
	1. For each intersection and endpoint in in1, create a similarity score between it
	and all intersections in in2.
	2. For each connection in in1, match it with connections in in2 by
	looking for the one with the most similar intersections.
	%}
	
end


% with nx3 matrices of connections to other points
% col 1 is index of other intersection
% col 2 is the length
% col 3 is the angle
function list = getIntersectionList(connectionList)
	
	list = {};
	
	[numConn, ~] = size(connectionList);
	for i = 1:numConn
		if i == rem(i,2)
			
		end
		
		
		list{connectionList(i, 3)}(end+1) = ...
			connectionList(i, 1:2)
		
end