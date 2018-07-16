[r, c] = size(bwcompare);
img = zeros(r, c, 3, 'uint8');

img(:,:,1) = uint8((~bwcompare) * 255);

img(:,:,2) = 255;
for k = 1:size(transformedPixels)
	ind = sub2ind([r, c, 3], round(transformedPixels(k, 1)), round(transformedPixels(k, 2)), 2);
	img(ind) = 0;
end

imshow(img);