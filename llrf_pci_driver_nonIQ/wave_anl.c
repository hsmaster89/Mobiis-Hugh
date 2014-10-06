/*
  * Calculate Raw data stability program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base.h"

static double arr_i[LLRF_SAMPLING];

int waveAnl(int ch)
{
	double sum_i;
	double mean_i;
	double var_i;
	double temp;
	unsigned short bgix = 0;
	unsigned short enix = 0;
	unsigned short start_pnt, end_pnt, nSample = 0;
	int i;
	int indexi;

	bgix = llrfInfo.startpoint[ch];
	enix = llrfInfo.endpoint[ch];

	start_pnt = START_POINT(bgix);
	end_pnt = END_POINT(enix);
	nSample = RANGE(start_pnt, end_pnt);

	memset(arr_i, 0, sizeof(arr_i));
	indexi = 0;
	sum_i = 0;
	for(i = start_pnt; i <= end_pnt; i++) {
		sum_i += (double)g_raw[ch][i];
		arr_i[indexi++] = (double)g_raw[ch][i];
	}

	mean_i = sum_i/nSample;

	var_i = 0;
	for(i = 0; i < indexi; i++) {
		temp = (arr_i[i] - mean_i);
		var_i += temp * temp;
	}
	if(var_i == 0) var_i = 0;
	else var_i = sqrt(var_i/indexi);

	send_ioc.mean_i[ch] = CONVERT_R_TO_V(mean_i, 32767.5);
	send_ioc.sdev_i[ch] = CONVERT_R_TO_V(var_i, 32767.5);

	return 0;
}
