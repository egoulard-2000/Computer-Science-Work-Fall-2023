/*******************************************
 * Solutions for the CS:APP Performance Lab
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
  "Emile Goulard",     /* Full name */
  "u1244855@utah.edu",  /* Email address */
};

/***************
 * COMPLEX KERNEL
 ***************/

/******************************************************
 * Your different versions of the complex kernel go here
 ******************************************************/

/* 
 * naive_complex - The naive baseline version of complex 
 */
char naive_complex_descr[] = "naive_complex: Naive baseline implementation";
void naive_complex(int dim, pixel *src, pixel *dest)
{
  int i, j;

  for(i = 0; i < dim; i++)
    for(j = 0; j < dim; j++)
    {

      dest[RIDX(dim - j - 1, dim - i - 1, dim)].red = ((int)src[RIDX(i, j, dim)].red +
						      (int)src[RIDX(i, j, dim)].green +
						      (int)src[RIDX(i, j, dim)].blue) / 3;
      
      dest[RIDX(dim - j - 1, dim - i - 1, dim)].green = ((int)src[RIDX(i, j, dim)].red +
							(int)src[RIDX(i, j, dim)].green +
							(int)src[RIDX(i, j, dim)].blue) / 3;
      
      dest[RIDX(dim - j - 1, dim - i - 1, dim)].blue = ((int)src[RIDX(i, j, dim)].red +
						       (int)src[RIDX(i, j, dim)].green +
						       (int)src[RIDX(i, j, dim)].blue) / 3;

    }
}

// My Code added 10/2/23
void optimized_complex(int dim, pixel* src, pixel* dest)
{
  int i, j, sub_dimj, sub_dimi, add_rgb, rdix_subs, rdix_ijdim;

  // Prevent Blocking
  int W = 8;
  int ii, jj;

  for(i = 0; i < dim; i+=W)
    for(j = 0; j < dim; j+=W)
      for(ii = i; ii < i+W; ii++)
      {
        sub_dimi = dim - ii;
        for(jj = j; jj < j+W; jj++)
        {
          sub_dimj = dim - jj;
          rdix_subs = RIDX(sub_dimj - 1, sub_dimi - 1, dim);
          rdix_ijdim = RIDX(ii, jj, dim);

          add_rgb = ((int)src[rdix_ijdim].red + (int)src[rdix_ijdim].green + (int)src[rdix_ijdim].blue) / 3;

          dest[rdix_subs].red = add_rgb;
          dest[rdix_subs].green = add_rgb;
          dest[rdix_subs].blue = add_rgb;
        }
      }
}

/* 
 * complex - Your current working version of complex
 * IMPORTANT: This is the version you will be graded on
 */
char complex_descr[] = "complex: Current working version";
void complex(int dim, pixel *src, pixel *dest)
{
  optimized_complex(dim, src, dest);
}

/*********************************************************************
 * register_complex_functions - Register all of your different versions
 *     of the complex kernel with the driver by calling the
 *     add_complex_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_complex_functions() {
  add_complex_function(&complex, complex_descr);
  add_complex_function(&naive_complex, naive_complex_descr);
}


/***************
 * MOTION KERNEL
 **************/

/***************************************************************
 * Various helper functions for the motion kernel
 * You may modify these or add new ones any way you like.
 **************************************************************/


/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
static pixel weighted_combo(int dim, int i, int j, pixel *src) 
{
  int ii, jj;
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  int num_neighbors = 0;
  for(ii=0; ii < 3; ii++)
    for(jj=0; jj < 3; jj++) 
      if ((i + ii < dim) && (j + jj < dim)) 
      {
	num_neighbors++;
	red += (int) src[RIDX(i+ii,j+jj,dim)].red;
	green += (int) src[RIDX(i+ii,j+jj,dim)].green;
	blue += (int) src[RIDX(i+ii,j+jj,dim)].blue;
      }
  
  current_pixel.red = (unsigned short) (red / num_neighbors);
  current_pixel.green = (unsigned short) (green / num_neighbors);
  current_pixel.blue = (unsigned short) (blue / num_neighbors);
  
  return current_pixel;
}

// Code Added 10/2/23
static pixel motion_matrix_3x3_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 9;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim); // col 1
  int index_row_01 = RIDX(i, j + 1, dim); // col 2
  int index_row_02 = RIDX(i, j + 2, dim); // col 3

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim); // col 1
  int index_row_11 = RIDX(i + 1, j + 1, dim); // col 2
  int index_row_12 = RIDX(i + 1, j + 2, dim); // col 3

  // Indices for Row 3
  int index_row_20 = RIDX(i + 2, j, dim); // col 1
  int index_row_21 = RIDX(i + 2, j + 1, dim); // col 2
  int index_row_22 = RIDX(i + 2, j + 2, dim); // col 3

  r += (int)(src[index_row_00].red + src[index_row_01].red + src[index_row_02].red
            + src[index_row_10].red + src[index_row_11].red + src[index_row_12].red
            + src[index_row_20].red + src[index_row_21].red + src[index_row_22].red);

  b += (int)(src[index_row_00].blue + src[index_row_01].blue + src[index_row_02].blue
            + src[index_row_10].blue + src[index_row_11].blue + src[index_row_12].blue
            + src[index_row_20].blue + src[index_row_21].blue + src[index_row_22].blue);

  g += (int)(src[index_row_00].green + src[index_row_01].green + src[index_row_02].green
            + src[index_row_10].green + src[index_row_11].green + src[index_row_12].green
            + src[index_row_20].green + src[index_row_21].green + src[index_row_22].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_2x2_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 4;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim);
  int index_row_01 = RIDX(i, j + 1, dim);

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim);
  int index_row_11 = RIDX(i + 1, j + 1, dim);

  r += (int)(src[index_row_00].red + src[index_row_01].red
            + src[index_row_10].red + src[index_row_11].red);

  b += (int)(src[index_row_00].blue + src[index_row_01].blue
            + src[index_row_10].blue + src[index_row_11].blue);

  g += (int)(src[index_row_00].green + src[index_row_01].green
            + src[index_row_10].green + src[index_row_11].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_2x3_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 6;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim); // col 1
  int index_row_01 = RIDX(i, j + 1, dim); // col2
  int index_row_02 = RIDX(i, j + 2, dim); // col 3

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim); // col 1
  int index_row_11 = RIDX(i + 1, j + 1, dim); // col 2
  int index_row_12 = RIDX(i + 1, j + 2, dim); // col 3

  r += (int)(src[index_row_00].red + src[index_row_01].red + src[index_row_02].red
            + src[index_row_10].red + src[index_row_11].red + src[index_row_12].red);

  b += (int)(src[index_row_00].blue + src[index_row_01].blue + src[index_row_02].blue
            + src[index_row_10].blue + src[index_row_11].blue + src[index_row_12].blue);

  g += (int)(src[index_row_00].green + src[index_row_01].green + src[index_row_02].green
            + src[index_row_10].green + src[index_row_11].green + src[index_row_12].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_3x2_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 6;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim); // col 1
  int index_row_01 = RIDX(i, j + 1, dim); // col 2

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim); // col 1
  int index_row_11 = RIDX(i + 1, j + 1, dim); // col 2

  // Indices for Row 3
  int index_row_20 = RIDX(i + 2, j, dim); // col 1
  int index_row_21 = RIDX(i + 2, j + 1, dim); // col 2

  r += (int)(src[index_row_00].red + src[index_row_01].red
            + src[index_row_10].red + src[index_row_11].red
            + src[index_row_20].red + src[index_row_21].red);

  b += (int)(src[index_row_00].blue + src[index_row_01].blue
            + src[index_row_10].blue + src[index_row_11].blue
            + src[index_row_20].blue + src[index_row_21].blue);

  g += (int)(src[index_row_00].green + src[index_row_01].green
            + src[index_row_10].green + src[index_row_11].green
            + src[index_row_20].green + src[index_row_21].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_3x1_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 3;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim); // col 1

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim); // col 1

  // Indices for Row 3
  int index_row_20 = RIDX(i + 2, j, dim); // col 1

  r += (int)(src[index_row_00].red
            + src[index_row_10].red
            + src[index_row_20].red);

  b += (int)(src[index_row_00].blue
            + src[index_row_10].blue
            + src[index_row_20].blue);

  g += (int)(src[index_row_00].green
            + src[index_row_10].green
            + src[index_row_20].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_1x3_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 3;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j, dim); // col 1
  int index_row_01 = RIDX(i, j + 1, dim); // col 2
  int index_row_02 = RIDX(i, j + 2, dim); // col 3

  r += (int)(src[index_row_00].red + src[index_row_01].red + src[index_row_02].red);

  b += (int)(src[index_row_00].blue + src[index_row_01].blue + src[index_row_02].blue);

  g += (int)(src[index_row_00].green + src[index_row_01].green + src[index_row_02].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_1x2_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 2;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j , dim); // col 1
  int index_row_01 = RIDX(i, j + 1, dim); // col 2

  r += (int)(src[index_row_00].red
            + src[index_row_01].red);

  b += (int)(src[index_row_00].blue
            + src[index_row_01].blue);

  g += (int)(src[index_row_00].green
            + src[index_row_01].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}

static pixel motion_matrix_2x1_weighted(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int num_neighbors = 2;
  int r, g, b;
  r = g = b = 0;

  // Indices for Row 1
  int index_row_00 = RIDX(i, j , dim); // col 1

  // Indices for Row 2
  int index_row_10 = RIDX(i + 1, j, dim); // col 1

  r += (int)(src[index_row_00].red
            + src[index_row_10].red);

  b += (int)(src[index_row_00].blue
            + src[index_row_10].blue);

  g += (int)(src[index_row_00].green
            + src[index_row_10].green);
  
  current_pixel.red = (unsigned short) (r / num_neighbors);
  current_pixel.green = (unsigned short) (g / num_neighbors);
  current_pixel.blue = (unsigned short) (b / num_neighbors);

  return current_pixel;
}


/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/


/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: Naive baseline implementation";
void naive_motion(int dim, pixel *src, pixel *dst) 
{
  int i, j;
    
  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(i, j, dim)] = weighted_combo(dim, i, j, src);
}

// My code 10/2/23
void optimized_motion(int dim, pixel *src, pixel *dst) 
{
  int i, j;
  int index = 0;

  // -------------------- Handle 3x3 motions -------------------- //
  for (i = 0; i < dim - 2; i++)
    for (j = 0; j < dim - 2; j++)
    {
      index = RIDX(i, j, dim);
      dst[index] = motion_matrix_3x3_weighted(dim, i, j, src);
    }

  // -------------------- Handle Last 2 Rows (2x3) -------------------- //
  i = dim - 2;
  for (j = 0; j < dim - 2; j++)
  {
    index = RIDX(i, j, dim);
    dst[index] = motion_matrix_2x3_weighted(dim, i, j, src);
  }

  i = dim - 1;
  for (j = 0; j < dim - 2; j++)
  {
    index = RIDX(i, j, dim);
    dst[index] = motion_matrix_1x3_weighted(dim, i, j, src);
  }

  // -------------------- Handle Last 2 Columns (3x2) -------------------- //
  j = dim - 2;
  for (i = 0; i < dim - 2; i++)
  {
    index = RIDX(i, j, dim);
    dst[index] = motion_matrix_3x2_weighted(dim, i, j, src);
  }

  j = dim - 1;
  for (i = 0; i < dim - 2; i++)
  {
    index = RIDX(i, j, dim);
    dst[index] = motion_matrix_3x1_weighted(dim, i, j, src);
  }

  // -------------------- Handle Bottom Right Corner (2x2) -------------------- //

  // Top Left Corner
  i = dim - 2;
  j = dim - 2;
  index = RIDX(i, j, dim);
  dst[index] = motion_matrix_2x2_weighted(dim, i, j, src);

  // Top Right Corner
  i = dim - 2;
  j = dim - 1;
  index = RIDX(i, j, dim);
  dst[index] = motion_matrix_2x1_weighted(dim, i, j, src);

  // Bottom Left Corner
  i = dim - 1;
  j = dim - 2;
  index = RIDX(i, j, dim);
  dst[index] = motion_matrix_1x2_weighted(dim, i, j, src);

  // Last Corner
  i = dim - 1;
  j = dim - 1;
  index = RIDX(i, j, dim);
  dst[index] = src[index];
}

/*
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void motion(int dim, pixel *src, pixel *dst) 
{
  optimized_motion(dim, src, dst);
}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions() {
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
}
