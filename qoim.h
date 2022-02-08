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
//    qoim *qm = qoim_open( "out.qoim", "w");
//        qoim_canvas *c = qoim_canvas_new(400, 300);
//        qoim_putframe(qm, c, usec);
//    qoim_close(qm);
//
//  write a movie in real time
//
//    qoim *qm = qoim_open( "out.qoim", "w");
//        qoim_canvas *c = qoim_canvas_new(400, 300);
//        qoim_putframenow(qm, c);
//    qoim_close(qm);
//
//  read a movie
//
//    qoim *qm = qoim_open( "out.qoim", "r");
//    for(int frameno = 0; frameno<qoim_getnframes(qm); frameno++) {
//        int usec;
//        qoim_canvas *c = qoim_getframe(qm, frameno, &usec);
//        qoim_canvas_free(c);
//    }
//    qoim_close(qm);
//
//  print
//
//    qoim *qm = qoim_open( "out.qoim", "r");
//    qoim_print(qm, "test");
//    qoim_close(qm);
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

#define START_DIR_STILL (0)
#define START_DIR_INC   (1)
#define START_DIR_DEC   (2)

#define BOUNCE_STOP     (0)
#define BOUNCE_REV      (1)
#define BOUNCE_CYCLE    (2)

typedef struct qoim_header {
    int magic;    
    int nframes;
    int duration;
    int sizex;                  /* first frame */
    int sizey;                  /* fisrt frame */
    int default_starttime;      /* usec */
    int default_startdir;       /* still left right */
    int default_leftbounce;     /* stop rev cycle */
    int default_rightbounce;    /* stop rec cycle */
} qoim_header;

typedef struct qoim_frameinfo {
    int time;
    int sizex;
    int sizey;
    int offset;
    int size;
} qoim_frameinfo;

#define QIOM_HEADER_SIZE        (sizeof(qoim_header))
#define QIOM_FRAME_SIZE         (sizeof(qoim_frame))

typedef struct qoim {
    qoim_header header;
    FILE *outf;
    FILE *inf;
    int error;
    int offset;
    int starttime;
    qoim_frameinfo *frames;
    int framealloc;
} qoim;

qoim_canvas *qoim_canvas_new(int sizex, int sizey);
qoim_canvas *qoim_canvas_new_withdata(int sizex, int sizey, void *data);
void qoim_canvas_free(qoim_canvas *c);

qoim *qoim_open(const char *filename, const char *mode);
void qoim_putframe(qoim *qm, qoim_canvas *c, int usec);
void qoim_putframenow(qoim *qm, qoim_canvas *c);
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

#define oldQOIM_MAGIC (0x54FE)
#define QOIM_MAGIC (0x54FF)

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
        fprintf(stderr, "qoiwriteframe encode error\n");
        exit(1);
    }
    int bytes_write = fwrite(encoded, 1, size, f);
    if(bytes_write != size) {
        fprintf(stderr, "qoiwriteframe error\n");
        exit(1);
    }
    free(encoded);
    return size;
}

static qoim_canvas *_qoim_readframe(FILE *f, int offset, int size) {
    qoi_desc desc;
    fseek(f, offset, SEEK_SET);
    void *data = malloc(size);
    int bytes_read = fread(data, 1, size, f);
    void *pixels = qoi_decode(data, bytes_read, &desc, 4);
    if(!pixels) {
        fprintf(stderr, "qoim_readframe: qoi_decode error\n");
        exit(1);
    }
    free(data);

    int channels = desc.channels;
    int sizex = desc.width;
    int sizey = desc.height;
    return qoim_canvas_new_withdata(sizex, sizey, pixels);
}

static void _qoim_addframeinfo(qoim *qm, qoim_frameinfo *fi, int pos)
{
    if(pos >= qm->framealloc) {
        if(qm->framealloc == 0) {
            qm->framealloc = 30;
            qm->frames = (qoim_frameinfo *)malloc(qm->framealloc*sizeof(qoim_frameinfo));
        } else {
            qm->framealloc = ((3*qm->framealloc)/2) + 1;
            qm->frames = (qoim_frameinfo *)realloc(qm->frames, qm->framealloc*sizeof(qoim_frameinfo));
        }
    }
    qm->frames[pos] = *fi;
}

static qoim_frameinfo *_qoim_getframeinfo(qoim *qm, int index)
{
    if((index<0) || (index>=qm->header.nframes)) {
        fprintf(stderr, "_qoim_ilistget: index out of range\n");
        assert(0);
    }
    return qm->frames+index;
}


/* from qoi.h */
static void qoim_write_32(unsigned char *bytes, int *p, unsigned int v) {
    int i = *p;
    bytes[i  ] = (0xff000000 & v) >> 24;
    bytes[i+1] = (0x00ff0000 & v) >> 16;
    bytes[i+2] = (0x0000ff00 & v) >> 8;
    bytes[i+3] = (0x000000ff & v);
    *p = i+4;
}

static unsigned int qoim_read_32(const unsigned char *bytes, int *p) {
    int i = *p;
    unsigned int a = bytes[i  ];
    unsigned int b = bytes[i+1];
    unsigned int c = bytes[i+2];
    unsigned int d = bytes[i+3];
    *p = i+4;
    return a << 24 | b << 16 | c << 8 | d;
}

static void _qoim_writeint(qoim *qm, int val) 
{
    int p = 0;
    qoim_write_32((unsigned char *)&val, &p, val);
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
    int p = 0;
    return qoim_read_32((unsigned char *)&val, &p);
}


static void _qoim_free(qoim *qm)
{
    if(!qm)
        return;
    if(qm->frames)
        free(qm->frames);
    free(qm);
}

static void _qoim_readheader(qoim *qm) 
{
    qm->header.magic = _qoim_readint(qm);    
    qm->header.nframes = _qoim_readint(qm);
    qm->header.duration = _qoim_readint(qm);
    qm->header.sizex = _qoim_readint(qm);
    qm->header.sizey = _qoim_readint(qm);
    qm->header.default_starttime = _qoim_readint(qm);    
    qm->header.default_startdir = _qoim_readint(qm);     
    qm->header.default_leftbounce = _qoim_readint(qm);   
    qm->header.default_rightbounce = _qoim_readint(qm);   
}

static void _qoim_writeheader(qoim *qm) 
{
    qm->header.magic = QOIM_MAGIC;
    _qoim_writeint(qm, qm->header.magic);
    _qoim_writeint(qm, qm->header.nframes);
    _qoim_writeint(qm, qm->header.duration);
    _qoim_writeint(qm, qm->header.sizex);
    _qoim_writeint(qm, qm->header.sizey);
    _qoim_writeint(qm, qm->header.default_starttime);
    _qoim_writeint(qm, qm->header.default_startdir);
    _qoim_writeint(qm, qm->header.default_leftbounce);
    _qoim_writeint(qm, qm->header.default_rightbounce);
}

static void _qoim_openwrite(qoim *qm, const char *filename) 
{
    qm->outf = fopen(filename, "wb");
    if(!qm->outf) {
        fprintf(stderr, "qoim: can't open output file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    _qoim_writeheader(qm);
}


static void _qoim_readframeinfo(qoim *qm) 
{
    qm->frames = (qoim_frameinfo *)malloc(qm->header.nframes*sizeof(qoim_frameinfo));
    qm->framealloc = qm->header.nframes;
    qoim_frameinfo *fi = qm->frames;
    for(int i=0; i<qm->header.nframes; i++) {
        fi->time = _qoim_readint(qm);    
        fi->sizex = _qoim_readint(qm);    
        fi->sizey = _qoim_readint(qm);    
        fi->offset = _qoim_readint(qm);    
        fi->size = _qoim_readint(qm);    
        fi++;
    }
}

static void _qoim_writeframeinfo(qoim *qm) 
{
    qoim_frameinfo *fi = qm->frames;
    for(int i=0; i<qm->header.nframes; i++) {
        _qoim_writeint(qm, fi->time);
        _qoim_writeint(qm, fi->sizex);
        _qoim_writeint(qm, fi->sizey);
        _qoim_writeint(qm, fi->offset);
        _qoim_writeint(qm, fi->size);
        fi++;
    }
}

static void _qoim_openread(qoim *qm, const char *filename) 
{
    qm->inf = fopen(filename, "rb");
    if(!qm->inf) {
        fprintf(stderr, "qoim: can't open input file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    _qoim_readheader(qm);
    if(qm->header.magic != QOIM_MAGIC) {
        fprintf(stderr, "qoim: good magic: 0x%x  bad magic 0x%x\n", QOIM_MAGIC, qm->header.magic);
        qm->error = 1;
        return;
    }
    fseek(qm->inf, -(qm->header.nframes*sizeof(qoim_frameinfo)), SEEK_END);
    _qoim_readframeinfo(qm);
}

qoim *qoim_open(const char *filename, const char *mode)
{
    qoim *qm = (qoim *)malloc(sizeof(qoim));
    qm->header.magic = 0;
    qm->header.nframes = 0;
    qm->header.duration = 0;
    qm->header.sizex = -1;
    qm->header.sizey = -1;
    qm->header.default_starttime = 0;  
    qm->header.default_startdir = START_DIR_INC;
    qm->header.default_leftbounce = BOUNCE_REV;
    qm->header.default_rightbounce = BOUNCE_REV;
    qm->outf = 0;
    qm->inf = 0;
    qm->error = 0;
    qm->starttime = 0;
    qm->frames = 0;
    qm->framealloc = 0;

    if(strcmp(mode, "r") == 0) {
        _qoim_openread(qm, filename);
    } else if(strcmp(mode, "w") == 0) {
        _qoim_openwrite(qm, filename);
    } else {
        fprintf(stderr, "qoim: strange mode for open [%s]\n", mode);
        _qoim_free(qm);
        return 0;
    }
    return qm;
}

int qoim_getnframes(qoim *qm) 
{
    return qm->header.nframes;
}

qoim_canvas *qoim_getframe(qoim *qm, int n, int *usec) 
{
    if(qm->inf) {
        qoim_frameinfo *info = _qoim_getframeinfo(qm, n);
        return _qoim_readframe(qm->inf, info->offset, info->size);
    } else {
        fprintf(stderr, "qoim: can not getframe from movie being written\n");
        qm->error = 1;
        return 0;
    }
}

int qoim_getduration(qoim *qm) 
{
    return qm->header.duration;
}

void qoim_putframe(qoim *qm, qoim_canvas *c, int usec) 
{
    if(qm->outf) {
        int curframetime;
        if(qoim_getnframes(qm) == 0) {
            qm->header.sizex = c->sizex;
            qm->header.sizey = c->sizey;
            qm->offset = sizeof(qoim_header);
            qm->starttime = usec;
            curframetime = 0;
        } else {
            curframetime = usec-qm->starttime;
        }
        int size = _qoim_writeframe(c, qm->outf);
        qoim_frameinfo info;
        info.time = curframetime;
        info.sizex = c->sizex;
        info.sizey = c->sizey;
        info.offset = qm->offset;
        info.size = size;
        _qoim_addframeinfo(qm, &info, qm->header.nframes);
        qm->header.nframes++;
        qm->header.duration = curframetime;
        qm->offset += size;
    } else {
        fprintf(stderr, "qoim: can't put a frame while reading a movie\n");
        qm->error = 1;
    }
}

void qoim_putframenow(qoim *qm, qoim_canvas *c) 
{
    qoim_putframe(qm, c, _qoim_getusec()); 
}

void qoim_print(qoim *qm, const char *label) {
    fprintf(stderr, "\n");
    fprintf(stderr, "qoim %s:\n", label);
    fprintf(stderr, "    Size: %d x %d (of first frame)\n", qm->header.sizex, qm->header.sizey);
    fprintf(stderr, "    N frames: %d\n", qoim_getnframes(qm));
    fprintf(stderr, "    Duration: %f sec\n", qm->header.duration/(1000.0*1000.0));
    for(int n=0; n<qoim_getnframes(qm); n++) {
        qoim_frameinfo *info = _qoim_getframeinfo(qm, n);
        fprintf(stderr, "    frame: %d  size: %dx%d  time: %d  offset %d  size %d\n", n, info->sizex, info->sizey, info->time, info->offset, info->size);
    }
    int totpixels = 0;
    int totdata = 0;
    for(int i=0; i<qoim_getnframes(qm); i++) {
        qoim_frameinfo *fi = _qoim_getframeinfo(qm, i);
        totpixels += fi->sizex*fi->sizey;
        totdata += fi->size;
    }
    float totMpix = totpixels/(1024.0*1024.0);
    fprintf(stderr, "Summary\n");
    fprintf(stderr, "    %d frames  %f Mega pixels\n", qoim_getnframes(qm), totMpix);
    fprintf(stderr, "    %d compressed bytes  %d expanded bytes\n", totdata, totpixels*4);
    fprintf(stderr, "    %f compression ratio\n", totdata/(totpixels*4.0));
    fprintf(stderr, "\n");
}

int qoim_close(qoim *qm) 
{
    if(qm->outf) {
        _qoim_writeframeinfo(qm);
        fseek(qm->outf, 0, SEEK_SET);
        _qoim_writeheader(qm);
        fclose(qm->outf);
        qm->outf = 0;
    } else {
        fclose(qm->inf);
        qm->inf = 0;
    }
    int error = qm->error;
    _qoim_free(qm);
    return error;
}

void qoim_readbenchmark(const char *filename) 
{
    qoim *qm = qoim_open(filename, "r");
    int t0 = _qoim_getusec();
    int nframes = qoim_getnframes(qm);
    int totpixels = 0;
    int totdata = 0;
    for(int i=0; i<nframes; i++) {
        int usec;
        qoim_canvas *c = qoim_getframe(qm, i, &usec);
        qoim_canvas_free(c);
        qoim_frameinfo *fi = _qoim_getframeinfo(qm, i);
        totpixels += fi->sizex*fi->sizey;
        totdata += fi->size;
    }
    int totusec = _qoim_getusec()-t0;
    float totMpix = totpixels/(1024.0*1024.0);
    fprintf(stderr, "Benchmark reading %s:\n", filename);
    fprintf(stderr, "    %d frames  %f Mega pixels\n", nframes, totMpix);
    fprintf(stderr, "    %d compressed bytes  %d expanded bytes\n", totdata, totpixels*4);
    fprintf(stderr, "    %f compression ratio\n", totdata/(totpixels*4.0));
    fprintf(stderr, "    %d usec total time  %f usec per Mpix\n", totusec, totusec/totMpix);
    qoim_close(qm);
}

#endif /* QOIM_IMPLEMENTATION */
