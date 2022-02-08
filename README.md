# QOImovie - Using QOI format to encode video

This uses the [QOI image format](https://github.com/phoboslab/qoi) to save sequences of images

Tested on M1 macOS, but it should work fine most anywhere.


write a movie

   qoim *qm = qoim_open( "out.qoim", "w");
       qoim_canvas *c = qoim_canvas_new(400, 300);
      qoim_putframe(qm, c, usec);
   qoim_close(qm);

write a movie in real time

    qoim *qm = qoim_open( "out.qoim", "w");
    qoim_canvas *c = qoim_canvas_new(400, 300);
        qoim_putframenow(qm, c);
    qoim_close(qm);

read a movie

    qoim *qm = qoim_open( "out.qoim", "r");
    for(int frameno = 0; frameno<qoim_getnframes(qm); frameno++) {
        int usec;
        qoim_canvas *c = qoim_getframe(qm, frameno, &usec);
        qoim_canvas_free(c);
    }
    qoim_close(qm);

print

    qoim *qm = qoim_open( "out.qoim", "r");
    qoim_print(qm, "test");
    qoim_close(qm);

To make the program qoimutil:

    % make
    

To convert a series of png files into a QOI movie:

    % ./qoimutil -toqoim 00.png 01.png 02.png test.qoim

To read images from a QOI movie:

    % ./qoimutil -topng test.qoim outfamily
    
To print a summary of a QOImovie:
    
    % ./qoimutil -print test.qoim
    
To concatenate several .qoim files:
    
    % ./qoimcat in1.qoim in2.qoim in3.qoim out.qoim
    
  
To test everything:

    # make testall
 
I'm thinking this could be a nice way to store animated icons and UI elements.

Adding multi-resolution support so the movie could be stored in any number of different sizes.

File format may be changing (a tiny bit) in the near future.

All suggestions welcome.

