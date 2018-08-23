coresDir = 'Working Files/Extracted characters/cores';

imgName = 'Working Files/Zuozhai book 1/11.jpeg';
%character = imcrop(imread(imgName));

disp(findChar(character, 'Working Files/Extracted characters/cores'));
