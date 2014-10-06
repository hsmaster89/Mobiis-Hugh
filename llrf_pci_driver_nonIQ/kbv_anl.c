/*
  * Calculate Klystron beam voltage/current Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base.h"

static double arr_i[LLRF_SAMPLING/4];

int kbvAnl(int ch)
{
	double sum_i[LLRF_PERIOD];
	double sum;
	double *di;
	double mean;
	double var;
	double temp;
	unsigned short bgix1 = 0, bgix2 = 0;
	unsigned short enix1 = 0, enix2 = 0;
	unsigned short start_pnt1, end_pnt1;
	unsigned short start_pnt2, end_pnt2;
	unsigned short nSample1 = 0, nSample2 = 0;
	int i;
	int index, index2;

	if( !ch ) {
		bgix1 = llrfInfo.kbv_startpoint[0];
		enix1 = llrfInfo.kbv_endpoint[0];
		bgix2 = llrfInfo.kbv_startpoint[1];
		enix2 = llrfInfo.kbv_endpoint[1];
	} else {
		bgix1 = llrfInfo.kbc_startpoint[0];
		enix1 = llrfInfo.kbc_endpoint[0];
		bgix2 = llrfInfo.kbc_startpoint[1];
		enix2 = llrfInfo.kbc_endpoint[1];
	}
	
	start_pnt1 = START_POINT(bgix1);
	end_pnt1 = END_POINT(enix1);
	nSample1 = RANGE(start_pnt1, end_pnt1);

	start_pnt2 = START_POINT(bgix2);
	end_pnt2 = END_POINT(enix2);
	nSample2 = RANGE(start_pnt2, end_pnt2);
	
	if( !nSample1 || !nSample2 )
		return (-1);

	/* Time window 1 */
	sum = 0;
	for(i = start_pnt1; i <= end_pnt1; i++) {
		sum += (double)g_raw[LLRF_CHANNEL-2+ch][i];
	}

	mean = sum/nSample1;

	var = 0;
	for(i = start_pnt1; i < end_pnt1; i++) {
		temp = ((double)g_raw[LLRF_CHANNEL-2+ch][i] - mean);
		var += temp * temp;
	}
	var = sqrt(var/nSample1);
	
	if( !ch ) {
		send_ioc.kbv_mean[0] = CONVERT_R_TO_V(mean, 0);
		send_ioc.kbv_sdev[0] = CONVERT_R_TO_V(var, 0);
		send_ioc.kbv_perc[0] = PERCENTAGE(send_ioc.kbv_sdev[0], send_ioc.kbv_mean[0]);

		if( send_ioc.kbv_perc[0] > llrfInfo.kbv_stb_xr[0] )
			send_ioc.kbv_stb_over[0] = 1;
		else
			send_ioc.kbv_stb_over[0] = 0;
	} else {
		send_ioc.kbc_mean[0] = CONVERT_R_TO_V(mean, 0)/50;
		send_ioc.kbc_sdev[0] = CONVERT_R_TO_V(var, 0)/50;
		send_ioc.kbc_perc[0] = PERCENTAGE(send_ioc.kbc_sdev[0], send_ioc.kbc_mean[0]);

		if( send_ioc.kbc_perc[0] > llrfInfo.kbc_stb_xr[0] )
			send_ioc.kbc_stb_over[0] = 1;
		else
			send_ioc.kbc_stb_over[0] = 0;
	}
	
	/* Time window 2 */
	sum = 0;
	for(i = start_pnt2; i <= end_pnt2; i++) {
		sum += (double)g_raw[LLRF_CHANNEL-2+ch][i];
	}

	mean = sum/nSample2;

	var = 0;
	for(i = start_pnt2; i < end_pnt2; i++) {
		temp = ((double)g_raw[LLRF_CHANNEL-2+ch][i] - mean);
		var += temp * temp;
	}
	var = sqrt(var/nSample2);

	if( !ch ) {
		send_ioc.kbv_mean[1] = CONVERT_R_TO_V(mean, 0);
		send_ioc.kbv_sdev[1] = CONVERT_R_TO_V(var, 0);
		send_ioc.kbv_perc[1] = PERCENTAGE(send_ioc.kbv_sdev[1], send_ioc.kbv_mean[1]);

		if( send_ioc.kbv_perc[1] > llrfInfo.kbv_stb_xr[1] )
			send_ioc.kbv_stb_over[1] = 1;
		else
			send_ioc.kbv_stb_over[1] = 0;
	} else {
		send_ioc.kbc_mean[1] = CONVERT_R_TO_V(mean, 0)/50;
		send_ioc.kbc_sdev[1] = CONVERT_R_TO_V(var, 0)/50;
		send_ioc.kbc_perc[1] = PERCENTAGE(send_ioc.kbc_sdev[1], send_ioc.kbc_mean[1]);

		if( send_ioc.kbc_perc[1] > llrfInfo.kbc_stb_xr[1] )
			send_ioc.kbc_stb_over[1] = 1;
		else
			send_ioc.kbc_stb_over[1] = 0;
	}
	
	return 0;
}
