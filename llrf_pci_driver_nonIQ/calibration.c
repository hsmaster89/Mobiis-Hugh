/*
  * Make Calibration table Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "base.h"

#define ZLP_STATUS			12345670		/* Zero Location Phase status */

static double pha0[LLRF_CHANNEL-2], vpha0[LLRF_CHANNEL-2];
static int hloc[LLRF_CHANNEL-2], vloc[LLRF_CHANNEL-2];

CALIB_VAR calib_vars;

/* user define */
#if 1
static double gphase_amp[TABLE_SIZE] = {
	0
	,0.00244152
	,0.00481136
	,0.00718982
	,0.00955852
	,0.0118973
	,0.0142897
	,0.0166789
	,0.0190513
	,0.021433
	,0.0238046
	,0.026191
	,0.0285688
	,0.0309176
	,0.0332917
	,0.0356615
	,0.0380654
	,0.0404505
	,0.0428305
	,0.0451813
	,0.0475727
	,0.0499299
	,0.0523052
	,0.0547016
	,0.0570823
	,0.059459
	,0.0618573
	,0.0642381
	,0.0665917
	,0.0689523
	,0.0713464
	,0.0737355
	,0.0761376
	,0.0785072
	,0.0808731
	,0.0832689
	,0.0856368
	,0.0880267
	,0.090385
	,0.0927626
	,0.0951399
	,0.0975311
	,0.0999103
	,0.102291
	,0.10467
	,0.10705
	,0.109416
	,0.111785
	,0.114172
	,0.116567
	,0.118944
	,0.121353
	,0.123729
	,0.1261
	,0.128472
	,0.130856
	,0.133227
	,0.135599
	,0.137974
	,0.140372
	,0.14276
	,0.145104
	,0.147544
	,0.149912
	,0.152282
	,0.154698
	,0.157069
	,0.15944
	,0.161813
	,0.164196
	,0.166611
	,0.168981
	,0.171382
	,0.17377
	,0.176132
	,0.17853
	,0.180894
	,0.183301
	,0.185698
	,0.188076
	,0.190446
	,0.192818
	,0.195192
	,0.197557
	,0.199948
	,0.202349
	,0.20473
	,0.207101
	,0.20948
	,0.21187
	,0.214266
	,0.216651
	,0.219016
	,0.221418
	,0.223803
	,0.226171
	,0.228564
	,0.230936
	,0.233324
	,0.235678
	,0.238091
};

static double gphase_offset[TABLE_SIZE] = {
	0.585584
	,0.158974
	,-0.267636
	,-0.12572
	,-0.141734
	,-0.0732216
	,-0.0553862
	,0.159675
	,0.234375
	,0.0937688
	,0.0539315
	,0.122832
	,0.092167
	,0.0525015
	,-0.00310013
	,0.00862702
	,0.0293899
	,-0.024456
	,-0.0197979
	,-0.00668914
	,-0.00324925
	,-0.00722462
	,-0.0133305
	,0.0115873
	,0.011679
	,0
	,-0.0069889
	,0.0492856
	,0.0441459
	,0.0361523
	,0.0098059
	,-0.00355669
	,-0.0440032
	,-0.0405354
	,-0.0449821
	,-0.0518577
	,-0.0371531
	,-0.0631852
	,-0.0474876
	,-0.0510719
	,-0.0974121
	,-0.0985547
	,-0.11526
	,-0.136769
	,-0.157754
	,-0.140914
	,-0.147722
	,-0.1456
	,-0.137473
	,-0.123496
	,-0.135661
	,-0.131039
	,-0.125983
	,-0.141958
	,-0.153696
	,-0.153686
	,-0.157396
	,-0.158731
	,-0.14865
	,-0.155444
	,-0.141667
	,-0.149313
	,-0.152694
	,-0.151651
	,-0.168583
	,-0.154469
	,-0.177869
	,-0.195558
	,-0.201595
	,-0.205146
	,-0.211193
	,-0.218498
	,-0.228593
	,-0.220798
	,-0.225201
	,-0.221612
	,-0.205368
	,-0.226017
	,-0.22273
	,-0.228915
	,-0.233829
	,-0.23673
	,-0.236062
	,-0.241462
	,-0.233292
	,-0.230415
	,-0.228722
	,-0.231665
	,-0.238667
	,-0.235654
	,-0.24018
	,-0.247462
	,-0.254442
	,-0.260155
	,-0.253136
	,-0.264058
	,-0.267853
	,-0.274671
	,-0.275917
	,-0.284516
	,-0.291901
};

static double gphase_ratio[TABLE_SIZE] = {
	-174.731
	,-174.731
	,-180.017
	,59.6672
	,-6.7606
	,29.2944
	,7.45482
	,90.0159
	,31.4863
	,-59.0372
	,-16.7974
	,28.8726
	,-12.8966
	,-16.8876
	,-23.4196
	,4.94872
	,8.63715
	,-22.5762
	,1.95715
	,5.57642
	,1.43842
	,-1.68651
	,-2.57057
	,10.3978
	,0.0385239
	,-4.91397
	,-2.91416
	,23.6367
	,-2.18373
	,-3.38629
	,-11.0049
	,-5.59312
	,-16.8376
	,1.46345
	,-1.87949
	,-2.86988
	,6.2099
	,-10.8925
	,6.65636
	,-1.50753
	,-19.4929
	,-0.477836
	,-7.0217
	,-9.03421
	,-8.82162
	,7.07366
	,-2.87839
	,0.895514
	,3.40428
	,5.83593
	,-5.11826
	,1.9181
	,2.12852
	,-6.73688
	,-4.94791
	,0.0042009
	,-1.5645
	,-0.562982
	,4.24399
	,-2.83364
	,5.77011
	,-3.26174
	,-1.38549
	,0.440517
	,-7.14386
	,5.84224
	,-9.87
	,-7.46186
	,-2.54395
	,-1.48966
	,-2.50461
	,-3.08215
	,-4.20489
	,3.26372
	,-1.86379
	,1.49683
	,6.86969
	,-8.57976
	,1.37133
	,-2.60165
	,-2.07301
	,-1.22288
	,0.281184
	,-2.28312
	,3.4158
	,1.19848
	,0.710852
	,-1.24103
	,-2.94352
	,1.26059
	,-1.88875
	,-3.05259
	,-2.95279
	,-2.37811
	,2.94174
	,-4.61373
	,-1.58594
	,-2.87358
	,-0.521566
	,-3.65296
	,-3.0608
};

#endif
void init_calib_table()
{
	int i;
	
	memset(&calib_vars, 0, sizeof(CALIB_VAR));

	memcpy(calib_vars.aoff[0], gphase_amp, sizeof(gphase_amp));
	memcpy(calib_vars.poff[0], gphase_offset, sizeof(gphase_offset));
	memcpy(calib_vars.ofrt[0], gphase_ratio, sizeof(gphase_ratio));
}

int make_calib_table(int ch)
{	
	unsigned short run_mode;
	unsigned short shch;
	double valA, valP;
	double t1, t2;
	double temp;
	double poffset;
	double *at, *pt, *a, *p, *ar, *pr;
	double diff;
	double tmpPha;
	int index = 0;
	int index_p;
	int loc;
	int hori, vert;
	int i = 0;

	run_mode = llrfInfo.calib_mode;
	valA = amp_vars.g_amplitude[ch];
	valP = pha_vars.g_phase[ch];
	index_p = cal_index;
	shch = llrfInfo.selected_ch;

	loc = index_p;
	if( run_mode == PAC_CAL ) {
		if( ch != 1 ) { // ?
			return (-1);
		}

		if( (loc < 0) || ((1010 <= loc) && (loc <= 1050)) || (2060 <= loc ) )
			return (-1);

		if( loc >= 1050 ) {
			hori = (loc - 1050) % TABLE_HORI_SIZE;
			vert = (loc - 1050) / TABLE_HORI_SIZE;

			if( hori == 0 ) {
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] = 0;
				vert = 0;
			} else if( hori == (TABLE_HORI_SIZE-1) ) {
				if( vert == 0 ) {
					calib_vars.ptblv[ch][(TABLE_HORI_SIZE * index) + loc] = valP;
					pha0[ch] = calib_vars.ptblv[ch][TABLE_HORI_SIZE * index] / (TABLE_HORI_SIZE-2);
				} else if( vert == 1 ) {
					calib_vars.ptblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valP - pha0[ch];
					calib_vars.ptbl[ch][vert] = calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
					calib_vars.ptbl[ch][0] = calib_vars.ptbl[ch][vert];
				} else {
					calib_vars.ptblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valP - pha0[ch];
					calib_vars.ptbl[ch][vert] = calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
				}
			} else {
				calib_vars.ptblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valP - pha0[ch];
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] += (valP -pha0[ch]);
			}
		}else {
			hori = loc % TABLE_HORI_SIZE;
			vert = loc / TABLE_HORI_SIZE;

			if( hori == 0 ) {
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] = 0;
				calib_vars.atbl[ch][vert] = 0;
			} else if ( hori == (TABLE_HORI_SIZE - 1) ) {
				calib_vars.atblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valA;
				calib_vars.atbl[ch][vert] = calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
			} else {
				calib_vars.atblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valA;
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] += valA;
			}
		}
		if( loc == ((TABLE_SIZE*10)-1) ) {
			calib_vars.ptbl[ch][0] = calib_vars.ptbl[ch][index];
			calib_vars.atbl[ch][0] = 0;
		}
	} else if( run_mode == PAD_CAL ) {
		if( shch != ch ) {
			return (-1);
		}
		
		if( !(((loc >= 0) && (loc < 101)) || ((loc >= 106) && (loc < 207))) ) {
			hloc[ch] = 0;
			vloc[ch] = 0;
			return (-1);
		}

		if( loc >= 106 ) {
			loc -= 106;
			if( vloc[ch] != loc )
				hloc[ch] = 0;
			
			vloc[ch] = loc;

			if( hloc[ch] >= TABLE_HORI_SIZE ) {
				goto CAL_SKIP;
			}

			hori = hloc[ch]++;
			vert = vloc[ch];

			at = calib_vars.aont[ch];
			pt = calib_vars.pont[ch];
			a = calib_vars.aon[ch];
			p = calib_vars.pon[ch];
			pr = calib_vars.onrt[ch];

			if( hori <= 1 ) {
				at[(TABLE_HORI_SIZE * vert) + hori] = 0;
				pt[(TABLE_HORI_SIZE * vert) + hori] = 0;
			} else if( hori == (TABLE_HORI_SIZE-1) ) {
				if( vert == 0 ) {
					at[(TABLE_HORI_SIZE * vert) + hori] = 0;
					pt[(TABLE_HORI_SIZE * vert) + hori] = 0;
					a[vert] = 0;
					p[vert] = 0;
					pha0[ch] = valP;
				} else {
					at[(TABLE_HORI_SIZE * vert) + hori] = valA;
					at[TABLE_HORI_SIZE * vert] += valA;

					pt[(TABLE_HORI_SIZE * vert) + hori] = valP;
					pt[TABLE_HORI_SIZE * vert] += valP;

					a[vert] = at[TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
					p[vert] = pt[TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
				}
				
				calib_vars.agin[ch][vert] = calib_vars.aon[ch][vert] - calib_vars.aoff[ch][vert];

				if( vert == (TABLE_SIZE-1) ) {
					poffset = p[25];
					p[0] = p[1] + (p[1] - p[2]);
					pt[hori] = p[0];
					
					for(i = 0; i < TABLE_SIZE; i++) {
						temp = poffset - p[i];
						if( temp > 300 ) temp = p[i] + 360;
						else if( temp < -300 ) temp = p[i] - 360;
						else temp = p[i];

						p[i] = temp - poffset;
					}
					for(i = 1; i < TABLE_SIZE; i++)
						pr[i] = (p[i] - p[i-1]) / (a[i] - a[i-1]);
					pr[0] = pr[1];
				}
			} else {
				at[(TABLE_HORI_SIZE * vert) + hori] = valA;
				at[TABLE_HORI_SIZE * vert] += valA;

				pt[(TABLE_HORI_SIZE * vert) + hori] = valP;
				pt[TABLE_HORI_SIZE * vert] += valP;
			}
		}else {
			if( vloc[ch] != loc )
				hloc[ch] = 0;

			vloc[ch] = loc;

			if( hloc[ch] >= TABLE_HORI_SIZE )
				goto CAL_SKIP;

			hori = hloc[ch]++;
			vert=  vloc[ch];

			at = calib_vars.aofft[ch];
			pt = calib_vars.pofft[ch];
			a = calib_vars.aoff[ch];
			p = calib_vars.poff[ch];
			pr = calib_vars.ofrt[ch];

			if( hori <= 1 ) {
				at[(TABLE_HORI_SIZE * vert) + hori] = 0;
				pt[(TABLE_HORI_SIZE * vert) + hori] = 0;
			} else if( hori == (TABLE_HORI_SIZE-1) ) {
				if( vert == 0 ) {
					at[(TABLE_HORI_SIZE * vert) + hori] = 0;
					pt[(TABLE_HORI_SIZE * vert) + hori] = 0;
					a[vert] = 0;
					p[vert] = 0;
					pha0[ch] = valP;
				} else {
					at[(TABLE_HORI_SIZE * vert) + hori] = valA;
					at[TABLE_HORI_SIZE * vert] += valA;

					pt[(TABLE_HORI_SIZE * vert) + hori] = valP;
					pt[TABLE_HORI_SIZE * vert] += valP;

					a[vert] = at[TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
					p[vert] = pt[TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
				}

				if( vert == (TABLE_SIZE-1) ) {
					poffset = p[25];
					p[0] = p[1] + (p[1] - p[2]);
					pt[hori] = p[0];

					for(i = 0; i < TABLE_SIZE; i++) {
						temp = poffset - p[i];
						if( temp > 300 ) temp = p[i] + 360;
						else if( temp < -300 ) temp = p[i] - 360;
						else temp = p[i];

						p[i] = temp - poffset;
					}

					for(i = 1; i < TABLE_SIZE; i++)
						pr[i] = (p[i] - p[i-1]) / (a[i] - a[i-1]);
					pr[0] = pr[1];
				}
			} else {
				at[(TABLE_HORI_SIZE * vert) + hori] = valA;
				at[TABLE_HORI_SIZE * vert] += valA;

				pt[(TABLE_HORI_SIZE * vert) + hori] = valP;
				pt[TABLE_HORI_SIZE * vert] += valP;
			}
		}
	} else if( run_mode == PSK_CAL ) {
		if( ch != 1 )
			return (-1);

		if( !(((loc >= 0) && (loc < 101)) || ((loc >= 106) && (loc < 207))) ) {
			hloc[ch] = 0;
			vloc[ch] = 0;
			return (-1);
		}

		if( loc >= 106 ) {
			loc -= 106;
			if( vloc[ch] != loc )
				hloc[ch] = 0;

			vloc[ch] = loc;

			if( hloc[ch] >= TABLE_HORI_SIZE )
				goto CAL_SKIP;

			hori = hloc[ch]++;
			vert = vloc[ch];

			if( hori <= 1 ) {
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] = 0;
				calib_vars.patbl[ch][vert] = 0;
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] = 0;
				calib_vars.pptbl[ch][vert] = 0;
				pha0[ch] = ZLP_STATUS;
			} else {
				if( pha0[ch] == ZLP_STATUS ) {
					pha0[ch] = valP;
				} else {
					diff = pha0[ch] - valP;
					if( diff > 300 ) valP += 360;
					if( diff < -300 ) valP -= 360;
				}

				calib_vars.atblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valA;
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] += valA;
				calib_vars.ptblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valP;
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] += valP;

				if( hori == (TABLE_HORI_SIZE-1) ) {
					calib_vars.patbl[ch][vert] = calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
					tmpPha = calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);

					if( vert <= 1 ) {
						vpha0[ch] = tmpPha;
					} else {
						diff = vpha0[ch] - tmpPha;
						if( diff > 300 ) tmpPha += 360;
						if( diff < -300 ) tmpPha -= 360;
					}
					calib_vars.pptbl[ch][vert] = tmpPha;
					
					calib_vars.pvoff[ch][vert] = llrfInfo.psk_const * (calib_vars.atbl[ch][vert] - calib_vars.patbl[ch][vert]);
					if( calib_vars.ptbl[ch][vert] > calib_vars.pptbl[ch][vert] )
						calib_vars.ppoff[ch][vert] = (calib_vars.ptbl[ch][vert] - 180) - calib_vars.pptbl[ch][vert];
					else
						calib_vars.ppoff[ch][vert] = (calib_vars.ptbl[ch][vert] + 180) - calib_vars.pptbl[ch][vert];
				}
			}
		} else {
			if( vloc[ch] != loc )
				hloc[ch] = 0;

			vloc[ch] = loc;

			if( hloc[ch] >= TABLE_HORI_SIZE )
				goto CAL_SKIP;

			hori = hloc[ch]++;
			vert = vloc[ch];

			if( hori <= 1 ) {
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] = 0;
				calib_vars.atbl[ch][vert] = 0;
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] = 0;
				calib_vars.ptbl[ch][vert] = 0;
				pha0[ch] = ZLP_STATUS;
			} else {
				if( pha0[ch] == ZLP_STATUS ) {
					pha0[ch] = valP;
				} else {
					diff = pha0[ch] - valP;
					if( diff > 300 ) valP += 360;
					if( diff < -300 ) valP -= 360;
				}

				calib_vars.atblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valA;
				calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] += valA;
				calib_vars.ptblv[ch][(TABLE_HORI_SIZE * vert) + hori] = valP;
				calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] += valP;

				if( hori == (TABLE_HORI_SIZE-1) ) {
					calib_vars.atbl[ch][vert] = calib_vars.atblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);
					tmpPha = calib_vars.ptblv[ch][TABLE_HORI_SIZE * vert] / (TABLE_HORI_SIZE-2);

					if( vert <= 1 ) {
						vpha0[ch] = tmpPha;
					} else {
						diff = vpha0[ch] - tmpPha;
						if( diff > 300 ) tmpPha += 360;
						if( diff < -300 ) tmpPha -= 360;
					}
					calib_vars.ptbl[ch][vert] = tmpPha;
				}
			}
		}
	}
CAL_SKIP:
	return 0;
}
