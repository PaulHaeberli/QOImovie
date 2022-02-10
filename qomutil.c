#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "unistd.h"
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"
#define QOM_IMPLEMENTATION
#include "qom.h"

// support for reading and writing png images for testing

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

// test program follows

void qom_trim(qom *qm_in, qom *qm_out, int frame0, int frame1)
{
    int nframes = qom_getnframes(qm_in);
    if(nframes == 0)
        return;
    if(frame0<0) frame0 = 0;
    if(frame1<0) frame1 = 0;
    if(frame0>=nframes) frame0 = nframes-1;
    if(frame1>=nframes) frame0 = nframes-1;
    if(frame1<frame0) {
        int temp = frame1; int frame1 = frame0; int frame0 = temp;
    }
    for(int frameno = 0; frameno<nframes; frameno++) {
        if((frameno>=frame0) && (frameno<=frame1)) {
            int usec;
            gfx_canvas *c = qom_getframe(qm_in, frameno, &usec);
            qom_putframe(qm_out, c, usec);
            gfx_canvas_free(c);
        }
    }
}

static int firsted = 0;

void qom_randseg(qom *qm_in, qom *qm_out, int nframes)
{
    int shift = qom_getnframes(qm_in)-nframes;
    if(!firsted) {
        srandom(getpid());
        firsted = 1;
    }
    int offset = random()%shift;
    int frame0 = offset;
    int frame1 = offset+nframes-1;
    qom_trim(qm_in, qm_out, frame0, frame1);
}

#define DEFAULT_FRAMETIME       ((1000*1000)/30.0)

int main(int argc, char **argv) 
{ 
    if(argc<3) {
        fprintf(stderr, "\nusage: qomutil -toqom 00.png 01.png 02.png out.qom\n\n");
        fprintf(stderr, "usage: qomutil -topng in.qom outfamily\n\n");
        fprintf(stderr, "usage: qomutil -print in.qom\n\n");
        fprintf(stderr, "usage: qomutil -trim in.qom out.qom startframe endframe\n\n");
        fprintf(stderr, "usage: qomutil -benchmark in.qom\n\n");
        exit(1);
    }
    if(strcmp(argv[1], "-toqom") == 0) {
        int usec = 0;
        qom *qm = qom_open(argv[argc-1], "w");
        if(!qm)
            exit(1);
        for(int argp = 2; argp<argc-1; argp++) {
            gfx_canvas *c = gfx_canvas_frompng(argv[argp]);
            qom_putframe(qm, c, usec);
            gfx_canvas_free(c);
            usec += DEFAULT_FRAMETIME;
        }
        qom_close(qm);
    } else if(strcmp(argv[1], "-topng") == 0) {
        qom *qm = qom_open(argv[2], "r");
        if(!qm)
            exit(1);
        for(int frameno = 0; frameno<qom_getnframes(qm); frameno++) {
            char outfname[1024];
            int usec;
            gfx_canvas *c = qom_getframe(qm, frameno, &usec);
            sprintf(outfname, argv[3], frameno);
            gfx_canvas_topng(c, outfname);
            gfx_canvas_free(c);
        }
        qom_close(qm);
    } else if(strcmp(argv[1], "-print") == 0) {
        qom *qm = qom_open(argv[2], "r");
        if(!qm)
            exit(1);
        qom_print(qm, "test");
        qom_close(qm);
    } else if(strcmp(argv[1], "-trim") == 0) {
        qom *qm_in = qom_open(argv[1], "r");
        if(!qm_in)
            exit(1);
        qom *qm_out = qom_open(argv[2], "w");
        if(!qm_out)
            exit(1);
        qom_trim(qm_in, qm_out, atoi(argv[3]), atoi(argv[4]));
        qom_close(qm_out);
        qom_close(qm_in);
    } else if(strcmp(argv[1], "-randseg") == 0) {
        qom *qm_in = qom_open(argv[2], "r");
        if(!qm_in)
            exit(1);
        qom *qm_out = qom_open(argv[3], "w");
        if(!qm_out)
            exit(1);
        qom_randseg(qm_in, qm_out, atoi(argv[4]));
        qom_close(qm_out);
        qom_close(qm_in);
    } else if(strcmp(argv[1], "-benchmark") == 0) {
        qom_readbenchmark(argv[2]);
    } else {
        fprintf(stderr, "strange option [%s]\n", argv[1]);
        exit(1);
    }
    return 0;
}
