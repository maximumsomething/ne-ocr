function [imgOut] = colorBinarize(imgIn)
	imgOut = any(imbinarize(imgIn), 3);
end

