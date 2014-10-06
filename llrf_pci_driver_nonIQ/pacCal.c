/*
  * Make MM Writing value and Do Calibration program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#include "base.h"

static double aoffset = 0.223606798;
static double ai = 0, pi = 0;
static double old_psk_offset;
static int loc;
static unsigned short old_calib_mode;
static double org_deg[LLRF_PERIOD*2] = 
{
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13,
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13
};
//static double deg[8] = {0, 90, 180, 270, 10, 100, 190, 280};
static double deg[LLRF_PERIOD*2] =
{
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13,
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13
};

static double psk_deg[LLRF_PERIOD*2] =
{
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13,
	1080.0/14*0,
	1080.0/14*1,
	1080.0/14*2,
	1080.0/14*3,
	1080.0/14*4,
	1080.0/14*5,
	1080.0/14*6,
	1080.0/14*7,
	1080.0/14*8,
	1080.0/14*9,
	1080.0/14*10,
	1080.0/14*11,
	1080.0/14*12,
	1080.0/14*13
};

int cal_index; // idex
unsigned short icont[LLRF_SAMPLING];
unsigned short qcont[LLRF_SAMPLING];
unsigned short ampon;

static int save_table()
{
	STO_CALIB stored_calib;
	int fd;
	int i;

	for(i = 0; i < LLRF_CHANNEL; i++) {
		memcpy(stored_calib.aon[i], calib_vars.aon[i], sizeof(calib_vars.aon[i]));
		memcpy(stored_calib.aoff[i], calib_vars.aoff[i], sizeof(calib_vars.aoff[i]));
		memcpy(stored_calib.pon[i], calib_vars.pon[i], sizeof(calib_vars.pon[i]));
		memcpy(stored_calib.poff[i], calib_vars.poff[i], sizeof(calib_vars.poff[i]));
		memcpy(stored_calib.onrt[i], calib_vars.onrt[i], sizeof(calib_vars.onrt[i]));
		memcpy(stored_calib.ofrt[i], calib_vars.ofrt[i], sizeof(calib_vars.ofrt[i]));
		memcpy(stored_calib.agin[i], calib_vars.agin[i], sizeof(calib_vars.agin[i]));
	}

	memcpy(stored_calib.atbl, calib_vars.atbl[1], sizeof(calib_vars.atbl[1]));
	memcpy(stored_calib.ptbl, calib_vars.ptbl[1], sizeof(calib_vars.ptbl[1]));
	memcpy(stored_calib.patbl, calib_vars.patbl[1], sizeof(calib_vars.patbl[1]));
	memcpy(stored_calib.pptbl, calib_vars.pptbl[1], sizeof(calib_vars.pptbl[1]));
	memcpy(stored_calib.pvoff, calib_vars.pvoff[1], sizeof(calib_vars.pvoff[1]));
	memcpy(stored_calib.ppoff, calib_vars.ppoff[1], sizeof(calib_vars.ppoff[1]));

	if( (fd = open("llrf_table.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1 ) {
		printf("%s:%s [%d], open error\n", __FILE__, __func__, __LINE__);
		return(-1);
	}

	if( write(fd, &stored_calib, sizeof(STO_CALIB)) == -1 ) {
		printf("%s:%s [%d], write error\n", __FILE__, __func__, __LINE__);
		return(-1);
	}
	fdatasync(fd);

	close(fd);
}

int timer;
static double cal_psk_amp(double in_A)
{
#if 0
	int i;
	int len = TABLE_SIZE;
	int start, end, mid;
	double out;
	double temp;

	start = 1;
	end = len-1;
	while(1) {
		++timer;
		mid = (end+start)/2;
		temp = mid*0.0125;
		if(in_A == (mid*0.0125) || start > end) {
			i = mid;
			break;
		} else if(in_A < mid*0.0125) {
			end = mid-1;
		} else {
			start = mid+1;
		}
	}
#if 0
	printf("====\n");
	printf("Start point = %d, end point = %d\n", start, end);
	printf("in_A = %lf, mid = %lf, index = %d, %d times\n", in_A, temp, i, timer);
	printf("====\n");
#endif
	timer = 0;
	out = calib_vars.pvoff[1][i-1] + ((in_A - (i-1)*0.0125) / 0.0125) * (calib_vars.pvoff[1][i] - calib_vars.pvoff[1][i-1]);
/*	printf("%lf, %lf, %d, %lf, %lf, %lf\n", out, in_A, i, (i * 0.0125), pc->pvoff[i-1], pc->pvoff[i]);*/

	return in_A+out;
#endif
	return in_A;
}

static double cal_psk_phase(double in_P, double in_A)
{
#if 0
	int i;
	int len = TABLE_SIZE;
	int start, end, mid;
	double out;

	start = 1;
	end = len-1;
	while(1) {
		mid = (end+start)/2;
		if(in_A == mid*0.0125 || start > end) {
			i = mid;
			break;
		} else if(in_A < mid*0.0125) {
			end = mid-1;
		} else {
			start = mid+1;
		}
	}
	out = calib_vars.ppoff[1][i-1] + ((in_A - (i-1)*0.0125) / 0.0125) * (calib_vars.ppoff[1][i] - calib_vars.ppoff[1][i-1]);
/*	printf("%lf, %lf, %d, %lf, %lf, %lf\n", out, in_A, i, (i * 0.0125), pc->ppoff[i-1], pc->ppoff[i]);*/

    return in_P + out;
#endif
	return in_P;
}

static double calib_amplitude(double in)
{
	int i;
	int len = TABLE_SIZE - 1;
	int start, end, mid;
	double target;
	double out;

	target = (in/1.25)*(calib_vars.patbl[1][len]-calib_vars.patbl[1][0]);

	while(start <= end) {
		mid = (end+start)/2;
		if(target == calib_vars.patbl[1][mid+1]) {
			i = mid;
			break;
		} else if(target < calib_vars.patbl[1][mid+1]) {
			end = mid-1;
		} else {
			start = mid+1;
		}
	}
	out = TABLE_AMP_STEP*(i+(target-calib_vars.patbl[1][i])/(calib_vars.patbl[1][i+1]-calib_vars.patbl[1][i]));

	return (out > 1.25) ? 1.25 : out;
}

unsigned short KxRound(const double lfExp)   
{   
	unsigned short lfVal;   

	/* 반올림을 위해 0.5 더한다.   */
	lfVal = (unsigned short)(lfExp + 0.5);   

	return lfVal;   
}  

int do_pac_cal()
{	
	unsigned short ch;
	unsigned short startp, endp;
	double adiff;
	double pdiff;
	double tmpAmp = 0;
	double tmpPha = 0, pskPha = 0;
	double again, pgain;
	unsigned short temp_i[LLRF_PERIOD * 2];
	unsigned short temp_q[LLRF_PERIOD * 2];
	int i = 0, j = 0, idx = 0;
	unsigned int loct;

	if( old_psk_offset != llrfInfo.psk_offset ) {
		for(i = 0; i < LLRF_PERIOD; i++)
			deg[i+LLRF_PERIOD] = org_deg[i] + llrfInfo.psk_offset;
	}
	old_psk_offset = llrfInfo.psk_offset;

	if( old_calib_mode != llrfInfo.calib_mode ) {
		again = 0;
		pgain = 0;
		ai = 0;
		pi = 0;
		switch(llrfInfo.calib_mode) {
			case PAC_CAL:
			case PAD_CAL:
				printf("PAD_CAL\n");
				loc = -20;
				printf("%d\n", loc);
				break;
			case PSK_CAL:
				memcpy((void*)deg, (void*)org_deg, sizeof(deg));
				loc = -20;
				break;
		}
	}
	old_calib_mode = llrfInfo.calib_mode;

	adiff = dac_aval;
	pdiff = dac_pval;
	ch = llrfInfo.selected_ch;

	ampon = AMP_OPERATION;
	switch(llrfInfo.calib_mode) {
		case PID:
			again = adiff;
			pgain = pdiff;

			for(i = 0; i < LLRF_PERIOD; i++) {
				temp_i[i] = KxRound(CONVERT_DAC_I(again, pgain, deg[i]));
				temp_q[i] = KxRound(CONVERT_DAC_Q(again, pgain, deg[i]));
			}

			if(llrfInfo.psk_mode == 1) {
				/* PSK HW ON */
				tmpAmp = cal_psk_amp(again);
				tmpPha = cal_psk_phase(pgain, again);
				memcpy((void*)deg, (void*)org_deg, sizeof(deg));

				for(i = LLRF_PERIOD; i < LLRF_PERIOD*2; i++) {
					temp_i[i] = KxRound(CONVERT_DAC_I(tmpAmp, tmpPha, deg[i]));
					temp_q[i] = KxRound(CONVERT_DAC_Q(tmpAmp, tmpPha, deg[i]));
				}				
			} else {
				/* PSK OFF & PSK SW ON */
				for(i = LLRF_PERIOD; i < LLRF_PERIOD*2; i++) {
					temp_i[i] = temp_i[i-LLRF_PERIOD] * (-1);
					temp_q[i] = temp_q[i-LLRF_PERIOD] * (-1);
				}
			}
			
			if(llrfInfo.pulse_width - llrfInfo.psk_time <= 0 || llrfInfo.psk_mode == 0) {
				/* ignore psk */
				startp = 0;
				endp = llrfInfo.pulse_width;
				for(i = startp; i < endp; i++) {
					icont[i] = temp_i[i%LLRF_PERIOD];		// I data
					qcont[i] = temp_q[i%LLRF_PERIOD];	// Q data
				}
				startp = endp;
				endp = LLRF_SAMPLING;
				for(i = startp; i < endp; i++) {
					icont[i] = 0x7fff;	// I data
					qcont[i] = 0x8000;	// Q data
				}
			} else {
				/* apply psk */
				startp = 0;
				endp = llrfInfo.psk_time;
				for(i = startp; i < endp; i++) {
					icont[i] = temp_i[i%LLRF_PERIOD];		//  I data
					qcont[i] = temp_q[i%LLRF_PERIOD];	//  Q data
				}
				startp = endp;
				endp = llrfInfo.pulse_width;
				for(i = startp; i < endp; i++) {
					icont[i] = temp_i[(i%LLRF_PERIOD)+LLRF_PERIOD];	// I data
					qcont[i] = temp_q[(i%LLRF_PERIOD)+LLRF_PERIOD];	// Q data
				}
				startp = endp;
				endp = LLRF_SAMPLING;
				for(i = startp; i < endp ; i++) {
					icont[i] = 0x7fff;	// I data
					qcont[i] = 0x8000;	// Q data
				}
			}
			break;
		case PAC_CAL:
			loc += 1;
			if( (loc >= 0) && (loc < 1010) ) {
				tmpAmp = TABLE_AMP_STEP * (loc / 10);
			} else if( (loc >= 1050) && (loc < 2060) ) {
				loct = loc % 1050;
				tmpAmp = calib_amplitude(TABLE_AMP_STEP * (loc / 10));
			}

			for(i = 0; i < LLRF_PERIOD; i++) {
				temp_i[i] = KxRound(CONVERT_DAC_I(tmpAmp, 90, deg[i]));
				temp_q[i] = KxRound(CONVERT_DAC_Q(tmpAmp, 90, deg[i]));
			}

			for(i = 0; i < LLRF_SAMPLING; i++) {
				icont[i] = temp_i[i%LLRF_PERIOD];
				qcont[i] = temp_q[i%LLRF_PERIOD];
			}

			if( loc == -15 ) {
				ampon = AMP_OFF;
			} else if( loc >= 2061 ) {
				llrfInfo.calib_mode = PID;
				again = 0;
				pgain = 0;
				ai = 0;
				pi = 0;
				ampon = AMP_RELOAD;

				/* save table */
				save_table();
			}
			break;
		case PAD_CAL:
			loc += 1;
			printf("loc = [%d]\n", loc);
			if( ((loc >= 0) && (loc < 1515)) || ((loc >= 1590) && (loc < 3105)) ) {
				loct = loc % 1590;
				cal_index = loc / 15;
				tmpAmp = TABLE_AMP_STEP * (loct / 15);
				tmpPha = 90;
			}

			for(i = 0; i < LLRF_PERIOD; i++) {
				temp_i[i] = KxRound(CONVERT_DAC_I(tmpAmp, tmpPha, deg[i]));
				temp_q[i] = KxRound(CONVERT_DAC_Q(tmpAmp, tmpPha, deg[i]));
			}

			for(i = 0; i < LLRF_SAMPLING; i++) {
				icont[i] = temp_i[i%LLRF_PERIOD];
				qcont[i] = temp_q[i%LLRF_PERIOD];
			}

			if( loc == -19 ) {
				ampon = AMP_OFF;
			} else if( loc == 1515 ) {
				ampon = AMP_ON;
			} else if( loc == 3115 ) {
				llrfInfo.calib_mode = PID;
				again = 0;
				pgain = 0;
				ai = 0;
				pi = 0;
				ampon = AMP_RELOAD;

				/* save table */
				save_table();
			} else {
				printf("End of PAD calibration\n");
			}
			break;
		case PSK_CAL:
			loc += 1;
			if( ((loc >= 0) && (loc < 1515)) ) {
				cal_index = loc / 15;
				tmpAmp = TABLE_AMP_STEP * cal_index;
				tmpPha = 90;
				pskPha = 90;
				again = tmpAmp;
			} else if( ((loc >= 1590) && (loc < 3105)) ) {
				cal_index = loc / 15;
				tmpAmp = TABLE_AMP_STEP * ((loc - 1590)/15);
				tmpPha = 90;
				pskPha = 90;
			}

			for(i = 0; i < LLRF_PERIOD; i++) {
				temp_i[i] = KxRound(CONVERT_DAC_I(tmpAmp, tmpPha, deg[i]));
				temp_q[i] = KxRound(CONVERT_DAC_Q(tmpAmp, tmpPha, deg[i]));
			}

			for(i = 0; i < LLRF_SAMPLING; i++) {
				icont[i] = temp_i[i%LLRF_PERIOD];
				qcont[i] = temp_q[i%LLRF_PERIOD];
			}

			if( loc == -19 ) {
				ampon = PSK_CAL_STEP_1;
			} else if( loc == 1515 ) {
				ampon = PSK_CAL_STEP_2;
			} else if( loc >= 3105 ) {
				llrfInfo.calib_mode = PID;
				ampon = AMP_RELOAD;

				/* save table */
				save_table();
			}
			break;
		default:
			again = 0;
			pgain = 0;
	}
#if 0
	for(i = 0; i < LLRF_SAMPLING; i++)
		printf("[%d] 0x%hx\n", i, qcont[i]);
#endif
	return 0;
}
