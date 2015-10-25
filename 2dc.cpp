#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
extern "C" {
#include "ppma_io.h"
}
#include "Timer.h"

struct ppm {
  int xsize;   
  int ysize;
  int rgb_max;
  int *r;
  int *g;
  int *b;
};

void trip(const char *message) {
  int err = errno;
  fprintf(stderr, "Error: %s (%d: %s)\n", message, err, strerror(err));
}

static int min(int a, int b) {
  if(a < b) 
    return a; 
  else 
    return b;
}

static int max(int a, int b) {
  if(a > b) 
    return a; 
  else 
    return b;
}

static int lin(int y, int x, int size) {
  return y * size + x;
}

int alloc_img(struct ppm *p) {
  p->r = (int *) malloc(p->xsize * p->ysize * sizeof(int));
  p->g = (int *) malloc(p->xsize * p->ysize * sizeof(int));
  p->b = (int *) malloc(p->xsize * p->ysize * sizeof(int));

  if(!p->r  || !p->g || !p->b) {
    fprintf(stderr, "Failed to allocate memory for image\n");
    return 0;
  }

  return 1;
}

int read_ppm(const char *f, struct ppm *p) {
  FILE *fh;

  fh = fopen(f, "r");
  if(!f) {
    trip("Error: could not open input file");
    return 0;
  }

  ppma_read_header(fh, &p->xsize, &p->ysize, &p->rgb_max);

  if(p->rgb_max > 255) {
    fprintf(stderr, "No support for more than 8 bits per pixel\n");
    return 0;
  }

  if(!alloc_img(p)) return 0;

  ppma_read_data(fh, p->xsize, p->ysize, p->r, p->g, p->b);

  fprintf(stderr, "Read image: %dx%d (rgb_max: %d)\n", p->xsize, p->ysize, p->rgb_max);

  return 1;
}

int write_ppm(char *f, struct ppm *p) {
  if(ppma_write(f, p->xsize, p->ysize, p->r, p->g, p->b)) {
    return 0;
  }

  return 1;
}

int read_convolution_matrix(const char *f, int *n, int **matrix) {
  // read a convolution matrix from file f into nxn array matrix

  FILE *fh;

  fh = fopen(f, "r");

  if(fh == NULL) {
    trip("Error: could not open convolution matrix file");
    return 0;
  }

  /* convolution matrix is a square matrix of size n where n is odd */
  if(fscanf(fh, "%d", n) != 1) {
    fprintf(stderr, "Failed to read matrix size\n");
    return 0;
  }

  if(*n % 2 == 0) {
    fprintf(stderr, "Can only handle square convolution matrix\n");
    return 0;
  }
  
  fprintf(stderr, "Reading %d x %d convolution matrix\n", *n, *n);

  *matrix = (int *) malloc((*n) * (*n) * sizeof(float));
  
  int *mm = *matrix;

  if(!mm) {
    fprintf(stderr, "Failed allocation\n");
    return 0;
  }

  for(int i = 0; i < (*n) * (*n); i++) {
    if(fscanf(fh, "%d", &mm[i]) != 1) {
      fprintf(stderr, "Failed to read matrix element %d\n", i);
      return 0;
    }
  }

  fprintf(stderr, "Done.\n");

  return 1;
}

int convolve(int n, int *cm, struct ppm *img, struct ppm *out) {
	out->xsize = img->xsize;
	out->ysize = img->ysize;

  if(!alloc_img(out)) return 0;
  
	int a;
	int cma = n*n;
	int row[cma]; 
	int column[cma];

	for(a = 0; a < cma; a++){
		row[a] = a / n - n / 2;
		column[a] = a % n - n / 2;
	}

	int block_xsize = n;
  while(img->xsize % block_xsize != 0){
    block_xsize ++;
  }
  int block_ysize = n;
  while(img->ysize % block_ysize != 0){
    block_ysize ++;
  }
	int b; 
  int c; 
  int d; 
  int e; 
  int f;
	

	//for loops for blocks
	for (b = 0; b < (img->xsize/block_xsize); b++){
		for (c = 0; c < (img->ysize/block_ysize); c++){
			//for loops to iterate through each block
			for (d = b*block_xsize; d < (b*block_xsize + block_xsize); d++){
				for (e = c*block_ysize; e < (c*block_ysize + block_ysize); e++){
          int accum_r = 0;
          int accum_g = 0;
          int accum_b = 0;

					//literate through convolution matrix and apply values
					//to pixels 
					for (f = 0; f < cma; f++){

						if(d + row[f] < 0) continue;
						if(e + column[f] < 0) continue;

						if(d + row[f] >= img->ysize) continue;
						if(e + column[f] >= img->xsize) continue;

						accum_r += img->r[lin(d + row[f], e + column[f], img->xsize)] * cm[f];
            accum_g += img->g[lin(d + row[f], e + column[f], img->xsize)] * cm[f];
            accum_b += img->b[lin(d + row[f], e + column[f], img->xsize)] * cm[f];
					}

          out->r[lin(d, e, img->xsize)] = min(max(accum_r, 0), 255);
          out->g[lin(d, e, img->xsize)] = min(max(accum_g, 0), 255); 
          out->b[lin(d, e, img->xsize)] = min(max(accum_b, 0), 255); 
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
  if(argc != 4) {
    fprintf(stderr, "Usage: %s convolution_matrix input_ppm output_ppm\n", argv[0]);
    exit(1);
  }

  int n, *cm;
  struct ppm img, out;

  if(!read_convolution_matrix(argv[1], &n, &cm)) {
    exit(1);
  }  
  
  if(!read_ppm(argv[2], &img)) {
    exit(1);
  }

  /* do any transformations to img here, if necessary */

  ggc::Timer t("convolve");

  t.start();
  if(!convolve(n, cm, &img, &out)) {
    exit(1);
  }
  t.stop();

  printf("Time: %llu ns\n", t.duration());

  /* do any transformations to out here, if necessary */

  if(!write_ppm(argv[3], &out)) {
    exit(1);
  }

  fprintf(stderr, "Done writing file!\n");
}
