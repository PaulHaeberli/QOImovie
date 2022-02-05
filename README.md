# QOImovie
Using the QOI image format to save sequences of images

Only tested on M1 macOS, but it should work fine on all intel and arm machines.

To make:

    % make
    
The program 

To convert a series of png files into a QOI movie.

    % ./QOImovie -toqoim 00.png 01.png 02.png test.qoim

To read images from a QOI movie

    % ./QOImovie -topng test.qoim outfamily
    
To print a summary of a QOImovie
    
    % ./QOImovie -print test.qoim
    
To test:

    # make test
