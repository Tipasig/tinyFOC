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
#include <stdlib.h> //abs
#include "tinyfoc_api.h"

typedef uint16_t q16angle_t;

extern q15_t tinyfoc_atan_q15(q15_t x, q15_t y);
extern q16angle_t q15_atan2(q15_t sine, q15_t cosine);

/**
 * @code Motor Model
 *  Ideal model: v = R*i + L*di/dt + e
 *   v: Phase Voltage v(t)
 *   i: Phase Current i(t)
 *   R: Phase Resistance of stator
 *   L: Phase Inductance of stator
 *   e: BEMF e(t)
 *  Discret expression: i(k+1) = F*i(k) + G*(v(k) - e(k))
 *   F = 1 - Ts*R/L
 *   G = Ts/L
 *  Q15 expression: qI = qF * qI + qG * (qV - qE)
 *   qF = 0x7FFF - q15_div(qR, qLdt)
 *   qG = q15_div(0x7FFF, qLdt)
 *
 * @abbr: I= Current of stator
 *        V= Voltage of stator
 *        A= Theta of rator angle position
 *        W= Omega of rator angle speed
 *        E= Back EMF
 *        Z= SMC output
 *        Ifoc, Vfoc: the output of FOC's Clarke, inv Park.
 *        Iest, Eest, Aest, West: the estimation value of Current, BEMF, Theta, Omega.
 *              Efin, Afin, Wfin: the final/filtered value of BEMF, Theta, Omega.
 *        Aerr: the difference of Theta used in Open-Loop and Close-Loop.
 *
 *    +---------------+          +-----------+
 * V->| PMSM hardware |->Ifoc--->|       Ksmc|      Ksmc = 0.85
 *    +---------------+          |    SMC    |      Kslf = Ts/RC = 1/(fFOC*Wfin)
Vfoc->| PMSM observer |->Iest--->|           |      Tvel = 1/fSPEED
 *    +----------^--^-+          +-----v-----+      Kw   = Ts/RC = 1/(fFOC*Wramp)
 *               |  |------------------|Z
 *               |               +-----v------+
 *               +-------Eest<---|    LPF Kslf|
 *               |               +------------+
 *        +------v-----+               +----+
 *        |    LPF Kslf|->Efin.alpha-->|    |
 *        +------------+               |atan|-+---->Aest--->| +Aerr   |->Afin
 *                            .beta -->|    | |
 *                                     +----+ |
 *                                            |
 *                              +-------------v--+          +---------+
 *                              |Sigma(Aest)*Tvel|->West--->| LPF Kw  |->Wfin
 *                              +----------------+          +---------+
 * @code SMC Current Observer
 *  PMSM hw w/ shunt:    Ialpha, Ibeta
 *          w/ PI+Park': Valpha, Vbeta
 *  PMSM sw observer:    Iest_alpha = F * Iest_alpha + G * (Valpha - Eest_alpha - Zalpha)
 *                       Iest_beta  = F * Iest_beta  + G * (Vbeta  - Eest_beta  - Zbeta)
 *   Iest,Eest,SMC initial values are 0;
 *   Z = Ksmc * (Iest - I) / SMCmax_err;
 *       0.85                   0.01
 *
 * @code BEMF Estimation
 *  LPF: Eest(k) = Eest(k-1) + Kslf*(Z(k)-Eest(k))
 *  Apply LPF once:  Eest = Eest + Kslf * (Z - Eest)
 *                   Eest is used to calculate Iest in SMC current observer.
 *  Apply LPF again: Efin = Efin + Kslf * (Eest - Efin)
 *                   Efin is used to calculate Aest = arctan(Efin_alpha,Efin_beta) 
 *
 * @note
 *  How to measure R?
 *   Use multimeter to measure phase to phase resistance, Rvalue;
 *   R = Rvalue / 2.
 *  How to measure L?
 *   Use multimeter to measure capatance of a resistor R1, C1value;
 *   multimeter's test signal frequency f0 = 1/(2*pai*R1*C1value);
 *   Use multimeter to measure capatance of phase to phase inductance, C2value;
 *   L = 1/(w^2*C2value)/2 = 1/((2*pai*f0)^2 * C2value)/2 =
 *       1/(C2value/(R1*C1value)^2)/2
 */

void tinyfoc_sensor_exec(tinyfoc_ctx_t *handle)
{
  tinyfoc_foc_t *foc = handle->foc;
  tinyfoc_sensor_t *smo = handle->sensor;
  int32_t Valpha = foc->Valpha;
  int32_t Vbeta  = foc->Vbeta;
  int32_t Iest, Ierr;

  /* 根据电机模型更新估算电流Iest. */
  Iest = smo->Iest_alpha;
  Iest = (smo->bemf_gain_f_norm * Iest + smo->bemf_gain_g_norm * (Valpha - (int32_t)smo->Eest_alpha - (int32_t)smo->Zalpha)) >> 15;
  smo->Iest_alpha = Iest;
  /* 根据估算电流和测量电流的误差Ierr, 更新SMO输出: 分线性区和饱和区，后者避免观测误差抖动过大 */
  Ierr = Iest - foc->Ialpha;
  if (abs(Ierr) < smo->smc_max_err_i_norm) {
    smo->Zalpha = smo->smc_gain_v_norm * Ierr / smo->smc_max_err_i_norm;
  } else if (Ierr > 0) {
    smo->Zalpha = smo->smc_gain_v_norm;
  } else {
    smo->Zalpha = -smo->smc_gain_v_norm;
  }
  /* 应用自适应滤波器更新反电动势Eest: 1st LPF延迟相位45deg, 2nd LPF再延迟相位45deg. */
  smo->Eest_alpha = __QADD16(smo->Eest_alpha, (smo->est_bemf_Kslf * ((int32_t)smo->Zalpha - (int32_t)smo->Eest_alpha)) >> 15);
  smo->Efin_alpha = __QADD16(smo->Efin_alpha, (smo->est_bemf_Kslf * ((int32_t)smo->Eest_alpha - (int32_t)smo->Efin_alpha)) >> 15);

  /* beta component. */
  Iest = smo->Iest_beta;
  Iest = (smo->bemf_gain_f_norm * Iest + smo->bemf_gain_g_norm * (Vbeta - (int32_t)smo->Eest_beta - (int32_t)smo->Zbeta)) >> 15;
  smo->Iest_beta = Iest;
  Ierr = Iest - foc->Ibeta;
  if (abs(Ierr) < smo->smc_max_err_i_norm) {
    smo->Zbeta = smo->smc_gain_v_norm * Ierr / smo->smc_max_err_i_norm;
  } else if (Ierr > 0) {
    smo->Zbeta = smo->smc_gain_v_norm;
  } else {
    smo->Zbeta = -smo->smc_gain_v_norm;
  }
  smo->Eest_beta = __QADD16(smo->Eest_beta, (smo->est_bemf_Kslf * ((int32_t)smo->Zbeta - (int32_t)smo->Eest_beta)) >> 15);
  smo->Efin_beta = __QADD16(smo->Efin_beta, (smo->est_bemf_Kslf * ((int32_t)smo->Eest_beta - (int32_t)smo->Efin_beta)) >> 15);

  /* 由两路BEMF更新转子电角度Aest: @appnote 加+号或-号以选择旋转方向。*/
  //smo->Afin = tinyfoc_atan_q15(smo->Efin_alpha, smo->Efin_beta);
  /* 由于FOC里调用的是swap版本的Clarke逆变换, 所以这里alpha和beta要交换回来。*/
  smo->Aest = q15_atan2(smo->Efin_beta, smo->Efin_alpha);
  /* 更新转子电角度累计偏移量Adelta，由于fFOC远高于转速，所以Adelta不会出现溢出。*/
  smo->Adelta += smo->Aest - smo->Aprev;
  smo->Aprev = smo->Aest;
  /* 由当前电角度偏移量更新转子电转速West: 更新频率小于fFOC. */
  smo->est_spd_cycles++;
  if (smo->est_spd_cycles == smo->est_spd_pace) {
    smo->West = (smo->Adelta * smo->rad_to_rpm_norm) >> 15;
    smo->est_spd_cycles = 0;
    smo->Adelta = 0;
  }
  /* 对电转速应用滤波器。*/
  smo->Wfin = __QADD16(smo->Wfin, (smo->est_spd_K * ((int32_t)smo->West - (int32_t)smo->Wfin)) >> 15);
  /* 由当前电转速更新自适应增益est_bemf_Kslf: 最小值est_spd_K以避免落入初值陷井. */
  smo->est_bemf_Kslf = (smo->Wfin * smo->rpm_to_lpf_norm) >> 15;
  if (smo->est_bemf_Kslf < smo->est_spd_K) {
    smo->est_bemf_Kslf = smo->est_spd_K;
  }
  /* 补偿2级滤波器引入的相位90deg延迟: 允许溢出回卷。*/
  smo->Aest -= Q15(1/2);
}
