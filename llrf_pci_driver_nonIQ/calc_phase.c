/*
  * Calculate Phase Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "base.h"

PHA_VAR pha_vars;

static double cal_phase(int ch, unsigned short amode, double pin, double ain)
{
	int i;
	int len = TABLE_SIZE - 1;
	int start, end, mid;
	double out;
	double *atbl, *ptbl, *ratio;
	
#if 1
	if( ON == amode ) {
		atbl = calib_vars.aon[ch];
		ptbl = calib_vars.pon[ch];
		ratio = calib_vars.onrt[ch];
	} else {
		atbl = calib_vars.aoff[ch];
		ptbl = calib_vars.poff[ch];
		ratio = calib_vars.ofrt[ch];
	}

	/* Binary search */
	start = 1;
	end = len-1;
	while(1) {
		mid = (end+start)/2;
		if(ain == atbl[mid] || start > end) {
			i = mid;
			break;
		} else if(ain < atbl[mid]) {
			end = mid-1;
		} else {
			start = mid+1;
		}
	}
	out = pin - (ptbl[i] + (ain - atbl[i]) * ratio[i]);
#else
	out = pin;
#endif
	return out;
}

int calc_phase(int ch)
{
	double sum;
	double temp;
	double phase;
	double degree[] = {0, 90, 180, 270};
	double diff;
	double cwmean, camean;
	double index0;
	double opha;
	double i_data, q_data;
	int i, index;
	unsigned short amode;
	unsigned short rmode;

	amode = llrfInfo.amp[ch];
	rmode = llrfInfo.calib_mode;
	camean = amp_vars.g_amplitude[ch];
	cwmean = pha_vars.g_phase[0];

	i_data = apcom_vars.i_data;
	q_data = apcom_vars.q_data;

	phase = CONVERT_R_TO_P(i_data, q_data);
	send_ioc.mean_pha[ch] = phase;
	opha = phase;

	if(!(PAD_CAL == rmode)) {
		temp = cal_phase(ch, amode, opha, camean);
		if( temp < 0 ) temp += 360;
		else if( temp >= 360 ) temp -= 360;
	}
	pha_vars.g_cpha[ch] = temp;

	if( ch != 0 ) temp = pha_vars.g_cpha[ch] - cwmean;
	if( temp < 0 ) pha_vars.g_phase[ch] = temp + 360;
	else if( temp >= 360 ) pha_vars.g_phase[ch] = temp - 360;

	if( temp < 0 ) pha_vars.g_phase[ch] = temp + 360;
	else if( temp >= 360 ) pha_vars.g_phase[ch] = temp - 360;
	else pha_vars.g_phase[ch] = temp;

	return 0;
}

