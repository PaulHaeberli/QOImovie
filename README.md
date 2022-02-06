# QOImovie - Using QOI format to encode video

This uses the [QOI image format](https://github.com/phoboslab/qoi) to save sequences of images

Only tested on M1 macOS, but it should work fine on all intel and arm machines.

To make the program qoimdemo:

    % make
    

To convert a series of png files into a QOI movie:

    % ./qoimdemo -toqoim 00.png 01.png 02.png test.qoim

To read images from a QOI movie:

    % ./qoimdemo -topng test.qoim outfamily
    
To print a summary of a QOImovie:
    
    % ./qoimdemo -print test.qoim
    
    
To test everything:

    # make testall
 

Any suggestings welcome.
