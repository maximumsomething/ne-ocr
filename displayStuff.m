[r, c] = size(bwcompare);
img = zeros(r, c, 3, 'uint8');

img(:,:,1) = uint8((~bwcompare) * 255);

img(:,:,2) = 255;
for k = 1:size(transedPixels)
	%ind = sub2ind([r, c, 3], round(transedPixels(k, 1)), round(transedPixels(k, 2)), 2);
	%img(ind) = 0;
	
	img = insertShape(img, 'FilledCircle', ...
		[round(transedPixels(k, 2)), round(transedPixels(k, 1)), 6],...
		'Color', 'blue', 'Opacity', 0.3);
end

imshow(img);