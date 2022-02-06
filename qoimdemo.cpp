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
#include <vector>

// support for canvas data structure

#define SHIFT_R         (0)
#define SHIFT_G         (8)
#define SHIFT_B         (16)
#define SHIFT_A         (24)

#define RVAL(l)         ((int)(((l)>>SHIFT_R)&0xff))
#define GVAL(l)         ((int)(((l)>>SHIFT_G)&0xff))
#define BVAL(l)         ((int)(((l)>>SHIFT_B)&0xff))
#define AVAL(l)         ((int)(((l)>>SHIFT_A)&0xff))

#define CPACK(r, g, b, a)  (((r)<<SHIFT_R) | ((g)<<SHIFT_G) | ((b)<<SHIFT_B) | ((a)<<SHIFT_A))

typedef struct canvas {
    unsigned int *data;
    int sizex, sizey;
} canvas;

canvas *canvas_new(int sizex, int sizey)
{
    canvas *c = (canvas *)malloc(sizeof(canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)malloc(sizex*sizey*sizeof(unsigned int));
    return c;
}

canvas *canvas_new_withdata(int sizex, int sizey, void *data)
{
    canvas *c = (canvas *)malloc(sizeof(canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)data;
    return c;
}

void canvas_free(canvas *c)
{
    if(!c)
        return;
    free(c->data);
    free(c);
}

// support for reading and writing png images for testing

canvas *canvas_frompng(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 0);
    if(!data) {
        fprintf(stderr, "canvas_frompng: error: problem reading %s\n", filename);
        exit(1);
    }
    return canvas_new_withdata(sizex, sizey, data);
}

void canvas_topng(canvas *in, const char *filename)
{
    stbi_write_png(filename, in->sizex, in->sizey, 4, in->data, 4*in->sizex);
}


// 
//  qoim
// 
//    qoim movie;
//
//   write a movie
//        movie.write(const char *outfilename);
//        movie.putframe(canvas *c);
//        movie.putframe(canvas *c);
//        movie.putframe(canvas *c);
//        movie.close();
//
//   read a movie
//        movie.read(const char *infilename);
//        canvas *c;
//        int usec;
//        c = getframe(int frameno, &usec);
//        c = getframe(int frameno, &usec);
//        c = getframe(int frameno, &usec);
//        movie.close();
//
//   get info
//        int moive.getnframes();
//
//   print
//        movie.read(const char *infilename);
//        movie.print(const char *label);
//        movie.close();
//
//   benchmark
//        movie.readbenchmark(const char *filename)
//
//
//    File looks like
//
//    int magic         0x54FE
//    int nframes
//    int duration
//    int sizex
//    int sizey
//    QOI frame
//    QOI frame
//    QOI frame
//    int timeframe0;
//    int offsetframe0;
//    int sizeframe0;
//    int timeframe1;
//    int offsetframe1;
//    int sizeframe1;
//    int timeframe2;
//    int offsetframe2;
//    int sizeframe2;
//
class qoim {
  public:
    FILE *outf;
    FILE *inf;
    int magic;
    int error;
    int offset, sizex, sizey;
    int starttime, duration;
    std::vector<int> frametimes;
    std::vector<int> frameoffsets;
    std::vector<int> framesizes;

    qoim() {
        init();
    }
    void init() {
        outf = 0;
        inf = 0;
        magic = 0x54FE;
        error = 0;
        sizex = -1;
        sizey = -1;
        duration = 0;
        frametimes.clear();
        frameoffsets.clear();
        framesizes.clear();
    }
    void write(const char *filename) {
        init();
        outf = fopen(filename, "wb");
        if(!outf) {
            fprintf(stderr, "qoim: can't open output file [%s]\n", filename);
            error = 1;
            return;
        }
        _writeint(0);   // bad magic for now
        _writeint(0);   // nframes for now
        _writeint(0);   // duration for now
    }
    void read(const char *filename) {
        init();
        inf = fopen(filename, "rb");
        if(!inf) {
            fprintf(stderr, "qoim: can't open input file [%s]\n", filename);
            error = 1;
            return;
        }
        int val = _readint();
        if(val != magic) {
            fprintf(stderr, "qoim: magic: 0x%x bad magic 0x%x\n", magic, val);
            error = 1;
            return;
        }
        int nframes = _readint();
        duration = _readint();
        sizex = _readint();
        sizey = _readint();
        fseek(inf, -(3*nframes)*sizeof(int), SEEK_END);
        for(int i=0; i<nframes; i++) {
            frametimes.push_back(_readint());
            frameoffsets.push_back(_readint());
            framesizes.push_back(_readint());
        }
    }
    void _writeint(int val) {
        int bytes_write = fwrite(&val, 1, sizeof(int), outf);
        if(bytes_write != sizeof(int)) {
            fprintf(stderr, "qoim: _writeint error\n");
            error = 1;
            return;
        }
    }
    int _readint() {
        int val;
        int bytes_read = fread(&val, 1, sizeof(int), inf);
        if(bytes_read != sizeof(int)) {
            fprintf(stderr, "qoim: _readint error\n");
            error = 1;
            return 0;
        }
        return val;
    }
    int getnframes() {
        return frameoffsets.size();
    }
    canvas *getframe(int n, int *usec) {
        if(inf) {
            *usec = frametimes[n];
            return qoireadframe(inf, frameoffsets[n], framesizes[n]);
        } else {
            fprintf(stderr, "qoim: can not getframe from movie being written\n");
            error = 1;
            return 0;
        }
    }
    int getduration() {
        return duration;
    }
    void putframe(canvas *c) {
        if(outf) {
            int curframetime;
            if(getnframes() == 0) {
                sizex = c->sizex;
                sizey = c->sizey;
                _writeint(sizex);
                _writeint(sizey);
                offset = 5*sizeof(int);
                starttime = getusec();
                curframetime = 0;
            } else {
                if((sizex != c->sizex) || (sizey != c->sizey)) {
                    fprintf(stderr, "qoim: frames must be the same size\n");
                    error = 1;
                }
                curframetime = getusec()-starttime;
            }
            int size = qoiwriteframe(c, outf);
            frametimes.push_back(curframetime);
            frameoffsets.push_back(offset);
            framesizes.push_back(size);
            duration = curframetime;
            offset += size;
        } else {
            fprintf(stderr, "qoim: can't put a frame while reading a movie\n");
            error = 1;
        }
    }
    void print(const char *label) {
        fprintf(stderr, "\n");
        fprintf(stderr, "qoim %s:\n", label);
        fprintf(stderr, "    Size: %d x %d\n", sizex, sizey);
        fprintf(stderr, "    N frames: %d\n", getnframes());
        fprintf(stderr, "    Duration: %d usec\n", duration);
        for(int n=0; n<getnframes(); n++)
            fprintf(stderr, "    frame: %d  time: %d offset %d size %d\n", n, frametimes[n], frameoffsets[n], framesizes[n]);
        fprintf(stderr, "\n");
    }
    int close() {
        if(outf) {
            int bytes_write;
            int nframes = getnframes();
            int inittime;
            for(int i=0; i<nframes; i++) {
                _writeint(frametimes[i]);
                _writeint(frameoffsets[i]);
                _writeint(framesizes[i]);
            }
            fseek(outf, 0, SEEK_SET);
            _writeint(magic);
            _writeint(nframes);
            _writeint(duration);
            _writeint(sizex);
            _writeint(sizey);
            fclose(outf);
            outf = 0;
        } else {
            fclose(inf);
            inf = 0;
        }
        if(error)
            return 0;
        else
            return 1;
    }
    void readbenchmark(const char *filename) {
        int t0 = getusec();
        read(filename);
        int nframes = getnframes();
        for(int i=0; i<nframes; i++) {
            int usec;
            canvas *c = getframe(i, &usec);
            canvas_free(c);
        }
        close();
        int t1 = getusec();
        fprintf(stderr, "%d frames in %d usec\n", nframes, t1-t0);
    }
    unsigned int getusec(void)
    {
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        int sec = (int)tv.tv_sec;
        return (1000000*sec)+tv.tv_usec;
    }
    static int qoiwriteframe(canvas *c, FILE *f) {
        qoi_desc desc;
        desc.width = (unsigned int)c->sizex;
        desc.height = (unsigned int)c->sizey;
        desc.channels = 4;
        desc.colorspace = QOI_SRGB;
        int size;
        void *encoded = qoi_encode(c->data, &desc, &size);
        if (!encoded) {
            fprintf(stderr, "goiwriteframe encode error\n");
            exit(1);
        }
        int bytes_write = fwrite(encoded, 1, size, f);
        if(bytes_write != size) {
            fprintf(stderr, "goiwriteframe error\n");
            exit(1);
        }
        QOI_FREE(encoded);
        return size;
    }
    static canvas *qoireadframe(FILE *f, int offset, int size) {
        qoi_desc desc;
        fseek(f, offset, SEEK_SET);
        void *data = QOI_MALLOC(size);
        int bytes_read = fread(data, 1, size, f);
        void *pixels = qoi_decode(data, bytes_read, &desc, 4);
        QOI_FREE(data);

        int channels = desc.channels;
        int sizex = desc.width;
        int sizey = desc.height;
        return canvas_new_withdata(sizex, sizey, pixels);
    }
};

// test program follows

int main(int argc, char **argv) 
{ 
    class qoim movie;
    if(argc<3) {
        fprintf(stderr, "\nusage: qoimdemo -toqoim 00.png 01.png 02.png test.qoim\n\n");
        fprintf(stderr, "usage: qoimdemo -topng test.qoim outfamily\n\n");
        fprintf(stderr, "usage: qoimdemo -print test.qoim\n\n");
        exit(1);
    }
    if(strcmp(argv[1], "-toqoim") == 0) {
        movie.write(argv[argc-1]);
        for(int argp = 2; argp<argc-1; argp++) {
            canvas *c = canvas_frompng(argv[argp]);
            movie.putframe(c);
            canvas_free(c);
        }
        movie.close();
    } else if(strcmp(argv[1], "-topng") == 0) {
        movie.read(argv[2]);
        for(int frameno = 0; frameno<movie.getnframes(); frameno++) {
            char outfname[1024];
            int usec;
            canvas *c = movie.getframe(frameno, &usec);
            sprintf(outfname, "%s%03d.png", argv[3], frameno);
            canvas_topng(c, outfname);
            canvas_free(c);
        }
        movie.close();
    } else if(strcmp(argv[1], "-print") == 0) {
        movie.read(argv[2]);
        movie.print("test");
        movie.close();
    } else {
        fprintf(stderr, "strange option [%s]\n", argv[1]);
        exit(1);
    }
    return 0;
}
