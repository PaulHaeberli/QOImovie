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

Builds on top of Dominic Szablewski's QOI libary to store sequnces of images.

-- Synopsis

// Define `QOIM_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define QOIM_IMPLEMENTATION
#include "qoim.h"

//  qoim
// 
//
//  write a movie
//
//    qoim *qm = qoim_new();
//    qoim_write(qm, "out.qoim");
//        qoim_canvas *c = qoim_canvas_new(400, 300);
//        qoim_putframe(qm, c);
//    qoim_close(qm);
//    qoim_free(qm);
//
//  read a movie
//
//    qoim *qm = qoim_new();
//    qoim_read(qm, "in.qoim");
//    for(int frameno = 0; frameno<qoim_getnframes(qm); frameno++) {
//        int usec;
//        qoim_canvas *c = qoim_getframe(qm, frameno, &usec);
//        qoim_canvas_free(c);
//    }
//    qoim_close(qm);
//    qoim_free(qm);
//
//  print
//
//    qoim *qm = qoim_new();
//    qoim_read(qm, argv[2]);
//    qoim_print(qm, "test");
//    qoim_close(qm);
//    qoim_free(qm);
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

*/

/* -----------------------------------------------------------------------------
Header - Public functions */

#ifndef QOIM_H
#define QOIM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qoim_canvas {
    unsigned int *data;
    int sizex, sizey;
} qoim_canvas;

typedef struct _qoim_ilist {   /* internal */
    int len;
    int space;
    int *data;
} _qoim_ilist;

typedef struct qoim {
    FILE *outf;
    FILE *inf;
    int magic;
    int error;
    int offset, sizex, sizey;
    int starttime, duration;
    _qoim_ilist *frametimes;
    _qoim_ilist *frameoffsets;
    _qoim_ilist *framesizes;
} qoim;

qoim_canvas *qoim_canvas_new(int sizex, int sizey);
qoim_canvas *qoim_canvas_new_withdata(int sizex, int sizey, void *data);
void qoim_canvas_free(qoim_canvas *c);

qoim *qoim_new();
void qoim_free(qoim *qm);

void qoim_write(qoim *qm, const char *filename);
void qoim_putframe(qoim *qm, qoim_canvas *c);

void qoim_read(qoim *qm, const char *filename);
qoim_canvas *qoim_getframe(qoim *qm, int n, int *usec);
int qoim_getduration(qoim *qm);

int qoim_close(qoim *qm);

int qoim_getnframes(qoim *qm);
void qoim_print(qoim *qm, const char *label);
void qoim_readbenchmark(const char *filename);

#ifdef __cplusplus
}
#endif
#endif /* QOI_H */


/* -----------------------------------------------------------------------------
Implementation */

#ifdef QOIM_IMPLEMENTATION
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <sys/time.h>

/* support for canvas data structure */

qoim_canvas *qoim_canvas_new(int sizex, int sizey)
{
    qoim_canvas *c = (qoim_canvas *)malloc(sizeof(qoim_canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)malloc(sizex*sizey*sizeof(unsigned int));
    return c;
}

qoim_canvas *qoim_canvas_new_withdata(int sizex, int sizey, void *data)
{
    qoim_canvas *c = (qoim_canvas *)malloc(sizeof(qoim_canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)data;
    return c;
}

void qoim_canvas_free(qoim_canvas *c)
{
    if(!c)
        return;
    free(c->data);
    free(c);
}

/* internal utilities */

static unsigned int _qoim_getusec(void)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    int sec = (int)tv.tv_sec;
    return (1000000*sec)+tv.tv_usec;
}

static int _qoim_writeframe(qoim_canvas *c, FILE *f) {
    qoi_desc desc;
    desc.width = (unsigned int)c->sizex;
    desc.height = (unsigned int)c->sizey;
    desc.channels = 4;
    desc.colorspace = QOI_SRGB;
    int size;
    void *encoded = qoi_encode(c->data, &desc, &size);
    if (!encoded) {
        fclose(f);
        return 0;
    }
    int bytes_write = fwrite(encoded, 1, size, f);
    if(bytes_write != size) {
        fprintf(stderr, "goiwriteframe error\n");
        exit(1);
    }
    QOI_FREE(encoded);
    return size;
}

static qoim_canvas *_qoim_readframe(FILE *f, int offset, int size) {
    qoi_desc desc;
    fseek(f, offset, SEEK_SET);
    void *data = QOI_MALLOC(size);
    int bytes_read = fread(data, 1, size, f);
    void *pixels = qoi_decode(data, bytes_read, &desc, 4);
    QOI_FREE(data);

    int channels = desc.channels;
    int sizex = desc.width;
    int sizey = desc.height;
    return qoim_canvas_new_withdata(sizex, sizey, pixels);
}

/* support for _qoim_ilist */

static _qoim_ilist *_qoim_ilistnew(void)
{
    _qoim_ilist *il;

    il = (_qoim_ilist *)malloc(sizeof(_qoim_ilist));
    il->space = 0;
    il->len = 0;
    il->data = 0;
    return il;
}

static void _qoim_ilistfree(_qoim_ilist *il)
{
    if(!il)
        return;
    if(il->data)
        free(il->data);
    free(il);
}

static int _qoim_ilistsize(_qoim_ilist *il)
{
    return il->len;
}

static void _qoim_ilistclear(_qoim_ilist *il)
{
    il->len = 0;
}

static void _qoim_ilistadd(_qoim_ilist *il, int val)
{
    int pos;

    pos = il->len;
    il->len = pos+1;
    if(pos >= il->space) {
        if(il->space == 0) {
            il->space = 30;
            il->data = (int *)malloc(il->space*sizeof(int));
        } else {
            il->space = ((3*il->space)/2) + 1;
            il->data = (int *)realloc(il->data, il->space*sizeof(int));
        }
    }
    il->data[pos] = val;
}

static int _qoim_ilistget(_qoim_ilist *il, int index)
{
    if((index<0) || (index>=il->len)) {
        fprintf(stderr, "_qoim_ilistget: index out of range\n");
        assert(0);
    }
    return il->data[index];
}

static void _qoim_init(qoim *qm) {
    qm->outf = 0;
    qm->inf = 0;
    qm->magic = 0x54FE;
    qm->error = 0;
    qm->sizex = -1;
    qm->sizey = -1;
    qm->duration = 0;
    _qoim_ilistclear(qm->frametimes);
    _qoim_ilistclear(qm->frameoffsets);
    _qoim_ilistclear(qm->framesizes);
}

static void _qoim_writeint(qoim *qm, int val) 
{
    int bytes_write = fwrite(&val, 1, sizeof(int), qm->outf);
    if(bytes_write != sizeof(int)) {
        fprintf(stderr, "qoim: _qoim_writeint error\n");
        qm->error = 1;
        return;
    }
}

static int _qoim_readint(qoim *qm) 
{
    int val;
    int bytes_read = fread(&val, 1, sizeof(int), qm->inf);
    if(bytes_read != sizeof(int)) {
        fprintf(stderr, "qoim: _qoim_readint error\n");
        qm->error = 1;
        return 0;
    }
    return val;
}

qoim *qoim_new()
{
    qoim *qm = (qoim *)malloc(sizeof(qoim));
    qm->frametimes = _qoim_ilistnew();
    qm->frameoffsets = _qoim_ilistnew();
    qm->framesizes = _qoim_ilistnew();
    _qoim_init(qm);
    return qm;
}

void qoim_free(qoim *qm)
{
    if(!qm)
        return;
    _qoim_ilistfree(qm->frametimes);
    _qoim_ilistfree(qm->frameoffsets);
    _qoim_ilistfree(qm->framesizes);
    free(qm);
}

void qoim_write(qoim *qm, const char *filename) 
{
    _qoim_init(qm);
    qm->outf = fopen(filename, "wb");
    if(!qm->outf) {
        fprintf(stderr, "qoim: can't open output file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    _qoim_writeint(qm, 0);   /* bad magic for now */
    _qoim_writeint(qm, 0);   /* nframes for now */
    _qoim_writeint(qm, 0);   /* duration for now */
}

void qoim_read(qoim *qm, const char *filename) 
{
    _qoim_init(qm);
    qm->inf = fopen(filename, "rb");
    if(!qm->inf) {
        fprintf(stderr, "qoim: can't open input file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    int val = _qoim_readint(qm);
    if(val != qm->magic) {
        fprintf(stderr, "qoim: magic: 0x%x bad magic 0x%x\n", qm->magic, val);
        qm->error = 1;
        return;
    }
    int nframes = _qoim_readint(qm);
    qm->duration = _qoim_readint(qm);
    qm->sizex = _qoim_readint(qm);
    qm->sizey = _qoim_readint(qm);
    fseek(qm->inf, -(3*nframes)*sizeof(int), SEEK_END);
    for(int i=0; i<nframes; i++) {
        _qoim_ilistadd(qm->frametimes, _qoim_readint(qm));
        _qoim_ilistadd(qm->frameoffsets, _qoim_readint(qm));
        _qoim_ilistadd(qm->framesizes,_qoim_readint(qm));
    }
}

int qoim_getnframes(qoim *qm) 
{
    return _qoim_ilistsize(qm->frameoffsets);
}

qoim_canvas *qoim_getframe(qoim *qm, int n, int *usec) 
{
    if(qm->inf) {
        *usec = _qoim_ilistget(qm->frametimes, n);
        return _qoim_readframe(qm->inf, _qoim_ilistget(qm->frameoffsets, n), _qoim_ilistget(qm->framesizes, n));
    } else {
        fprintf(stderr, "qoim: can not getframe from movie being written\n");
        qm->error = 1;
        return 0;
    }
}

int qoim_getduration(qoim *qm) 
{
    return qm->duration;
}

void qoim_putframe(qoim *qm, qoim_canvas *c) 
{
    if(qm->outf) {
        int curframetime;
        if(qoim_getnframes(qm) == 0) {
            qm->sizex = c->sizex;
            qm->sizey = c->sizey;
            _qoim_writeint(qm, qm->sizex);
            _qoim_writeint(qm, qm->sizey);
            qm->offset = 5*sizeof(int);
            qm->starttime = _qoim_getusec();
            curframetime = 0;
        } else {
            if((qm->sizex != c->sizex) || (qm->sizey != c->sizey)) {
                fprintf(stderr, "qoim: frames must be the same size\n");
                qm->error = 1;
            }
            curframetime = _qoim_getusec()-qm->starttime;
        }
        int size = _qoim_writeframe(c, qm->outf);
        _qoim_ilistadd(qm->frametimes, curframetime);
        _qoim_ilistadd(qm->frameoffsets, qm->offset);
        _qoim_ilistadd(qm->framesizes, size);
        qm->duration = curframetime;
        qm->offset += size;
    } else {
        fprintf(stderr, "qoim: can't put a frame while reading a movie\n");
        qm->error = 1;
    }
}

void qoim_print(qoim *qm, const char *label) {
    fprintf(stderr, "\n");
    fprintf(stderr, "qoim %s:\n", label);
    fprintf(stderr, "    Size: %d x %d\n", qm->sizex, qm->sizey);
    fprintf(stderr, "    N frames: %d\n", qoim_getnframes(qm));
    fprintf(stderr, "    Duration: %d usec\n", qm->duration);
    for(int n=0; n<qoim_getnframes(qm); n++)
        fprintf(stderr, "    frame: %d  time: %d offset %d size %d\n", n, _qoim_ilistget(qm->frametimes, n), _qoim_ilistget(qm->frameoffsets, n), _qoim_ilistget(qm->framesizes, n));
    fprintf(stderr, "\n");
}

int qoim_close(qoim *qm) 
{
    if(qm->outf) {
        int bytes_write;
        int nframes = qoim_getnframes(qm);
        int inittime;
        for(int i=0; i<nframes; i++) {
            _qoim_writeint(qm, _qoim_ilistget(qm->frametimes, i));
            _qoim_writeint(qm, _qoim_ilistget(qm->frameoffsets, i));
            _qoim_writeint(qm, _qoim_ilistget(qm->framesizes, i));
        }
        fseek(qm->outf, 0, SEEK_SET);
        _qoim_writeint(qm, qm->magic);
        _qoim_writeint(qm, nframes);
        _qoim_writeint(qm, qm->duration);
        _qoim_writeint(qm, qm->sizex);
        _qoim_writeint(qm, qm->sizey);
        fclose(qm->outf);
    } else {
        fclose(qm->inf);
    }
    if(qm->error)
        return 0;
    else
        return 1;
}

void qoim_readbenchmark(const char *filename) 
{
    qoim *qm = qoim_new();
    int t0 = _qoim_getusec();
    qoim_read(qm, filename);
    int nframes = qoim_getnframes(qm);
    int totpixels = 0;
    for(int i=0; i<nframes; i++) {
        int usec;
        qoim_canvas *c = qoim_getframe(qm, i, &usec);
        qoim_canvas_free(c);
        totpixels += c->sizex*c->sizey;
    }
    qoim_close(qm);
    int totusec = _qoim_getusec()-t0;
    float totMpix = totpixels/(1024*1024);
    fprintf(stderr, "qoim benchmark reading [%s]\n", filename);
    fprintf(stderr, "    %d frames with %d x %d pixels\n", nframes, qm->sizex, qm->sizey);
    fprintf(stderr, "    %d usec  %f usec per Mpix\n", totusec, totusec/totMpix);
    qoim_free(qm);
}

#endif /* QOIM_IMPLEMENTATION */
