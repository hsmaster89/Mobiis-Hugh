/*
  * Calculate RMS Stability program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base.h"

static int first[LLRF_CHANNEL-2] = {0,}, last[LLRF_CHANNEL-2] = {0,};
static int curs[LLRF_CHANNEL-2] = {0,};
static double ratbl[LLRF_CHANNEL-2][MAX_RMS_SAMPLE] = {0,};
static double rptbl[LLRF_CHANNEL-2][MAX_RMS_SAMPLE] = {0,};
static double pamp[LLRF_CHANNEL-2] = {0,};
static double ppha[LLRF_CHANNEL-2] = {0,};

static double asum[LLRF_CHANNEL-2] = {0,};
static double amean[LLRF_CHANNEL-2] = {0,};
static double psum[LLRF_CHANNEL-2] = {0,};
static double pmean[LLRF_CHANNEL-2] = {0,};

static double asdev[LLRF_CHANNEL-2] = {0,};
static double psdev[LLRF_CHANNEL-2] = {0,};
static double rmsa[LLRF_CHANNEL-2] = {0,};
static double rmsp[LLRF_CHANNEL-2] = {0,};
static double perc[LLRF_CHANNEL-2] = {0,};

static double org_pha[LLRF_CHANNEL-2] = {0,};
static double invalid_amp[LLRF_CHANNEL-2][INVALID_TBL_SIZE] = {0,};
static double invalid_phase[LLRF_CHANNEL-2][INVALID_TBL_SIZE] = {0,};
static int ivc[LLRF_CHANNEL-2] = {0,};
static unsigned int old_range = 1;

static double olda[LLRF_CHANNEL-2];
static double oldp [LLRF_CHANNEL-2];

static void cal_rms(double aval, double pval, int ch)
{
	unsigned int range = llrfInfo.rms_sample;
	
	olda[ch] = 0;
	oldp[ch] = 0;
	
	if( curs[ch] >= range ) {
		olda[ch] = ratbl[ch][first[ch]];
		oldp[ch] = rptbl[ch][first[ch]];
		first[ch] = (++first[ch] >= range) ? 0 : first[ch];
	} else {
		curs[ch]++;
	}

	if( pval >= 180 ) pval -= 360;

	if( (pval - ppha[ch]) > 180 ) pval -=360;
	else if( (pval - ppha[ch]) < (-180) ) pval += 360;

	ratbl[ch][last[ch]] = aval;
	rptbl[ch][last[ch]] = pval;
	ppha[ch] = rptbl[ch][last[ch]];
	pamp[ch] = ratbl[ch][last[ch]];

	if( range == 1 ) {
		asum[ch] = ratbl[ch][last[ch]];
		amean[ch] = ratbl[ch][last[ch]];

		psum[ch] = rptbl[ch][last[ch]];
		pmean[ch] = rptbl[ch][last[ch]];
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
	memset(ratbl[ch], 0, sizeof(ratbl[ch]));
	memset(rptbl[ch], 0, sizeof(rptbl[ch]));
	first[ch] = 0;
	last[ch] = 0;
	asum[ch] = 0;
	psum[ch] = 0;
	ppha[ch] = 0;
	curs[ch] = 0;
	org_pha[ch] = 0;
	ivc[ch] = 0;
}

int make_rms_data(int ch)
{	
	double cDbm, oDbm, diff, diff2;
	double aval, pval;
	double avar, pvar;
	double temp;
	double eloss, mloss;
	double ivid;
	double ipid;
	unsigned int ivmi;
	int i = 0;
	int cur = 0;

	if( llrfInfo.rms_sample != old_range) {
		initial_val(ch);
		if(ch == 9)
			old_range = llrfInfo.rms_sample;
	}

	ivmi = llrfInfo.ivmi;
	ivid = llrfInfo.ivid;
	ipid = llrfInfo.ipid;

	eloss = llrfInfo.external_loss[ch];
	mloss = llrfInfo.module_loss[ch];

	aval = amp_vars.g_pamplitude[ch];
	pval = pha_vars.g_phase[ch];

	if( isnan(aval) || isinf(aval) || isnan(pval) || isinf(pval) )
		goto skip_cal;

#if 0
	cDbm = 10 * log10(aval);
	oDbm = 10 * log10(pamp[ch]);
	
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

	cur = first[ch];
	avar = 0;
	pvar = 0;

	while( i < curs[ch] ) {
		temp = ratbl[ch][cur] - amean[ch];
		avar += temp * temp;

		temp = rptbl[ch][cur] - pmean[ch];
		pvar += temp * temp;

		if( (++cur) >= curs[ch] )
			cur = 0;
		i++;
	}

	avar = sqrt(avar / curs[ch]);
	asdev[ch] = CONVERT_W_TO_D(avar);
	rmsa[ch] = CONVERT_W_TO_D(amean[ch]) + eloss + mloss;

	if( amean[ch] == 0 ) perc[ch] = 0;
	else perc[ch] = PERCENTAGE(avar, amean[ch]);

	pvar = pvar / curs[ch];
	psdev[ch] = sqrt(pvar);

	if( pmean[ch] > 360 ) rmsp[ch] = pmean[ch] - 360;
	else if( pmean[ch] < 0 ) rmsp[ch] = pmean[ch] + 360;
	else rmsp[ch] = pmean[ch];

	send_ioc.rms_perc[ch] = perc[ch];
	send_ioc.rms_psdev[ch] = psdev[ch];

	if( ch == 0 ) {
		if( send_ioc.rms_perc[ch] > llrfInfo.ref_amp_stb_xr )
			send_ioc.ref_amp_stb_over = 1;
		else
			send_ioc.ref_amp_stb_over = 0;

		if( send_ioc.rms_psdev[ch] > llrfInfo.ref_pha_stb_xr )
			send_ioc.ref_pha_stb_over = 1;
		else
			send_ioc.ref_pha_stb_over = 0;
	}
skip_cal:
	return 0;
}

