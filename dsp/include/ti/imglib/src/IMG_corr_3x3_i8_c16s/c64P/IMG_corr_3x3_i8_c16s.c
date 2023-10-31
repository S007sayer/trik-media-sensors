/* ======================================================================== *
 * IMGLIB -- TI Image and Video Processing Library                          *
 *                                                                          *
 *                                                                          *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/   *
 *                                                                          *
 *                                                                          *
 *  Redistribution and use in source and binary forms, with or without      *
 *  modification, are permitted provided that the following conditions      *
 *  are met:                                                                *
 *                                                                          *
 *    Redistributions of source code must retain the above copyright        *
 *    notice, this list of conditions and the following disclaimer.         *
 *                                                                          *
 *    Redistributions in binary form must reproduce the above copyright     *
 *    notice, this list of conditions and the following disclaimer in the   *
 *    documentation and/or other materials provided with the                *
 *    distribution.                                                         *
 *                                                                          *
 *    Neither the name of Texas Instruments Incorporated nor the names of   *
 *    its contributors may be used to endorse or promote products derived   *
 *    from this software without specific prior written permission.         *
 *                                                                          *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     *
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       *
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   *
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    *
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   *
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    *
 * ======================================================================== */

/* ======================================================================== */
/*  NAME                                                                    */
/*      IMG_corr_3x3_i8_c16s                                                */
/*                                                                          */
/*  USAGE                                                                   */
/*      This routine is C-callable and can be called as:                    */
/*                                                                          */
/*          void IMG_corr_3x3_i8_c16s                                       */
/*          (                                                               */
/*              const unsigned char    *restrict  imgin_ptr,                */
/*                    int              *restrict imgout_ptr,                */
/*                    short                           width,                */
/*                    short                           pitch,                */
/*              const short            *restrict   mask_ptr,                */
/*          )                                                               */
/*                                                                          */
/*      imgin_ptr :  pointer to an input  array of 8 bit pixels             */
/*      imgout_ptr:  pointer to an output array of 32 bit pixels            */
/*      pitch     :   No. of columns in the Input Image.                    */
/*      width     :   No. of output pixels to be calculated.                */
/*      mask_ptr  :  pointer to 16 bit signed filter mask.                  */
/*                                                                          */
/*  DESCRIPTION                                                             */
/*      The correlation kernel accepts 3 rows of 'pitch' input pixels       */
/*      and  produces one row of 'width' output pixels using the input      */
/*      mask of of 3x3. This correlation performs a point by point          */
/*      multiplications of 3x3 mask with the input image. The result of     */
/*      the 9 multiplications are then summed together to produce a         */
/*      32-bit sum  and then stored in an output array. The mask is         */
/*      moved one column at a time, advancing the mask over the entire      */
/*      image until the entire 'width' is covered. Masks are provided as    */
/*      16-bit signed values and the input image pixels xels are            */
/*      provided as 8-bit unsigned values and the output pixels will be     */
/*      32-bit signed. The mask to be correlated is typically part of       */
/*      the input image or another image.                                   */
/*                                                                          */
/*      The natural C implementation has no restrictions. The Optimized     */
/*      and IntrinsiC codes has restrictions as noted in the ASSUMPTIONS    */
/*      below.                                                              */
/*                                                                          */
/*  ASSUMPTIONS                                                             */
/*      1. width:       'width' >= 2 and multiple of 2.                     */
/*      2. pitch:       'pitch' >= 'width'                                  */
/*      3. Input and Output arrays should not overlap.                      */
/*      4. Output array should be double word-aligned.                      */
/*      5. no restrictions on the alignment of Input.                       */
/*      6. mask_ptr should be half-word aligned.                            */
/*                                                                          */
/* ======================================================================== */

#pragma CODE_SECTION(IMG_corr_3x3_i8_c16s, ".text:optimized");

void IMG_corr_3x3_i8_c16s
(
    const unsigned char *restrict  imgin_ptr,
    int                 *restrict imgout_ptr,
    short                              width,
    short                              pitch,
    const          short *restrict  mask_ptr
)
{
  int          i;  
  int          r1;
  int          sum1,       sum2;    
  
  int          mask22,     mask55,     mask88;
    
  double       mask3_0,    mask6_3,    mask8_6;
  double       temp2; 

  long long    temp1;
  long long    row_00;
  const unsigned char   *in,   *in1,   *in2; 
 

  in      = imgin_ptr;
  in1     = in  + pitch;
  in2     = in1 + pitch;
  
  /* ---------------------------------------------------------------------*/
  /*                         read  the mask elements                      */
  /* ---------------------------------------------------------------------*/
  mask3_0 =   _memd8((void *) mask_ptr); 
  mask22  =   _pack2(_hi(mask3_0), _hi(mask3_0));

  mask6_3 =   _memd8((void *) &mask_ptr[3] );
  mask55  =   _pack2(_hi(mask6_3), _hi(mask6_3));

  mask8_6 =   _memd8((void *) &mask_ptr[6] );
  mask88  =   _pack2(_hi(mask8_6), _hi(mask8_6));

  /* -------------------------------------------------------------------- */
  /*  Inform the compiler by _nasserts the following:                     */
  /*      a) The output array is dword aligned                            */
  /*      b) The width is greater than or equal to 2                      */
  /*      c) The width is a multiple of 2.                                */
  /* -------------------------------------------------------------------- */
  _nassert((int)imgout_ptr % 8 == 0);          /* Double-word aligned     */
  _nassert(width           % 2 == 0);
  _nassert(width               >= 2);

  /* -------------------------------------------------------------------- */
  /*  Loop is manually unrolled by 2.                                     */
  /* -------------------------------------------------------------------- */
  #pragma MUST_ITERATE(1, ,1);

  for (i=0; i<width; i+=2) {
    /* ----------------------------------------------------------------- */
    /* zero the accumulators for storing two output pixels in one        */
    /* iteration                                                         */
    /* ----------------------------------------------------------------- */
    sum1   = 0;
    sum2   = 0;

    /* ----------------------------------------------------------------- */
    /* Load 4 pixels that can be used to calculate two output pixels     */
    /* from the 1st row                                                  */
    /* ----------------------------------------------------------------- */
    r1     = _mem4_const((void *) &in[i]);

    /* ----------------------------------------------------------------- */
    /* Convert the 8 bit pixels to 16 bit values so that ddotpl2         */
    /* can be used                                                       */
    /* ----------------------------------------------------------------- */
    row_00 = _itoll(_unpkhu4(r1), _unpklu4(r1));

    /* ----------------------------------------------------------------- */
    /* Perform the partial multiplications of input pixel with corres-   */
    /* ponding mask for the first and second sample outputs              */
    /* ----------------------------------------------------------------- */
    temp1  = _ddotpl2(row_00, _lo(mask3_0));
    temp2  = _mpy2(_hill(row_00), mask22);
  
    /* ----------------------------------------------------------------- */
    /* Calculate sum1 and sum2 which holds the sums for two output       */
    /* pixels                                                            */
    /* ----------------------------------------------------------------- */
    sum1  += (_loll(temp1) + _lo(temp2));                             
    sum2  += (_hill(temp1) + _hi(temp2));  

    /* ----------------------------------------------------------------- */
    /* Load 4 pixels that can be used to calculate two output pixels     */
    /* from the 2nd row                                                  */
    /* ----------------------------------------------------------------- */
    r1     = _mem4_const((void *) &in1[i]);
   
    /* ----------------------------------------------------------------- */
    /* Convert the 8 bit pixels to 16 bit values so that ddotpl2         */
    /* can be used                                                       */
    /* ----------------------------------------------------------------- */
    row_00 = _itoll(_unpkhu4(r1), _unpklu4(r1));

    /* ----------------------------------------------------------------- */
    /* Perform the partial multiplications of input pixel with corre-    */
    /* ponding mask for the first and second sample outputs              */
    /* ----------------------------------------------------------------- */
    temp1  = _ddotpl2(row_00, _lo(mask6_3));
    temp2  = _mpy2(_hill(row_00), mask55);
 
    /* ----------------------------------------------------------------- */
    /* Calculate sum1 and sum2 which holds the sums for two output       */
    /* pixels                                                            */
    /* ----------------------------------------------------------------- */
    sum1  += (_loll(temp1) + _lo(temp2));                             
    sum2  += (_hill(temp1) + _hi(temp2)); 

    /* ----------------------------------------------------------------- */
    /* Load 4 pixels that can be used to calculate two output pixels     */
    /* from the kth row                                                  */
    /* ----------------------------------------------------------------- */
    r1     = _mem4_const((void *) &in2[i]);
    
    /* ----------------------------------------------------------------- */
    /* Convert the 8 bit pixels to 16 bit values so that ddotpl2         */
    /* can be used                                                       */
    /* ----------------------------------------------------------------- */
    row_00 = _itoll(_unpkhu4(r1), _unpklu4(r1));

    /* ----------------------------------------------------------------- */
    /* Perform the partial multiplications of input pixel with corre-    */
    /* ponding mask for the first and second sample outputs              */
    /* ----------------------------------------------------------------- */
    temp1  = _ddotpl2(row_00, _lo(mask8_6));
    temp2  = _mpy2(_hill(row_00), mask88);
 
    /* ----------------------------------------------------------------- */
    /* Calculate sum1 and sum2 which holds the sums for two output       */
    /* pixels and store it as a double word using _amemd8                */
    /* ----------------------------------------------------------------- */
    sum1  += (_loll(temp1) + _lo(temp2));
    sum2  += (_hill(temp1) + _hi(temp2));

    _amemd8((void *) &imgout_ptr[i]) = _itod(sum2, sum1);
  }
}

/* ======================================================================== */
/*  End of file:  IMG_corr_3x3_i8_c16s.c                                    */
/* ======================================================================== */

