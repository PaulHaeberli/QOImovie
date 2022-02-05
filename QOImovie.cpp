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

int qoiwriteframe(canvas *c, FILE *f)
{
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

canvas *qoireadframe(FILE *f, int offset, int size)
{
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


// 
//  QOImovie
// 
//    QOImovie movie;
//
//   write a movie
//        write(const char *outfilename)
// 	  void putframe(canvas *c)
// 	  void putframe(canvas *c)
// 	  void putframe(canvas *c)
// 	  void close()
//
//   read a movie
//        read(const char *infilename)
// 	  canvas *getframe(int frameno);
// 	  canvas *getframe(int frameno);
// 	  canvas *getframe(int frameno);
//        void close();
//
//   get info
//        int getnframes()
//
//   print
//        read(const char *infilename)
//        void print(const char *label)
//        void close();
//
//   benchmark
// 	  readbenchmark(const char *filename)
//
//
//    QOImovie movie;
//    movie.write("out.qoim");
//    for(int i=0; i<20; i++) {
//	  canvas *c = cannew(200,140);
// 	  movie.putframe(c);
//    }
//    fprintf(stderr, "nframes: %d\n", movie.getnframes());
//    movie.close();
//
//    QOImovie movie;
//    movie.read("out.qoim");
//    fprintf(stderr, "nframes: %d\n", movie.getnframes());
//    for(int i=0; i<movie.getnframes(); i++)
// 	  canvas *c = movie.getframe(i);
//	  canvas_free(c);
//    }
//    movie.close();
//
//
//    File looks like
//
//    int magic   	0x54FE
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
class QOImovie {
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

    QOImovie() {
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
	    fprintf(stderr, "QOImovie: can't open output file [%s]\n", filename);
	    error = 1;
	    return;
	}
        _writeint(0);	// bad magic for now
        _writeint(0);	// nframes for now
        _writeint(0);	// duration for now
    }
    void read(const char *filename) {
	init();
	inf = fopen(filename, "rb");
	if(!inf) {
	    fprintf(stderr, "QOImovie: can't open input file [%s]\n", filename);
	    error = 1;
	    return;
	}
        int val = _readint();
	if(val != magic) {
	    fprintf(stderr, "QOImovie: magic: 0x%x bad magic 0x%x\n", magic, val);
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
	    fprintf(stderr, "QOImovie: _writeint error\n");
	    error = 1;
	    return;
	}
    }
    int _readint() {
	int val;
	int bytes_read = fread(&val, 1, sizeof(int), inf);
	if(bytes_read != sizeof(int)) {
	    fprintf(stderr, "QOImovie: _readint error\n");
	    error = 1;
	    return 0;
	}
	return val;
    }
    int getnframes() {
	return frameoffsets.size();
    }
    canvas *getframe(int n) {
	if(inf) {
	    return qoireadframe(inf, frameoffsets[n], framesizes[n]);
	} else {
	    fprintf(stderr, "QOImovie: can not getframe from movie being written\n");
	    error = 1;
	    return 0;
	}
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
		    fprintf(stderr, "QOImovie: frames must be the same size\n");
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
	    fprintf(stderr, "QOImovie: can't put a frame while reading a movie\n");
	    error = 1;
	}
    }
    void print(const char *label) {
	fprintf(stderr, "\n");
	fprintf(stderr, "QOImovie %s:\n", label);
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
	} else {
	    fclose(inf);
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
	    canvas *c = getframe(i);
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
};

// test 

int main(int argc, char **argv) 
{ 
    class QOImovie movie;
    if(argc<3) {
        fprintf(stderr, "usage: QOImovie -toqoim 00.png 01.png 02.png test.qoim\n\n");
        fprintf(stderr, "usage: QOImovie -topng test.qoim outfamily\n\n");
        fprintf(stderr, "usage: QOImovie -print test.qoim\n\n");
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
	    canvas *c = movie.getframe(frameno);
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
    }
    return 0;
}
