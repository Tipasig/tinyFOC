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

#ifndef _TINYFOC_API_H_
#define _TINYFOC_API_H_

#include <stdint.h>
#include <stdbool.h>
#if defined(__arm__) && defined(CMSIS_DEV_HDR)
/* DSP saturation instructions. */
#include CMSIS_DEV_HDR
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/
#define TINYFOC_VER_MAJOR         0
#define TINYFOC_VER_MINOR         1
#define TINYFOC_VER_SVN           1100
#define TINYFOC_MAGIC             0x54464F43 //"TFOC"
#define TINYFOC_VERSION           ((TINYFOC_VER_MAJOR << 24u) | (TINYFOC_VER_MINOR << 16u) | (TINYFOC_VER_SVN << 0))

#define Q15_MAX               (32767)
#define Q15_MIN               (-32768)

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define Q15toF(q)             ((float)(q)/32768.0)
#define Q15(f) \
  ((f < 0.0) ? (int16_t)(32768.0 * (f) - 0.5) \
   : (int16_t)(32767.0 * (f) + 0.5))

#if (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))
#else
#define __QADD16(a,b)         q15_add(a, b)
#define __QSUB16(a,b)         q15_add(a, -(b))
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/
/**
 * @desc Q1.15 Fractional data.
 *    bit 15    14   13   12   ... 0
 *       -2^0 . 2^-1 2^-2 2^-3 ... 2^-15
 */
typedef int16_t q15_t;

/**
 * @desc Parameters related to ABC reference frame, phase current or voltage.
 */
typedef struct q15_abc_s
{
  /* Phase A component */
  q15_t a;
  /* Phase B component */
  q15_t b;
  /* Phase C component */
  q15_t c;
} q15_abc_t;

/**
 * @desc Parameters related to quadrature Alpha-Beta reference frame.
 */
typedef struct q15_ab_s
{
  /* Alpha component */
  q15_t alpha;
  /* Beta component */
  q15_t beta;
} q15_ab_t;

/**
 * @desc Parameters related to stationary D-Q reference frame.
 */
typedef struct q15_dq_s
{
  /* Component on Direct axis. */
  q15_t d;
  /* Component on Quadrature axis. */
  q15_t q;
} q15_dq_t;

/**
 * @desc Parameters related to Sine and Cosine components of the motor angle.
 */
typedef struct q15_sincos_s
{
  /* Sine component */
  q15_t sin;
  /* Cosine component */
  q15_t cos;
} q15_sincos_t;

/**
 * @desc Parameters related to the contex of PI Controller.
 */
typedef struct q15_pi_ctx_s
{
  /* Integrator sum */
  int32_t integrator;
  /* Proportional gain co-efficient term */
  q15_t Kp;
  /* Integral gain co-efficient term */
  q15_t Ki;
  /* Maximum output limit */
  int16_t outMax;
  /* Reference input */
  q15_t reference;
} q15_pi_ctx_t;

/**
 * @desc Parameters related to PWM module Duty Ratio for 3-phase inverter.
 *       [0,0x7fff] means [0%, 99.9+%]
 */
typedef struct q15_pwm_duty_s
{
  /* Duty ratio of PWM1 for phase-a */
  q15_t duty1;
  /* Duty ratio of PWM3 for phase-b */
  q15_t duty2;
  /* Duty ratio of PWM5 for phase-c */
  q15_t duty3;
} q15_pwm_duty_t;

/**
 * @brief Field Oriented Control data context.
 */
typedef struct
{
  /* The measured 3-phase current (AC) of stator. */
  q15_t Ia;
  q15_t Ib;
  q15_t Ic;
  /* The measured quadrature current in Alpha-Beta (AC) of stator. */
  q15_t Ialpha;
  q15_t Ibeta;
  /* The measured quadrature current in D-Q (DC input to the current PIs) of stator. */
  q15_t Id;
  q15_t Iq;
  /* The control voltage in D-Q, as output of the current PIs. */
  q15_t Vd;
  q15_t Vq;
  /* The control voltage in Alpha-Beta, as input of SVM. */
  q15_t Valpha;
  q15_t Vbeta;
  /* The control voltage of 3-phase, used by SVM to generate SVPWM waveform. */
  q15_t Va;
  q15_t Vb;
  q15_t Vc;
  /* PI controllers. */
  q15_pi_ctx_t PI_speed, PI_Iq, PI_Id;
  /* The electrical angle of Rotor. */
  q15_t angle_e;
  /* PWM duty ratio. */
  q15_pwm_duty_t pwm_duty;
} tinyfoc_foc_t;

/**
 * @brief Virtual Sensor's Slide Mode Observer data context.
 * @note  alpha, beta分别放在一起有利于计算性能.
 */
typedef struct
{
  /* 参数区 */
  /* The Gain F=1-Ts*Rs/Ls in motor model. */
  q15_t bemf_gain_f_norm;
  /* The Gain G=Ts/Ls in motor model. */
  q15_t bemf_gain_g_norm;
  /* The Gain of SMC output. */
  q15_t smc_gain_v_norm;
  /* The max error of estimated current in SMC. */
  q15_t smc_max_err_i_norm;
  /* The coeff is used to calculate W in RPM from A in rad/pi. */
  q15_t rad_to_rpm_norm;
  /* The coeff is used to calculate Kslf/Kspd of LPF from W in RPM. */
  q15_t rpm_to_lpf_norm;
  /* The drag time in Ts. */
  uint16_t drag_time_norm;
  /* The revup target speed in Ts. */
  uint16_t revup_speed_step_norm;
  uint32_t revup_speed_target_norm;
  uint8_t  revup_time_base_shift;
  /* The run pace to estimate electrical angle speed. */
  uint8_t est_spd_pace;
  /* The gain of LPF to filter the estimated electrical angle speed: fc=Wramp_target */
  q15_t   est_spd_K;

  /* 物理量区 */
  /* The estimated current of motor stator. */
  q15_t Iest_alpha;
  /* The output of SMC. */
  q15_t Zalpha;
  /* The estimated BEMF. */
  q15_t Eest_alpha;
  /* The final BEMF after the 2nd LPF. */
  q15_t Efin_alpha;
  /* beta component. */
  q15_t Iest_beta;
  q15_t Zbeta;
  q15_t Eest_beta;
  q15_t Efin_beta;
  /* The gain of self-adaptive LPF to filer the estimated BEMF: fc=Wfin/60 */
  q15_t est_bemf_Kslf;
  /* The estimated  electrical angle of motor rotor: unit radius/pi */
  q15_t Aest;
  /* The estimate   electrical angle of motor rotor during ramp up. */
  q15_t Aramp;
  /* The final      electrical angle of motor rotor after compensation. */
  q15_t Afin;
  /* The estimated electrical speed of motor rotor: unit RPM, max=32767 */
  q15_t West;
  /* The final     electrical speed of motor rotor after LPF: est_bemf_Kslf = Ts*2pi*Wfin/60 */
  q15_t Wfin;

  /* 内部变量区 */
  /* The difference between Aest and the final Aramp. */
  q15_t Aerr;
  /* The accumlated electrical angle delta of motor rotor during current speed PI loop. */
  q15_t Adelta;
  /* The previous   electrical angle of motor rotor of last speed PI loop. */
  q15_t Aprev;
  /* The run cycles to estimate electrical angle speed. */
  uint8_t est_spd_cycles;
} tinyfoc_sensor_t;

/**
 * @brief TINYFOC data context.
 */
typedef struct
{
  /* Consistence check between user layer and library layer. */
  uint32_t magic;
  uint32_t version;
  /* A handler per motor. */
  tinyfoc_foc_t    *foc;
  tinyfoc_sensor_t *sensor;
  /* Internal context. */
  bool is_open_loop;
  uint8_t transition_cycles;
  uint16_t openloop_drag_cycles;
  uint32_t openloop_revup_speed_norm;
} tinyfoc_ctx_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  q15_t q15_mul(q15_t multiplicand, q15_t multiplier);
  q15_t q15_add(q15_t addend, q15_t adder);
  q15_t q15_abs(q15_t num);

  /**
   * @brief The initialization of TINYFOC Sensorless FOC.
   * @note  This is a HAL routine for tuning coefficients by user.
   * @param [in] handle - Pointer to TINYFOC handle.
   * @return 0 for success, negative for failure.
   */
  int tinyfoc_motor_init(tinyfoc_ctx_t *handle);

  /**
   * @brief The prologue of FOC for speed PI's reference input.
   * @param [in] handle - Pointer to TINYFOC handle.
   */
  void tinyfoc_foc_prologue(tinyfoc_ctx_t *handle);
  /**
   * @brief The epilogue of FOC for current electrical angle.
   * @param [in] handle - Pointer to TINYFOC handle.
   */
  void tinyfoc_foc_epilogue(tinyfoc_ctx_t *handle);

  /**
   * @brief The core engine of FOC.
   * @param [in] handle - Pointer to FOC handle.
   */
  void tinyfoc_foc_exec(tinyfoc_foc_t *handle);

  /**
   * @brief The core engine of virtual sensor.
   * @param [in] handle - Pointer to TINYFOC handle.
   */
  void tinyfoc_sensor_exec(tinyfoc_ctx_t *handle);

  /**
   * @brief  Start PWM's internal counters.
   */
  void tinyfoc_pwm_start(void);

  /**
   * @brief  Turn on PWM waveform generation.
   */
  void tinyfoc_pwm_drive_on(void);

  /**
   * @brief  Turn off PWM waveform generation.
   */
  void tinyfoc_pwm_drive_off(void);

  /**
   * @brief  Turn off the high sides of power switches.
   * @note   It is used for boot charge or emergency stop.
   */
  void tinyfoc_pwm_drive_low(void);

  /**
   * @brief  Writes into peripheral registers the new duty cycles and sampling point.
   * 
   * @param [in] p_pwm_duty - pointer to the new duty cycles of PWM.
   * @return 0 if no error occurred or
   *         -1 if the duty cycles were set too late.
   */
  int tinyfoc_pwm_set_duty(q15_pwm_duty_t *p_pwm_duty);

  /**
   * @brief  The user main entry of tinysoc motor.
   */
  void tinyfoc_motor_app_main(void);

  /**
   * @brief  The user command to start/stop motor.
   * @param [in] cmd - 0=stop 1=start -1=toggle
   * @return 0 for prev stopped;
   *         1 for prev started;
   *         -1 for unknown status.
   */
  int tinyfoc_motor_app_startstop(int cmd);

  /**
   * @brief  The user command to set target mechanical speed.
   * @param [in] rpm - target mechanical speed in RPM.
   * @return 0 for ok; -1 for failure.
   */
  int tinyfoc_motor_app_set_target_speed(int rpm);

#ifdef __cplusplus
}
#endif

#endif /* _TINYFOC_API_H_ */
