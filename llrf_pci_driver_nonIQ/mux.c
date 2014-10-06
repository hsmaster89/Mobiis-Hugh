/*
  * Make Output value Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "base.h"

#define MAX_INPT_AMP	14.9

static double prev_ai[2] = {0,};
static double prev_dac_pval[2] = {0,};
static int flag = 0;
static int do_flag_amp;
static int do_flag_pha;

double dac_aval;
double dac_pval;

static void make_val(double *tarAmp, double *curAmp
					, double *tarPha, double *curPha
					, double *ploav, double *phiav
					, unsigned short *doubled_loop)
{
	double weight;
	double temp;
	int i;

	weight = 0;
	for(i = 0; i < 5; i++) {
		if( llrfInfo.pfb[i] ) {
			*curPha = (pha_vars.g_phase[i+1] * llrfInfo.pha_weight[i]);
			*tarPha = (llrfInfo.pta[i] * llrfInfo.pha_weight[i]);
			weight += llrfInfo.pha_weight[i];
		}
	}

	if( weight ) {
		*curPha /= weight;
		*tarPha /= weight;
		do_flag_pha = 0;
	} else {
		*curPha = 0;
		*tarPha = 0;
		do_flag_pha = 1;
	}
	
	weight = 0;
	for(i = 0; i < 5; i++) {
		if( llrfInfo.afb[i] ) {
			temp = CONVERT_V_TO_D(amp_vars.g_oamplitude[i+1]) + llrfInfo.module_loss[i+1];
			temp = CONVERT_D_TO_V(temp);
			*curAmp += (temp * llrfInfo.amp_weight[i]);
			temp = llrfInfo.ata[i] - llrfInfo.external_loss[i+1];
			if( temp > llrfInfo.target_max ) temp = llrfInfo.target_max;
			temp = CONVERT_D_TO_V(temp);
			*tarAmp += (temp * llrfInfo.amp_weight[i]);
			weight += llrfInfo.amp_weight[i];
		}
	}

	if( weight ) {
		*curAmp /= weight;
		*tarAmp /= weight;
		do_flag_amp = 0;
	} else {
		*curAmp = 0;
		*tarAmp = 0;
		do_flag_amp = 1;
	}
	
	*doubled_loop = llrfInfo.doubled_loop;
	*ploav = CONVERT_D_TO_V(llrfInfo.ploa);
	*phiav = CONVERT_D_TO_V(llrfInfo.phia);
}

static double llrf_mod(double val)
{
	/* If val is bigger than 360, make val < 360 and maintain point value */
	double ret = 0;
	double input = fabs(val);
	double temp = 0;
 	long mask = 0;
	int minus = 0;

	if( val < 0 )
		minus = 1;

	mask = input;

	temp = input - mask;
	mask = mask % 360;
	ret = mask + temp;
	if( minus ) {
		ret = ret * (-1);
	}
	return (ret);
}

int mux()
{	
	double tarAmp = 0, tarPha = 0;
	double curAmp = 0, curPha = 0;
	double ploav, phiav;
	double diff;
	double temp;
	double ai;
	unsigned short doubled_loop;

	static int timer = 0;

	make_val(&tarAmp, &curAmp, &tarPha, &curPha, &ploav, &phiav, &doubled_loop);

	/* Feedback amplitude */
	if(llrfInfo.pac_power == 0) {
		/* Power off */
		/* If PAC power off -> applying -80dBm value */
		if(step_flag == 2) {
			dac_aval = CONVERT_D_TO_V(step_const_power);
		} else {
			dac_aval = 0.00002236067977499790;
			/* Initial value of step_const_power */
			step_const_power = -40;
		}
	} else {
		/* Power On */
		/* Feedback mode : 0 -> Local, 1 -> Beam-based feedback */
		if(llrfInfo.feedback_mode == 0) {
			/* If state is ready to feedback, do feedback, else using constant power */
			if(!do_flag_amp) {
				/* Do feedback */
				diff = tarAmp - curAmp;

				if(doubled_loop) {
					if(timer == 1) {
						ai = prev_ai[(flag-1)*(-1)];
					} else {
						ai = prev_ai[flag];
					}
				} else {
					ai = prev_ai[flag];
				}
				
				if( fabs(diff) > 0.001 ) ai += (llrfInfo.aki * diff);
				else if( fabs(diff) > 0.0001 ) ai += ((llrfInfo.aki/2) * diff);

				if( ai < ploav ) ai = ploav;
				else if( ai > phiav ) ai = phiav;

				if( ai >= 1.25 ) prev_ai[flag] = 1.25;
				else if( ai <= 0 ) prev_ai[flag] = 0.00000000001;
				else prev_ai[flag] = ai;

				temp = CONVERT_V_TO_D(prev_ai[flag]) + llrfInfo.pac_loss;
				dac_aval = CONVERT_D_TO_V(temp);

				if( dac_aval >= 1.25 ) dac_aval = 1.25;
				else if( dac_aval < 0 ) dac_aval = 0;
			} else {
				/* Constant power */
				if(step_flag == 1) {
					/* When PAC power state is changed 'OFF' to others */
					temp = step_const_power;
				} else {
					temp = llrfInfo.initpower - llrfInfo.constant_loss + llrfInfo.pac_loss;
				}
				temp = CONVERT_D_TO_V(temp);

				if(temp >= 1.25) prev_ai[flag] = 1.25;
				else if(temp <= 0) prev_ai[flag] = 0.00000000001;
				else prev_ai[flag] = temp;
				
				dac_aval = temp;
				if( dac_aval >= 1.25 ) dac_aval = 1.25;
				else if( dac_aval < 0 ) dac_aval = 0;
			}
		}  else if(llrfInfo.feedback_mode == 1) {
			/* Test value */
			dac_aval = 0.354392892;
		}

		/* Maintain current power in step constant power */
		/* This routine must operate in PAC power on state */
		if(step_flag == 0)
			step_const_power = send_ioc.outputAmp;
	}

	send_ioc.outputAmp = CONVERT_V_TO_D(dac_aval);

	/* Feedback phase */
	if(llrfInfo.pac_power == 0) {
		dac_pval = 0;
	} else {
		if(llrfInfo.feedback_mode == 0) {
			if(!do_flag_pha) {
				diff = tarPha - curPha;

				if( diff > 179 ) diff = 179;
				else if( diff < -179 ) diff = -179;

				if( fabs(diff) > 0.1 ) {
					if( doubled_loop ) {
						if(timer == 1) {
							prev_dac_pval[flag] = prev_dac_pval[(flag-1)*(-1)];
						} else {
							prev_dac_pval[flag] += (llrfInfo.pki * diff);
						}
					} else {
						dac_pval += (llrfInfo.pki * diff);
					}
				} else if( fabs(diff) > 0.01 ) {
					if( doubled_loop ) {
						if(timer == 1) {
							prev_dac_pval[flag] = prev_dac_pval[(flag-1)*(-1)];
						} else {
							prev_dac_pval[flag] += ((llrfInfo.pki/2) * diff);
						}
					} else {
						dac_pval += ((llrfInfo.pki/2) * diff);
					}
				}
				
				if( doubled_loop ) dac_pval = llrf_mod(prev_dac_pval[flag]);
				else dac_pval = llrf_mod(dac_pval);
			} else {
				dac_pval = llrf_mod(llrfInfo.initphase);
			}
		} else if(llrfInfo.feedback_mode == 1){
			dac_pval = 20;
		}
	}

	send_ioc.outputPha = dac_pval;

	if( doubled_loop ) {
		flag = (flag - 1) * (-1);
		if(timer < 2) ++timer;
	} else {
		flag = 0;
		timer = 0;
	}

	return 0;
}
