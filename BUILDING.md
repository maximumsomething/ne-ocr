This program has three components: the OCR program in C++, the dictionary parser in MATLAB, and the GUI in Java.

The C++ portion is currently setup using XCode but is simple enough to be easily adapted to another build system (it only requires the OpenCV library). It builds two executables: `connection_compare` (from all c++ files except `seperateAlternates.cpp`), and `seperateAlternates` (from `seperateAlternates.cpp`). The former is called by the Java program and the latter is used in the dictionary parsing process.

To run the dictionary parser, first extract all the pages of the dictionary with names of the form `Page XXX.jpg`. Then, set Matlab's working directory to the MATLAB directory of this project, set the `inputFolder` parameter at the top of `dict_extractor.m` to the page images, and run that file. For some documents, alternates may be helpful. To extract these, first set the input of and run `alternatesExtractor.m` as before, then set the sepAltExec variable at the top of `seperateAlternates.sh` to the built `seperateAlternates` executable and run that script. 

Finally, run coreCache.m to generate the skeletons.

If any of the Matlab scripts complain about a directory not existing, you may need to create it.

The Java GUI is set up to be built with IntelliJ or it can be built with another build system (it requires the JavaFX library). It expects the `Extracted Characters` directory and the connection_compare executable to be in the same directory as its JAR, or in a directory passed as an argument.
