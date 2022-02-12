/*

imgproc.h - process in memory pixels

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


-- Synopsis

// Define `IMGPROC_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define IMGPROC_IMPLEMENTATION
#include "imgproc.h"

//  imgproc
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//

*/


/* -----------------------------------------------------------------------------
Header - Public functions */

#ifndef IMGPROC_H
#define IMGPROC_H

#ifdef __cplusplus
extern "C" {
#endif

gfx_canvas *gfx_canvas_fromqoi(const char *filename);
void gfx_canvas_toqoi(gfx_canvas *in, const char *filename);

gfx_canvas *gfx_canvas_frompng(const char *filename);
void gfx_canvas_topng(gfx_canvas *in, const char *filename);

gfx_canvas *gfx_canvas_fromjpeg(const char *filename);
void gfx_canvas_tojpeg(gfx_canvas *in, const char *filename);

gfx_canvas *gfx_canvas_clone(gfx_canvas *c);

void gfx_canvas_swap(gfx_canvas *a, gfx_canvas *b);

float gfx_canvas_diameter(gfx_canvas *in);

int gfx_canvas_sizecheck(gfx_canvas *c1, gfx_canvas *c2);

gfx_canvas *gfx_canvas_resize(gfx_canvas *in, int sizex, int sizey);

gfx_canvas *gfx_canvas_blur(gfx_canvas *in, float smalldiam);

void gfx_canvas_mix(gfx_canvas *dst, gfx_canvas *src, float factor);

gfx_canvas *gfx_canvas_zoom(gfx_canvas *in, float x, float y);

gfx_canvas *gfx_canvas_zoom_to_size(gfx_canvas *in, float x, float y);

void gfx_canvas_saturate(gfx_canvas *in, float sat);

void gfx_canvas_sharpen(gfx_canvas *in, float smalldiam, float blend);

void gfx_canvas_gammawarp(gfx_canvas *in, float gamma);

void gfx_canvas_softfocus(gfx_canvas *in, float smalldiam, float blend);

gfx_canvas *gfx_canvas_enlighten(gfx_canvas *in, float smalldiam, float param);

void gfx_canvas_expand(gfx_canvas *in, float min, float max);

void gfx_canvas_perhist(gfx_canvas *c, float min, float max);

void gfx_canvas_scalergba(gfx_canvas *in, float scaler, float scaleg, float scaleb, float scalea);

void gfx_canvas_noblack(gfx_canvas *c);

void gfx_canvas_chromablur(gfx_canvas *in, float smalldiam);

void gfx_canvas_addframe(gfx_canvas *in, int width, float r, float g, float b, float a);

void gfx_canvas_roundcorners(gfx_canvas *in, float radius, float exp) ;

void gfx_canvas_softedge(gfx_canvas *c, float width);

void gfx_canvas_set_aspect(gfx_canvas *c, float aspect);

float gfx_smoothstep(float x, float min, float max);

#ifdef __cplusplus
}
#endif
#endif /* IMGPROC_H */

/* -----------------------------------------------------------------------------
Implementation */

#ifdef IMGPROC_IMPLEMENTATION

/* utils */

#define gfx_SHIFT_R         (0)
#define gfx_SHIFT_G         (8)
#define gfx_SHIFT_B         (16)
#define gfx_SHIFT_A         (24)

#define gfx_RVAL(l)                 ((int)(((l)>>gfx_SHIFT_R)&0xff))
#define gfx_GVAL(l)                 ((int)(((l)>>gfx_SHIFT_G)&0xff))
#define gfx_BVAL(l)                 ((int)(((l)>>gfx_SHIFT_B)&0xff))
#define gfx_AVAL(l)                 ((int)(((l)>>gfx_SHIFT_A)&0xff))

#define gfx_CPACK(r,g,b,a)  (((r)<<gfx_SHIFT_R) | ((g)<<gfx_SHIFT_G) | ((b)<<gfx_SHIFT_B) | ((a)<<gfx_SHIFT_A))

#define gfx_RLUM            (0.224)
#define gfx_GLUM            (0.697)
#define gfx_BLUM            (0.079)

#define gfx_RINTLUM         (57)
#define gfx_GINTLUM         (179)
#define gfx_BINTLUM         (20)

#define gfx_ILUM(r,g,b)     ((int)(gfx_RINTLUM*(r)+gfx_GINTLUM*(g)+gfx_BINTLUM*(b))>>8)
#define gfx_LUM(r,g,b)      (gfx_RLUM*(r)+gfx_GLUM*(g)+gfx_BLUM*(b))

#define gfx_DEFGAMMA        (2.2)
#define gfx_DEFINVGAMMA     (1.0/2.2)

float gfx_flerp(float f0, float f1, float p)
{
    if(f0==f1)
        return f0;
    return ((f0*(1.0-p))+(f1*p));
}

int gfx_ilerp(int f0, int a0, int f1, int a1) 
{
    return ((f0)*(a0)+(f1)*(a1))>>8;
}

int gfx_ilerplimit(int v0, int a0, int v1, int a1)
{     
    int pix = (v0*a0+v1*a1)>>8;
    if(pix<0) 
        return 0;
    if(pix>255) 
        return 255;
    return pix;
}

void gfx_canvas_print(gfx_canvas *c, const char *label)
{
    fprintf(stderr, "gfx_canvas %s: sizex: %d sizey: %d\n", label, c->sizex, c->sizey); 
}

gfx_canvas *gfx_canvas_fromqoi(const char *filename)
{
    qoi_desc desc;
    void *pixels = qoi_read(filename, &desc, 0);
    int channels = desc.channels;
    int sizex = desc.width;
    int sizey = desc.height;
    return gfx_canvas_new_withdata(sizex, sizey, pixels);
}

void gfx_canvas_toqoi(gfx_canvas *in, const char *filename)
{
    qoi_desc desc;
    desc.width = in->sizex;
    desc.height = in->sizey;
    desc.channels = 4;
    int encoded = qoi_write(filename, in->data, &desc);
}

gfx_canvas *gfx_canvas_frompng(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 4);
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

gfx_canvas *gfx_canvas_fromjpeg(const char *filename)
{
    int sizex, sizey, n;
    unsigned char *data = stbi_load(filename, &sizex, &sizey, &n, 4);
    if(!data) {
        fprintf(stderr, "gfx_canvas_fromjpeg: error: problem reading %s\n", filename);
        exit(1);
    }
    return gfx_canvas_new_withdata(sizex, sizey, data);
}

void gfx_canvas_tojpeg(gfx_canvas *in, const char *filename)
{
    stbi_write_jpg(filename, in->sizex, in->sizey, 4, in->data, 100);
}

gfx_canvas *gfx_canvas_clone(gfx_canvas *c)
{
    gfx_canvas *cc = gfx_canvas_new(c->sizex, c->sizey);
    memcpy(cc->data, c->data, c->sizex*c->sizey*sizeof(unsigned int));
    return cc;
}

void gfx_canvas_swap(gfx_canvas *a, gfx_canvas *b)
{
    gfx_canvas temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

float gfx_canvas_diameter(gfx_canvas *in)
{
    return sqrt(in->sizex*in->sizex + in->sizey*in->sizey);
}

int gfx_canvas_sizecheck(gfx_canvas *c1, gfx_canvas *c2)
{
    if((c1->sizex != c2->sizex) || (c1->sizey != c2->sizey)) {
        fprintf(stderr, "gfx_canvas_sizecheck failed!\n");
        fprintf(stderr, "can1: %d by %d\n", c1->sizex, c1->sizey);
        fprintf(stderr, "can2: %d by %d\n", c2->sizex, c2->sizey);
        return 0;
    }
    return 1;
}

gfx_canvas *gfx_canvas_resize(gfx_canvas *in, int sizex, int sizey)
{
    gfx_canvas *out = gfx_canvas_new(sizex, sizey);
    stbir_resize_uint8((unsigned char *) in->data,  in->sizex,  in->sizey, 0,
                       (unsigned char *)out->data, out->sizex, out->sizey, 0, 4);
    return out;
}

gfx_canvas *gfx_canvas_blur(gfx_canvas *in, float smalldiam) 
{
    float indiam = gfx_canvas_diameter(in);
    float scaledown = smalldiam/indiam;
    int smallsizex = round(scaledown*in->sizex);
    int smallsizey = round(scaledown*in->sizey);
    gfx_canvas *small = gfx_canvas_resize(in, smallsizex, smallsizey); 
    gfx_canvas *big = gfx_canvas_resize(small, in->sizex, in->sizey);
    gfx_canvas_free(small);
    return big;
}

void gfx_canvas_mix(gfx_canvas *dst, gfx_canvas *src, float factor)
{
    int n;
    unsigned int *dptr, *sptr;
    int a0, a1, ia, pix;

    if(!gfx_canvas_sizecheck(dst, src))
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
            *dptr = gfx_CPACK(gfx_ilerp(gfx_RVAL(dptr[0]), a0, gfx_RVAL(sptr[0]), a1), 
                          gfx_ilerp(gfx_GVAL(dptr[0]), a0, gfx_GVAL(sptr[0]), a1), 
                          gfx_ilerp(gfx_BVAL(dptr[0]), a0, gfx_BVAL(sptr[0]), a1), 
                          gfx_ilerp(gfx_AVAL(dptr[0]), a0, gfx_AVAL(sptr[0]), a1)); 
            dptr++;
            sptr++;
        }
    } else {
        while(n--) {
            *dptr = gfx_CPACK(gfx_ilerplimit(gfx_RVAL(dptr[0]), a0, gfx_RVAL(sptr[0]), a1), 
                          gfx_ilerplimit(gfx_GVAL(dptr[0]), a0, gfx_GVAL(sptr[0]), a1), 
                          gfx_ilerplimit(gfx_BVAL(dptr[0]), a0, gfx_BVAL(sptr[0]), a1), 
                          gfx_ilerplimit(gfx_AVAL(dptr[0]), a0, gfx_AVAL(sptr[0]), a1));
            dptr++;
            sptr++;
        }
    }
}

/* zoom */

gfx_canvas *gfx_canvas_zoom(gfx_canvas *in, float x, float y)
{
    return gfx_canvas_resize(in, round(x * in->sizex), round(y * in->sizey));
}

/* zoom to size */

gfx_canvas *gfx_canvas_zoom_to_size(gfx_canvas *in, float x, float y)
{
    return gfx_canvas_resize(in, x, y);
}

/* saturate */

void gfx_canvas_saturate(gfx_canvas *in, float sat)
{
    unsigned int *lptr = in->data;
    int n = in->sizex*in->sizey;
    int is = round(256.0*sat);
    int a1 = is;
    int a0 = 256-a1;
    if(is == 0) {
        while(n--) {
            int r = gfx_RVAL(*lptr);
            int g = gfx_GVAL(*lptr);
            int b = gfx_BVAL(*lptr);
            int a = gfx_AVAL(*lptr);
            int lum = gfx_ILUM(r, g, b);
            *lptr++ = gfx_CPACK(lum, lum, lum, a);
        }
    } else if((sat>0.0 && sat<1.0)) {
        while(n--) {
            int r = gfx_RVAL(*lptr);
            int g = gfx_GVAL(*lptr);
            int b = gfx_BVAL(*lptr);
            int a = gfx_AVAL(*lptr);
            int lum = gfx_ILUM(r, g, b);
            *lptr++ = gfx_CPACK(gfx_ilerp(lum, a0, r, a1), gfx_ilerp(lum, a0, g, a1), gfx_ilerp(lum, a0, b, a1), a);
        }
    } else {
        while(n--) {
            int r = gfx_RVAL(*lptr);
            int g = gfx_GVAL(*lptr);
            int b = gfx_BVAL(*lptr);
            int a = gfx_AVAL(*lptr);
            int lum = gfx_ILUM(r, g, b);
            *lptr++ = gfx_CPACK(gfx_ilerplimit(lum, a0, r, a1), gfx_ilerplimit(lum, a0, g, a1), gfx_ilerplimit(lum, a0, b, a1), a);
        }
    }
}

/* sharpen */

void gfx_canvas_sharpen(gfx_canvas *in, float smalldiam, float blend)
{
    gfx_canvas *blur = gfx_canvas_blur(in, smalldiam);
    gfx_canvas_mix(in, blur, -blend);
    gfx_canvas_free(blur);
}

/* gammawarp */

void gfx_canvas_gammawarp(gfx_canvas *in, float gamma)
{
    unsigned int *dptr = in->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = round(255.0*pow(gfx_RVAL(*dptr)/255.0, gamma));
        int g = round(255.0*pow(gfx_GVAL(*dptr)/255.0, gamma));
        int b = round(255.0*pow(gfx_BVAL(*dptr)/255.0, gamma));
        int a = gfx_AVAL(*dptr);
        *dptr++ = gfx_CPACK(r, g, b, a);
    }
}

/* softfocus */

void gfx_canvas_softfocus(gfx_canvas *in, float smalldiam, float blend)
{
    gfx_canvas *blur = gfx_canvas_blur(in, smalldiam);
    gfx_canvas_mix(in, blur, blend);
    gfx_canvas_free(blur);
}

/* enlighten */

gfx_canvas *gfx_canvas_maxrgb(gfx_canvas *in)
{
    gfx_canvas *out = gfx_canvas_new(in->sizex, in->sizey);
    unsigned int *iptr = in->data;
    unsigned int *optr = out->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = gfx_RVAL(*iptr);
        int g = gfx_GVAL(*iptr);
        int b = gfx_BVAL(*iptr);
        int a = gfx_AVAL(*iptr);
        int max = r;
        if(max < g) max = g;
        if(max < b) max = b;
        *optr = gfx_CPACK(max, max, max, a);
        iptr++;
        optr++;
    }
    return out;
}

gfx_canvas *gfx_canvas_brighten(gfx_canvas *in, gfx_canvas *maxrgbblur, float param)
{
    float illummin = 1.0/gfx_flerp(1.0, 10.0, param*param);
    float illummax = 1.0/gfx_flerp(1.0, 1.111, param*param);
    gfx_canvas *out = gfx_canvas_new(in->sizex, in->sizey);
    unsigned int *iptr = in->data;
    unsigned int *bptr = maxrgbblur->data;
    unsigned int *optr = out->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        float illum = gfx_RVAL(*bptr)/255.0;
        int r = gfx_RVAL(*iptr);
        int g = gfx_GVAL(*iptr);
        int b = gfx_BVAL(*iptr);
        int a = gfx_AVAL(*iptr);
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
        *optr = gfx_CPACK(r, g, b, a);
        iptr++;
        bptr++;
        optr++;
    }
    return out;
}

gfx_canvas *gfx_canvas_enlighten(gfx_canvas *in, float smalldiam, float param)
{
    // make a b/w image that has max of [r, g, b] of the input
    gfx_canvas *maxrgb = gfx_canvas_maxrgb(in);

    gfx_canvas *maxrgbblur = gfx_canvas_blur(maxrgb, smalldiam);
    gfx_canvas_free(maxrgb);

    // divide the input image by maxrgbblur to brighten the image
    // param is in the range 0.0 .. 1.0 and normally controlled by a slider.
    gfx_canvas *ret = gfx_canvas_brighten(in, maxrgbblur, param);
    gfx_canvas_free(maxrgbblur);
    return ret;
}

/* expand */

void gfx_canvas_apply_tab(gfx_canvas *in, unsigned char *tab)
{
    unsigned int *lptr = in->data;
    int n = in->sizex*in->sizey;
    while(n) {
        *lptr = gfx_CPACK(tab[gfx_RVAL(lptr[0])], tab[gfx_GVAL(lptr[0])], tab[gfx_BVAL(lptr[0])], gfx_AVAL(lptr[0]));
        lptr++;
        n--;
    }
}

void gfx_canvas_expand(gfx_canvas *in, float min, float max)
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
    gfx_canvas_apply_tab(in, tab);
}

/* perhist */

typedef struct gfx_hist {
    int dirty;
    double count[256];
    double dist[256];
    double mean, median, stddev;
    double max, total;
    int maxpos;
    int maxbright;
} gfx_hist;

#define gfx_CHAN_R          (1)
#define gfx_CHAN_G          (2)
#define gfx_CHAN_B          (3)
#define gfx_CHAN_A          (4)
#define gfx_CHAN_RGB        (6)

#define gfx_EPSILON (0.00000000000000000001)
#define gfx_RERRWGT 0.25
#define gfx_GERRWGT 0.50
#define gfx_BERRWGT 0.25

#define gfx_mybzero(a,b)      memset((a),0,(b))

void gfx_histclear(gfx_hist *h)
{
    gfx_mybzero(h->count, 256*sizeof(double));
    gfx_mybzero(h->dist, 256*sizeof(double));
    h->mean = 0.0;
    h->median = 0.0;
    h->stddev = 0.0;
    h->max = 0.0;
    h->total = 0.0;
    h->maxpos = 0;
    h->maxbright = 0;
    h->dirty = 1;
}

gfx_hist *gfx_histnew(void)
{
    gfx_hist *h = (gfx_hist *)malloc(sizeof(gfx_hist));
    gfx_histclear(h);
    return h;
}

void gfx_histfree(gfx_hist *h)
{
    if(!h)
        return;
    free(h);
}

void gfx_histcalc(gfx_hist *h)
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

gfx_hist *gfx_canvas_hist(gfx_canvas *c, int chan)
{
    gfx_hist *h = gfx_histnew();
    unsigned int *lptr = c->data;
    int n = c->sizex * c->sizey;
    double *cptr = h->count;
    double one = 1.0;
    switch(chan) {
        case gfx_CHAN_R:
            while(n--) {
                cptr[gfx_RVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case gfx_CHAN_G:
            while(n--) {
                cptr[gfx_GVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case gfx_CHAN_B:
            while(n--) {
                cptr[gfx_BVAL(*lptr)] += one;
                lptr++;
            }
            break;
        case gfx_CHAN_RGB:
            while(n--) {
                cptr[gfx_RVAL(*lptr)] += one;
                cptr[gfx_GVAL(*lptr)] += one;
                cptr[gfx_BVAL(*lptr)] += one;
                lptr++;
            }
    }
    h->dirty = 1;
    return h;
}

#define gfx_DELPOW3(del)    ((del)*(del)*(del))
#define gfx_DELPOW2(del)    ((del)*(del))
#define gfx_DELPOW1(del)    (del)

double clamperr(gfx_hist *h, int pos, int end)
{
    double err, del;
    int i;

    err = 0.0;
    if(end == 0) {
        for(i=pos-1; i>=0; i--) {
           del = (pos-i)/255.0;
           err += gfx_DELPOW2(del)*h->count[i];
        }
    } else {
        for(i=pos+1; i<=255; i++) {
           del = (i-pos)/255.0;
           err += gfx_DELPOW2(del)*h->count[i];
        }
    }
    return pow(err/h->total, 1.0/2.0);
}

void getminmax(gfx_hist *hr, gfx_hist *hg, gfx_hist *hb, float min, float max, int *imin, int *imax)
{
    double thresh, err;
    int i;

    gfx_histcalc(hr);
    gfx_histcalc(hg);
    gfx_histcalc(hb);
    thresh = min;
    if(thresh<gfx_EPSILON) {
        *imin = 0;
    } else {
        for(i=1; i<256; i++) {
            err = (gfx_RERRWGT*clamperr(hr, i, 0)) + (gfx_GERRWGT*clamperr(hg, i, 0)) + (gfx_BERRWGT*clamperr(hb, i, 0));
            if(err>thresh)
                break;
        }
        *imin = i-1;
    }
    thresh = 1.0-max;
    if(thresh<gfx_EPSILON) {
        *imax = 255;
    } else {
        for(i=254; i>=0; i--) {
            err = (gfx_RERRWGT*clamperr(hr, i, 1)) + (gfx_GERRWGT*clamperr(hg, i, 1)) + (gfx_BERRWGT*clamperr(hb, i, 1));
            if(err>thresh)
                break;
        }
        *imax = i+1;
    }
}

void gfx_canvas_perhistvals(gfx_canvas *c, float min, float max, float *emin, float *emax)
{
    int imin, imax;

    gfx_hist *hr = gfx_canvas_hist(c, gfx_CHAN_R);
    gfx_hist *hg = gfx_canvas_hist(c, gfx_CHAN_G);
    gfx_hist *hb = gfx_canvas_hist(c, gfx_CHAN_B);
    getminmax(hr, hg, hb, min, max, &imin, &imax);
    gfx_histfree(hr);
    gfx_histfree(hg);
    gfx_histfree(hb);

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

void gfx_canvas_perhist(gfx_canvas *c, float min, float max)
{
    float emin, emax;
    gfx_canvas_perhistvals(c, min, max, &emin, &emax);
    gfx_canvas_expand(c, emin, emax);
}

/* scalergb */

void gfx_canvas_scalergba(gfx_canvas *in, float scaler, float scaleg, float scaleb, float scalea)
{
    unsigned int *dptr = in->data;
    int n = in->sizex*in->sizey;
    while(n--) {
        int r = round(gfx_RVAL(*dptr)*scaler);
        int g = round(gfx_GVAL(*dptr)*scaleg);
        int b = round(gfx_BVAL(*dptr)*scaleb);
        int a = round(gfx_AVAL(*dptr)*scaleb);
        if(r>255) r = 255;
        if(g>255) g = 255;
        if(b>255) b = 255;
        if(a>255) a = 255;
        if(r<0) r = 0;
        if(g<0) g = 0;
        if(b<0) b = 0;
        if(a<0) a = 0;
        *dptr++ = gfx_CPACK(r, g, b, a);
    }
}

/* chromablur */

void gfx_noblack(int *r, int *g, int *b)
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

void gfx_canvas_noblack(gfx_canvas *c)
{
    unsigned int *lptr = c->data;
    int n = c->sizex*c->sizey;
    while(n--) {
        int r = gfx_RVAL(*lptr);
        int g = gfx_GVAL(*lptr);
        int b = gfx_BVAL(*lptr);
        int a = gfx_AVAL(*lptr);
        gfx_noblack(&r, &g, &b);
        *lptr++ = gfx_CPACK(r, g, b, a);
    }
}

#define gfx_LINSTEPS        (16*256)
#define gfx_GAMSTEPS        (256)

static unsigned char *gfx_TOGAMTAB;
static short *gfx_RLUMTAB;
static short *gfx_GLUMTAB;
static short *gfx_BLUMTAB;

#define gfx_ILUMLIN(r,g,b)          (gfx_TOGAMTAB[gfx_RLUMTAB[(r)]+gfx_GLUMTAB[(g)]+gfx_BLUMTAB[(b)]])

void gfx_canvas_setlum(gfx_canvas *c, gfx_canvas *l)
{
    float sc;

    if(!gfx_canvas_sizecheck(c, l))
        return;
    if(!gfx_TOGAMTAB) {
        gfx_TOGAMTAB = (unsigned char *)malloc(gfx_LINSTEPS*sizeof(char));
        for(int i=0; i<gfx_LINSTEPS; i++)
            gfx_TOGAMTAB[i] = round(255.0*pow(i/(gfx_LINSTEPS-1.0), gfx_DEFINVGAMMA));
        gfx_RLUMTAB = (short *)malloc(gfx_GAMSTEPS*sizeof(short));
        sc = (gfx_LINSTEPS-1.0)*gfx_RLUM;
        for(int i=0; i<gfx_GAMSTEPS; i++)
            gfx_RLUMTAB[i] = round(sc*pow(i/(gfx_GAMSTEPS-1.0), gfx_DEFGAMMA));
        gfx_GLUMTAB = (short *)malloc(gfx_GAMSTEPS*sizeof(short));
        sc = (gfx_LINSTEPS-1.0)*gfx_GLUM;
        for(int i=0; i<gfx_GAMSTEPS; i++)
            gfx_GLUMTAB[i] = round(sc*pow(i/(gfx_GAMSTEPS-1.0), gfx_DEFGAMMA));
        gfx_BLUMTAB = (short *)malloc(gfx_GAMSTEPS*sizeof(short));
        sc = (gfx_LINSTEPS-1.0)*gfx_BLUM;
        for(int i=0; i<gfx_GAMSTEPS; i++)
            gfx_BLUMTAB[i] = round(sc*pow(i/(gfx_GAMSTEPS-1.0), gfx_DEFGAMMA));
    }

    unsigned int *cptr = c->data;
    unsigned int *lptr = l->data;
    int n = c->sizex*c->sizey;
    while(n--) {
        int r = gfx_RVAL(*cptr);
        int g = gfx_GVAL(*cptr);
        int b = gfx_BVAL(*cptr);
        int a = gfx_AVAL(*cptr);
        gfx_noblack(&r, &g, &b);
        int lum = gfx_ILUMLIN(r, g, b);
        int wantlum = gfx_RVAL(*lptr++);
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
        *cptr++ = gfx_CPACK(r, g, b, a);
    }
}

void gfx_canvas_chromablur(gfx_canvas *in, float smalldiam)
{
    gfx_canvas *lum = gfx_canvas_clone(in);
    gfx_canvas_saturate(lum, 0.0);
    gfx_canvas *temp = gfx_canvas_clone(in);
    gfx_canvas_noblack(temp);
    gfx_canvas *blur = gfx_canvas_blur(temp, smalldiam);
    gfx_canvas_free(temp);
    gfx_canvas_setlum(blur, lum);
    gfx_canvas_free(lum);
    gfx_canvas_swap(blur, in);
    gfx_canvas_free(blur);
}

/* canframe */

typedef struct gfx_Rect {
    int originx;
    int originy;
    int sizex;
    int sizey;
} gfx_Rect;

gfx_Rect gfx_RectOffset(gfx_Rect r, int offsetx, int offsety)
{
    r.originx += offsetx;
    r.originy += offsety;
    return r;
}

gfx_Rect gfx_RectMake(int origx, int origy, int sizex, int sizey) {
    gfx_Rect r;
    r.originx = origx;
    r.originy = origy;
    r.sizex = sizex;
    r.sizey = sizey;
    return r;
}

int gfx_RectMaxX(gfx_Rect r) {
    return r.originx + r.sizex;
}

int gfx_RectMaxY(gfx_Rect r) {
    return r.originy + r.sizey;
}

gfx_Rect gfx_RectInset(gfx_Rect r, int dist)
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

float gfx_RectAspect(gfx_Rect r)
{
    return r.sizex/(float)r.sizey;
}

void gfx_RectPrint(gfx_Rect r, const char *label)
{
    fprintf(stderr, "gfx_Rect %s:  origin: %d %d  size: %d %d\n", label, r.originx, r.originy, r.sizex, r.sizey);
}

gfx_Rect gfx_canvas_Rect(gfx_canvas *c)
{
    return gfx_RectMake(0, 0, c->sizex, c->sizey);
}

float gfx_canvas_aspect(gfx_canvas *c)
{
    return c->sizex/(float)c->sizey;
}

gfx_Rect gfx_RectAspectInside(gfx_Rect r, float aspect)
{
    float raspect = gfx_RectAspect(r);
    if(aspect>raspect) {
        int sizey = round(r.sizex/aspect);
        if(sizey==0) sizey++;
        int orgy = (r.sizey-sizey)/2;
        return gfx_RectMake(0, orgy, r.sizex, sizey);
    } else {
        int sizex = round(r.sizey*aspect);
        if(sizex==0) sizex++;
        int orgx = (r.sizex-sizex)/2;
        return gfx_RectMake(orgx, 0, sizex, r.sizey);
    }
}

float gfx_RectDist(gfx_Rect r, float posx, float posy, float exp)
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

#define gfx_XY_POS_PTR(c, x, y) ((c)->data+(((y)*c->sizex)+(x)))

void gfx_canvas_setrect(gfx_canvas *c, gfx_Rect rect, float r, float g, float b, float a) 
{
    unsigned int *dptr = c->data;
    int ir = r*255.0;
    int ig = g*255.0;
    int ib = b*255.0;
    int ia = a*255.0;
    unsigned int color = gfx_CPACK(ir, ig, ib, ia);
    for(int y=0; y<rect.sizey; y++) {
        dptr = gfx_XY_POS_PTR(c, rect.originx, rect.originy+y);
        for(int x=0; x<rect.sizex; x++)
            *dptr++ = color;
    }
}

void gfx_canvas_setframe(gfx_canvas *c, int sizex, int sizey, gfx_Rect irect, float r, float g, float b, float a)
{
    gfx_canvas_setrect(c, gfx_RectMake(0, 0, irect.originx, sizey), r, g, b, a);
    gfx_canvas_setrect(c, gfx_RectMake(sizex-irect.sizex, 0, irect.sizex, sizey), r, g, b, a);
    gfx_canvas_setrect(c, gfx_RectMake(irect.originx, gfx_RectMaxY(irect), irect.sizex, sizey-irect.sizey), r, g, b, a);
    gfx_canvas_setrect(c, gfx_RectMake(irect.originx, 0, irect.sizex, irect.originy), r, g, b, a);
}

void gfx_canvas_copy_offset(gfx_canvas *scan, gfx_canvas *dcan, int offsetx, int offsety)
{
    unsigned int *sptr = scan->data;
    unsigned int *dptr = dcan->data;
    for(int y=0; y<scan->sizey; y++) {
        dptr = gfx_XY_POS_PTR(dcan, offsetx, offsety+y);
        sptr = gfx_XY_POS_PTR(scan, 0, y);
        for(int x=0; x<scan->sizex; x++)
            *dptr++ = *sptr++;
    }
}

void gfx_canvas_addframe(gfx_canvas *in, int width, float r, float g, float b, float a)
{
    if(width<1)
        return;
    int sizex = in->sizex;
    int sizey = in->sizey;
    gfx_canvas *f = gfx_canvas_new(sizex+2*width, sizey+2*width);

    /* set 4 rects to make the frame */

    gfx_canvas_setframe(f, f->sizex, f->sizey, gfx_RectOffset(gfx_canvas_Rect(in), width, width), r, g, b, a);
    gfx_canvas_copy_offset(in, f, width, width);
    gfx_canvas_swap(f, in);
    gfx_canvas_free(f);
}

/* roundcorners */

void gfx_canvas_roundcorners(gfx_canvas *in, float radius, float exp) 
{
    gfx_Rect inside = gfx_RectInset(gfx_canvas_Rect(in), radius);
    unsigned int *dptr = in->data;
    for(int y=0; y<in->sizey; y++) {
        float posy = y+0.5;
        for(int x=0; x<in->sizex; x++) {
            float posx = x+0.5;
            float dist = gfx_RectDist(inside, posx, posy, exp);
            float alpha = radius-dist;
            if(alpha > 1.0) alpha = 1.0;
            if(alpha < 0.0) alpha = 0.0;
            int r = alpha*gfx_RVAL(*dptr);
            int g = alpha*gfx_GVAL(*dptr);
            int b = alpha*gfx_BVAL(*dptr);
            int a = alpha*gfx_AVAL(*dptr);
            *dptr++ = gfx_CPACK(r, g, b, a);
        }
    }
}

/* softedge */

float gfx_smoothstep(float x, float min, float max) 
{
    if (x <= min) return 0;
    if (x >= max) return 1;
    x = (x - min) / (max - min);
    return x * x * (3 - 2 * x);
}

static float *gfx_softweights(float width, int n)
{
    float *buf = (float *)malloc(n*sizeof(float));
    float *w = buf;
    float max = 0.0;
    for(int x=0; x<n; x++) {
        float p = x+0.5;
        float val = gfx_smoothstep(p, 0.0, width) * gfx_smoothstep(n-p, 0.0, width);
        *w++ = val;
        if(max < val)
            max = val;
    }
    w = buf;
    for(int x=0; x<n; x++) {
        *w = *w/max;
        w++;
    }
    return buf;
}

void gfx_canvas_softedge(gfx_canvas *c, float width)
{
    int sizex = c->sizex;
    int sizey = c->sizey;
    float *wx = gfx_softweights(width, sizex);
    float *wy = gfx_softweights(width, sizey);
    unsigned int *dptr = c->data;
    for(int y=0; y<sizey; y++) {
        for(int x=0; x<sizex; x++) {
            float alpha = wx[x] * wy[y];
            int r = alpha*gfx_RVAL(*dptr);
            int g = alpha*gfx_GVAL(*dptr);
            int b = alpha*gfx_BVAL(*dptr);
            int a = alpha*gfx_AVAL(*dptr);
            *dptr++ = gfx_CPACK(r, g, b, a);
        }
    }
    free(wx);
    free(wy);
}

void gfx_canvas_copyto(gfx_canvas *src, gfx_Rect sr, gfx_canvas *dst)
{
    assert(sr.sizex == dst->sizex);
    assert(sr.sizey == dst->sizey);
    unsigned int *dptr = dst->data;
    for(int y=0; y<sr.sizey; y++) {
        unsigned int *sptr = src->data + (src->sizex*(y+sr.originy) + sr.originx);
        for(int x=0; x<sr.sizex; x++)
            *dptr++ = *sptr++;
    }
}

void gfx_canvas_set_aspect(gfx_canvas *c, float aspect)
{
    gfx_canvas *out;
    gfx_Rect r = gfx_RectAspectInside(gfx_canvas_Rect(c), aspect);
    out = gfx_canvas_new(r.sizex, r.sizey);
    gfx_canvas_copyto(c, r, out);
    gfx_canvas_swap(c, out);
    gfx_canvas_free(out);
}

#endif /* IMGPROC_IMPLEMENTATION */
