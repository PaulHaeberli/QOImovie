# QOImovie - Using QOI format to encode video

This uses the [QOI image format](https://github.com/phoboslab/qoi) to save sequences of images

Tested on M1 macOS, but it should work fine most anywhere.


write a movie

    qom *qm = qom_open( "out.qom", "w");
        gfx_canvas *c = gfx_canvas_new(400, 300);
        qom_putframe(qm, c, usec);
    qom_close(qm);

write a movie in real time

    qom *qm = qom_open( "out.qom", "w");
    gfx_canvas *c = gfx_canvas_new(400, 300);
        qom_putframenow(qm, c);
    qom_close(qm);

read a movie

    qom *qm = qom_open( "out.qom", "r");
    for(int frameno = 0; frameno<qom_getnframes(qm); frameno++) {
        double usec;
        gfx_canvas *c = gfx_getframe(qm, frameno, &usec);
        gfx_canvas_free(c);
    }
    qom_close(qm);

print

    qom *qm = qom_open( "out.qom", "r");
    qom_print(qm, "test");
    qom_close(qm);

To make the program qomutil:

    % make
    

To convert a series of png files into a QOI movie:

    % ./qomutil -toqom 00.png 01.png 02.png test.qom

To read images from a QOI movie:

    % ./qomutil -topng test.qom outfamily
    
To print a summary of a QOImovie:
    
    % ./qomutil -print test.qom
    
To concatenate several .qom files:
    
    % ./qomcat in1.qom in2.qom in3.qom out.qom
    
  
To test everything:

    # make test
 
I'm thinking this could be a nice way to store animated icons and UI elements.

Adding multi-resolution support so the movie could be stored in any number of different sizes.

File format may be changing (a tiny bit) in the near future.

All suggestions welcome.

