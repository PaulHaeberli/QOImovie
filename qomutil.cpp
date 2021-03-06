/*

QOImovie - Using QOI format to encode video

Paul Haeberli - https://twitter.com/GraficaObscura

-- LICENSE: The MIT License(MIT)

Copyright(c) 2022 Paul Haeberli

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


-- About

Builds on top of Dominic Szablewski's QOI libary to store sequences of images.

*/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"
#define QOM_IMPLEMENTATION
#include "qom.h"
#include <vector>

// support for canvas data structure

// 
//  qom
// 
//    qom movie;
//
//   write a movie
//        movie.open(const char *outfilename, "w");
//        movie.putframe(gfx_canvas *c, double usec);
//        movie.putframe(gfx_canvas *c, double usec);
//        movie.putframe(gfx_canvas *c, double usec);
//        movie.close();
//
//   read a movie
//        movie.open(const char *infilename, "r");
//        gfx_canvas *c;
//        double usec;
//        c = getframe(int frameno, &usec);
//        c = getframe(int frameno, &usec);
//        c = getframe(int frameno, &usec);
//        movie.close();
//
//   get info
//        int moive.getnframes();
//
//   print
//        movie.open(const char *infilename, "r");
//        movie.print(const char *label);
//        movie.close();
//
//   benchmark
//        movie.readbenchmark(const char *filename)
//
//
//    File looks like
//
//    int magic;                        header
//    int nframes;
//    int duration;
//    int sizex;
//    int sizey;
//    int default_starttime;
//    int default_startdir;
//    int default_leftbounce;
//    int default_rightbounce;
//    QOI frame1
//    QOI frame2
//    QOI frame3
//    int time;                 frameinfo1
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//    int time;                 frameinfo1
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//    int time;                 frameinfo2
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//
class QOIm {
  public:
    qom *qm;

    QOIm() {
        qm = 0;
    }
    QOIm(const char *filename, const char *mode) {
        open(filename, mode);
    }
    ~QOIm() {
        if(qm)
            qom_close(qm);
    }
    void open(const char *filename, const char *mode) {
        qm = qom_open(filename, mode);
        if(!qm)
            exit(1);
    }
    int getnframes() {
        return qom_getnframes(qm);
    }
    gfx_canvas *getframe(int frameno, double *usec) {
        return qom_getframe(qm, frameno, usec);
    }
    int getduration() {
        return qom_getduration(qm);
    }
    void putframe(gfx_canvas *c, double usec) {
        qom_putframe(qm, c, usec);
    }
    void putframenow(gfx_canvas *c) {
        qom_putframenow(qm, c);
    }
    void print(const char *label) {
        qom_print(qm, label);
    }
    int close() {
        int ret = qom_close(qm);
        qm = 0;
        return ret;
    }
    void readbenchmark(const char *filename) {
        qom_readbenchmark(filename);
    }
};

// test program follows

gfx_canvas *gfx_canvas_frompng(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 0);
    if(!data) {
        fprintf(stderr, "gfx_canvas_frompng: error: problem reading %s\n", filename);
        exit(1);
    }
    return gfx_canvas_new_withdata(sizex, sizey, data);
}

void gfx_canvas_topng(gfx_canvas *in, const char *filename)
{
    stbi_write_png(filename, in->sizex, in->sizey, 4, in->data, 4*in->sizex);
}

#define DEFAULT_FRAMETIME       ((1000*1000)/30.0)

int main(int argc, char **argv) 
{ 
    class QOIm movie;
    if(argc<3) {
        fprintf(stderr, "\nusage: qomutil -toqom 00.png 01.png 02.png test.qom\n\n");
        fprintf(stderr, "usage: qomutil -topng test.qom tmp/OUT%%04d.png\n\n");
        fprintf(stderr, "usage: qomutil -print test.qom\n\n");
        exit(1);
    }
    if(strcmp(argv[1], "-toqom") == 0) {
        double usec = 0;
        movie.open(argv[argc-1], "w");
        for(int argp = 2; argp<argc-1; argp++) {
            gfx_canvas *c = gfx_canvas_frompng(argv[argp]);
            movie.putframe(c, usec);
            gfx_canvas_free(c);
            usec += DEFAULT_FRAMETIME;
        }
        movie.close();
    } else if(strcmp(argv[1], "-topng") == 0) {
        movie.open(argv[2], "r");
        for(int frameno = 0; frameno<movie.getnframes(); frameno++) {
            char outfname[1024];
            double usec;
            gfx_canvas *c = movie.getframe(frameno, &usec);
            sprintf(outfname, "%s%04d.png", argv[3], frameno);
            gfx_canvas_topng(c, outfname);
            gfx_canvas_free(c);
        }
        movie.close();
    } else if(strcmp(argv[1], "-print") == 0) {
        movie.open(argv[2], "r");
        movie.print("test");
        movie.close();
    } else if(strcmp(argv[1], "-benchmark") == 0) {
        movie.readbenchmark(argv[2]);
    } else {
        fprintf(stderr, "strange option [%s]\n", argv[1]);
        exit(1);
    }
    return 0;
}
