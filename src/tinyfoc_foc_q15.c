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

#include <stdlib.h>
#include "tinyfoc_api.h"

/* Excess gain co-efficient term */
#define PI_K_EXCESS           Q15(0.999)

/* 过渡期间, 电角度渐变的步长。单位degree. */
#define ANGLE_TRANS_STEP      0.05
/* 过渡期间, 电角度渐变的步长的标幺值。单位rad/pi */
#define ANGLE_TRANS_STEP_NORM Q15(ANGLE_TRANS_STEP/180)

extern void tinyfoc_sin_cos_q15(q15_t angle, q15_sincos_t *pSinCos);

/**
 * @desc Clark transform from an a-b-c coordinate to an alpha-beta coordinate.
 * @param [in]  pABC - Pointer to a-b-c reference frame.
 * @param [out] pAlphaBeta - Pointer to alpha-beta reference frame.
 * @note The input & output must be consistent in any Q format.
 *
 * @code
 *  alpha = a
 *  beta  = (a+2*b) / sqrt(3)
 *   1/sqrt(3) *32768 = 0.577350 *32768 = 18918.6 ~ 0x49e7
 */
void tinyfoc_clarke_q15(const q15_abc_t *pABC, q15_ab_t *pAlphaBeta)
{
  /* Needn't int64_t but need q15_add: 0x7fff*0x49e7*2*3=0xddb3 4496 */
  int32_t a = (int32_t)pABC->a;
  int32_t b = (int32_t)pABC->b;
  pAlphaBeta->alpha = pABC->a;
  /* trick: >>14 rather than >>15, otherwise 0x49e7 *2 is overflow in Q15. */
  b = (b * 0x49e7) >> 14;
  a = (a * (0x49e7/2)) >> 14;
  pAlphaBeta->beta  = __QADD16(a, b);
}

/**
 * @desc Clarke inverse transform from an alpha-beta coordinate to an a-b-c coordinate.
 * @param [in]  pAlphaBeta - Pointer to alpha-beta reference frame.
 * @param [out] pABC - Pointer to a-b-c reference frame.
 * @note The input & output must be consistent in any Q format.
 *
 * @code
    a = alpha
    b = (-alpha + sqrt(3) * beta) / 2
    c = (-alpha - sqrt(3) * beta) / 2
     sqrt(3)/2 *32768 = 28377.9 ~ 0x6eda
 */
void tinyfoc_inv_clarke_q15(const q15_ab_t *pAlphaBeta, q15_abc_t *pABC)
{
  /* Needn't int64_t but need q15_add/sub: 0x7fff*0x6eda*2+0x3fff0000=0xaed8 224c */
  int32_t alpha = (int32_t)pAlphaBeta->alpha;
  int32_t beta  = (int32_t)pAlphaBeta->beta;
  pABC->a = alpha;
  alpha  = (alpha * 0xc000/*-1/2*/) >> 15;
  beta = (beta * 0x6eda) >> 15;
  pABC->b = __QADD16(beta, alpha);
  pABC->c = __QSUB16(beta, alpha);
}

/**
 * @desc Clarke inverse transform (var.) from an alpha-beta coordinate to an a-b-c coordinate.
 * @param [in]  pAlphaBeta - Pointer to alpha-beta reference frame.
 * @param [out] pABC - Pointer to a-b-c reference frame.
 * @note The input & output must be consistent in any Q format.
 * @note 与标准Clarke逆变换比较，alpha和beta交换了一下位置，以匹配SVPWM的生成.
 *
 * @code
    a = beta
    b = (-beta + sqrt(3) * alpha) / 2
    c = (-beta - sqrt(3) * alpha) / 2
     sqrt(3)/2 *32768 = 28377.9 ~ 0x6eda
 */
void tinyfoc_inv_clarke_swap_q15(const q15_ab_t *pAlphaBeta, q15_abc_t *pABC)
{
  /* Needn't int64_t but need q15_add/sub: 0x7fff*0x6eda*2+0x3fff0000=0xaed8 224c */
  int32_t alpha = (int32_t)pAlphaBeta->alpha;
  int32_t beta  = (int32_t)pAlphaBeta->beta;
  pABC->a = beta;
  /* Bug: beta * 0xc000 */
  beta  = (-beta * 0x4000/*1/2*/) >> 15;
  alpha = (alpha * 0x6eda) >> 15;
  pABC->b = __QADD16(beta, alpha);
  pABC->c = __QSUB16(beta, alpha);
}

/**
 * @desc Park transform from an alpha-beta coordinate to a stationary d-q coordinate.
 * @param [in]  pAlphaBeta - Pointer to alpha-beta reference frame.
 * @param [in]  pSinCos - Pointer to Sine and Cosine components of angle.
 * @param [out] pDQ - Pointer to d-q reference frame.
 * @note The input & output must be consistent in any Q format.
 *
 * @code
    d = alpha*cos + beta*sin
    q = -alpha*sin + beta*cos
 */
void tinyfoc_park_q15(const q15_ab_t *pAlphaBeta, const q15_sincos_t *pSinCos, q15_dq_t *pDQ)
{
  /* Needn't int64_t: 0x7fff*0x7fff*2=0x7ffe0002 */
  int32_t alpha = pAlphaBeta->alpha;
  int32_t beta  = pAlphaBeta->beta;
  q15_t sin   = pSinCos->sin;
  q15_t cos   = pSinCos->cos;
  int32_t m1, m2, m3, m4;
  m1 = (beta  * sin) >> 15;
  m2 = (alpha * cos) >> 15;
  m3 = (beta  * cos) >> 15;
  m4 = (alpha * sin) >> 15;
  pDQ->d = __QADD16(m1, m2);
  pDQ->q = __QSUB16(m3, m4);
}

/**
 * @desc Park inverse transform from a stationary d-q coordinate to an alpha-beta coordinate.
 * @param [in]  pDQ - Pointer to d-q reference frame.
 * @param [in]  pSinCos - Pointer to Sine and Cosine components of angle.
 * @param [out] pAlphaBeta - Pointer to alpha-beta reference frame.
 * @note The input & output must be consistent in any Q format.

 * @code
    alpha = d*cos - q*sin
    beta = d*sin + q*cos
 */
void tinyfoc_inv_park_q15(const q15_dq_t *pDQ, const q15_sincos_t *pSinCos, q15_ab_t *pAlphaBeta)
{
  int32_t d     = pDQ->d;
  int32_t q     = pDQ->q;
  q15_t sin   = pSinCos->sin;
  q15_t cos   = pSinCos->cos;
  int32_t m1, m2, m3, m4;
  m1 = (d * cos) >> 15;
  m2 = (q * sin) >> 15;
  m3 = (d * sin) >> 15;
  m4 = (q * cos) >> 15;
  pAlphaBeta->alpha = __QSUB16(m1, m2);
  pAlphaBeta->beta  = __QADD16(m3, m4);
}

/**
 * @desc PI controller outpus correction from a given measured input and a reference.
 * @param [in] inMeasure   - Measured input in Q1.15 format.
 * @param [in/out] pPI     - Pointer to the context of PI controller.
 * @param [out] pOut       - Pointer to the output of PI controller.
 * @note
    This routine requires inputs in the 1.15 format, except for Kp which is in 1.11 format.
    The constant Kp is scaled so it can be represented in 1.15 format by adjusting the constant by a power of 2.

 * @code
    out = Kp*(inReference-inMeasure) + Ki*Integral[inReference-inMeasure, dt] - Kc*Excess
    Where,
    out = Fractional 1.15 output, is limited to between outMax and outMin.
    Kp = Proportional gain co-efficient term
    Ki = Integral gain co-efficient term
    Kc = Excess gain co-efficient term
    Excess = Excess error after "out" is limited to between outMax and outMin.
    This implementation includes an anti-windup term to limit the integral windup.
 */
int16_t tinyfoc_pi_update(int16_t inMeasure, q15_pi_ctx_t *pPI)
{
  int32_t exceed, err = (int32_t)pPI->reference - (int32_t)inMeasure;
  int32_t sum = pPI->integrator;
  /* Kp * err = 5.11 * 1.15 = 6.26 <<1; 5.27 << 4 = 1.31 */
  int32_t U = ((pPI->Kp * err) << 5/*FIXME: 4*/) + sum;
  q15_t out;
  U = U >> 16;
  out = U;
  if (U > pPI->outMax) out = pPI->outMax;
  if (U < -pPI->outMax) out = -pPI->outMax;
  //*pOut = out;

  exceed = U - out;
  pPI->integrator += (pPI->Ki * err*2 - PI_K_EXCESS * exceed*2);
  return out;
}

/**
 * @desc This function calculates the duty cycle values based on the three scaled reference
 *   vectors in the a-b-c reference frame and the PWM period value. 
 * @param [in]  pABC       - Pointer to a-b-c reference frame.
 * @param [out] pDutyRatio - Pointer to PWM duty ratio of 3-phase.
 * @note The input & output must be consistent in any Q format.
 * @note This function is designed to work with the tinyfoc_inv_clarke_swap_q15().
 * @note Refer luwei's SVPWM OneNote for Sector; Tx,Ty; Tmin,Tmid,Tmax.
 */
void tinyfoc_svpwm_gen_fast_q15(const q15_abc_t *pABC, q15_pwm_duty_t *pDutyRatio)
{
  int32_t Vr1 = pABC->a;
  int32_t Vr2 = pABC->b;
  int32_t Vr3 = pABC->c;
  int combo = 0;
  q15_t duty1, duty2, duty3;
  q15_t period = 0x7fff;

  /* Map from abc sign bits: combo(bit2 bit1 bit0) to SV Sector. */
  if (Vr3 >= 0) combo = (1 << 0);
  if (Vr2 >= 0) combo |= (1 << 1);
  if (Vr1 >= 0) combo |= (1 << 2);

  /* Choose Tx,Ty from Vr1,Vr2,Vr3 according Sector no. */
  /* Duty calculation sequence: Tmin, Tmid, Tmax. */
  switch (combo) {
  case 6: /* Sector I */
    duty3 = (period - Vr2 - Vr1) / 2;
    duty2 = duty3 + Vr1;
    duty1 = duty2 + Vr2;
    break;
  case 4: /* Sector II */
    duty3 = (period + Vr2 + Vr3) / 2;
    duty1 = duty3 - Vr3;
    duty2 = duty1 - Vr2;
    break;
  case 5: /* Sector III */
    duty1 = (period - Vr1 - Vr3) / 2;
    duty3 = duty1 + Vr3;
    duty2 = duty3 + Vr1;
    break;
  case 1: /* Sector IV */
    duty1 = (period + Vr1 + Vr2) / 2;
    duty2 = duty1 - Vr2;
    duty3 = duty2 - Vr1;
    break;
  case 3: /* Sector V */
    duty2 = (period - Vr3 - Vr2) / 2;
    duty1 = duty2 + Vr2;
    duty3 = duty1 + Vr3;
    break;
  case 2: /* Sector VI */
    duty2 = (period + Vr3 + Vr1) / 2;
    duty3 = duty2 - Vr1;
    duty1 = duty3 - Vr3;
    break;
  default: /* special case for Vr1==Vr2==Vr3==0 due to (Valpha,Vbeta) = (0,0) */
    duty1 = duty2 = duty3 = period / 2;
    break;
  }

  pDutyRatio->duty1 = duty1;
  pDutyRatio->duty2 = duty2;
  pDutyRatio->duty3 = duty3;
}

/**
 * @desc This function calculates the duty cycle values based on the three scaled reference
    vectors in the a-b-c reference frame and the PWM period value. 

    This function works with the conventional Clark inverse transform variants 
    in order to simplify the calculation of three-phase duty cycle values from a given
    set of inputs in the alpha-beta reference frame.
    The duty-cycle generation part of the function is identical to the phase shifted version
    for SVM generation, so the initial part of this function compensates for the  
    swapped input inverse clark function. This ensures the motor rotation direction remains
    the same.
 * @code
     2/sqrt(3) *32768 = 1.154701 *32768 = 37837.2 ~ 0x93cd
 * @note This routine works for any q format so long as it is consistent across input and output.
 */
void tinyfoc_svpwm_gen(const q15_abc_t *pABC, uint16_t iPwmPeriod, q15_pwm_duty_t *pDutyCycleOut)
{
  int32_t a = pABC->a;
  int32_t b = pABC->b;
  int32_t c = pABC->c;
  int combo;
  int32_t dBC, dCA, dAB;
  uint16_t BCpwm, CApwm, ABpwm, duty1 = 0, duty2 = 0, duty3 = 0;
  a = (a * 0x93cd) >> 16;
  b = (b * 0x93cd) >> 16;
  c = (c * 0x93cd) >> 16;
  dBC = b - c;
  dCA = c - a;
  dAB = a - b;
  BCpwm = (dBC * iPwmPeriod * 2) >> 16;
  CApwm = (dCA * iPwmPeriod * 2) >> 16;
  ABpwm = (dAB * iPwmPeriod * 2) >> 16;
  combo = (b > c ? 4 : 0) | (c > a ? 2 : 0) | (a > b ? 1 : 0);
  switch (combo) {
  case 0:
    duty3 = (iPwmPeriod - BCpwm - ABpwm) / 2;
    duty2 = duty3 + BCpwm;
    duty1 = duty2 + ABpwm;
    break;
  case 1:
    duty1 = (iPwmPeriod - CApwm - BCpwm) / 2;
    duty3 = duty1 + CApwm;
    duty2 = duty3 + BCpwm;
    break;
  case 2:
    duty3 = (iPwmPeriod + CApwm + ABpwm) / 2;
    duty1 = duty3 - CApwm;
    duty2 = duty1 - ABpwm;
    break;
  case 3:
    duty2 = (iPwmPeriod - ABpwm - CApwm) / 2;
    duty1 = duty2 + ABpwm;
    duty3 = duty1 + CApwm;
    break;
  case 4:
    duty2 = (iPwmPeriod + BCpwm + CApwm) / 2;
    duty3 = duty2 - BCpwm;
    duty1 = duty3 - CApwm;
    break;
  case 5:
    duty1 = (iPwmPeriod + ABpwm + BCpwm) / 2;
    duty2 = duty1 - ABpwm;
    duty3 = duty2 - BCpwm;
    break;
  default:
    break;
  }
  pDutyCycleOut->duty1 = duty1;
  pDutyCycleOut->duty2 = duty2;
  pDutyCycleOut->duty3 = duty3;
}

void tinyfoc_foc_exec(tinyfoc_foc_t *handle)
{
  q15_sincos_t sin_cos;

  tinyfoc_sin_cos_q15(handle->angle_e, &sin_cos);
  tinyfoc_clarke_q15((const q15_abc_t *)&handle->Ia, (q15_ab_t *)&handle->Ialpha);
  tinyfoc_park_q15((const q15_ab_t *)&handle->Ialpha, &sin_cos, (q15_dq_t *)&handle->Id);

  handle->Vq = tinyfoc_pi_update(handle->Iq, &handle->PI_Iq);
  handle->Vd = tinyfoc_pi_update(handle->Id, &handle->PI_Id);

  tinyfoc_inv_park_q15((const q15_dq_t *)&handle->Vd, &sin_cos, (q15_ab_t *)&handle->Valpha);
  tinyfoc_inv_clarke_swap_q15((const q15_ab_t *)&handle->Valpha, (q15_abc_t *)&handle->Va);
  tinyfoc_svpwm_gen_fast_q15((const q15_abc_t *)&handle->Va, &handle->pwm_duty);

  tinyfoc_pwm_set_duty(&handle->pwm_duty);
}

void tinyfoc_foc_prologue(tinyfoc_ctx_t *handle)
{
  if (handle->is_open_loop) {
    /* Open Loop: IqREF = max, IdREF = 0 */
  } else {
    /* Close Loop: speed control mode. */
    handle->foc->PI_Iq.reference = tinyfoc_pi_update(handle->sensor->Wfin, &handle->foc->PI_speed);
    /* TBD: FW */
  }
}

void tinyfoc_foc_epilogue(tinyfoc_ctx_t *handle)
{
  if (handle->is_open_loop) {
    /* 开环期间，电角度采用加速度方法估计Aramp. */
    if (handle->openloop_drag_cycles < handle->sensor->drag_time_norm) {
      /* 开环强拖期间，电机还未动。*/
      handle->openloop_drag_cycles++;
    } else if (handle->openloop_revup_speed_norm < handle->sensor->revup_speed_target_norm) {
      /* 开环加速期间，电机转速逐渐增加（以一个固定角加速度）: 在w-t坐标图里w线性增加，该变量本质已经是w*dt即当前dt的面积。*/
      handle->openloop_revup_speed_norm += handle->sensor->revup_speed_step_norm;
    } else {
      handle->is_open_loop = 0;
      /* 过渡开始点，电角度误差Aerr处于最大值。*/
      handle->sensor->Aerr = handle->sensor->Aramp - handle->sensor->Aest;
    }
    /* 角度允许溢出回卷, 注意openloop_revup_speed_norm单位为1/1024*radpupi/cycle: 在w-t坐标图里w*t的总面积累加上当前dt的面积。*/
    handle->sensor->Aramp += (int16_t)(handle->openloop_revup_speed_norm >> handle->sensor->revup_time_base_shift);
    /* @appnote Aramp前加-号，revup期间旋转方向相反。*/
    handle->foc->angle_e = handle->sensor->Aramp;
  } else {
    /* 过渡期间, 电角度误差Aerr要逐渐变小，直到接近观测器输出的估计值Aest. */
    handle->transition_cycles++;
    if (handle->transition_cycles == handle->sensor->est_spd_pace/4) {
      handle->transition_cycles = 0;
      if (abs(handle->sensor->Aerr) > ANGLE_TRANS_STEP_NORM) {
        if (handle->sensor->Aerr < 0) {
          handle->sensor->Aerr += ANGLE_TRANS_STEP_NORM;
        } else {
          handle->sensor->Aerr -= ANGLE_TRANS_STEP_NORM;
        }
      }
    }
    handle->foc->angle_e = handle->sensor->Aest + handle->sensor->Aerr;
  }
}

int tinyfoc_motor_init(tinyfoc_ctx_t *handle)
{
  if (handle->magic != TINYFOC_MAGIC) return -1;
  if (handle->version != TINYFOC_VERSION) return -2;

  handle->is_open_loop = 1;
  handle->openloop_drag_cycles = 0;
  handle->openloop_revup_speed_norm = 0;

  handle->foc->angle_e = 0;

  /* PI - Speed Control */
  handle->foc->PI_speed.integrator = 0;

  /* PI - Torque Current Control */
  handle->foc->PI_Iq.integrator = 0;

  /* PI - Flux Current Control */
  handle->foc->PI_Id.integrator = 0;

  handle->sensor->Iest_alpha = handle->sensor->Iest_beta = 0;
  handle->sensor->Zalpha     = handle->sensor->Zbeta = 0;
  handle->sensor->Eest_alpha = handle->sensor->Eest_beta = 0;
  handle->sensor->Efin_alpha = handle->sensor->Efin_beta = 0;
  handle->sensor->Adelta     = handle->sensor->Aprev = 0;
  handle->sensor->Aramp      = handle->sensor->Aest = 0;
  return 0;
}
