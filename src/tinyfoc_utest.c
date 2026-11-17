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
#include <stdlib.h> //exit
#include <math.h>
#include "tinyfoc_api.h"

typedef uint16_t q16angle_t;
extern q15_t tinyfoc_atan_q15(q15_t x, q15_t y);
extern q16angle_t q15_atan2(q15_t sine, q15_t cosine);

extern void tinyfoc_sin_cos_q15(q15_t angle, q15_sincos_t *pSinCos);
extern void tinyfoc_inv_park_q15(const q15_dq_t *pDQ, const q15_sincos_t *pSinCos, q15_ab_t *pAlphaBeta);
extern void tinyfoc_inv_clarke_swap_q15(const q15_ab_t *pAlphaBeta, q15_abc_t *pABC);
extern void tinyfoc_svpwm_gen_fast_q15(const q15_abc_t *pABC, q15_pwm_duty_t *pDutyRatio);

/* hook */
int tinyfoc_pwm_set_duty(q15_pwm_duty_t *p_pwm_duty)
{
  return -1;
}

static void utest_qadd(void)
{
  q15_t val, step = 0x4001;
  int idx;

  val = 0x8000;
  for (idx = 0; idx < 8; idx++) {
    val = __QADD16(val, step);
    printf("0x%04x ", val);
  }
  printf("\n");

  val = 0x7fff;
  for (idx = 0; idx < 8; idx++) {
    val = __QSUB16(val, step);
    printf("0x%04x ", val);
  }
  printf("\n");
}

static void utest_atan2_cordic(void)
{
  q15_t angle;
  q15_sincos_t sin_cos;
  q15_t angle_atan;

  angle = 0x2000;
  tinyfoc_sin_cos_q15(angle, &sin_cos);
  //print the internal variables to show the trace of CORDIC.
  angle_atan = tinyfoc_atan_q15(sin_cos.cos, sin_cos.sin);
  printf("%d %d 0x%04x %04x %fdeg\n", sin_cos.sin, sin_cos.cos, (uint16_t)angle, angle_atan, Q15toF(angle_atan)*180);

  angle = 0x0000;
  tinyfoc_sin_cos_q15(angle, &sin_cos);
  //print the internal variables to show the trace of CORDIC.
  angle_atan = tinyfoc_atan_q15(sin_cos.cos, sin_cos.sin);
  printf("%d %d 0x%04x %04x %fdeg\n", sin_cos.sin, sin_cos.cos, (uint16_t)angle, angle_atan, Q15toF(angle_atan)*180);

  angle = 0x8000;
  tinyfoc_sin_cos_q15(angle, &sin_cos);
  //print the internal variables to show the trace of CORDIC.
  angle_atan = tinyfoc_atan_q15(sin_cos.cos, sin_cos.sin);
  printf("%d %d 0x%04x %04x %fdeg\n", sin_cos.sin, sin_cos.cos, (uint16_t)angle, angle_atan, Q15toF(angle_atan)*180);

  angle = 0x7FFF;
  tinyfoc_sin_cos_q15(angle, &sin_cos);
  //print the internal variables to show the trace of CORDIC.
  angle_atan = tinyfoc_atan_q15(sin_cos.cos, sin_cos.sin);
  printf("%d %d 0x%04x %04x %fdeg\n", sin_cos.sin, sin_cos.cos, (uint16_t)angle, angle_atan, Q15toF(angle_atan)*180);

  exit(0);
}

static void utest_triangle(void)
{
  q15_t angle;
  q15_sincos_t sin_cos;
  int idx = 0, step = 32;
  float fmax = 0, fval;
  q15_t angle_max, angle_atan;

  /* Redirect the output to sin.txt, which is the input of tinyfoc_utest_sin.py. */
  angle = 0x8000;
  while (angle != (0x7fff & ~(step-1))) {
    float ferr;
    int16_t angle_atan2;
    tinyfoc_sin_cos_q15(angle, &sin_cos);
    angle_atan = tinyfoc_atan_q15(sin_cos.sin, sin_cos.cos);
    angle_atan2 = q15_atan2(sin_cos.cos, sin_cos.sin);
    printf("%d %d %d 0x%04x %04x %04x %fdeg %f\n", idx, sin_cos.sin, sin_cos.cos, (uint16_t)angle, angle_atan, angle_atan2,
           Q15toF(angle_atan)*180, Q15toF(angle_atan2)*180);
    fval = sinf(Q15toF(angle)*M_PI);
    ferr = fabs(fval - Q15toF(sin_cos.sin));
    if (ferr > fmax) {
      fmax = ferr;
      angle_max = angle;
    }
    idx++;
    angle += step;
  }
  /*
   * angle=0.445282~0x38ff~14591
   * sin=0.985261~32284~32273
   * sin_max_err=0.000367~11 due to linear interpolation.
   */
  //printf("angle=%f~0x%04x~%d\n", Q15toF(angle_max), (uint16_t)angle_max, angle_max);
  //fval = sinf(Q15toF(angle_max)*M_PI);
  //tinyfoc_sin_cos_q15(angle_max, &sin_cos);
  //printf("sin=%f~%d~%d\n", fval, Q15(fval), sin_cos.sin);
  //printf("sin_max_err=%f~%d\n", fmax, Q15(fval)-sin_cos.sin);

  #if 0
  /* Debug data flow of SslLutCosSin.scala */
  angle = 0x007e;
  for (int i=1; i<6; i++) {
    tinyfoc_sin_cos_q15(angle, &sin_cos);
    printf("angle=0x%x: cos=%d sin=%d\n", angle, sin_cos.cos, sin_cos.sin);
    angle += 0x007e;
  }
  #endif
  exit(0);
}

static void utest_svpwm(void)
{
  q15_t angle;
  q15_sincos_t sin_cos;
  int idx = 0, step = 32;
  q15_abc_t Vabc;
  q15_ab_t Valpha_beta;
  q15_dq_t Vdq;
  q15_pwm_duty_t pwm_duty;

  /* Redirect the output to svpwm.txt, which is the input of tinyfoc_utest_svpwm.py. */
  step = 2048;
  angle = 0x8000;
  /* Adjust torque */
  Vdq.d = 0x100;
  /* Adjust velocity/rpm in [0x8000,0x7fff]: abs越小，电压幅度越低，PWM占空比差异越小 */
  Vdq.q = 0x7fff;//0x49e9;
  for (idx = 0; idx <= 0x20000/step; idx++) {
    tinyfoc_sin_cos_q15(angle, &sin_cos);
    tinyfoc_inv_park_q15(&Vdq, &sin_cos, &Valpha_beta);
    //tinyfoc_inv_clarke_q15(&Valpha_beta, &Vabc);
    tinyfoc_inv_clarke_swap_q15(&Valpha_beta, &Vabc);
    tinyfoc_svpwm_gen_fast_q15(&Vabc, &pwm_duty);
    /* Quadrature Valpha,Vbeta; 3-phase Va,Vb,Vc; PWM duty cycles */
    printf("%d %d %d %d %d %d %d %d %d 0x%04x\n", idx, Valpha_beta.alpha, Valpha_beta.beta, Vabc.a, Vabc.b, Vabc.c, pwm_duty.duty1, pwm_duty.duty2, pwm_duty.duty3, (uint16_t)angle);
    idx++;
    angle += step;
  }
}

/* comment this line if EBF_PY32F0 */
#define EBF407 

tinyfoc_foc_t tinyfoc_foc_handle = {
};

/* Sensorless algorithm uses Slide Mode Observer. */
tinyfoc_sensor_t tinyfoc_sensor_handle = {
#if defined(EBF407)
  /* BEMF增益标幺值。*/
  .bemf_gain_f_norm      = 29226,//BEMF_GAIN_F_NORM,
  .bemf_gain_g_norm      = 3730,//BEMF_GAIN_G_NORM,
  /* 滑膜控制器参数标幺值。*/
  .smc_gain_v_norm       = Q15(0.40),
  .smc_max_err_i_norm    = Q15(0.01),
  
  /* 小角度to转速 标幺值。*/
  .rad_to_rpm_norm = 29999,//RAD_TO_RPM_NORM,
  /* 转速to滤波器系数 标幺值。*/
  .rpm_to_lpf_norm = 14055,//RPM_TO_LPF_NORM,

  /* 强拖阶段 标幺值。*/
  .drag_time_norm = 6400,//DRAG_TIME_MS*(fFOC/1000),
  /* 加速阶段 标幺值。*/
  .revup_speed_target_norm = 69905,//REVUP_SPEED_TARGET_NORM,
  .revup_speed_step_norm   = 8,//REVUP_SPEED_STEP_NORM,
  .revup_time_base_shift   = 9,//T_BASE_SHIFT,

  /* 计算转速的步长。*/
  .est_spd_pace            = 16,//fFOC / fTINYFOC,
  /* 计算转速的滤波器系数，也是自适应滤波器系数的最小值。*/
  .est_spd_K               = 857,//(REVUP_TARGET_RPM * POLE_PAIRS * RPM_TO_LPF_NORM)>>15,
#else //ebf_py32f0
  /* BEMF增益标幺值。*/
  .bemf_gain_f_norm      = 25686,//BEMF_GAIN_F_NORM,
  .bemf_gain_g_norm      = 7465,//BEMF_GAIN_G_NORM,
  /* 滑膜控制器参数标幺值。*/
  .smc_gain_v_norm       = Q15(0.20),
  .smc_max_err_i_norm    = Q15(0.02),

  /* 小角度to转速 标幺值。*/
  .rad_to_rpm_norm = 29999,//RAD_TO_RPM_NORM,
  /* 转速to滤波器系数 标幺值。*/
  .rpm_to_lpf_norm = 28110,//RPM_TO_LPF_NORM,

  /* 强拖阶段 标幺值。*/
  .drag_time_norm = 3200,//DRAG_TIME_MS*(fFOC/1000),
  /* 加速阶段 标幺值。*/
  .revup_speed_target_norm = 139810,//REVUP_SPEED_TARGET_NORM,
  .revup_speed_step_norm   = 16,//REVUP_SPEED_STEP_NORM,
  .revup_time_base_shift   = 9,//T_BASE_SHIFT,

  /* 计算转速的步长。*/
  .est_spd_pace            = 8,//fFOC / fTINYFOC,
  /* 计算转速的滤波器系数，也是自适应滤波器系数的最小值。*/
  .est_spd_K               = 1715,//(REVUP_TARGET_RPM * POLE_PAIRS * RPM_TO_LPF_NORM)>>15,
#endif
};

/* One FOC handle every motor control. */
tinyfoc_ctx_t tinyfoc_handle = {
  .magic = TINYFOC_MAGIC,
  .version = TINYFOC_VERSION,
  /* TBD: dual-motor support. */
  .foc = &tinyfoc_foc_handle,
  .sensor = &tinyfoc_sensor_handle,
};

static void utest_smc(void)
{
  FILE *fp_in, *fp_out;
  #define LINE_BUF_LEN 256
  char line_buf[LINE_BUF_LEN];
  int timestamp, angle;

#if defined(EBF407)
  fp_in = fopen("ebf_tinyfoc_stm32f407_Aest_Valpha_Vbeta_Ialpha_Ibeta_smc.csv", "r");
#else
  fp_in = fopen("ebf_tinyfoc_py32f0_Aest_Valpha_Vbeta_Ialpha_Ibeta_startup.csv", "r");
#endif
  if (NULL == fp_in) {
    printf("fopen error\n");
    return;
  }
  fp_out = fopen("smc.csv", "w+");
  if (NULL == fp_out) {
    printf("fopen error\n");
    fclose(fp_in);
    return;
  }
  /* Write the title of output file, w/o Timestamp. */
  //fprintf(fp_out, "Valpha;Vbeta;Aest;Ialpha;Ibeta\n");
  fprintf(fp_out, "Valpha;Vbeta;Ialpha;Ibeta;Aest;Ialpha_est;Ibeta_est;Ealpha;Ebeta;Zalpha;Zbeta\n");

  /* Skip the title of input file. */
  fgets(line_buf, LINE_BUF_LEN, fp_in);
  printf("%s\n", line_buf);
  while (NULL != fgets(line_buf, LINE_BUF_LEN, fp_in)) {
    //printf("%s\n", line_buf);
#if defined(EBF407)
    sscanf(line_buf, "%d;%hd;%hd;%hd;%hd;%d",
           &timestamp,
           &tinyfoc_foc_handle.Valpha, &tinyfoc_foc_handle.Vbeta,
           &tinyfoc_foc_handle.Ialpha, &tinyfoc_foc_handle.Ibeta,
           &angle);
#else
    sscanf(line_buf, "%d;%hd;%hd;%d;%hd;%hd",
           &timestamp,
           &tinyfoc_foc_handle.Valpha, &tinyfoc_foc_handle.Vbeta,
           &angle,
           &tinyfoc_foc_handle.Ialpha, &tinyfoc_foc_handle.Ibeta);
#endif
    //printf("Valpha=%hd Vbeta=%hd Ialpha=%hd Ibeta=%hd\n", tinyfoc_foc_handle.Valpha, tinyfoc_foc_handle.Vbeta, tinyfoc_foc_handle.Ialpha, tinyfoc_foc_handle.Ibeta);
    tinyfoc_sensor_exec(&tinyfoc_handle);
    /* Skip the full zero vectors. */
    if ((0==tinyfoc_foc_handle.Valpha) && (0==tinyfoc_foc_handle.Vbeta) &&
        (0==tinyfoc_foc_handle.Ialpha) && (0==tinyfoc_foc_handle.Ibeta)) {
      continue;
    }
    printf("angle: utest=%d csv=%d\n", tinyfoc_sensor_handle.Aest, angle);
    fprintf(fp_out, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n", 
            tinyfoc_foc_handle.Valpha, tinyfoc_foc_handle.Vbeta,
            tinyfoc_foc_handle.Ialpha, tinyfoc_foc_handle.Ibeta,
            tinyfoc_sensor_handle.Aest,
            tinyfoc_sensor_handle.Iest_alpha, tinyfoc_sensor_handle.Iest_beta,
            tinyfoc_sensor_handle.Efin_alpha, tinyfoc_sensor_handle.Efin_beta,
            tinyfoc_sensor_handle.Zalpha, tinyfoc_sensor_handle.Zbeta);
  }
  fclose(fp_in);
  fclose(fp_out);
}

int main(void)
{
  //utest_qadd();
  //utest_atan2_cordic();
  utest_triangle();
  //utest_svpwm();
  //utest_smc();
}
