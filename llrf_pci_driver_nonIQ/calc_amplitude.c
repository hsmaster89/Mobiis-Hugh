/*
  * Calculate Amplitude program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"

AMP_VAR amp_vars;
AMP_PHA_COMMON apcom_vars;

static double cal_amplifier_offset(int ch, double in)
{
	int i;
	int len = TABLE_SIZE;
	int start, end, mid;
	double out;

	start = 1;
	end = len-1;
	while(1) {
		mid = (end+start)/2;
		if(in == calib_vars.aon[ch][mid] || start > end) {
			i = mid;
			break;
		} else if(in < calib_vars.aon[ch][mid]) {
			end = mid-1;
		} else {
			start = mid+1;
		}
	}

	if((calib_vars.aon[ch][i] - calib_vars.aon[ch][i-1]) == 0)
		out = 0;
	else
		out = calib_vars.agin[ch][i-1] + ((in - calib_vars.aon[ch][i-1]) / (calib_vars.aon[ch][i] - calib_vars.aon[ch][i-1])) * (calib_vars.agin[ch][i] - calib_vars.agin[ch][i-1]);
	return out;	
}

int calc_amplitude(int ch)
{
	double sum;
	double eloss, mloss;
	double interAtten1, interAtten2;
	double temp;
	double cmp_dbm;
	double mean;
	double sum_i, sum_q;
	double peak_i, peak_q, peak;
	double temp_i, temp_q;
	double max_peak;
	int bgix, enix;
	int start_pnt, end_pnt, nSample;
	int nSample_I, nSample_Q;
	int i, index;
	double *di;
	unsigned short amode;

	bgix = llrfInfo.startpoint[ch];
	enix = llrfInfo.endpoint[ch];
	amode = llrfInfo.amp[ch];
	eloss = llrfInfo.external_loss[ch];
	mloss = llrfInfo.module_loss[ch];
	interAtten1 = llrfInfo.interAtten[0];
	interAtten2 = llrfInfo.interAtten[1];
	
	start_pnt = START_POINT(bgix);
	end_pnt = END_POINT(enix);
	nSample = NSAMPLES(start_pnt, end_pnt);

	nSample_I = NSAMPLE_I(nSample);
	nSample_Q = NSAMPLE_Q(nSample);

	if( !nSample ) {
		printf("%s:%s [%d], nSample is zero\n", __FILE__, __func__, __LINE__);
		return (-1);
	}

	sum_i = sum_q = 0;
	peak_i = peak_q = 0;
	max_peak = -99999;
	for(i = start_pnt; i <= end_pnt; i++) {
		index = i % LLRF_PERIOD;
		switch(index) {
			case 0:
				temp_q = (double)g_raw[ch][i];
				sum_q += temp_q;
				peak_q += temp_q;
				break;
			case 1:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+5])/1.949855824;
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+5])/0.445041868;
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
			case 2:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+3])/0.867767478;
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+3])/(-1.801937736);
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
			case 3:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+1])/(-1.563662965);
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+1])/(-1.246979604);
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
			case 7:
				temp_q = (double)g_raw[ch][i] * (-1);
				sum_q += temp_q;
				peak_q += temp_q;
				break;
			case 8:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+5])/(-1.949855824);
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+5])/(-0.445041868);
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
			case 9:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+3])/(-0.867767478);
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+3])/1.801937736;
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
			case 10:
				temp_i = ((double)g_raw[ch][i] + (double)g_raw[ch][i+1])/1.563662965;
				temp_q = ((double)g_raw[ch][i] - (double)g_raw[ch][i+1])/1.246979604;
				
				sum_i += temp_i;
				sum_q += temp_q;

				peak_i += temp_i;
				peak_q += temp_q;
				break;
		}

		/* Calculation peak power */
		if(index == LLRF_PERIOD-1) {
			peak_i = CONVERT_R_TO_V((peak_i/6), 32767.5);
			peak_q = CONVERT_R_TO_V((peak_q/8), 32767.5);
			peak = CONVERT_R_TO_A(peak_i, peak_q);

			if(max_peak < peak) {
				max_peak = peak;
			}
		}
	}

	/* Convert Voltage */
	sum_i = CONVERT_R_TO_V((sum_i/nSample_I), 32767.5);
	sum_q = CONVERT_R_TO_V((sum_q/nSample_Q), 32767.5);

	/* Using this variable in calc_phase program */
	/* Escape duplicate calculations to use this structures */
	apcom_vars.i_data = sum_i;
	apcom_vars.q_data = sum_q;
	
	/* Average */
	mean = CONVERT_R_TO_A(sum_i, sum_q);
	
	amp_vars.g_amplitude[ch] = mean;
	send_ioc.mean_amp[ch] = mean;

	amp_vars.g_peak[ch] = max_peak;

	if( amode == ON ) {
		amp_vars.g_oamplitude[ch] = amp_vars.g_amplitude[ch] - cal_amplifier_offset(ch, amp_vars.g_amplitude[ch]);
		temp = amp_vars.g_peak[ch] - cal_amplifier_offset(ch, amp_vars.g_peak[ch]);
	}else {
		amp_vars.g_oamplitude[ch] = amp_vars.g_amplitude[ch];
		temp = amp_vars.g_peak[ch];
	}

	/* ADC Overflow detect */
	cmp_dbm = CONVERT_V_TO_D(amp_vars.g_oamplitude[ch]);
	if( cmp_dbm > 14.9 )
		send_ioc.adc_over[ch] = 1;
	else
		send_ioc.adc_over[ch] = 0;

	if( 1 == ch ) {
		amp_vars.g_damplitude[ch] = CONVERT_V_TO_D(amp_vars.g_oamplitude[ch]) + mloss + eloss + interAtten1;
		send_ioc.peakpower[ch] = CONVERT_V_TO_D(temp) + mloss + eloss + interAtten1;
		amp_vars.g_pamplitude[ch] = CONVERT_D_TO_W(amp_vars.g_damplitude[ch]-mloss-eloss-interAtten1);
	} else if( 2 == ch ) {
		amp_vars.g_damplitude[ch] = CONVERT_V_TO_D(amp_vars.g_oamplitude[ch]) + mloss + eloss + interAtten2;
		send_ioc.peakpower[ch] = CONVERT_V_TO_D(temp) + mloss + eloss + interAtten2;
		amp_vars.g_pamplitude[ch] = CONVERT_D_TO_W(amp_vars.g_damplitude[ch]-mloss-eloss-interAtten2);
	} else {
		amp_vars.g_damplitude[ch] = CONVERT_V_TO_D(amp_vars.g_oamplitude[ch]) + mloss + eloss;
		send_ioc.peakpower[ch] = CONVERT_V_TO_D(temp) + mloss + eloss;
		amp_vars.g_pamplitude[ch] = CONVERT_D_TO_W(amp_vars.g_damplitude[ch]-mloss-eloss);

		if( ch >= 6 ) {
			if( send_ioc.peakpower[ch] > llrfInfo.ppmxr[ch-6] )
				send_ioc.pp_interlock[ch-6] = 1;
			else
				send_ioc.pp_interlock[ch-6] = 0;
		}
	}

	return 0;
}

