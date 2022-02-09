/*

qoimcat - concatenate several .qoim movies

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

Builds on top of Dominic Szablewski's QOI libary to store sequences of images.

*/

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

int main(int argc, char **argv) 
{ 
    if(argc<3) {
        fprintf(stderr, "usage: qoimcat in1.qoim in2.qoim in3.qoim .... out.qoim\n");
        exit(1);
    }
    qoim *qm_out = qoim_open(argv[argc-1], "w");
    for(int argp = 1; argp<argc-1; argp++) {
        qoim *qm_in = qoim_open(argv[argp], "r");
        for(int frameno = 0; frameno < qoim_getnframes(qm_in); frameno++) {
            int usec;
            gfx_canvas *c = qoim_getframe(qm_in, frameno, &usec);
            qoim_putframe(qm_out, c, usec);
            gfx_canvas_free(c);
        }
        qoim_close(qm_in);
    }
    qoim_close(qm_out);
    return 0;
}
