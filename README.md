# QOImovie - Using QOI format to encode video

This uses the [QOI image format](https://github.com/phoboslab/qoi) to save sequences of images

Tested on M1 macOS, but it should work fine on all Intel and Raspberry PI machines or any other little-endian achitectures.

If you have a good solution for handling big-endian machines, please let me know.

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
 
I'm thinking this could be a nice way to store animated icons and UI elements.

Now considering adding multi-resolution support so the movie could be stored in any number of different sizes.

WARNING: file format may be changing (a timy bit) in the near future.

All suggestions welcome.

