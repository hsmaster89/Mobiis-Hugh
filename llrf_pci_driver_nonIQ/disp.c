/*
  * Calculate Displayed data Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base.h"

static double org_pha[LLRF_CHANNEL-2] = {0,};
static double invalid_amp[LLRF_CHANNEL-2][INVALID_TBL_SIZE] = {0,};
static double invalid_phase[LLRF_CHANNEL-2][INVALID_TBL_SIZE] = {0,};
static int ivc[LLRF_CHANNEL-2] = {0,};
static unsigned int old_range = 1;
static int first[LLRF_CHANNEL-2] = {0,}, last[LLRF_CHANNEL-2] = {0,};
static int curs[LLRF_CHANNEL-2] = {0,};

static double datbl[LLRF_CHANNEL-2][MAX_RMS_SAMPLE] = {0,};
static double dptbl[LLRF_CHANNEL-2][MAX_RMS_SAMPLE] = {0,};
static double pamp[LLRF_CHANNEL-2] = {0,};
static double ppha[LLRF_CHANNEL-2] = {0,};

static double asum[LLRF_CHANNEL-2] = {0,};
static double amean[LLRF_CHANNEL-2] = {0,};
static double psum[LLRF_CHANNEL-2] = {0,};
static double pmean[LLRF_CHANNEL-2] = {0,};

static double olda[LLRF_CHANNEL-2] = {0,};
static double oldp [LLRF_CHANNEL-2]= {0,};

static void cal_rms(double aval, double pval, int ch)
{
	unsigned int range = llrfInfo.dis_sample;

	olda[ch] = 0;
	oldp[ch] = 0;
	
	if( curs[ch] >= range ) {
		olda[ch] = datbl[ch][first[ch]];
		oldp[ch] = dptbl[ch][first[ch]];
		first[ch] = (++first[ch] >= range) ? 0 : first[ch];
	} else {
		curs[ch]++;
	}

	datbl[ch][last[ch]] = aval;
	dptbl[ch][last[ch]] = pval;
	ppha[ch] = dptbl[ch][last[ch]];
	pamp[ch] = datbl[ch][last[ch]];

	if( range == 1 ) {
		asum[ch] = datbl[ch][last[ch]];
		amean[ch] = datbl[ch][last[ch]];

		psum[ch] = dptbl[ch][last[ch]];
		pmean[ch] = dptbl[ch][last[ch]];
	} else {
		asum[ch] = asum[ch] + aval - olda[ch];
		amean[ch] = asum[ch] / curs[ch];

		psum[ch] = psum[ch] + pval - oldp[ch];
		pmean[ch] = psum[ch] / curs[ch];
	}

	last[ch] = (++last[ch] >= range) ? 0 : last[ch];
}

static void initial_val(int ch)
{
	memset(datbl[ch], 0, sizeof(datbl[ch]));
	memset(dptbl[ch], 0, sizeof(dptbl[ch]));
	first[ch] = 0;
	last[ch] = 0;
	asum[ch] = 0;
	psum[ch] = 0;
	ppha[ch] = 0;
	curs[ch] = 0;
	org_pha[ch] = 0;
	ivc[ch] = 0;
}

int make_display_data(int ch)
{	
	double cDbm, oDbm, diff, diff2;
	double aval, pval;
	double ivid;
	double ipid;
	unsigned int ivmi;
	int i = 0;
	int cur = 0;
	
	if( llrfInfo.dis_sample != old_range) {
		initial_val(ch);
		if(ch == 9)
			old_range = llrfInfo.dis_sample;
	}

	ivmi = llrfInfo.ivmi;
	ivid = llrfInfo.ivid;
	ipid = llrfInfo.ipid;

	aval = amp_vars.g_damplitude[ch];
	pval = pha_vars.g_phase[ch];

	if( isnan(aval) || isinf(aval) || isnan(pval) || isinf(pval) )
		goto skip_cal;

#if 0
	cDbm = aval;
	oDbm = pamp[ch];
	diff = fabs(cDbm - oDbm);
	diff2 = fabs(org_pha[ch] - pval);

	if( (ivc[ch] < ivmi) && ((diff > ivid) || (diff2 > ipid)) ) {
		ivc[ch]++;
		invalid_amp[ch][ivc[ch]-1] = aval;
		invalid_phase[ch][ivc[ch]-1] = pval;
		goto skip_cal;
	} else if( (ivc[ch] >= ivmi) && (ivmi != 0) ) {
		for(i = 0; i < ivc[ch]; i++) {
			cal_rms(invalid_amp[ch][i], invalid_phase[ch][i], ch);
		}
	}
	ivc[ch] = 0;
#endif

	org_pha[ch] = pval;

	cal_rms(aval, pval, ch);

	if( ch == 0 ) {
		if( amean[ch] < -1 || amean[ch] > 6 )
			send_ioc.ref_range_over = 1;
		else
			send_ioc.ref_range_over = 0;
	}
	send_ioc.disp_amp[ch] = amean[ch];
	send_ioc.disp_pha[ch] = pmean[ch];
skip_cal:
	return 0;
}

