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
    canvas *pic = canvas_new(sizex, sizey);
    free(pic->data);
    pic->data = (unsigned int *)data;
    return pic;
}

void canvas_topng(canvas *in, const char *filename)
{
    stbi_write_png(filename, in->sizex, in->sizey, 4, in->data, 4*in->sizex);
}

// utilities

static unsigned int getusec(void)
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

// support for ilist

typedef struct ilist {
    int len;
    int space;
    int *data;
} ilist;

static ilist *ilistnew(void)
{
    ilist *il;

    il = (ilist *)malloc(sizeof(ilist));
    il->space = 0;
    il->len = 0;
    il->data = 0;
    return il;
}

static void ilistfree(ilist *il)
{
    if(!il)
        return;
    if(il->data)
        free(il->data);
    free(il);
}

static int ilistsize(ilist *il)
{
    return il->len;
}

static void ilistclear(ilist *il)
{
    il->len = 0;
}

static int reallocscale(int len)
{
    return len + len/2 + 1;
}

static void ilistadd(ilist *il, int val)
{
    int pos;

    pos = il->len;
    il->len = pos+1;
    if(pos >= il->space) {
        if(il->space == 0) {
            il->space = 30;
            il->data = (int *)malloc(il->space*sizeof(int));
        } else {
            il->space = reallocscale(il->space);
            il->data = (int *)realloc(il->data, il->space*sizeof(int));
        }
    }
    il->data[pos] = val;
}

static int ilistget(ilist *il, int index)
{
    if((index<0) || (index>=il->len)) {
        fprintf(stderr, "ilistget: index out of range\n");
        assert(0);
    }
    return il->data[index];
}

// 
//  QOImovie
// 
//
//  write a movie
//
//    QOImovie *qm = QOImovie_new();
//    QOImovie_write(qm, "out.qoim");
//        canvas *c = canvas_new(400, 300);
//        QOImovie_putframe(qm, c);
//    QOImovie_close(qm);
//    QOImovie_free(qm);
//
//  read a movie
//
//    QOImovie *qm = QOImovie_new();
//    QOImovie_read(qm, "in.qoim");
//    for(int frameno = 0; frameno<QOImovie_getnframes(qm); frameno++) {
//        canvas *c = QOImovie_getframe(qm, frameno);
//        canvas_free(c);
//    }
//    QOImovie_close(qm);
//    QOImovie_free(qm);
//
//  print
//
//    QOImovie *qm = QOImovie_new();
//    QOImovie_read(qm, argv[2]);
//    QOImovie_print(qm, "test");
//    QOImovie_close(qm);
//    QOImovie_free(qm);
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

typedef struct QOImovie {
    FILE *outf;
    FILE *inf;
    int magic;
    int error;
    int offset, sizex, sizey;
    int starttime, duration;
    ilist *frametimes;
    ilist *frameoffsets;
    ilist *framesizes;
} QOImovie;

static void _init(QOImovie *qm) {
    qm->outf = 0;
    qm->inf = 0;
    qm->magic = 0x54FE;
    qm->error = 0;
    qm->sizex = -1;
    qm->sizey = -1;
    qm->duration = 0;
    ilistclear(qm->frametimes);
    ilistclear(qm->frameoffsets);
    ilistclear(qm->framesizes);
}

void _writeint(QOImovie *qm, int val) 
{
    int bytes_write = fwrite(&val, 1, sizeof(int), qm->outf);
    if(bytes_write != sizeof(int)) {
        fprintf(stderr, "QOImovie: _writeint error\n");
        qm->error = 1;
        return;
    }
}

int _readint(QOImovie *qm) 
{
    int val;
    int bytes_read = fread(&val, 1, sizeof(int), qm->inf);
    if(bytes_read != sizeof(int)) {
        fprintf(stderr, "QOImovie: _readint error\n");
        qm->error = 1;
        return 0;
    }
    return val;
}

QOImovie *QOImovie_new()
{
    QOImovie *qm = (QOImovie *)malloc(sizeof(QOImovie));
    qm->frametimes = ilistnew();
    qm->frameoffsets = ilistnew();
    qm->framesizes = ilistnew();
    _init(qm);
    return qm;
}

void QOImovie_free(QOImovie *qm)
{
    if(!qm)
        return;
    ilistfree(qm->frametimes);
    ilistfree(qm->frameoffsets);
    ilistfree(qm->framesizes);
    free(qm);
}

void QOImovie_write(QOImovie *qm, const char *filename) 
{
    _init(qm);
    qm->outf = fopen(filename, "wb");
    if(!qm->outf) {
        fprintf(stderr, "QOImovie: can't open output file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    _writeint(qm, 0);   // bad magic for now
    _writeint(qm, 0);   // nframes for now
    _writeint(qm, 0);   // duration for now
}

void QOImovie_read(QOImovie *qm, const char *filename) 
{
    _init(qm);
    qm->inf = fopen(filename, "rb");
    if(!qm->inf) {
        fprintf(stderr, "QOImovie: can't open input file [%s]\n", filename);
        qm->error = 1;
        return;
    }
    int val = _readint(qm);
    if(val != qm->magic) {
        fprintf(stderr, "QOImovie: magic: 0x%x bad magic 0x%x\n", qm->magic, val);
        qm->error = 1;
        return;
    }
    int nframes = _readint(qm);
    qm->duration = _readint(qm);
    qm->sizex = _readint(qm);
    qm->sizey = _readint(qm);
    fseek(qm->inf, -(3*nframes)*sizeof(int), SEEK_END);
    for(int i=0; i<nframes; i++) {
        ilistadd(qm->frametimes, _readint(qm));
        ilistadd(qm->frameoffsets, _readint(qm));
        ilistadd(qm->framesizes,_readint(qm));
    }
}

int QOImovie_getnframes(QOImovie *qm) 
{
    return ilistsize(qm->frameoffsets);
}

canvas *QOImovie_getframe(QOImovie *qm, int n) 
{
    if(qm->inf) {
        return qoireadframe(qm->inf, ilistget(qm->frameoffsets, n), ilistget(qm->framesizes, n));
    } else {
        fprintf(stderr, "QOImovie: can not getframe from movie being written\n");
        qm->error = 1;
        return 0;
    }
}

void QOImovie_putframe(QOImovie *qm, canvas *c) 
{
    if(qm->outf) {
        int curframetime;
        if(QOImovie_getnframes(qm) == 0) {
            qm->sizex = c->sizex;
            qm->sizey = c->sizey;
            _writeint(qm, qm->sizex);
            _writeint(qm, qm->sizey);
            qm->offset = 5*sizeof(int);
            qm->starttime = getusec();
            curframetime = 0;
        } else {
            if((qm->sizex != c->sizex) || (qm->sizey != c->sizey)) {
                fprintf(stderr, "QOImovie: frames must be the same size\n");
                qm->error = 1;
            }
            curframetime = getusec()-qm->starttime;
        }
        int size = qoiwriteframe(c, qm->outf);
        ilistadd(qm->frametimes, curframetime);
        ilistadd(qm->frameoffsets, qm->offset);
        ilistadd(qm->framesizes, size);
        qm->duration = curframetime;
        qm->offset += size;
    } else {
        fprintf(stderr, "QOImovie: can't put a frame while reading a movie\n");
        qm->error = 1;
    }
}

void QOImovie_print(QOImovie *qm, const char *label) {
    fprintf(stderr, "\n");
    fprintf(stderr, "QOImovie %s:\n", label);
    fprintf(stderr, "    Size: %d x %d\n", qm->sizex, qm->sizey);
    fprintf(stderr, "    N frames: %d\n", QOImovie_getnframes(qm));
    fprintf(stderr, "    Duration: %d usec\n", qm->duration);
    for(int n=0; n<QOImovie_getnframes(qm); n++)
        fprintf(stderr, "    frame: %d  time: %d offset %d size %d\n", n, ilistget(qm->frametimes, n), ilistget(qm->frameoffsets, n), ilistget(qm->framesizes, n));
    fprintf(stderr, "\n");
}

int QOImovie_close(QOImovie *qm) 
{
    if(qm->outf) {
        int bytes_write;
        int nframes = QOImovie_getnframes(qm);
        int inittime;
        for(int i=0; i<nframes; i++) {
            _writeint(qm, ilistget(qm->frametimes, i));
            _writeint(qm, ilistget(qm->frameoffsets, i));
            _writeint(qm, ilistget(qm->framesizes, i));
        }
        fseek(qm->outf, 0, SEEK_SET);
        _writeint(qm, qm->magic);
        _writeint(qm, nframes);
        _writeint(qm, qm->duration);
        _writeint(qm, qm->sizex);
        _writeint(qm, qm->sizey);
        fclose(qm->outf);
    } else {
        fclose(qm->inf);
    }
    if(qm->error)
        return 0;
    else
        return 1;
}

void readbenchmark(const char *filename) 
{
    QOImovie *qm = QOImovie_new();
    int t0 = getusec();
    QOImovie_read(qm, filename);
    int nframes = QOImovie_getnframes(qm);
    int totpixels = 0;
    for(int i=0; i<nframes; i++) {
        canvas *c = QOImovie_getframe(qm, i);
        canvas_free(c);
        totpixels += c->sizex*c->sizey;
    }
    QOImovie_close(qm);
    int t1 = getusec();
    int totusec = t1-t0;
    float totMpix = totpixels/(1024*1024);
    fprintf(stderr, "QOImovie benchmark reading [%s]\n", filename);
    fprintf(stderr, "    %d frames with %d x %d pixels\n", nframes, qm->sizex, qm->sizey);
    fprintf(stderr, "    %d usec  %f usec per Mpix\n", totusec, totusec/totMpix);
    QOImovie_free(qm);
}

// test program follows

int main(int argc, char **argv) 
{ 
    if(argc<3) {
        fprintf(stderr, "\nusage: QOImovie -toqoim 00.png 01.png 02.png test.qoim\n\n");
        fprintf(stderr, "usage: QOImovie -topng test.qoim outfamily\n\n");
        fprintf(stderr, "usage: QOImovie -print test.qoim\n\n");
        exit(1);
    }
    if(strcmp(argv[1], "-toqoim") == 0) {
        QOImovie *qm = QOImovie_new();
        QOImovie_write(qm, argv[argc-1]);
        for(int argp = 2; argp<argc-1; argp++) {
            canvas *c = canvas_frompng(argv[argp]);
            QOImovie_putframe(qm, c);
            canvas_free(c);
        }
        QOImovie_close(qm);
        QOImovie_free(qm);
    } else if(strcmp(argv[1], "-topng") == 0) {
        QOImovie *qm = QOImovie_new();
        QOImovie_read(qm, argv[2]);
        for(int frameno = 0; frameno<QOImovie_getnframes(qm); frameno++) {
            char outfname[1024];
            canvas *c = QOImovie_getframe(qm, frameno);
            sprintf(outfname, "%s%03d.png", argv[3], frameno);
            canvas_topng(c, outfname);
            canvas_free(c);
        }
        QOImovie_close(qm);
        QOImovie_free(qm);
    } else if(strcmp(argv[1], "-print") == 0) {
        QOImovie *qm = QOImovie_new();
        QOImovie_read(qm, argv[2]);
        QOImovie_print(qm, "test");
        QOImovie_close(qm);
        QOImovie_free(qm);
    } else {
        fprintf(stderr, "strange option [%s]\n", argv[1]);
        exit(1);
    }
    return 0;
}
