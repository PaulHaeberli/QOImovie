/*

imgproc - process qom movie files

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

*/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"
#define QOM_IMPLEMENTATION
#include "qom.h"
#define IMGPROC_IMPLEMENTATION
#include "imgproc.h"

#define FILT_ZOOM               ( 0)
#define FILT_ZOOMTOSIZE         ( 1)
#define FILT_SATURATE           ( 2)
#define FILT_SHARPEN            ( 3)
#define FILT_SOFTFOCUS          ( 4)
#define FILT_ENLIGHTEN          ( 5)
#define FILT_PERHIST            ( 6)
#define FILT_EXPAND             ( 7)
#define FILT_GAMMAWARP          ( 8)
#define FILT_SCALERGB           ( 9)
#define FILT_CHROMABLUR         (10)
#define FILT_FRAME              (11)
#define FILT_ROUNDCORNERS       (12)
#define FILT_SOFTEDGE           (13)

/* qomfilter */

void qomfilter(gfx_canvas *can_in, int filtmode, float arg1, float arg2, float arg3, float arg4, float arg5) 
{
    gfx_canvas *temp;
    switch(filtmode) {
        case FILT_ZOOM:
            temp = gfx_canvas_zoom(can_in, arg1, arg2);
            gfx_canvas_swap(can_in, temp);
            gfx_canvas_free(temp);
            break;
        case FILT_ZOOMTOSIZE:
            temp = gfx_canvas_zoom_to_size(can_in, arg1, arg2);
            gfx_canvas_swap(can_in, temp);
            gfx_canvas_free(temp);
            break;
        case FILT_SATURATE:
            gfx_canvas_saturate(can_in, arg1);
            break;
        case FILT_SHARPEN:
            gfx_canvas_sharpen(can_in, arg1, arg2);
            break;
        case FILT_SOFTFOCUS:
            gfx_canvas_softfocus(can_in, arg1, arg2);
            break;
        case FILT_ENLIGHTEN:
            temp = gfx_canvas_enlighten(can_in, arg1, arg2);
            gfx_canvas_swap(can_in, temp);
            gfx_canvas_free(temp);
            break;
        case FILT_PERHIST:
            gfx_canvas_perhist(can_in, arg1, arg2);
            break;
        case FILT_EXPAND:
            gfx_canvas_expand(can_in, arg1, arg2);
            break;
        case FILT_GAMMAWARP:
            gfx_canvas_gammawarp(can_in, arg1);
            break;
        case FILT_SCALERGB:
            gfx_canvas_scalergb(can_in, arg1, arg2, arg3);
            break;
        case FILT_CHROMABLUR:
            gfx_canvas_chromablur(can_in, arg1);
            break;
        case FILT_FRAME:
            gfx_canvas_addframe(can_in, arg1, arg2, arg3, arg4, arg5);
            break;
        case FILT_ROUNDCORNERS:
            gfx_canvas_roundcorners(can_in, arg1, arg2);
            break;
        case FILT_SOFTEDGE:
            gfx_canvas_softedge(can_in, arg1);
            break;
    }
}

#define NOARG   (0.0)

void doprocess(gfx_canvas *can_in, int argc, char **argv, int frameno, int nframes)
{
    int movie;
    if(nframes == 0)
        movie = 0;
    else
        movie = 1;
    for(int i=3; i<argc; i++) {
        if(strcmp(argv[i],"zoom") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float zoomx = atof(argv[i]);
            i++;
            float zoomy = atof(argv[i]);
            qomfilter(can_in, FILT_ZOOM, zoomx, zoomy, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"zoomtosize") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            int sizex = atoi(argv[i]);
            i++;
            int sizey = atoi(argv[i]);
            qomfilter(can_in, FILT_ZOOM, sizex, sizey, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"saturate") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                exit(1);
            }
            i++;
            float sat = atof(argv[i]);
            qomfilter(can_in, FILT_SATURATE, sat, NOARG, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"sharpen") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qomfilter(can_in, FILT_SHARPEN, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"softfocus") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qomfilter(can_in, FILT_SOFTFOCUS, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"enlighten") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qomfilter(can_in, FILT_ENLIGHTEN, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"perhist") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float min = atof(argv[i]);
            i++;
            float max = atof(argv[i]);
            qomfilter(can_in, FILT_PERHIST, min, max, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"expand") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float min = atof(argv[i]);
            i++;
            float max = atof(argv[i]);
            qomfilter(can_in, FILT_EXPAND, min, max, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"gammawarp") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                exit(1);
            }
            i++;
            float gamma = atof(argv[i]);
            qomfilter(can_in, FILT_GAMMAWARP, gamma, NOARG, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"scalergb") == 0) {
            if((i+3) >= argc) { 
                fprintf(stderr, "error: %s needs 3 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float scaler = atof(argv[i]);
            i++;
            float scaleg = atof(argv[i]);
            i++;
            float scaleb = atof(argv[i]);
            qomfilter(can_in, FILT_SCALERGB, scaler, scaleg, scaleb, NOARG, NOARG);
        } else if(strcmp(argv[i],"chromablur") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                    exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            qomfilter(can_in, FILT_CHROMABLUR, smalldiam, NOARG, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"frame") == 0) {
            if((i+5) >= argc) { 
                fprintf(stderr, "error: %s needs 5 arguments!\n", argv[i]);
                    exit(1);
            }
            i++;
            float width = atof(argv[i])*gfx_canvas_diameter(can_in);
            if(width<1) width=1;
            i++;
            float r = atof(argv[i]);
            i++;
            float g = atof(argv[i]);
            i++;
            float b = atof(argv[i]);
            i++;
            float a = atof(argv[i]);
            qomfilter(can_in, FILT_FRAME, width, r, g, b, a);
        } else if(strcmp(argv[i],"roundcorners") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                    exit(1);
            }
            i++;
            float radius = atof(argv[i])*gfx_canvas_diameter(can_in);
            i++;
            float exp = atof(argv[i]);
            qomfilter(can_in, FILT_ROUNDCORNERS, radius, exp, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"softedge") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                    exit(1);
            }
            i++;
            float width = atof(argv[i])*gfx_canvas_diameter(can_in);
            qomfilter(can_in, FILT_SOFTEDGE, width, NOARG, NOARG, NOARG, NOARG);
        } else {
            fprintf(stderr,"imgproc: strange option [%s]\n",argv[i]);
            exit(1);
        }
    }
}

int strendswith(const char *buf, const char *suf)
{
    int lbuf, lsuf;

    lbuf = (int)strlen(buf);
    lsuf = (int)strlen(suf);
    if(lbuf<lsuf)
        return 0;
    if(strcasecmp(suf, buf+lbuf-lsuf) == 0)
        return 1;
    return 0;
}

static int isjpegfilename(const char *name)
{
    if(strendswith(name, ".jpg"))
        return 1;
    if(strendswith(name, ".jpeg"))
        return 1;
    return 0;
}

static int ispngfilename(const char *name)
{
    if(strendswith(name, ".png"))
        return 1;
    return 0;
}

static int isqoifilename(const char *name)
{
    if(strendswith(name, ".qoi"))
        return 1;
    return 0;
}

static int isqomfilename(const char *name)
{
    if(strendswith(name, ".qom"))
        return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if(argc<5) {
        fprintf(stderr,"\n");
        fprintf(stderr,"jpg  usage: imgproc in.jpg out.jpg\n");
        fprintf(stderr,"png  usage: imgproc in.png out.png\n");
        fprintf(stderr,"qoi  usage: imgproc in.qoi out.qoi\n");
        fprintf(stderr,"qom usage: imgproc in.qom out.qom\n");
        fprintf(stderr,"\t[zoom xscale yscale]      zoom 1.5 1.5\n");
        fprintf(stderr,"\t[zoomtosize sizex sizey]  zoomtosize 640 480\n");
        fprintf(stderr,"\t[saturate sat]            saturate 1.5\n");
        fprintf(stderr,"\t[sharpen smalldiam mag]   sharpen 30.0 0.5\n");
        fprintf(stderr,"\t[softfocus smalldiam mag] softfocus 10.0 0.5\n");
        fprintf(stderr,"\t[enlighten smalldiam mag] enlighten 20.0 0.9\n");
        fprintf(stderr,"\t[perhist min max]         perhist 0.01 0.99\n");
        fprintf(stderr,"\t[expand min max]          expand 0.2 0.8\n");
        fprintf(stderr,"\t[gammawarp pow]           gammawarp 0.4\n");
        fprintf(stderr,"\t[scalergb r g b]          scalergb 0.9 1.1 1.2\n");
        fprintf(stderr,"\t[chromablur smalldiam]    chromablur 20\n");
        fprintf(stderr,"\t[frame width r g b a]     frame 0.01 0.5 0.5 0.5 1.0\n");
        fprintf(stderr,"\t[roundcorners radius exp] roundcorners 0.05 2.0\n");
        fprintf(stderr,"\t[softedge width]          softedge 0.05\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"ops can be chained like this:\n");
        fprintf(stderr,"\timgproc in.jpg out.jpg zoom 0.5 0.5 saturate 1.5 expand 0.1 0.9\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"you can also process .png images like this:\n");
        fprintf(stderr,"\timgproc in.png out.png zoom 0.5 0.5 saturate 1.5 expand 0.1 0.9\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"you can also process .qoi images like this:\n");
        fprintf(stderr,"\timgproc in.qoi out.qoi zoom 0.5 0.5 saturate 1.5 expand 0.1 0.9\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"you can also process .qom movies like this:\n");
        fprintf(stderr,"\timgproc in.qom out.qom zoom 0.5 0.5 saturate 1.5 expand 0.1 0.9\n");
        fprintf(stderr,"\n");
        exit(1);
    }

    if(isqomfilename(argv[1]) && isqomfilename(argv[2])) {
        qom *qm_in = qom_open(argv[1], "r");
        qom *qm_out = qom_open(argv[2], "w");
        for(int frameno = 0; frameno<qom_getnframes(qm_in); frameno++) {
            int usec;
            gfx_canvas *can_in = qom_getframe(qm_in, frameno, &usec);
            gfx_canvas *temp;
            doprocess(can_in, argc, argv, frameno, qom_getnframes(qm_in));
            qom_putframe(qm_out, can_in, usec);
        }
        qom_close(qm_in);
        qom_close(qm_out);
    } else if(isjpegfilename(argv[1]) && isjpegfilename(argv[2])) {
        gfx_canvas *can_in = gfx_canvas_fromjpeg(argv[1]);
        doprocess(can_in, argc, argv, 0, 0);
        gfx_canvas_tojpeg(can_in, argv[2]);
    } else if(ispngfilename(argv[1]) && ispngfilename(argv[2])) {
        gfx_canvas *can_in = gfx_canvas_frompng(argv[1]);
        doprocess(can_in, argc, argv, 0, 0);
        gfx_canvas_topng(can_in, argv[2]);
    } else if(isqoifilename(argv[1]) && isqoifilename(argv[2])) {
        gfx_canvas *can_in = gfx_canvas_fromqoi(argv[1]);
        doprocess(can_in, argc, argv, 0, 0);
        gfx_canvas_toqoi(can_in, argv[2]);
    } else {
        fprintf(stderr,"imgproc: strange file names\n");
        fprintf(stderr,"input and output files must be of the same type\n");
        fprintf(stderr,".png and .qom files are supported\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.jpg out.jpg zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.png out.png zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.qoi out.qoi zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.qom out.qom zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
    }
    exit(0);
    return 0;
}
