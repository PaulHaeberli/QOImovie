/*

imgproc - process qoimmovie files

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
#define QOIM_IMPLEMENTATION
#include "qoim.h"

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

/* utils */

#define SHIFT_R         (0)
#define SHIFT_G         (8)
#define SHIFT_B         (16)
#define SHIFT_A         (24)

#define RVAL(l)                 ((int)(((l)>>SHIFT_R)&0xff))
#define GVAL(l)                 ((int)(((l)>>SHIFT_G)&0xff))
#define BVAL(l)                 ((int)(((l)>>SHIFT_B)&0xff))
#define AVAL(l)                 ((int)(((l)>>SHIFT_A)&0xff))

#define CPACK(r,g,b,a)  (((r)<<SHIFT_R) | ((g)<<SHIFT_G) | ((b)<<SHIFT_B) | ((a)<<SHIFT_A))

#define RLUM            (0.224)
#define GLUM            (0.697)
#define BLUM            (0.079)

#define RINTLUM         (57)
#define GINTLUM         (179)
#define BINTLUM         (20)

#define ILUM(r,g,b)     ((int)(RINTLUM*(r)+GINTLUM*(g)+BINTLUM*(b))>>8)
#define LUM(r,g,b)      (RLUM*(r)+GLUM*(g)+BLUM*(b))

#define DEFGAMMA        (2.2)
#define DEFINVGAMMA     (1.0/2.2)

float flerp(float f0, float f1, float p)
{
    if(f0==f1)
        return f0;
    return ((f0*(1.0-p))+(f1*p));
}

int ilerp(int f0, int a0, int f1, int a1) 
{
    return ((f0)*(a0)+(f1)*(a1))>>8;
}

int ilerplimit(int v0, int a0, int v1, int a1)
{     
    int pix = (v0*a0+v1*a1)>>8;
    if(pix<0) 
        return 0;
    if(pix>255) 
        return 255;
    return pix;
}

qoim_canvas *qoim_canvas_fromqoi(const char *filename)
{
    qoi_desc desc;
    void *pixels = qoi_read(filename, &desc, 0);
    int channels = desc.channels;
    int sizex = desc.width;
    int sizey = desc.height;
    return qoim_canvas_new_withdata(sizex, sizey, pixels);
}

void qoim_canvas_toqoi(qoim_canvas *in, const char *filename)
{
    qoi_desc desc;
    desc.width = in->sizex;
    desc.height = in->sizey;
    desc.channels = 4;
    int encoded = qoi_write(filename, in->data, &desc);
}

qoim_canvas *qoim_canvas_frompng(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 4);
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

qoim_canvas *qoim_canvas_fromjpeg(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 4);
    if(!data) {
        fprintf(stderr, "qoim_canvas_fromjpeg: error: problem reading %s\n", filename);
        exit(1);
    }
    return qoim_canvas_new_withdata(sizex, sizey, data);
}

void qoim_canvas_tojpeg(qoim_canvas *in, const char *filename)
{
    stbi_write_jpg(filename, in->sizex, in->sizey, 4, in->data, 100);
}

qoim_canvas *qoim_canvas_clone(qoim_canvas *c)
{
    qoim_canvas *cc = qoim_canvas_new(c->sizex, c->sizey);
    memcpy(cc->data, c->data, c->sizex*c->sizey*sizeof(unsigned int));
    return cc;
}

void qoim_canvas_swap(qoim_canvas *a, qoim_canvas *b)
{
    qoim_canvas temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

float qoim_canvas_diameter(qoim_canvas *in)
{
    return sqrt(in->sizex*in->sizex + in->sizey*in->sizey);
}

int qoim_canvas_sizecheck(qoim_canvas *c1, qoim_canvas *c2)
{
    if((c1->sizex != c2->sizex) || (c1->sizey != c2->sizey)) {
        fprintf(stderr, "qoim_canvas_sizecheck failed!\n");
        fprintf(stderr, "can1: %d by %d\n", c1->sizex, c1->sizey);
        fprintf(stderr, "can2: %d by %d\n", c2->sizex, c2->sizey);
        return 0;
    }
    return 1;
}

qoim_canvas *qoim_canvas_resize(qoim_canvas *in, int sizex, int sizey)
{
    qoim_canvas *out = qoim_canvas_new(sizex, sizey);
    stbir_resize_uint8((unsigned char *) in->data,  in->sizex,  in->sizey, 0,
                       (unsigned char *)out->data, out->sizex, out->sizey, 0, 4);
    return out;
}

qoim_canvas *qoim_canvas_blur(qoim_canvas *in, float smalldiam) 
{
    float indiam = qoim_canvas_diameter(in);
    float scaledown = smalldiam/indiam;
    int smallsizex = round(scaledown*in->sizex);
    int smallsizey = round(scaledown*in->sizey);
    qoim_canvas *small = qoim_canvas_resize(in, smallsizex, smallsizey); 
    qoim_canvas *big = qoim_canvas_resize(small, in->sizex, in->sizey);
    qoim_canvas_free(small);
    return big;
}

void qoim_canvas_mix(qoim_canvas *dst, qoim_canvas *src, float factor)
{
    int n;
    unsigned int *dptr, *sptr;
    int a0, a1, ia, pix;

    if(!qoim_canvas_sizecheck(dst, src))
        return;
    ia = round(256.0*factor);
    if(ia==0)
        return;
    a1 = ia;
    a0 = 256-a1;
    n = src->sizex * src->sizey;
    dptr = dst->data;
    sptr = src->data;
    if(ia == 256) {
        while(n--) {
            *dptr++ = *sptr++;
        }
    } else if((ia>0) && (ia<255)) {
        while(n--) {
            *dptr = CPACK(ilerp(RVAL(dptr[0]), a0, RVAL(sptr[0]), a1), 
                          ilerp(GVAL(dptr[0]), a0, GVAL(sptr[0]), a1), 
                          ilerp(BVAL(dptr[0]), a0, BVAL(sptr[0]), a1), 
                          ilerp(AVAL(dptr[0]), a0, AVAL(sptr[0]), a1)); 
            dptr++;
            sptr++;
        }
    } else {
        while(n--) {
            *dptr = CPACK(ilerplimit(RVAL(dptr[0]), a0, RVAL(sptr[0]), a1), 
                          ilerplimit(GVAL(dptr[0]), a0, GVAL(sptr[0]), a1), 
                          ilerplimit(BVAL(dptr[0]), a0, BVAL(sptr[0]), a1), 
                          ilerplimit(AVAL(dptr[0]), a0, AVAL(sptr[0]), a1));
            dptr++;
            sptr++;
        }
    }
}

/* zoom */

qoim_canvas *qoim_canvas_zoom(qoim_canvas *in, float x, float y)
{
    return qoim_canvas_resize(in, round(x * in->sizex), round(y * in->sizey));
}

/* zoom to size */

qoim_canvas *qoim_canvas_zoom_to_size(qoim_canvas *in, float x, float y)
{
    return qoim_canvas_resize(in, x, y);
}

/* saturate */

void qoim_canvas_saturate(qoim_canvas *in, float sat)
{
    unsigned int *lptr = in->data;
    int n = in->sizex*in->sizey;
    int is = round(256.0*sat);
    int a1 = is;
    int a0 = 256-a1;
    if(is == 0) {
        while(n--) {
            int r = RVAL(*lptr);
            int g = GVAL(*lptr);
            int b = BVAL(*lptr);
            int a = AVAL(*lptr);
            int lum = ILUM(r, g, b);
            *lptr++ = CPACK(lum, lum, lum, a);
        }
    } else if((sat>0.0 && sat<1.0)) {
        while(n--) {
            int r = RVAL(*lptr);
            int g = GVAL(*lptr);
            int b = BVAL(*lptr);
            int a = AVAL(*lptr);
            int lum = ILUM(r, g, b);
            *lptr++ = CPACK(ilerp(lum, a0, r, a1), ilerp(lum, a0, g, a1), ilerp(lum, a0, b, a1), a);
        }
    } else {
        while(n--) {
            int r = RVAL(*lptr);
            int g = GVAL(*lptr);
            int b = BVAL(*lptr);
            int a = AVAL(*lptr);
            int lum = ILUM(r, g, b);
            *lptr++ = CPACK(ilerplimit(lum, a0, r, a1), ilerplimit(lum, a0, g, a1), ilerplimit(lum, a0, b, a1), a);
        }
    }
}

/* sharpen */

void qoim_canvas_sharpen(qoim_canvas *in, float smalldiam, float blend)
{
    qoim_canvas *blur = qoim_canvas_blur(in, smalldiam);
    qoim_canvas_mix(in, blur, -blend);
    qoim_canvas_free(blur);
}

/* gammawarp */

void qoim_canvas_gammawarp(qoim_canvas *in, float gamma)
{
    unsigned int *dptr = in->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = round(255.0*pow(RVAL(*dptr)/255.0, gamma));
        int g = round(255.0*pow(GVAL(*dptr)/255.0, gamma));
        int b = round(255.0*pow(BVAL(*dptr)/255.0, gamma));
        int a = AVAL(*dptr);
        *dptr++ = CPACK(r, g, b, a);
    }
}

/* softfocus */

void qoim_canvas_softfocus(qoim_canvas *in, float smalldiam, float blend)
{
    qoim_canvas *temp = qoim_canvas_clone(in);
    qoim_canvas_gammawarp(temp, DEFGAMMA);
    qoim_canvas *blur = qoim_canvas_blur(temp, smalldiam);
    qoim_canvas_free(temp);
    qoim_canvas_gammawarp(blur, DEFINVGAMMA);
    qoim_canvas_mix(in, blur, blend);
    qoim_canvas_free(blur);
}

/* enlighten */

qoim_canvas *qoim_canvas_maxrgb(qoim_canvas *in)
{
    qoim_canvas *out = qoim_canvas_new(in->sizex, in->sizey);
    unsigned int *iptr = in->data;
    unsigned int *optr = out->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = RVAL(*iptr);
        int g = GVAL(*iptr);
        int b = BVAL(*iptr);
        int a = AVAL(*iptr);
        int max = r;
        if(max < g) max = g;
        if(max < b) max = b;
        *optr = CPACK(max, max, max, a);
        iptr++;
        optr++;
    }
    return out;
}

qoim_canvas *qoim_canvas_brighten(qoim_canvas *in, qoim_canvas *maxrgbblur, float param)
{
    float illummin = 1.0/flerp(1.0, 10.0, param*param);
    float illummax = 1.0/flerp(1.0, 1.111, param*param);
    qoim_canvas *out = qoim_canvas_new(in->sizex, in->sizey);
    unsigned int *iptr = in->data;
    unsigned int *bptr = maxrgbblur->data;
    unsigned int *optr = out->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        float illum = RVAL(*bptr)/255.0;
        int r = RVAL(*iptr);
        int g = GVAL(*iptr);
        int b = BVAL(*iptr);
        int a = AVAL(*iptr);
        if(illum < illummin)
            illum = illummin;
        if(illum < illummax) {
            float p = illum/illummax;
            float scale = (0.4+p*0.6)*(illummax/illum);
            r = scale*r;
            if(r>255) r = 255;
            g = scale*g;
            if(g>255) g = 255;
            b = scale*b;
            if(b>255) b = 255;
        }
        *optr = CPACK(r, g, b, a);
        iptr++;
        bptr++;
        optr++;
    }
    return out;
}

qoim_canvas *qoim_canvas_enlighten(qoim_canvas *in, float smalldiam, float param)
{
    // make a b/w image that has max of [r, g, b] of the input
    qoim_canvas *maxrgb = qoim_canvas_maxrgb(in);

    qoim_canvas *maxrgbblur = qoim_canvas_blur(maxrgb, smalldiam);
    qoim_canvas_free(maxrgb);

    // divide the input image by maxrgbblur to brighten the image
    // param is in the range 0.0 .. 1.0 and normally controlled by a slider.
    qoim_canvas *ret = qoim_canvas_brighten(in, maxrgbblur, param);
    qoim_canvas_free(maxrgbblur);
    return ret;
}

/* expand */

void qoim_canvas_apply_tab(qoim_canvas *in, unsigned char *tab)
{
    unsigned int *lptr = in->data;
    int n = in->sizex*in->sizey;
    while(n) {
        *lptr = CPACK(tab[RVAL(lptr[0])], tab[GVAL(lptr[0])], tab[BVAL(lptr[0])], AVAL(lptr[0]));
        lptr++;
        n--;
    }
}

void qoim_canvas_expand(qoim_canvas *in, float min, float max)
{
    unsigned char tab[256];

    float delta = max-min;
    if(delta<0.0001)
        delta = 0.0001;
    for(int i=0; i<256; i++) {
        int val = round(255.0*((i/255.0)-min)/delta);
        if(val>255) val = 255;
        if(val<0) val = 0;
        tab[i] = val;
    }
    qoim_canvas_apply_tab(in, tab);
}

/* perhist */

typedef struct hist {
    int dirty;
    double count[256];
    double dist[256];
    double mean, median, stddev;
    double max, total;
    int maxpos;
    int maxbright;
} hist;

#define CHAN_R          (1)
#define CHAN_G          (2)
#define CHAN_B          (3)
#define CHAN_A          (4)
#define CHAN_RGB        (6)

#define EPSILON (0.00000000000000000001)
#define RERRWGT 0.25
#define GERRWGT 0.50
#define BERRWGT 0.25

#define mybzero(a,b)      memset((a),0,(b))

void histclear(hist *h)
{
    mybzero(h->count, 256*sizeof(double));
    mybzero(h->dist, 256*sizeof(double));
    h->mean = 0.0;
    h->median = 0.0;
    h->stddev = 0.0;
    h->max = 0.0;
    h->total = 0.0;
    h->maxpos = 0;
    h->maxbright = 0;
    h->dirty = 1;
}

hist *histnew(void)
{
    hist *h = (hist *)malloc(sizeof(hist));
    histclear(h);
    return h;
}

void histfree(hist *h)
{
    if(!h)
        return;
    free(h);
}

void histcalc(hist *h)
{
    if(h->dirty) {              /* calc max, maxpos and total */
        double max = 0.0;
        int maxpos = 0;
        double total = 0.0;
        double *cptr = h->count;
        for(int i=0; i<256; i++) {
            total += cptr[i];
            if(max < cptr[i]) {
                max = cptr[i];
                maxpos = i;
            }
            if(cptr[i] > 0.0)
                h->maxbright = i;
        }
        h->max = max;
        h->maxpos = maxpos;
        h->total = total;

    /* calc dist */
        double f = 0.0;
        for(int i=0; i<256; i++) {
            h->dist[i] = f/total;
            f += cptr[i];
        }

    /* calc mean, median, stddev */
        cptr = h->count;
        double mean = 0.0;
        for(int i=0; i<256; i++)
            mean += (i/255.0)*cptr[i];
        h->mean = mean/h->total;
        for(int i=0; i<256; i++) {
            if(h->dist[i]>0.5) {
                h->median = i/255.0;
                break;
            }
        }
        double stddev = 0.0;
        for(int i=0; i<256; i++) {
            double dev = i/255.0 - h->mean;
            stddev += (dev*dev)*cptr[i];
        }
        h->stddev = sqrt(stddev/h->total);

    /* mark it clean */
        h->dirty = 0;
    }
}

hist *qoim_canvas_hist(qoim_canvas *c, int chan)
{
    hist *h = histnew();
    unsigned int *lptr = c->data;
    int n = c->sizex * c->sizey;
    double *cptr = h->count;
    double one = 1.0;
    switch(chan) {
        case CHAN_R:
            while(n--) {
                cptr[RVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case CHAN_G:
            while(n--) {
                cptr[GVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case CHAN_B:
            while(n--) {
                cptr[BVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case CHAN_RGB:
            while(n--) {
                cptr[RVAL(*lptr)] += one;
                cptr[GVAL(*lptr)] += one;
                cptr[BVAL(*lptr)] += one;
                lptr++;
            }
    }
    h->dirty = 1;
    return h;
}

#define DELPOW3(del)    ((del)*(del)*(del))
#define DELPOW2(del)    ((del)*(del))
#define DELPOW1(del)    (del)

double clamperr(hist *h, int pos, int end)
{
    double err, del;
    int i;

    err = 0.0;
    if(end == 0) {
        for(i=pos-1; i>=0; i--) {
           del = (pos-i)/255.0;
           err += DELPOW2(del)*h->count[i];
        }
    } else {
        for(i=pos+1; i<=255; i++) {
           del = (i-pos)/255.0;
           err += DELPOW2(del)*h->count[i];
        }
    }
    return pow(err/h->total, 1.0/2.0);
}

void getminmax(hist *hr, hist *hg, hist *hb, float min, float max, int *imin, int *imax)
{
    double thresh, err;
    int i;

    histcalc(hr);
    histcalc(hg);
    histcalc(hb);
    thresh = min;
    if(thresh<EPSILON) {
        *imin = 0;
    } else {
        for(i=1; i<256; i++) {
            err = (RERRWGT*clamperr(hr, i, 0)) + (GERRWGT*clamperr(hg, i, 0)) + (BERRWGT*clamperr(hb, i, 0));
            if(err>thresh)
                break;
        }
        *imin = i-1;
    }
    thresh = 1.0-max;
    if(thresh<EPSILON) {
        *imax = 255;
    } else {
        for(i=254; i>=0; i--) {
            err = (RERRWGT*clamperr(hr, i, 1)) + (GERRWGT*clamperr(hg, i, 1)) + (BERRWGT*clamperr(hb, i, 1));
            if(err>thresh)
                break;
        }
        *imax = i+1;
    }
}

void qoim_canvas_perhistvals(qoim_canvas *c, float min, float max, float *emin, float *emax)
{
    int imin, imax;

    hist *hr = qoim_canvas_hist(c, CHAN_R);
    hist *hg = qoim_canvas_hist(c, CHAN_G);
    hist *hb = qoim_canvas_hist(c, CHAN_B);
    getminmax(hr, hg, hb, min, max, &imin, &imax);
    histfree(hr);
    histfree(hg);
    histfree(hb);

    float maxlo = 0.2;
    float minhi = 0.4;
    maxlo *= 255.0;
    minhi *= 255.0;
    if(imin>maxlo) imin = maxlo;
    if(imax<minhi) imax = minhi;
    if(imax<imin) {
        int t = imax;
        imax = imin;
        imin = t;
    }
    if(imax == imin) {
        if(imax>127)
            imin--;
        else
            imax++;
    }
    *emin = imin/255.0;
    *emax = imax/255.0;
}

void qoim_canvas_perhist(qoim_canvas *c, float min, float max)
{
    float emin, emax;
    qoim_canvas_perhistvals(c, min, max, &emin, &emax);
    qoim_canvas_expand(c, emin, emax);
}

/* scalergb */

void qoim_canvas_scalergb(qoim_canvas *in, float scaler, float scaleg, float scaleb)
{
    unsigned int *dptr = in->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = round(RVAL(*dptr)*scaler);
        int g = round(GVAL(*dptr)*scaleg);
        int b = round(BVAL(*dptr)*scaleb);
        int a = AVAL(*dptr);
        if(r>255) r = 255;
        if(g>255) g = 255;
        if(b>255) b = 255;
        if(r<0) r = 0;
        if(g<0) g = 0;
        if(b<0) b = 0;
        *dptr++ = CPACK(r, g, b, a);
    }
}

/* chromablur */

void noblack(int *r, int *g, int *b)
{
    int i = *r;
    if(*g>i) i = *g;
    if(*b>i) i = *b;
    if(i>0) {
        *r = (255*(*r))/i;
        *g = (255*(*g))/i;
        *b = (255*(*b))/i;
    } else {
        *r = 255;
        *g = 255;
        *b = 255;
    }
}

void qoim_canvas_noblack(qoim_canvas *c)
{
    unsigned int *lptr = c->data;
    int n = c->sizex*c->sizey;
    while(n--) {
        int r = RVAL(*lptr);
        int g = GVAL(*lptr);
        int b = BVAL(*lptr);
        int a = BVAL(*lptr);
        noblack(&r, &g, &b);
        *lptr++ = CPACK(r, g, b, a);
    }
}

#define LINSTEPS        (16*256)
#define GAMSTEPS        (256)

static unsigned char *TOGAMTAB;
static short *RLUMTAB;
static short *GLUMTAB;
static short *BLUMTAB;

#define ILUMLIN(r,g,b)          (TOGAMTAB[RLUMTAB[(r)]+GLUMTAB[(g)]+BLUMTAB[(b)]])

void qoim_canvas_setlum(qoim_canvas *c, qoim_canvas *l)
{
    float sc;

    if(!qoim_canvas_sizecheck(c, l))
        return;
    if(!TOGAMTAB) {
        TOGAMTAB = (unsigned char *)malloc(LINSTEPS*sizeof(char));
        for(int i=0; i<LINSTEPS; i++)
            TOGAMTAB[i] = round(255.0*pow(i/(LINSTEPS-1.0), DEFINVGAMMA));
        RLUMTAB = (short *)malloc(GAMSTEPS*sizeof(short));
        sc = (LINSTEPS-1.0)*RLUM;
        for(int i=0; i<GAMSTEPS; i++)
            RLUMTAB[i] = round(sc*pow(i/(GAMSTEPS-1.0), DEFGAMMA));
        GLUMTAB = (short *)malloc(GAMSTEPS*sizeof(short));
        sc = (LINSTEPS-1.0)*GLUM;
        for(int i=0; i<GAMSTEPS; i++)
            GLUMTAB[i] = round(sc*pow(i/(GAMSTEPS-1.0), DEFGAMMA));
        BLUMTAB = (short *)malloc(GAMSTEPS*sizeof(short));
        sc = (LINSTEPS-1.0)*BLUM;
        for(int i=0; i<GAMSTEPS; i++)
            BLUMTAB[i] = round(sc*pow(i/(GAMSTEPS-1.0), DEFGAMMA));
    }

    unsigned int *cptr = c->data;
    unsigned int *lptr = l->data;
    int n = c->sizex*c->sizey;
    while(n--) {
        int r = RVAL(*cptr);
        int g = GVAL(*cptr);
        int b = BVAL(*cptr);
        int a = BVAL(*cptr);
        noblack(&r, &g, &b);
        int lum = ILUMLIN(r, g, b);
        int wantlum = RVAL(*lptr++);
        if(wantlum<=lum) {
            if(lum>0) {
                r = (r * wantlum)/lum;
                g = (g * wantlum)/lum;
                b = (b * wantlum)/lum;
            } else {
                r = 0;
                g = 0;
                b = 0;
            }
        } else {
            int colorness = 255-wantlum;
            int whiteness = 255*(wantlum-lum);
            int div = 255-lum;
            r = (r*colorness + whiteness)/div;
            g = (g*colorness + whiteness)/div;
            b = (b*colorness + whiteness)/div;
        }
        *cptr++ = CPACK(r, g, b, a);
    }
}

void qoim_canvas_chromablur(qoim_canvas *in, float smalldiam)
{
    qoim_canvas *lum = qoim_canvas_clone(in);
    qoim_canvas_saturate(lum, 0.0);
    qoim_canvas *temp = qoim_canvas_clone(in);
    qoim_canvas_noblack(temp);
    qoim_canvas *blur = qoim_canvas_blur(temp, smalldiam);
    qoim_canvas_free(temp);
    qoim_canvas_setlum(blur, lum);
    qoim_canvas_free(lum);
    qoim_canvas_swap(blur, in);
    qoim_canvas_free(blur);
}

/* canframe */

typedef struct qoimRect {
    int originx;
    int originy;
    int sizex;
    int sizey;
} qoimRect;

qoimRect qoimRectOffset(qoimRect r, int offsetx, int offsety)
{
    r.originx += offsetx;
    r.originy += offsety;
    return r;
}

qoimRect qoimRectMake(int origx, int origy, int sizex, int sizey) {
    qoimRect r;
    r.originx = origx;
    r.originy = origy;
    r.sizex = sizex;
    r.sizey = sizey;
    return r;
}

int qoimRectMaxX(qoimRect r) {
    return r.originx + r.sizex;
}

int qoimRectMaxY(qoimRect r) {
    return r.originy + r.sizey;
}

qoimRect qoimRectInset(qoimRect r, int dist)
{
    if(r.sizex<dist*2)
        dist = r.sizex/2;
    if(r.sizey<dist*2)
        dist = r.sizey/2;
    r.originx += dist;
    r.originy += dist;
    r.sizex -= 2*dist;
    r.sizey -= 2*dist;
    return r;
}

qoimRect qoim_canvas_Rect(qoim_canvas *c)
{
    return qoimRectMake(0, 0, c->sizex, c->sizey);
}

float qoimRectDist(qoimRect r, float posx, float posy, float exp)
{
    float dx = 0;
    if(posx < r.originx)
        dx = r.originx-posx;
    if(posx > r.originx+r.sizex) {
        float d = posx - (r.originx+r.sizex);
        if(dx < d)
            dx = d;
    } 
    float dy = 0;
    if(posy < r.originy)
        dy = r.originy-posy;
    if(posy > r.originy+r.sizey) {
        float d = posy - (r.originy+r.sizey);
        if(dy < d)
            dy = d;
    }
    if((dx == 0) && (dy == 0))
        return 0.0;
    return pow(pow(dx, exp)+pow(dy, exp), 1.0/exp);
}

#define XY_POS_PTR(c, x, y) ((c)->data+(((y)*c->sizex)+(x)))

void qoim_canvas_setrect(qoim_canvas *c, qoimRect rect, float r, float g, float b, float a) 
{
    unsigned int *dptr = c->data;
    int ir = r*255.0;
    int ig = g*255.0;
    int ib = b*255.0;
    int ia = a*255.0;
    unsigned int color = CPACK(ir, ig, ib, ia);
    for(int y=0; y<rect.sizey; y++) {
        dptr = XY_POS_PTR(c, rect.originx, rect.originy+y);
        for(int x=0; x<rect.sizex; x++)
            *dptr++ = color;
    }
}

void qoim_canvas_setframe(qoim_canvas *c, int sizex, int sizey, qoimRect irect, float r, float g, float b, float a)
{
    qoim_canvas_setrect(c, qoimRectMake(0, 0, irect.originx, sizey), r, g, b, a);
    qoim_canvas_setrect(c, qoimRectMake(sizex-irect.sizex, 0, irect.sizex, sizey), r, g, b, a);
    qoim_canvas_setrect(c, qoimRectMake(irect.originx, qoimRectMaxY(irect), irect.sizex, sizey-irect.sizey), r, g, b, a);
    qoim_canvas_setrect(c, qoimRectMake(irect.originx, 0, irect.sizex, irect.originy), r, g, b, a);
}

void qoim_canvas_copy_offset(qoim_canvas *scan, qoim_canvas *dcan, int offsetx, int offsety)
{
    unsigned int *sptr = scan->data;
    unsigned int *dptr = dcan->data;
    for(int y=0; y<scan->sizey; y++) {
        dptr = XY_POS_PTR(dcan, offsetx, offsety+y);
        sptr = XY_POS_PTR(scan, 0, y);
        for(int x=0; x<scan->sizex; x++)
            *dptr++ = *sptr++;
    }
}

void qoim_canvas_addframe(qoim_canvas *in, int width, float r, float g, float b, float a)
{
    if(width<1)
        return;
    int sizex = in->sizex;
    int sizey = in->sizey;
    qoim_canvas *f = qoim_canvas_new(sizex+2*width, sizey+2*width);

    /* set 4 rects to make the frame */

    qoim_canvas_setframe(f, f->sizex, f->sizey, qoimRectOffset(qoim_canvas_Rect(in), width, width), r, g, b, a);
    qoim_canvas_copy_offset(in, f, width, width);
    qoim_canvas_swap(f, in);
    qoim_canvas_free(f);
}

/* roundcorners */

void qoim_canvas_roundcorners(qoim_canvas *in, float radius, float exp) 
{
    qoimRect inside = qoimRectInset(qoim_canvas_Rect(in), radius);
    unsigned int *dptr = in->data;
    for(int y=0; y<in->sizey; y++) {
        float posy = y+0.5;
        for(int x=0; x<in->sizex; x++) {
            float posx = x+0.5;
            float dist = qoimRectDist(inside, posx, posy, exp);
            float alpha = radius-dist;
            if(alpha > 1.0) alpha = 1.0;
            if(alpha < 0.0) alpha = 0.0;
            int r = alpha*RVAL(*dptr);
            int g = alpha*GVAL(*dptr);
            int b = alpha*BVAL(*dptr);
            int a = alpha*AVAL(*dptr);
            *dptr++ = CPACK(r, g, b, a);
        }
    }
}

/* softedge */

static float qoim_smoothstep(float x, float min, float max) 
{
    if (x <= min) return 0;
    if (x >= max) return 1;
    x = (x - min) / (max - min);
    return x * x * (3 - 2 * x);
}

static float *qoim_softweights(float width, int n)
{
    float *buf = (float *)malloc(n*sizeof(float));
    float *w = buf;
    for(int x=0; x<n; x++) {
        float p = x+0.5;
        *w++ = qoim_smoothstep(x, 0.0, width)  * qoim_smoothstep(n-x, 0.0, width);
    }
    return buf;
}

void qoim_canvas_softedge(qoim_canvas *c, float width)
{
    int sizex = c->sizex;
    int sizey = c->sizey;
    float *wx = qoim_softweights(width, sizex);
    float *wy = qoim_softweights(width, sizex);
    unsigned int *dptr = c->data;
    for(int y=0; y<sizey; y++) {
        for(int x=0; x<sizex; x++) {
            float alpha = wx[x]*wy[y];
            int r = alpha*RVAL(*dptr);
            int g = alpha*GVAL(*dptr);
            int b = alpha*BVAL(*dptr);
            int a = alpha*AVAL(*dptr);
            *dptr++ = CPACK(r, g, b, a);
        }
    }
    free(wx);
    free(wy);
}



/* qoimfilter */

void qoimfilter(qoim_canvas *can_in, int filtmode, float arg1, float arg2, float arg3, float arg4, float arg5) 
{
    qoim_canvas *temp;
    switch(filtmode) {
        case FILT_ZOOM:
            temp = qoim_canvas_zoom(can_in, arg1, arg2);
            qoim_canvas_swap(can_in, temp);
            qoim_canvas_free(temp);
            break;
        case FILT_ZOOMTOSIZE:
            temp = qoim_canvas_zoom_to_size(can_in, arg1, arg2);
            qoim_canvas_swap(can_in, temp);
            qoim_canvas_free(temp);
            break;
        case FILT_SATURATE:
            qoim_canvas_saturate(can_in, arg1);
            break;
        case FILT_SHARPEN:
            qoim_canvas_sharpen(can_in, arg1, arg2);
            break;
        case FILT_SOFTFOCUS:
            qoim_canvas_softfocus(can_in, arg1, arg2);
            break;
        case FILT_ENLIGHTEN:
            temp = qoim_canvas_enlighten(can_in, arg1, arg2);
            qoim_canvas_swap(can_in, temp);
            qoim_canvas_free(temp);
            break;
        case FILT_PERHIST:
            qoim_canvas_perhist(can_in, arg1, arg2);
            break;
        case FILT_EXPAND:
            qoim_canvas_expand(can_in, arg1, arg2);
            break;
        case FILT_GAMMAWARP:
            qoim_canvas_gammawarp(can_in, arg1);
            break;
        case FILT_SCALERGB:
            qoim_canvas_scalergb(can_in, arg1, arg2, arg3);
            break;
        case FILT_CHROMABLUR:
            qoim_canvas_chromablur(can_in, arg1);
            break;
        case FILT_FRAME:
            qoim_canvas_addframe(can_in, arg1, arg2, arg3, arg4, arg5);
            break;
        case FILT_ROUNDCORNERS:
            qoim_canvas_roundcorners(can_in, arg1, arg2);
            break;
        case FILT_SOFTEDGE:
            qoim_canvas_softedge(can_in, arg1);
            break;
    }
}

#define NOARG   (0.0)

void doprocess(qoim_canvas *can_in, int argc, char **argv)
{
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
            qoimfilter(can_in, FILT_ZOOM, zoomx, zoomy, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"zoomtosize") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            int sizex = atoi(argv[i]);
            i++;
            int sizey = atoi(argv[i]);
            qoimfilter(can_in, FILT_ZOOM, sizex, sizey, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"saturate") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                exit(1);
            }
            i++;
            float sat = atof(argv[i]);
            qoimfilter(can_in, FILT_SATURATE, sat, NOARG, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"sharpen") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qoimfilter(can_in, FILT_SHARPEN, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"softfocus") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qoimfilter(can_in, FILT_SOFTFOCUS, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"enlighten") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            i++;
            float mag = atof(argv[i]);
            qoimfilter(can_in, FILT_ENLIGHTEN, smalldiam, mag, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"perhist") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float min = atof(argv[i]);
            i++;
            float max = atof(argv[i]);
            qoimfilter(can_in, FILT_PERHIST, min, max, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"expand") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                exit(1);
            }
            i++;
            float min = atof(argv[i]);
            i++;
            float max = atof(argv[i]);
            qoimfilter(can_in, FILT_EXPAND, min, max, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"gammawarp") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                exit(1);
            }
            i++;
            float gamma = atof(argv[i]);
            qoimfilter(can_in, FILT_GAMMAWARP, gamma, NOARG, NOARG, NOARG, NOARG);
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
            qoimfilter(can_in, FILT_SCALERGB, scaler, scaleg, scaleb, NOARG, NOARG);
        } else if(strcmp(argv[i],"chromablur") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                    exit(1);
            }
            i++;
            float smalldiam = atof(argv[i]);
            qoimfilter(can_in, FILT_CHROMABLUR, smalldiam, NOARG, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"frame") == 0) {
            if((i+5) >= argc) { 
                fprintf(stderr, "error: %s needs 5 arguments!\n", argv[i]);
                    exit(1);
            }
            i++;
            float width = atof(argv[i])*qoim_canvas_diameter(can_in);
            if(width<1) width=1;
            i++;
            float r = atof(argv[i]);
            i++;
            float g = atof(argv[i]);
            i++;
            float b = atof(argv[i]);
            i++;
            float a = atof(argv[i]);
            qoimfilter(can_in, FILT_FRAME, width, r, g, b, a);
        } else if(strcmp(argv[i],"roundcorners") == 0) {
            if((i+2) >= argc) { 
                fprintf(stderr, "error: %s needs 2 arguments!\n", argv[i]);
                    exit(1);
            }
            i++;
            float radius = atof(argv[i])*qoim_canvas_diameter(can_in);
            i++;
            float exp = atof(argv[i]);
            qoimfilter(can_in, FILT_ROUNDCORNERS, radius, exp, NOARG, NOARG, NOARG);
        } else if(strcmp(argv[i],"softedge") == 0) {
            if((i+1) >= argc) { 
                fprintf(stderr, "error: %s needs 1 argument!\n", argv[i]);
                    exit(1);
            }
            i++;
            float width = atof(argv[i])*qoim_canvas_diameter(can_in);
            qoimfilter(can_in, FILT_SOFTEDGE, width, NOARG, NOARG, NOARG, NOARG);
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

static int isqoimfilename(const char *name)
{
    if(strendswith(name, ".qoim"))
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
        fprintf(stderr,"qoim usage: imgproc in.qoim out.qoim\n");
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
        fprintf(stderr,"you can also process .qoim movies like this:\n");
        fprintf(stderr,"\timgproc in.qoim out.qoim zoom 0.5 0.5 saturate 1.5 expand 0.1 0.9\n");
        fprintf(stderr,"\n");
        exit(1);
    }

    if(isqoimfilename(argv[1]) && isqoimfilename(argv[2])) {
        qoim *qm_in = qoim_open(argv[1], "r");
        qoim *qm_out = qoim_open(argv[2], "w");
        for(int frameno = 0; frameno<qoim_getnframes(qm_in); frameno++) {
            int usec;
            qoim_canvas *can_in = qoim_getframe(qm_in, frameno, &usec);
            qoim_canvas *temp;
            doprocess(can_in, argc, argv);
            qoim_putframe(qm_out, can_in, usec);
        }
        qoim_close(qm_in);
        qoim_close(qm_out);
    } else if(isjpegfilename(argv[1]) && isjpegfilename(argv[2])) {
        qoim_canvas *can_in = qoim_canvas_fromjpeg(argv[1]);
        doprocess(can_in, argc, argv);
        qoim_canvas_tojpeg(can_in, argv[2]);
    } else if(ispngfilename(argv[1]) && ispngfilename(argv[2])) {
        qoim_canvas *can_in = qoim_canvas_frompng(argv[1]);
        doprocess(can_in, argc, argv);
        qoim_canvas_topng(can_in, argv[2]);
    } else if(isqoifilename(argv[1]) && isqoifilename(argv[2])) {
        qoim_canvas *can_in = qoim_canvas_fromqoi(argv[1]);
        doprocess(can_in, argc, argv);
        qoim_canvas_toqoi(can_in, argv[2]);
    } else {
        fprintf(stderr,"imgproc: strange file names\n");
        fprintf(stderr,"input and output files must be of the same type\n");
        fprintf(stderr,".png and .qoim files are supported\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.jpg out.jpg zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.png out.png zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.qoi out.qoi zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\timgproc in.qoim out.qoim zoom 0.5 0.5 perhist 0.01 0.99 enlighten 20.0 0.7 saturate 1.2\n");
        fprintf(stderr,"\n");
    }
    exit(0);
    return 0;
}
