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
#define QOIM_IMPLEMENTATION
#include "qoim.h"

#define SHIFT_R         (0)
#define SHIFT_G         (8)
#define SHIFT_B         (16)
#define SHIFT_A         (24)

#define RVAL(l)         ((int)(((l)>>SHIFT_R)&0xff))
#define GVAL(l)         ((int)(((l)>>SHIFT_G)&0xff))
#define BVAL(l)         ((int)(((l)>>SHIFT_B)&0xff))
#define AVAL(l)         ((int)(((l)>>SHIFT_A)&0xff))

#define CPACK(r, g, b, a)  (((r)<<SHIFT_R) | ((g)<<SHIFT_G) | ((b)<<SHIFT_B) | ((a)<<SHIFT_A))

// support for reading and writing png images for testing

qoim_canvas *qoim_canvas_frompng(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 0);
    if(!data) {
        fprintf(stderr, "qoim_canvas_frompng: error: problem reading %s\n", filename);
        exit(1);
    }
    return qoim_canvas_new_withdata(sizex, sizey, data);
}

void qoim_canvas_topng(qoim_canvas *in, const char *filename)
{
    stbi_write_png(filename, in->sizex, in->sizey, 4, in->data, 4*in->sizex);
}

// test program follows

int main(int argc, char **argv) 
{ 
    if(argc<3) {
        fprintf(stderr, "\nusage: qoimutil -toqoim 00.png 01.png 02.png test.qoim\n\n");
        fprintf(stderr, "usage: qoimutil -topng test.qoim outfamily\n\n");
        fprintf(stderr, "usage: qoimutil -print test.qoim\n\n");
        exit(1);
    }
    if(strcmp(argv[1], "-toqoim") == 0) {
        qoim *qm = qoim_open(argv[argc-1], "w");
        for(int argp = 2; argp<argc-1; argp++) {
            qoim_canvas *c = qoim_canvas_frompng(argv[argp]);
            qoim_putframe(qm, c);
            qoim_canvas_free(c);
        }
        qoim_close(qm);
    } else if(strcmp(argv[1], "-topng") == 0) {
        qoim *qm = qoim_open(argv[2], "r");
        for(int frameno = 0; frameno<qoim_getnframes(qm); frameno++) {
            char outfname[1024];
            int usec;
            qoim_canvas *c = qoim_getframe(qm, frameno, &usec);
            sprintf(outfname, "%s%03d.png", argv[3], frameno);
            qoim_canvas_topng(c, outfname);
            qoim_canvas_free(c);
        }
        qoim_close(qm);
    } else if(strcmp(argv[1], "-print") == 0) {
        qoim *qm = qoim_open(argv[2], "r");
        qoim_print(qm, "test");
        qoim_close(qm);
    } else {
        fprintf(stderr, "strange option [%s]\n", argv[1]);
        exit(1);
    }
    return 0;
}
