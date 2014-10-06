/*
  * Make Output value Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base.h"

#define MAX_INPT_AMP	14.9

extern LLRF_INFO llrfInfo;
extern IOC_SEND send_ioc;
extern AMP_VAR amp_vars;
extern PHA_VAR pha_vars;

static int do_flag_amp;
static int do_flag_pha;

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
			temp = 10 * log10((amp_vars.g_oamplitude[i+1] * amp_vars.g_oamplitude[i+1]) * 20);
			temp += llrfInfo.module_loss[i+1];
			temp = sqrt(pow(10, temp/10)/20);
			*curAmp += (temp * llrfInfo.amp_weight[i]);
			temp = llrfInfo.ata[i] - llrfInfo.external_loss[i+1];
			if( temp > llrfInfo.target_max ) temp = llrfInfo.target_max;
			temp = sqrt(pow(10, temp/10)/20);
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
	*ploav = sqrt(pow(10, llrfInfo.ploa/10)/20);
	*phiav = sqrt(pow(10, llrfInfo.phia/10)/20);
}

static double llrf_mod(double val)
{
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
	static double prev_ai[2] = {0,}, ai = 0, oldai = 0, aval = 0;
	static double prev_pval[2] = {0,}, pval = 0;
	static int flag = 0;
	double tarAmp = 0, tarPha = 0;
	double curAmp = 0, curPha = 0;
	double ploav, phiav;
	double diff;
	double temp;
	unsigned short doubled_loop;
	make_val(&tarAmp, &curAmp, &tarPha, &curPha, &ploav, &phiav, &doubled_loop);

	if(llrfInfo.pac_power == 1) {
		// PAC power off
		aval = 0.0000223607;
		pval = 0;
		send_ioc.outputAmp = -80;
		send_ioc.outputPha = pval;
	} else {
		// PAC power on (PM or CW)
		if( !do_flag_amp && llrfInfo.feedback_mode == 1 ) {
			diff = tarAmp - curAmp;

			if( doubled_loop ) ai = prev_ai[flag];
			else ai = oldai;

			if( fabs(diff) > 0.001 ) ai += (llrfInfo.aki * diff);
			else if( fabs(diff) > 0.0001 ) ai += ((llrfInfo.aki/2) * diff);

			if( ai < ploav ) ai = ploav;
			else if( ai > phiav ) ai = phiav;

			if( ai >= 1.25 ) oldai = 1.25;
			else if( ai <= 0 ) oldai = 0.00000000001;
			else oldai = ai;

			if( doubled_loop ) prev_ai[flag] = oldai;

			temp = 10 * log10((oldai * oldai) * 20) + llrfInfo.pac_loss;
			aval = sqrt(0.05 * pow(10, temp/10));

			if( aval >= 1.25 ) aval = 1.25;
			else if( aval < 0 ) aval = 0;
		} else {
			temp = llrfInfo.initpower - llrfInfo.constant_loss + llrfInfo.pac_loss;
			aval = sqrt(0.05 * pow(10, temp/10));
			if( aval >= 1.25 ) aval = 1.25;
			else if( aval < 0 ) aval = 0;
		}
		if(aval == 0)
			send_ioc.outputAmp = -80;
		else
			send_ioc.outputAmp = 10 * log10(pow(aval, 2) * 20);

		if( !do_flag_pha && llrfInfo.feedback_mode == 1 ) {
			diff = tarPha - curPha;

			if( diff > 179 ) diff = 179;
			else if( diff < -179 ) diff = -179;

			if( fabs(diff) > 0.1 ) {
				if( doubled_loop ) prev_pval[flag] += (llrfInfo.pki * diff);
				else pval += (llrfInfo.pki * diff);
			}

			if( fabs(diff) > 0.01 ) {
				if( doubled_loop ) prev_pval[flag] += ((llrfInfo.pki/2) * diff);
				else pval += ((llrfInfo.pki/2) * diff);
			}

			if( doubled_loop ) pval = llrf_mod(prev_pval[flag]);
			else pval = llrf_mod(pval);
			
		} else {
			pval = llrf_mod(llrfInfo.initphase);
		}
		send_ioc.outputPha = pval;

		if( doubled_loop ) flag = (flag - 1) * (-1);
		else flag = 0;
	}
	
	return 0;
}
