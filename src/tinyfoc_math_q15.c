/**
 *  _   _               _____ ___   ____ 
 * | |_(_)_ __  _   _  |  ___/ _ \ / ___|
 * | __| | '_ \| | | | | |_ | | | | |    
 * | |_| | | | | |_| |_|  _|| |_| | |___ 
 *  \__|_|_| |_|\__, (_)_|   \___/ \____| by luwei62wa
 *              |___/    
 *
 * @author    luwei.arca@live.cn
 * @license   SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "tinyfoc_api.h"

#define ATAN1DIV1     (int16_t)8192
#define ATAN1DIV2     (int16_t)4836
#define ATAN1DIV4     (int16_t)2555
#define ATAN1DIV8     (int16_t)1297
#define ATAN1DIV16    (int16_t)651
#define ATAN1DIV32    (int16_t)326
#define ATAN1DIV64    (int16_t)163
#define ATAN1DIV128   (int16_t)81
#define ATAN1DIV256   (int16_t)41
#define ATAN1DIV512   (int16_t)20
#define ATAN1DIV1024  (int16_t)10
#define ATAN1DIV2048  (int16_t)5
#define ATAN1DIV4096  (int16_t)3
#define ATAN1DIV8192  (int16_t)1

static const q15_t sin_tbl_step128[] = {
  /* sin(pi/0x40)*32768=0.049068*32768=1607.8 =0x648 */
  /* 0x00 */0x0000, 0x0648, 0x0c8c, 0x12c8, 0x18f9, 0x1f1a, 0x2528, 0x2b1f,
  /* 0x08 */0x30fb, 0x36ba, 0x3c56, 0x41ce, 0x471c, 0x4c3f, 0x5133, 0x55f5,
  /* 0x10 */0x5a82, 0x5ed7, 0x62f1, 0x66cf, 0x6a6d, 0x6dc9, 0x70e2, 0x73b5,
  /* 0x18 */0x7641, 0x7884, 0x7a7c, 0x7c29, 0x7d89, 0x7e9c, 0x7f61, 0x7fd8,
  /* 0x20 */0x7fff, 0x7fd8, 0x7f61, 0x7e9c, 0x7d89, 0x7c29, 0x7a7c, 0x7884,
  /* 0x28 */0x7641, 0x73b5, 0x70e2, 0x6dc9, 0x6a6d, 0x66cf, 0x62f1, 0x5ed7,
  /* 0x30 */0x5a82, 0x55f5, 0x5133, 0x4c3f, 0x471c, 0x41ce, 0x3c56, 0x36ba,
  /* 0x38 */0x30fb, 0x2b1f, 0x2528, 0x1f1a, 0x18f9, 0x12c8, 0x0c8c, 0x0648,
  /* 0x40 */0x0000, 0xf9b8, 0xf374, 0xed38, 0xe707, 0xe0e6, 0xdad8, 0xd4e1,
  /* 0x48 */0xcf05, 0xc946, 0xc3aa, 0xbe32, 0xb8e4, 0xb3c1, 0xaecd, 0xaa0b,
  /* 0x50 */0xa57e, 0xa129, 0x9d0f, 0x9931, 0x9593, 0x9237, 0x8f1e, 0x8c4b,
  /* 0x58 */0x89bf, 0x877c, 0x8584, 0x83d7, 0x8277, 0x8164, 0x809f, 0x8028,
  /* 0x60 */0x8001, 0x8028, 0x809f, 0x8164, 0x8277, 0x83d7, 0x8584, 0x877c,
  /* 0x68 */0x89bf, 0x8c4b, 0x8f1e, 0x9237, 0x9593, 0x9931, 0x9d0f, 0xa129,
  /* 0x70 */0xa57e, 0xaa0b, 0xaecd, 0xb3c1, 0xb8e4, 0xbe32, 0xc3aa, 0xc946,
  /* 0x78 */0xcf05, 0xd4e1, 0xdad8, 0xe0e6, 0xe707, 0xed38, 0xf374, 0xf9b8,
  /* 0x80 */0x0000, 0x0648, 0x0c8c, 0x12c8, 0x18f9, 0x1f1a, 0x2528, 0x2b1f,
  /* 0x88 */0x30fb, 0x36ba, 0x3c56, 0x41ce, 0x471c, 0x4c3f, 0x5133, 0x55f5,
  /* 0x90 */0x5a82, 0x5ed7, 0x62f1, 0x66cf, 0x6a6d, 0x6dc9, 0x70e2, 0x73b5,
  /* 0x98 */0x7641, 0x7884, 0x7a7c, 0x7c29, 0x7d89, 0x7e9c, 0x7f61, 0x7fd8,
  /* 0x100 */0x7fff
};

/**
 * @desc This function calculates the Sine and Cosine values for a specified angle input.
 * @param [in]  angle - The angle in radius / pi.
 * @param [out] pSinCos - Pointer to Sine and Cosine components of angle.
 * @note
    For integer scaling the Angle is scaled such that 0 <= Angle < 2*pi corresponds
    to 0 <= Ang < 0xFFFF. The resulting Sine and Cosine values are returned scaled to
    -32769 -> 32767 i.e. (0x8000 -> 0x7FFF).
    For Q1.15 scaling the Angle is scaled such that -pi <= Angle < pi corresponds to
    -1 -> 0.9999 i.e. (0x8000 <= Ang < 0x7FFF). The resulting Sine and Cosine values are
    returned scaled to -1 -> 0.9999 i.e. (0x8000 -> 0x7FFF).   
 */
void tinyfoc_sin_cos_q15(q15_t angle, q15_sincos_t *pSinCos)
{
  /*
   * The input range of angle is 0 to 2*pi in radius, and
   * the step  of sin table is 1/128 (2*pi/128 in radius), so
   * angle * 128 to index the sin table.
   * 
   * sin(-pi) = 0:
   *  angle=-1     in PerUnit;
   *  angle=0x8000 in Q15;
   *  0x8000 * 128 = 0x00400000
   *  offset = 0x0000
   *  index  = 0x40
   *  sin_tbl_step128[0x40] = 0
   *
   * sin(pi/2) = 1:
   *  angle=1/2    in PerUnit;
   *  angle=0x4000 in Q15;
   *  0x4000 * 128 = 0x00200000
   *  offset = 0x0000
   *  index  = 0x20
   *  sin_tbl_step128[0x20] = 0x7FFF in Q15
   */
  uint32_t u32Acc = ((uint32_t)angle & 0xFFFF) * 128;
  uint16_t off = u32Acc & 0xFFFF;
  uint16_t idx = u32Acc >> 16;
  q15_t sin, cos;
  int ret;

  if (0 == off) {
    /* Direct lookup. */
    sin = sin_tbl_step128[idx];
    cos = sin_tbl_step128[idx + 0x40/2];
    ret = 1;
  } else {
    /* Linear interpolation */
    int32_t i32Acc, base, delta;
    base = sin_tbl_step128[idx];
    delta = sin_tbl_step128[idx+1] - base;
    i32Acc = delta * (int32_t)off;
    sin = base + (i32Acc >> 16);

    idx += 0x40/2;
    base = sin_tbl_step128[idx];
    delta = sin_tbl_step128[idx+1] - base;
    i32Acc = delta * (int32_t)off;
    cos = base + (i32Acc >> 16);
    ret = 2;
  }
  pSinCos->sin = sin;
  pSinCos->cos = cos;
  //return ret;
  (void)ret;
}

/**
 * @brief  It executes CORDIC algorithm for rotor position extraction from B-emf
 *         alpha and beta
 * @param  wBemf_alfa_est estimated Bemf alpha on the stator reference frame
 *         wBemf_beta_est estimated Bemf beta on the stator reference frame
 * @retval int16_t rotor electrical angle (s16degrees), pupi, i.e. base = PI
 */
q15_t tinyfoc_atan_q15(q15_t x, q15_t y)
{
  int32_t wBemf_alfa_est = x << 16;
  int32_t wBemf_beta_est = y << 16;
  int16_t hAngle;
  int32_t wXi, wYi, wXold;

  /*Determining quadrant*/
  if (wBemf_alfa_est < 0)
  {
    if (wBemf_beta_est < 0)
    {
      /*Quadrant III, add 90 degrees so as to move to quadrant IV*/
      hAngle = 16384;
      wXi = - ( wBemf_beta_est / 2 );
      wYi = wBemf_alfa_est / 2;
    }
    else
    {
      /*Quadrant II, subtract 90 degrees so as to move to quadrant I*/
      hAngle = -16384;
      wXi = wBemf_beta_est / 2;
      wYi = - (wBemf_alfa_est / 2);
    }
  }
  else
  {
    /* Quadrant I or IV*/
    hAngle = 0;
    wXi = wBemf_alfa_est / 2;
    wYi = wBemf_beta_est / 2;
  }
  wXold = wXi;

  /*begin the successive approximation process*/
  /*iteration0*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV1;
    wXi = wXi - wYi;
    wYi = wXold + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV1;
    wXi = wXi + wYi;
    wYi = -wXold + wYi;
  }
  wXold = wXi;

  /*iteration1*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV2;
    wXi = wXi - (wYi / 2);
    wYi = (wXold / 2) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV2;
    wXi = wXi + (wYi / 2);
    wYi = (-wXold / 2) + wYi;
  }
  wXold = wXi;

  /*iteration2*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV4;
    wXi = wXi - (wYi / 4);
    wYi = (wXold / 4) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV4;
    wXi = wXi + (wYi / 4);
    wYi = (-wXold / 4) + wYi;
  }
  wXold = wXi;

  /*iteration3*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV8;
    wXi = wXi - (wYi / 8);
    wYi = (wXold / 8) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV8;
    wXi = wXi + (wYi / 8);
    wYi = (-wXold / 8) + wYi;
  }
  wXold = wXi;

  /*iteration4*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV16;
    wXi = wXi - (wYi / 16);
    wYi = (wXold / 16) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV16;
    wXi = wXi + (wYi / 16);
    wYi = (-wXold / 16) + wYi;
  }
  wXold = wXi;

  /*iteration5*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV32;
    wXi = wXi - (wYi / 32);
    wYi = (wXold / 32) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV32;
    wXi = wXi + (wYi / 32);
    wYi = (-wXold / 32) + wYi;
  }
  wXold = wXi;

  /*iteration6*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV64;
    wXi = wXi - (wYi / 64);
    wYi = (wXold / 64) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV64;
    wXi = wXi + (wYi / 64);
    wYi = (-wXold / 64) + wYi;
  }
  wXold = wXi;

  /*iteration7*/
  if (wYi < 0)
  {
    /*vector is in Quadrant IV*/
    hAngle += ATAN1DIV128;
    wXi = wXi - (wYi / 128);
    wYi = (wXold / 128) + wYi;
  }
  else
  {
    /*vector is in Quadrant I*/
    hAngle -= ATAN1DIV128;
    wXi = wXi + (wYi / 128);
    wYi = (-wXold / 128) + wYi;
  }

  return (-hAngle);

}
