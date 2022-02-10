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

// Define `QOM_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define QOM_IMPLEMENTATION
#include "qom.h"

//  qom
// 
//
//  write a movie
//
//    qom *qm = qom_open( "out.qom", "w");
//        gfx_canvas *c = gfx_canvas_new(400, 300);
//        qom_putframe(qm, c, usec);
//    qom_close(qm);
//
//  write a movie in real time
//
//    qom *qm = qom_open( "out.qom", "w");
//        gfx_canvas *c = gfx_canvas_new(400, 300);
//        qom_putframenow(qm, c);
//    qom_close(qm);
//
//  read a movie
//
//    qom *qm = qom_open( "out.qom", "r");
//    for(int frameno = 0; frameno<qom_getnframes(qm); frameno++) {
//        double usec;
//        gfx_canvas *c = qom_getframe(qm, frameno, &usec);
//        gfx_canvas_free(c);
//    }
//    qom_close(qm);
//
//  print
//
//    qom *qm = qom_open( "out.qom", "r");
//    qom_print(qm, "test");
//    qom_close(qm);
//
//
//    File looks like
//
//    int magic;                        header
//    int nframes;
//    unsigned int duration_lo;
//    unsigned int duration_hi;
//    int sizex;
//    int sizey;
//    unsigned int default_starttime_lo; 
//    unsigned int default_starttime_hi; 
//    int default_startdir; 
//    int default_leftbounce;
//    int default_rightbounce;
//    int frameencoding1;
//
//    QOI frame1
//    int frameencoding2;
//    QOI frame2
//    int frameencoding3;
//    QOI frame3
//
//    unsigned int time_lo;              frameinfo1
//    unsigned int time_hi;           
//    int encoding;
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//    int encoding_usec;
//
//    unsigned int time_lo;              frameinfo2
//    unsigned int time_hi;           
//    int encoding;
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//    int encoding_usec;
//
//    unsigned int time_lo;              frameinfo3
//    unsigned int time_hi;           
//    int encoding;
//    int sizex;
//    int sizey;
//    int offset;
//    int size;
//    int encoding_usec;
//

*/

/* -----------------------------------------------------------------------------
Header - Public functions */

#ifndef QOM_H
#define QOM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gfx_canvas {
    unsigned int *data;
    int sizex, sizey;
} gfx_canvas;

#define qomENCODING_LITERAL 	(0)
#define qomENCODING_QOI 		(1)
#define qomENCODING_PNG 		(2)
#define qomENCODING_JPG 		(3)	/* no transparency */

#define qomSTART_DIR_STILL (0)
#define qomSTART_DIR_INC   (1)
#define qomSTART_DIR_DEC   (2)

#define qomBOUNCE_STOP     (0)
#define qomBOUNCE_REV      (1)
#define qomBOUNCE_CYCLE    (2)

#define qomERROR_NONE			(0)
#define qomERROR_OPEN_READ		(1)
#define qomERROR_OPEN_WRITE		(2)
#define qomERROR_WRITE			(3)
#define qomERROR_READ			(4)
#define qomERROR_MAGIC			(5)
#define qomERROR_PUTFRAME_WHILE_READ	(6)
#define qomERROR_GETFRAME_WHILE_WRITE	(7)
#define qomERROR_FORMAT			(8)

typedef struct qom_header {
    int magic;    			/* magic number */
    int nframes;			/* nframes */
    unsigned int duration_lo;		/* total duration */
    unsigned int duration_hi;
    int sizex;                  	/* first frame */
    int sizey;                  	/* fisrt frame */
    unsigned int default_starttime_lo;  /* usec */
    unsigned int default_starttime_hi;  /* usec */
    int default_startdir;       	/* still left right */
    int default_leftbounce;     	/* stop rev cycle */
    int default_rightbounce;    	/* stop rec cycle */
} qom_header;

typedef struct qom_frameinfo {
    unsigned int time_lo;		/* timestamp of the frame */
    unsigned int time_hi;
    int encoding;			/* encoding method */
    int sizex;				/* image width */
    int sizey;				/* image height */
    int offset;				/* file offset of the frame data */
    int size;				/* size of the frame data */
    int encoding_usec;			/* the time used to encode and write the data */
} qom_frameinfo;

#define QIOM_HEADER_SIZE        (sizeof(qom_header))
#define QIOM_FRAME_SIZE         (sizeof(qom_frame))

#define qomMODE_NONE		(0)
#define qomMODE_R		(1)
#define qomMODE_W		(2)
#define qomMODE_RW		(3)

typedef struct qom {
    qom_header header;
    int mode;
    FILE *f;
    int error;
    int offset;
    double firstframe_usec;
    int output_encoding;
    qom_frameinfo *frames;
    int framealloc;
} qom;

gfx_canvas *gfx_canvas_new(int sizex, int sizey);
gfx_canvas *gfx_canvas_new_withdata(int sizex, int sizey, void *data);
void gfx_canvas_free(gfx_canvas *c);

qom *qom_open(const char *filename, const char *mode);
void qom_putframe(qom *qm, gfx_canvas *c, double usec);
void qom_putframenow(qom *qm, gfx_canvas *c);
gfx_canvas *qom_getframe(qom *qm, int n, double *usec);
double qom_getduration(qom *qm);
int qom_close(qom *qm);

int qom_getnframes(qom *qm);
void qom_print(qom *qm, const char *label);
void qom_readbenchmark(const char *filename);

int qom_geterror(qom *qm);

void qom_setoutputencoding(qom *qm, int encoding);
int qom_getoutputencoding(qom *qm);

void qom_setstarttime(qom *qm, double starttime);
void qom_setstartdir(qom *qm, int dir);
void qom_setleftbounce(qom *qm, int bounce);
void qom_setrightbounce(qom *qm, int bounce);

double qom_getstartusec(qom *qm);
int qom_getstartdir(qom *qm);
int qom_getleftbounce(qom *qm);
int qom_getrightbounce(qom *qm);

#ifdef __cplusplus
}
#endif
#endif /* QOM_H */


/* -----------------------------------------------------------------------------
Implementation */

#ifdef QOM_IMPLEMENTATION
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <sys/time.h>

#define oQOM_MAGIC (0x54FF)
#define ooQOM_MAGIC (0x54FE)
#define oooQOM_MAGIC (0x5501)
#define QOM_MAGIC (0x5301)

/* support for canvas data structure */

gfx_canvas *gfx_canvas_new(int sizex, int sizey)
{
    gfx_canvas *c = (gfx_canvas *)malloc(sizeof(gfx_canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)malloc(sizex*sizey*sizeof(unsigned int));
    return c;
}

gfx_canvas *gfx_canvas_new_withdata(int sizex, int sizey, void *data)
{
    gfx_canvas *c = (gfx_canvas *)malloc(sizeof(gfx_canvas));
    c->sizex = sizex;
    c->sizey = sizey;
    c->data = (unsigned int *)data;
    return c;
}

void gfx_canvas_free(gfx_canvas *c)
{
    if(!c)
        return;
    free(c->data);
    free(c);
}

/* internal utilities */

static int _qom_startsec = 0;

static double _qom_getusec(void)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    int sec = (int)tv.tv_sec;
    if(_qom_startsec == 0)
	_qom_startsec = sec;
    return (1000000*(sec-_qom_startsec))+tv.tv_usec;
}

static int _qom_writeframe_LITERAL(qom *qm, gfx_canvas *c) {
    int size = 4*c->sizex * c->sizey;
    int bytes_write = fwrite(c->data, 1, size, qm->f);
    if(bytes_write != size) {
        fprintf(stderr, "qoiwriteframe error\n");
        exit(1);
    }
    return size;
}

/* from qoi.h */
static void qom_write_32(unsigned char *bytes, int *p, unsigned int v) {
    int i = *p;
    bytes[i  ] = (0xff000000 & v) >> 24;
    bytes[i+1] = (0x00ff0000 & v) >> 16;
    bytes[i+2] = (0x0000ff00 & v) >> 8;
    bytes[i+3] = (0x000000ff & v);
    *p = i+4;
}

static unsigned int qom_read_32(const unsigned char *bytes, int *p) {
    int i = *p;
    unsigned int a = bytes[i  ];
    unsigned int b = bytes[i+1];
    unsigned int c = bytes[i+2];
    unsigned int d = bytes[i+3];
    *p = i+4;
    return a << 24 | b << 16 | c << 8 | d;
}

static void _qom_writeint(qom *qm, int val) 
{
    int p = 0;
    qom_write_32((unsigned char *)&val, &p, val);
    int bytes_write = fwrite(&val, 1, sizeof(int), qm->f);
    if(bytes_write != sizeof(int)) {
        fprintf(stderr, "qom: _qom_writeint error\n");
	exit(1);
        qm->error = qomERROR_WRITE;
        return;
    }
}

static int _qom_readint(qom *qm) 
{
    int val;
    int bytes_read = fread(&val, 1, sizeof(int), qm->f);
    if(bytes_read != sizeof(int)) {
        fprintf(stderr, "qom: _qom_readint error\n");
	exit(1);
        qm->error = qomERROR_READ;
        return 0;
    }
    int p = 0;
    return qom_read_32((unsigned char *)&val, &p);
}


static int _qom_writeframe_QOI(qom *qm, gfx_canvas *c) {
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
    int bytes_write = fwrite(encoded, 1, size, qm->f);
    if(bytes_write != size) {
        fprintf(stderr, "qoiwriteframe error\n");
        exit(1);
    }
    free(encoded);
    return size;
}

static int _qom_writeframe_PNG(qom *qm, gfx_canvas *c) 
{
    int size;
    unsigned char *encoded = stbi_write_png_to_mem((unsigned char *)c->data, 4*c->sizex, c->sizex, c->sizey, 4, &size);
    int bytes_write = fwrite(encoded, 1, size, qm->f);
    if(bytes_write != size) {
        fprintf(stderr, "qoiwriteframe error\n");
        exit(1);
    }
    free(encoded);
    return size;
}

static int _qom_writeframe_JPG(qom *qm, gfx_canvas *c) 
{
    return 0;
}


static gfx_canvas *_qom_readframe_LITERAL(qom *qm, int size) 
{
    int sizex = _qom_readint(qm);
    int sizey = _qom_readint(qm);
    void *data = malloc(4 *sizex * sizey);
    int bytes_read = fread(data, 1, size, qm->f);
    return gfx_canvas_new_withdata(sizex, sizey, data);
}

static gfx_canvas *_qom_readframe_QOI(qom *qm, int size) 
{
    qoi_desc desc;
    void *data = malloc(size);
    int bytes_read = fread(data, 1, size, qm->f);
    void *pixels = qoi_decode(data, bytes_read, &desc, 4);
    if(!pixels) {
        fprintf(stderr, "qom_readframe_QOI: decode error\n");
        exit(1);
    }
    free(data);

    int channels = desc.channels;
    int sizex = desc.width;
    int sizey = desc.height;
    return gfx_canvas_new_withdata(sizex, sizey, pixels);
}

static gfx_canvas *_qom_readframe_PNG(qom *qm, int size) 
{
    int sizex, sizey, n;
    void *data = stbi_load_from_file(qm->f, &sizex, &sizey, &n, 4);
    if(!data) {
        fprintf(stderr, "qom_readframe_PNG: decode error\n");
        exit(1);
    }
    return gfx_canvas_new_withdata(sizex, sizey, data);
}

static gfx_canvas *_qom_readframe_JPG(qom *qm, int size)
{
    return gfx_canvas_new(1,1);
}


static void _qom_addframeinfo(qom *qm, qom_frameinfo *fi, int pos)
{
    if(pos >= qm->framealloc) {
        if(qm->framealloc == 0) {
            qm->framealloc = 30;
            qm->frames = (qom_frameinfo *)malloc(qm->framealloc*sizeof(qom_frameinfo));
        } else {
            qm->framealloc = ((3*qm->framealloc)/2) + 1;
            qm->frames = (qom_frameinfo *)realloc(qm->frames, qm->framealloc*sizeof(qom_frameinfo));
        }
    }
    qm->frames[pos] = *fi;
}

static qom_frameinfo *_qom_getframeinfo(qom *qm, int index)
{
    if((index<0) || (index>=qm->header.nframes)) {
        fprintf(stderr, "_qom_ilistget: index out of range\n");
        assert(0);
    }
    return qm->frames+index;
}

static void _qom_free(qom *qm)
{
    if(!qm)
        return;
    if(qm->frames)
        free(qm->frames);
    free(qm);
}

static void _qom_readheader(qom *qm) 
{
    qm->header.magic = _qom_readint(qm);    
    qm->header.nframes = _qom_readint(qm);
    qm->header.duration_lo = _qom_readint(qm);
    qm->header.duration_hi = _qom_readint(qm);
    qm->header.sizex = _qom_readint(qm);
    qm->header.sizey = _qom_readint(qm);
    qm->header.default_starttime_lo = _qom_readint(qm);    
    qm->header.default_starttime_hi = _qom_readint(qm);    
    qm->header.default_startdir = _qom_readint(qm);     
    qm->header.default_leftbounce = _qom_readint(qm);   
    qm->header.default_rightbounce = _qom_readint(qm);   
}

static void _qom_writeheader(qom *qm) 
{
    qm->header.magic = QOM_MAGIC;
    _qom_writeint(qm, qm->header.magic);
    _qom_writeint(qm, qm->header.nframes);
    _qom_writeint(qm, qm->header.duration_lo);
    _qom_writeint(qm, qm->header.duration_hi);
    _qom_writeint(qm, qm->header.sizex);
    _qom_writeint(qm, qm->header.sizey);
    _qom_writeint(qm, qm->header.default_starttime_lo);
    _qom_writeint(qm, qm->header.default_starttime_hi);
    _qom_writeint(qm, qm->header.default_startdir);
    _qom_writeint(qm, qm->header.default_leftbounce);
    _qom_writeint(qm, qm->header.default_rightbounce);
}

static int _qom_openwrite(qom *qm, const char *filename) 
{
    qm->f = fopen(filename, "wb");
    qm->mode = qomMODE_W;
    if(!qm->f) {
        fprintf(stderr, "qom: can't open output file [%s]\n", filename);
        qm->error = qomERROR_OPEN_WRITE;
        return 0;
    }
    _qom_writeheader(qm);
    return 1;
}

static void _qom_readframeinfo(qom *qm) 
{
    qm->frames = (qom_frameinfo *)malloc(qm->header.nframes*sizeof(qom_frameinfo));
    qm->framealloc = qm->header.nframes;
    qom_frameinfo *fi = qm->frames;
    for(int i=0; i<qm->header.nframes; i++) {
        fi->time_lo = _qom_readint(qm);    
        fi->time_hi = _qom_readint(qm);    
        fi->encoding = _qom_readint(qm);    
        fi->sizex = _qom_readint(qm);    
        fi->sizey = _qom_readint(qm);    
        fi->offset = _qom_readint(qm);    
        fi->size = _qom_readint(qm);    
        fi->encoding_usec = _qom_readint(qm);    
        fi++;
    }
}

static void _qom_writeframeinfo(qom *qm) 
{
    qom_frameinfo *fi = qm->frames;
    for(int i=0; i<qm->header.nframes; i++) {
        _qom_writeint(qm, fi->time_lo);
        _qom_writeint(qm, fi->time_hi);
        _qom_writeint(qm, fi->encoding);
        _qom_writeint(qm, fi->sizex);
        _qom_writeint(qm, fi->sizey);
        _qom_writeint(qm, fi->offset);
        _qom_writeint(qm, fi->size);
        _qom_writeint(qm, fi->encoding_usec);
        fi++;
    }
}

static int _qom_openread(qom *qm, const char *filename) 
{
    qm->f = fopen(filename, "rb");
    qm->mode = qomMODE_R;
    if(!qm->f) {
        fprintf(stderr, "qom: can't open input file [%s]\n", filename);
        qm->error = qomERROR_OPEN_READ;
        return 0;
    }
    _qom_readheader(qm);
    if(qm->header.magic != QOM_MAGIC) {
        fprintf(stderr, "qom: good magic: 0x%x  bad magic 0x%x\n", QOM_MAGIC, qm->header.magic);
        qm->error = qomERROR_MAGIC;
        return 0;
    }
    fseek(qm->f, -(qm->header.nframes*sizeof(qom_frameinfo)), SEEK_END);
    _qom_readframeinfo(qm);
    return 1;
}

qom *qom_open(const char *filename, const char *mode)
{
    qom *qm = (qom *)malloc(sizeof(qom));
    qm->header.magic = 0;
    qm->header.nframes = 0;
    qm->header.duration_lo = 0;
    qm->header.duration_hi = 0;
    qm->header.sizex = -1;
    qm->header.sizey = -1;
    qm->header.default_starttime_lo = 0;  
    qm->header.default_starttime_hi = 0;  
    qm->header.default_startdir = qomSTART_DIR_INC;
    qm->header.default_leftbounce = qomBOUNCE_REV;
    qm->header.default_rightbounce = qomBOUNCE_REV;
    qm->mode = qomMODE_NONE;
    qm->f = 0;
    qm->error = qomERROR_NONE;
    qm->firstframe_usec = 0;
    qm->frames = 0;
    qm->framealloc = 0;
    qm->output_encoding = qomENCODING_QOI;

    if(strcmp(mode, "r") == 0) {
        if(!_qom_openread(qm, filename)) {
           if(qm->f)
               fclose(qm->f);
            _qom_free(qm);
            return 0;
        }
    } else if(strcmp(mode, "w") == 0) {
        if(!_qom_openwrite(qm, filename)) {
           if(qm->f)
               fclose(qm->f);
            _qom_free(qm);
            return 0;
        }
    } else {
        fprintf(stderr, "qom: strange mode for open [%s]\n", mode);
        _qom_free(qm);
        return 0;
    }
    return qm;
}

int qom_getnframes(qom *qm) 
{
    return qm->header.nframes;
}

double gfx_64ToUsec(unsigned int lo, unsigned int hi)
{
    double dlo = lo;
    double dhi = hi;
    double dscale = 256*256;
    dscale = dscale*dscale;
    return ((dhi*dscale) + lo);
}

void gfx_UsecTo64(double t, unsigned int *lo, unsigned int *hi)
{
    double dscale = 256*256;
    dscale = dscale*dscale;
    double dhi = floor(t/dscale);
    double dlo = t-(dhi*dscale);
    *hi = dhi;
    *lo = dlo;
}

gfx_canvas *qom_getframe(qom *qm, int n, double *usec) 
{
    if((qm->mode == qomMODE_R) || (qm->mode == qomMODE_RW)) {
        qom_frameinfo *info = _qom_getframeinfo(qm, n);

	fseek(qm->f, info->offset, SEEK_SET);
	int input_encoding = _qom_readint(qm);
	int imgdatasize = info->size-4;
	*usec = gfx_64ToUsec(info->time_lo, info->time_hi);
	switch(input_encoding) {
	    case qomENCODING_LITERAL:
		return _qom_readframe_LITERAL(qm, imgdatasize);
	    case qomENCODING_QOI:
		return _qom_readframe_QOI(qm, imgdatasize);
	    case qomENCODING_PNG:
		return _qom_readframe_PNG(qm, imgdatasize);
	    case qomENCODING_JPG:
		return _qom_readframe_JPG(qm, imgdatasize);
	    default:
        	fprintf(stderr, "qom: strange frame encoding %d\n", input_encoding);
		qm->error = qomERROR_FORMAT;
		return 0;
	}
    } else {
        fprintf(stderr, "qom: can't getframe from movie being written\n");
        qm->error = qomERROR_GETFRAME_WHILE_WRITE;
        return 0;
    }
}

double qom_getduration(qom *qm) 
{
    return gfx_64ToUsec(qm->header.duration_lo, qm->header.duration_hi);
}

void qom_putframe(qom *qm, gfx_canvas *c, double usec) 
{
    if((qm->mode == qomMODE_W) || (qm->mode == qomMODE_RW)) {
        if(qom_getnframes(qm) == 0) {
            qm->header.sizex = c->sizex;
            qm->header.sizey = c->sizey;
            qm->offset = sizeof(qom_header);
            qm->firstframe_usec = usec;
        }
	double curframe_usec = usec-qm->firstframe_usec;
	double startput_usec = _qom_getusec();
	int size;
	switch(qm->output_encoding) {
	    case qomENCODING_LITERAL:
    		_qom_writeint(qm, qomENCODING_LITERAL);
		size = 4 + _qom_writeframe_LITERAL(qm, c);
		break;
	    case qomENCODING_QOI:
    		_qom_writeint(qm, qomENCODING_QOI);
		size = 4 + _qom_writeframe_QOI(qm, c);
		break;
	    case qomENCODING_PNG:
    		_qom_writeint(qm, qomENCODING_PNG);
		size = 4 + _qom_writeframe_PNG(qm, c);
		break;
	    case qomENCODING_JPG:
    		_qom_writeint(qm, qomENCODING_JPG);
		size = 4 + _qom_writeframe_JPG(qm, c);
		break;
	    default:
        	fprintf(stderr, "qom: strange frame encoding %d\n", qm->output_encoding);
		qm->error = qomERROR_FORMAT;
	       	return;
	}

        qom_frameinfo fi;
	gfx_UsecTo64(curframe_usec, &fi.time_lo, &fi.time_hi);
        fi.encoding = qm->output_encoding;
        fi.sizex = c->sizex;
        fi.sizey = c->sizey;
        fi.offset = qm->offset;
        fi.size = size;
        fi.encoding_usec = _qom_getusec()-startput_usec;
        _qom_addframeinfo(qm, &fi, qm->header.nframes);

	gfx_UsecTo64(curframe_usec, &qm->header.duration_lo, &qm->header.duration_hi);
        qm->header.nframes++;
        qm->offset += size;
    } else {
        fprintf(stderr, "qom: can't put a frame while reading a movie\n");
        qm->error = qomERROR_PUTFRAME_WHILE_READ;
    }
}

void qom_putframenow(qom *qm, gfx_canvas *c) 
{
    qom_putframe(qm, c, _qom_getusec()); 
}

static const char *qom_encodingname(int encoding) 
{
    switch(encoding) {
	case qomENCODING_LITERAL:
	    return "LIT";
	case qomENCODING_QOI:
	    return "QOI";
	case qomENCODING_PNG:
	    return "PNG";
	case qomENCODING_JPG:
	    return "JPG";
    }
    return "strange....";
}

void qom_print(qom *qm, const char *label) {
    fprintf(stderr, "\n");
    fprintf(stderr, "qom %s:\n", label);
    fprintf(stderr, "    Size: %d x %d (of first frame)\n", qm->header.sizex, qm->header.sizey);
    fprintf(stderr, "    N frames: %d\n", qom_getnframes(qm));
    fprintf(stderr, "    Duration: %f sec\n", gfx_64ToUsec(qm->header.duration_lo, qm->header.duration_hi)/(1000.0*1000.0));
    for(int n=0; n<qom_getnframes(qm); n++) {
        qom_frameinfo *info = _qom_getframeinfo(qm, n);
	double time = gfx_64ToUsec(info->time_lo, info->time_hi);
        fprintf(stderr, "    %s frame: %d  size: %dx%d  time: %f  offset %d  size %d\n", qom_encodingname(info->encoding), n, info->sizex, info->sizey, time, info->offset, info->size);
    }
    int totencode_usec = 0;
    int totpixels = 0;
    int totdata = 0;
    for(int i=0; i<qom_getnframes(qm); i++) {
        qom_frameinfo *fi = _qom_getframeinfo(qm, i);
        totpixels += fi->sizex*fi->sizey;
        totdata += fi->size;
	totencode_usec += fi->encoding_usec;
    }
    float totMpix = totpixels/(1024.0*1024.0);
    fprintf(stderr, "Summary\n");
    fprintf(stderr, "    %d frames  %f Mega pixels\n", qom_getnframes(qm), totMpix);
    fprintf(stderr, "    %f total encode time\n", totencode_usec/(1000.0*1000.0));
    fprintf(stderr, "    %d compressed bytes  %d expanded bytes\n", totdata, totpixels*4);
    fprintf(stderr, "    %f compression ratio\n", totdata/(totpixels*4.0));
    fprintf(stderr, "\n");
}

int qom_close(qom *qm) 
{
    if(qm->f) {
	if((qm->mode == qomMODE_W) || (qm->mode == qomMODE_RW)) {
	    _qom_writeframeinfo(qm);
	    fseek(qm->f, 0, SEEK_SET);
	    _qom_writeheader(qm);
   	}
        fclose(qm->f);
        qm->f = 0;
    }
    int error = qm->error;
    _qom_free(qm);
    return error;
}

int qom_geterror(qom *qm)
{
    return qm->error;
}


void qom_setoutputencoding(qom *qm, int encoding)
{
    qm->output_encoding = encoding;
}

int qom_getoutputencoding(qom *qm)
{
    return qm->output_encoding;
}


void qom_setstartusec(qom *qm, double startusec)
{
    gfx_UsecTo64(startusec, &qm->header.default_starttime_lo, &qm->header.default_starttime_hi);
}

void qom_setstartdir(qom *qm, int dir)
{
    qm->header.default_startdir = dir;
}

void qom_setleftbounce(qom *qm, int bounce)
{
    qm->header.default_leftbounce = bounce;
}

void qom_setrightbounce(qom *qm, int bounce)
{
    qm->header.default_rightbounce = bounce;
}


double qom_getstartusec(qom *qm)
{
    return gfx_64ToUsec(qm->header.default_starttime_lo, qm->header.default_starttime_hi);
}

int qom_getstartdir(qom *qm)
{
    return qm->header.default_startdir;
}

int qom_getleftbounce(qom *qm)
{
    return qm->header.default_leftbounce;
}

int qom_getrightbounce(qom *qm)
{
    return qm->header.default_rightbounce;
}

void qom_readbenchmark(const char *filename) 
{
    qom *qm = qom_open(filename, "r");
    if(!qm)
        exit(1);
    int t0 = _qom_getusec();
    int nframes = qom_getnframes(qm);
    int totpixels = 0;
    int totdata = 0;
    for(int i=0; i<nframes; i++) {
        double usec;
        gfx_canvas *c = qom_getframe(qm, i, &usec);
        gfx_canvas_free(c);
        qom_frameinfo *fi = _qom_getframeinfo(qm, i);
        totpixels += fi->sizex*fi->sizey;
        totdata += fi->size;
    }
    int tot_usec = _qom_getusec()-t0;
    float totMpix = totpixels/(1024.0*1024.0);
    fprintf(stderr, "Benchmark reading %s:\n", filename);
    fprintf(stderr, "    %d frames  %f Mega pixels\n", nframes, totMpix);
    fprintf(stderr, "    %d compressed bytes  %d expanded bytes\n", totdata, totpixels*4);
    fprintf(stderr, "    %f compression ratio\n", totdata/(totpixels*4.0));
    fprintf(stderr, "    %d usec total time  %f usec per Mpix  %f Mpix per sec\n", tot_usec, tot_usec/totMpix, 1000.0*1000.0*(totMpix/tot_usec));
    qom_close(qm);
}

#endif /* QOM_IMPLEMENTATION */
