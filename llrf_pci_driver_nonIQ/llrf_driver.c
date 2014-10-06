//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name: ui.c

Abstract: Boot loader user interface (menu) functions.

Functions:

    UIMenu
    UIWatchForBootHalt

Notes:

--*/

/*
  * PAL-LLRF Driver Program
  * by LHS, mobiis
  * email: hsmaster89@mobiis.com
  */
  
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <math.h>


#include <stdarg.h>
#include <sys/mman.h>
#include <ctype.h>
#include <pthread.h>

#include "llrf_access.h"
#include "base.h"

#ifndef TRUE
#define	TRUE	1
#endif

#ifndef FALSE
#define	FALSE	0
#endif

#define	printStr	printf

/* Send socket */
int g_ssock = -1;
int sport = 3601;
char *sendIP = "127.0.0.1";
struct sockaddr_in sendAddr;

/* Receive socket */
int g_rsock = -1;
int rport = 3600;
struct sockaddr_in recvAddr;

/* Global variables */
int gbExit = FALSE;
int g_fd = -1;

// protocol
LLRF_INFO llrfInfo;
LLRF_INFO buffer;
IOC_SEND send_ioc;
GRP_RAW_SEND grp_raw;
GRP_AMP_SEND grp_amp;
GRP_PHA_SEND grp_pha;
int graph_flag;

// read variables
unsigned short g_raw[LLRF_CHANNEL][LLRF_SAMPLING];
unsigned short g_interlock[3];
unsigned int misc2_container;

// Data storage variables
int intr_flag1;
int intr_flag2;
int il_flag, user_flag;

// Applying constant power 5dBm per 0.5 sec
double step_const_power;
int step_flag;
unsigned short prev_pac_power;

extern int waveAnl(int ch);
extern int kbvAnl(int ch);
extern int calc_amplitude(int ch);
extern int calc_phase(int ch);
extern int make_display_data(int ch);
extern int make_rms_data(int ch);
extern int mux();
extern void init_calib_table();
extern int make_calib_table(int ch);
extern int do_pac_cal();
extern int loopback();

unsigned int llrf_get_dma(unsigned int pAddr, unsigned int * pBuff, unsigned int len)
{
	llrf_ioctl_t llrf_ioctl;
	llrf_ioctl.offset = pAddr;
	llrf_ioctl.len = len;
	llrf_ioctl.buf = pBuff;

	ioctl(g_fd, LLRF_IOCTL_CMD_DMA_GET_D_R, (unsigned long)&llrf_ioctl);
	return len;
}

unsigned int llrf_up_b(unsigned int pAddr, unsigned int mask, unsigned int data)
{
	llrf_ioctl_t llrf_ioctl;
	unsigned int buf;
	llrf_ioctl.offset = pAddr;
	llrf_ioctl.len = 1;
	llrf_ioctl.buf = &buf;

	ioctl(g_fd, LLRF_IOCTL_CMD_GET_DATA, (unsigned long)&llrf_ioctl);
	buf &= ~mask;
	buf |= (mask & data);
	ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl);
	return buf;
}

unsigned int llrf_wr(unsigned int pAddr, unsigned int data)
{
	llrf_ioctl_t llrf_ioctl;
	unsigned int buf = data;
	llrf_ioctl.offset = pAddr;
	llrf_ioctl.len = 1;
	llrf_ioctl.buf = &buf;

	ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl);
	return buf;
}

unsigned int llrf_rd(unsigned int pAddr)
{
    llrf_ioctl_t llrf_ioctl;
    unsigned int buf;
    llrf_ioctl.offset = pAddr;
    llrf_ioctl.len = 1;
    llrf_ioctl.buf = &buf;

    ioctl(g_fd, LLRF_IOCTL_CMD_GET_DATA, (unsigned long)&llrf_ioctl);
    return buf;
}

static void readFPGAData(unsigned int *pBuf)
{
	int buf_idx, i, j;
	unsigned int adc_ctrl;
	unsigned int temper;
	unsigned int misc2;
	int ret;
	double temp;
	
	/* Read data */
	ret = llrf_get_dma(ADDR_ADC1_DATA, pBuf, READ_SIZE);
	
	buf_idx = 0;
	for(i = 0; i < LLRF_CHANNEL; i++) {
		for(j = 0; j < LLRF_SAMPLING; j+=2) {
			buf_idx = (i * 512) + (j / 2);
			g_raw[i][j] = (unsigned short)(pBuf[buf_idx] & 0xffff);
			g_raw[i][j+1] = (unsigned short)((pBuf[buf_idx] & 0xffff0000) >> 16);
		}
	}
	memset((void *)pBuf, 0, sizeof(pBuf));

	/* Read temperature data */
	temper = llrf_rd(ADDR_TEMP_SPI+(0x3 << 2));
	temp = (double)(temper & 0x00ffffff);
	send_ioc.temper[TEMPER_SYS] = (temp - 4598857.781) / 16644.0625;
	
	temper = llrf_rd(ADDR_TEMP_SPI+(0x4 << 2));
	temp = (double)(temper & 0x00ffffff);
	send_ioc.temper[TEMPER_RFM] = (temp - 4598857.781) / 16644.0625;

	if( send_ioc.temper[TEMPER_SYS] > 60 )
		send_ioc.temper_over[TEMPER_SYS] = 1;
	else
		send_ioc.temper_over[TEMPER_SYS] = 0;

	if( send_ioc.temper[TEMPER_RFM] > 60 )
		send_ioc.temper_over[TEMPER_RFM] = 1;
	else
		send_ioc.temper_over[TEMPER_RFM] = 0;

	/* External interlock */
	misc2 = llrf_rd(ADDR_MISC_CONTROL2);
	misc2 >>= 20;
	send_ioc.ext_interlock[0] = misc2 & 0x1;
	send_ioc.ext_interlock[1] = (misc2 >> 1) & 0x1;
	send_ioc.ext_interlock[2] = (misc2 >> 2) & 0x1;
	
}

int sendReqData()
{
	IOC_SEND buffer;
	int status;

	memset(&buffer, 0, sizeof(buffer));
	buffer.cmd = LLRF_CMD_REQ_INIT;
	status = sendto(g_ssock, (void*)&buffer, sizeof(IOC_SEND), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
	if( status < 0 ) {
		printf("%s:%s [%d], sendto error\n", __FILE__, __func__, __LINE__);
		return (-1);
	}
	
	return 0;
}

static void check_sled_interlock()
{
	unsigned int buff;
	unsigned short limit1, limit2;

	buff = llrf_rd(ADDR_DAC_INVERT_DELAY);

	limit1 = ((buff & 0x00000030) >> 4) & 0x3;
	limit2 = ((buff & 0x000000c0) >> 6) & 0x3;

	if( limit1 != limit2 )
		send_ioc.sled_interlock = 1;
	else
		send_ioc.sled_interlock = 0;
}

static int check_interlock_storage()
{
	int inter_il = 0, ext_il = 0;
	int i;

	for(i = 0; i < 3; i++) {
		if(send_ioc.ext_interlock[i]) ext_il = 1;
	}

	/* External interlock is occurred?? */
	send_ioc.il_monitoring[1] = ext_il;
	
	for(i = 0; i < 10; i++) {
		if(send_ioc.adc_over[i]) inter_il = 1;
	}
	for(i = 0; i < 4; i++) {
		if(send_ioc.pp_interlock[i]) inter_il = 1;
	}
	for(i = 0; i < 2; i++) {
		if(send_ioc.temper_over[i]) inter_il = 1;
		if(send_ioc.kbv_stb_over[i]) inter_il = 1;
		if(send_ioc.kbc_stb_over[i]) inter_il = 1;
	}
	if(send_ioc.ref_range_over) inter_il = 1;
	if(send_ioc.ref_amp_stb_over) inter_il = 1;
	if(send_ioc.ref_pha_stb_over) inter_il = 1;
	if(send_ioc.sled_interlock) inter_il = 1;

	/* Internal interlock is occurred?? */
	send_ioc.il_monitoring[0] = inter_il;

	return (ext_il || inter_il);
}

int sendData()
{
	int i;
	int status;
	
//	printf("Make amplitude/phase & waveform analysis\n");
	for(i = 0; i < LLRF_CHANNEL-2; i++) {
		waveAnl(i);
		calc_amplitude(i);
		calc_phase(i);
	}

//	printf("KBV & KBC analysis\n");
	for(i = 0; i < 2; i++) {
		kbvAnl(i);
	}

//	printf("Make display data & rms data\n");
	for(i = 0; i < LLRF_CHANNEL-2; i++) {
		make_display_data(i);
		make_rms_data(i);
	}

//	printf("Mux\n");
	mux();

//	printf("Calibration\n");
	for(i = 0; i < LLRF_CHANNEL-2; i++) {
		make_calib_table(i);
	}

//	printf("Make result\n");
	do_pac_cal();

//	printf("Loopback\n");
	loopback();

//	printf("ADC start\n");
#if 1
	/* ADC Start */
	llrf_up_b(ADDR_ADC_CTRL, 0x1, 0x1);
#endif

	check_sled_interlock();

	send_ioc.cmd = LLRF_CMD_NORMAL;
#if 1
	/* Test code: Ignore interlock */
	memset(send_ioc.ext_interlock, 0, sizeof(send_ioc.ext_interlock));
	memset(send_ioc.adc_over, 0, sizeof(send_ioc.adc_over));
	memset(send_ioc.pp_interlock, 0, sizeof(send_ioc.pp_interlock));
	memset(send_ioc.temper_over, 0, sizeof(send_ioc.temper_over));
	memset(send_ioc.kbv_stb_over, 0, sizeof(send_ioc.kbv_stb_over));
	memset(send_ioc.kbc_stb_over, 0, sizeof(send_ioc.kbc_stb_over));
	send_ioc.ref_range_over = 0;
	send_ioc.ref_amp_stb_over = 0;
	send_ioc.ref_pha_stb_over = 0;
	send_ioc.sled_interlock = 0;
#endif

	/* Storage check in interlock condition */
	il_flag = check_interlock_storage();

	/* Storage check in user switch */
	user_flag = llrfInfo.userStSwitch;

	status = sendto(g_ssock, &send_ioc, sizeof(IOC_SEND), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr));
	if( status < 0 ) {
		printf("%s:%s [%d], sendto error\n", __FILE__, __func__, __LINE__);
		return (-1);
	}

	return 0;
}

void * sendThread(void * param)
{
	int ret;
	struct pollfd events;
	unsigned int *pBuff;
	unsigned int adc_ctrl;
#if 0
	struct timespec new_t;
	struct timespec old_t;
	unsigned long gap;
#endif
	printStr("Send Thread Started!!!\r\n");

	/* pBuff allocation */
	pBuff = (unsigned int *)malloc(sizeof(unsigned int) * READ_SIZE);
	if( pBuff == NULL ) {
		printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
		return;
	}

	events.fd = g_fd;
	events.events = POLLIN | POLLERR;

	while(!gbExit){
		ret = poll((struct pollfd *)&events, 1, 500);
		if(ret < 0){
			perror("poll error : ");
			continue;
		}
		if(ret == 0){
//			printStr("Poll Time Out!!!\r\n");
			continue;
		}
		if(events.revents & POLLIN){

//			printf("POLL IN!!\n");
			/* storing flag setting */
			intr_flag1 = 1;
			intr_flag2 = 1;
			
			llrf_wr(0x758,0x20000000 ^ llrf_rd(0x758));


			readFPGAData(pBuff);
#if 0
			clock_gettime(CLOCK_REALTIME, &old_t);
#endif
			sendData();
#if 0
			clock_gettime(CLOCK_REALTIME, &new_t);
#endif

#if 0	
			if(new_t.tv_nsec >= old_t.tv_nsec){
		                gap = (new_t.tv_nsec - old_t.tv_nsec);
		        }
		        else{
		                gap = (new_t.tv_nsec+1000000000 - old_t.tv_nsec);
		        }

			printf("Elapsed = %ldus\n", gap/1000);
#endif
			continue;
		}
		else{
			printStr("Received Events = 0x%x!!!\r\n", events.revents);
			continue;
		}
	}

	/* Memory free */
	free(pBuff);
	pBuff = NULL;
	
	return NULL;
}

void set_reg_misc_control2(unsigned int val)
{
	unsigned int misc2, delayReg;
	unsigned int psk;
	int ret;

	misc2 = llrf_rd(ADDR_MISC_CONTROL2);
	delayReg = llrf_rd(ADDR_DAC_CTRL);

	delayReg = delayReg & 0xF000FFFF;
	psk = val & (0xC) >> 2;

	if( PSK_HWON == psk ) {
		delayReg |= (0x3 << 16);
	} else if( PSK_SWON == psk ) {
		val &= 0xFFFFFFF3;
		delayReg |= (0x7 << 16);
	}

	misc2 = (misc2 & 0xFFFFC000) | val;

	llrf_wr(ADDR_MISC_CONTROL2, misc2);
	llrf_wr(ADDR_DAC_CTRL, delayReg);
}

static int llrfTrgFreq(unsigned int value)
{
	llrf_up_b(ADDR_MISC_CONTROL3, 0x7ffffff, value);

	return 0;
}

static int llrfTrgTime(unsigned short value)
{
	value &= 0xfff;

	llrf_up_b(ADDR_MISC_CONTROL1, 0xfff, value);

	return 0;
}

static int llrfPulseWidth(unsigned short value)
{	
	value &= 0x7ff;

	llrf_up_b(ADDR_DAC_SHIFT_SIZE, 0x7ff, value);

	return 0;
}

static int llrfPskTime(unsigned short value)
{
	value &= 0x7ff;

	llrf_up_b(ADDR_DAC_CTRL, 0x7ff, value);

	return 0;
}

static int llrfAmpMode(unsigned short value, int ch)
{
	if(value)
		misc2_container |= (0x1 << (ch+4));	// amplifier on
	else
		misc2_container &= ~(0x1 << (ch+4));	// amplifier off

	set_reg_misc_control2(misc2_container);

	return 0;
}

static int llrfPacPower(unsigned short value)
{
	if(value != prev_pac_power) {
		if(prev_pac_power == 0) {
			step_flag = 1;
			printf("step flag rising set!!!\n");
		} else if(prev_pac_power != 0 && value == 0) {
			step_flag = 2;
			printf("step flag falling set!!!\n");
		}
		prev_pac_power = value;
	}
	misc2_container = (misc2_container & 0xfffffffc) | (value & 0x3);
	set_reg_misc_control2(misc2_container);

	return 0;
}

static int llrfPskMode(unsigned short value)
{
	unsigned short data[] = {0, 2, 3};

	misc2_container &= 0xfffffff3;
	misc2_container |= ((data[value] & 0x3) << 2);

	set_reg_misc_control2(misc2_container);

	return 0;
}

static int llrfTrgSyncPol(unsigned short value)
{
	if( value )
		llrf_up_b(ADDR_MISC_CONTROL1, 0x8000, 0x8000);
	else
		llrf_up_b(ADDR_MISC_CONTROL1, 0x8000, 0x0);

	return 0;
}

static int llrfSledSwitch(unsigned short value, int devNum)
{
	if( !devNum ) {
		if( value )
			llrf_up_b(ADDR_DAC_INVERT_DELAY, 0x100, 0x100);
		else
			llrf_up_b(ADDR_DAC_INVERT_DELAY, 0x100, 0x0);
	} else {
		if( value )
			llrf_up_b(ADDR_DAC_INVERT_DELAY, 0x200, 0x200);
		else
			llrf_up_b(ADDR_DAC_INVERT_DELAY, 0x200, 0x0);
	}

	return 0;
}

void print_mem_data(int attr)
{
	int i;
	
	switch(attr) {
		case ATTR_TRG_FREQ:
			printf("## Trigger Frequency = [%u]\n", llrfInfo.trgfreq);
			break;
		case ATTR_PULSE_WIDTH:
			printf("## Pulse Width = [%hu]\n", llrfInfo.pulse_width);
			break;
		case ATTR_PSK_TIME:
			printf("## PSK Time = [%hu]\n", llrfInfo.psk_time);
			break;
		case ATTR_TRG_TIME:
			printf("## Trigger On Time = [%hu]\n", llrfInfo.trgtime);
			break;
		case ATTR_AMPLIFIER:
			for(i = 0; i < 10; i++)
				printf("Channel%d's amplifier = [%s]\n", i, (llrfInfo.amp[i] == 1 ? "ON" : "OFF"));
			break;
		case ATTR_PAC_POWER:
			printf("PAC Power = [%s]\n", (llrfInfo.pac_power == 1 ? "OFF" : (llrfInfo.pac_power == 3 ? "PM" : "CW")));
			break;
		case ATTR_PSK_MODE:
			printf("PSK Mode = [%hu]\n", llrfInfo.psk_mode);
			break;
		case ATTR_TRG_SYNC_POL:
			printf("Trigger sync pol = [%s]\n", (llrfInfo.trg_sync_pol == 0 ? "falling" : "rising"));
			break;
		case ATTR_SLED_SWITCH_1:
			printf("SLED 1 Switch = [%s]\n", (llrfInfo.sledswitch1 == 0 ? "LEFT" : "RIGHT"));
			break;
		case ATTR_SLED_SWITCH_2:
			printf("SLED 2 Switch = [%s]\n", (llrfInfo.sledswitch2 == 0 ? "LEFT" : "RIGHT"));
			break;
	}
}

void *recvThread(void *param)
{
	static struct sockaddr_in Addr;
	socklen_t clientlen = sizeof(Addr);
	int readlen = 0;
	int i;

	printStr("Receive Thread Started!!!\r\n");
	while(!gbExit) {
		readlen = recvfrom(g_rsock, (void*)&buffer, sizeof(LLRF_INFO), 0, (struct sockaddr *)&Addr, &clientlen);
		if( readlen != sizeof(LLRF_INFO)) {
			printf("recvfrom error! readlen = [%d]\n", readlen);
			continue;
		}

		memcpy(&llrfInfo, &buffer, sizeof(buffer));

#if 1
		if(llrfInfo.cmd == LLRF_CMD_ACK_INIT)	printf("Initial packet is arrived...\n");
		else if(llrfInfo.cmd == LLRF_CMD_ATTR)	printf("Attribute packet is arrived...\n");
#endif

		switch( llrfInfo.cmd ) {
			case LLRF_CMD_ACK_INIT:
				// Memory write
				llrfTrgFreq(llrfInfo.trgfreq);	// Trigger frequency
				llrfPulseWidth(llrfInfo.pulse_width);	// pulse width
				llrfPskTime(llrfInfo.psk_time);	// psk time
				llrfTrgTime(llrfInfo.trgtime);	// Trigger on time
				for(i = 0; i < LLRF_CHANNEL-2; i++)	// amplifier on/off
					llrfAmpMode(llrfInfo.amp[i], i);
				llrfPacPower(llrfInfo.pac_power);	// PAC power
				llrfPskMode(llrfInfo.psk_mode);
				llrfTrgSyncPol(llrfInfo.trg_sync_pol);	// Trigger sync pol
				llrfSledSwitch(llrfInfo.sledswitch1, 0);	// SLED 1 switch
				llrfSledSwitch(llrfInfo.sledswitch2, 1);	// SLED 2 switch
				break;
			case LLRF_CMD_ATTR:
				switch( llrfInfo.attr ) {
					case ATTR_TRG_FREQ:
						llrfTrgFreq(llrfInfo.trgfreq);
						print_mem_data(ATTR_TRG_FREQ);
						break;
					case ATTR_PULSE_WIDTH:
						llrfPulseWidth(llrfInfo.pulse_width);
						print_mem_data(ATTR_PULSE_WIDTH);
						break;
					case ATTR_PSK_TIME:
						llrfPskTime(llrfInfo.psk_time);
						print_mem_data(ATTR_PSK_TIME);
						break;
					case ATTR_TRG_TIME:
						llrfTrgTime(llrfInfo.trgtime);
						print_mem_data(ATTR_TRG_TIME);
						break;
					case ATTR_AMPLIFIER:
						for(i = 0; i < LLRF_CHANNEL-2; i++)
							llrfAmpMode(llrfInfo.amp[i], i);
						print_mem_data(ATTR_AMPLIFIER);
						break;
					case ATTR_PAC_POWER:
						llrfPacPower(llrfInfo.pac_power);
						print_mem_data(ATTR_PAC_POWER);
						break;
					case ATTR_PSK_MODE:
						llrfPskMode(llrfInfo.psk_mode);
						print_mem_data(ATTR_PSK_MODE);
						break;
					case ATTR_TRG_SYNC_POL:
						llrfTrgSyncPol(llrfInfo.trg_sync_pol);
						print_mem_data(ATTR_TRG_SYNC_POL);
						break;
					case ATTR_SLED_SWITCH_1:
						llrfSledSwitch(llrfInfo.sledswitch1, 0);
						print_mem_data(ATTR_SLED_SWITCH_1);
						break;
					case ATTR_SLED_SWITCH_2:
						llrfSledSwitch(llrfInfo.sledswitch2, 1);
						print_mem_data(ATTR_SLED_SWITCH_2);
						break;
					case ATTR_DEFAULT:
						break;
					default:
						printf("Invalid command error\n");
						break;
				}
				break;
			case LLRF_CMD_REQ_INIT:
				break;
			default:
				printf("Invalid Packet error\n");
				break;
		}
	}
	printf("End of Received routine\n");
}

int make_graph_data()
{
	int i, j;
	float temp;
	double degree[] = {0, 90, 180, 270};
	
	/* Do not collect back/forward 4 data */
	for(i = 0; i < LLRF_CHANNEL; i++) {
		for(j = 4; j < LLRF_SAMPLING-4; j++) {
			if(i == LLRF_CHANNEL-1)
				grp_raw.raw_data[i][j-4] = CONVERT_R_TO_V((float)g_raw[i][j], 0)/50;
			else if(i == LLRF_CHANNEL-2)
				grp_raw.raw_data[i][j-4] = CONVERT_R_TO_V((float)g_raw[i][j], 0);
			else
				grp_raw.raw_data[i][j-4] = CONVERT_R_TO_V((float)g_raw[i][j], 32767.5);
		}
		usleep(0);
	}

	for(i = 0; i < LLRF_CHANNEL-2; i++) {
		for(j = 0; j < LLRF_SAMPLING-8; j++) {
			// amplitude = sqrt(vI^2 + vQ^2)
			grp_amp.amp_grp[i][j] = CONVERT_R_TO_A(grp_raw.raw_data[i][(j+1)%LLRF_SAMPLING], grp_raw.raw_data[i][j%LLRF_SAMPLING]);
			
			// phase = atan2(vQ, vI)
			temp = CONVERT_R_TO_P(grp_raw.raw_data[i][(j+1)%LLRF_SAMPLING], grp_raw.raw_data[i][j%LLRF_SAMPLING]);
			temp = temp - degree[j%LLRF_PERIOD];
			
			if( temp > 360 ) temp -= 360;
			else if( temp < 0 ) temp += 360;

			if( temp > 360 ) temp -= 360;
			else if( temp < 0 ) temp += 360;

			grp_pha.pha_grp[i][j] = temp;
		}
		usleep(0);
	}
	
	return 0;
}

void *grpThread(void *param)
{	
	int status = 0;
	int i, j;
	double temp;

	printStr("Graph Thread Started!!!\r\n");
	while(!gbExit) {
		// Do work per 500ms
		usleep(500000);
		
		/* Step constant power routine */
		if(step_flag == 1) {
			temp = llrfInfo.initpower - llrfInfo.constant_loss + llrfInfo.pac_loss;
			if((step_const_power+5) <= temp) {
				step_const_power += 5;
			} else if((step_const_power+5) > temp) {
				step_flag = 0;
				step_const_power = -40;
			}
			printf("step power rising!! power = %lf\n", step_const_power);
		} else if(step_flag == 2) {
			if((step_const_power-5) >= -40) {
				step_const_power -= 5;
			} else {
				step_flag = 0;
				step_const_power = -40;
			}
			printf("step power falling!!! power = %lf\n", step_const_power);
		}
		
		make_graph_data();

		grp_raw.cmd = LLRF_CMD_RAW_GRAPH;
		status = sendto(g_ssock, &grp_raw, sizeof(GRP_RAW_SEND), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr));
		if( status < 0 ) {
			printf("%d\n", status);
			printf("%s:%s [%d], sendto error\n", __FILE__, __func__, __LINE__);
			return;
		}

		grp_amp.cmd = LLRF_CMD_AMP_GRAPH;
		status = sendto(g_ssock, &grp_amp, sizeof(GRP_AMP_SEND), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr));
		if( status < 0 ) {
			printf("%d\n", status);
			printf("%s:%s [%d], sendto error\n", __FILE__, __func__, __LINE__);
			return;
		}

		grp_pha.cmd = LLRF_CMD_PHA_GRAPH;
		status = sendto(g_ssock, &grp_pha, sizeof(GRP_PHA_SEND), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr));
		if( status < 0 ) {
			printf("%d\n", status);
			printf("%s:%s [%d], sendto error\n", __FILE__, __func__, __LINE__);
			return;
		}

	}
}

void *stoThread(void *param)
{
	int pos_il = 0, pos_user = 0;
	int i, j;
	DATA_STORAGE *il_store_buf;
	DATA_STORAGE *user_store_buf;


	time_t timer;
	
	printStr("Storing Thread Started!!!\r\n");

	il_store_buf = (DATA_STORAGE *)malloc(sizeof(DATA_STORAGE));
	if(il_store_buf == NULL) {
		printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
	} else {
		il_store_buf->buf = (unsigned short *)calloc(MAX_ST_SIZE, sizeof(unsigned short));
		if(il_store_buf->buf == NULL) {
			printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
			return;
		}

		il_store_buf->bsa = (VME_LLRF_DATA *)calloc(MAX_ST_PULSE, sizeof(VME_LLRF_DATA));
		if(il_store_buf->bsa == NULL) {
			printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
			return;
		}
	}

	user_store_buf = (DATA_STORAGE *)malloc(sizeof(DATA_STORAGE));
	if(user_store_buf == NULL) {
		printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
	} else {
		user_store_buf->buf = (unsigned short *)calloc(MAX_ST_SIZE, sizeof(unsigned short));
		if(user_store_buf->buf == NULL) {
			printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
			return;
		}

		user_store_buf->bsa = (VME_LLRF_DATA *)calloc(MAX_ST_PULSE, sizeof(VME_LLRF_DATA));
		if(user_store_buf->bsa == NULL) {
			printf("%s:%s [%d], malloc error\n", __FILE__, __func__, __LINE__);
			return;
		}
	}

	while(!gbExit) {
		if((intr_flag1 == 1) && (il_flag == 0)) {
			pos_il = il_store_buf->pos;
			pos_il = pos_il * (MAX_ST_SIZE/MAX_ST_PULSE);

			for(i = 0; i < LLRF_CHANNEL; i++) {
				for(j = 0; j < LLRF_SAMPLING; j++) {
					il_store_buf->buf[pos_il++] = g_raw[i][j];
#if 0
					// Debug
					if(i == 0 && j < 10) {
						if(j == 0) printf("===== Debug =====\n");
						printf("stored data = [%hu]\n", il_store_buf->buf[pos_il-1]);
					}
#endif
				}
			}
			il_store_buf->pos = (il_store_buf->pos+1)%MAX_ST_PULSE;
			intr_flag1 = 0;
		}

		if((intr_flag2 == 1) && (user_flag == 0)) {
			pos_user = user_store_buf->pos;
			pos_user = pos_user * (MAX_ST_SIZE/MAX_ST_PULSE);

			for(i = 0; i < LLRF_CHANNEL; i++) {
				for(j = 0; j < LLRF_SAMPLING; j++) {
					user_store_buf->buf[pos_user++] = g_raw[i][j];

#if 0
					// Debug
					if(i == 0 && j < 10) {
						if(j == 0) printf("===== Debug =====\n");
						printf("stored data = [%hu]\n", user_store_buf->buf[pos_user-1]);
					}
#endif

				}
			}
			user_store_buf->pos = (user_store_buf->pos+1)%MAX_ST_PULSE;
			intr_flag2 = 0;
		}
		usleep(0);
	}

	/* Free */
	free(il_store_buf->buf);
	il_store_buf->buf = NULL;
	free(il_store_buf->bsa);
	il_store_buf->bsa = NULL;
	free(il_store_buf);
	il_store_buf = NULL;

	free(user_store_buf->buf);
	user_store_buf->buf = NULL;
	free(user_store_buf->bsa);
	user_store_buf->bsa = NULL;
	free(user_store_buf);
	user_store_buf = NULL;
}

void init_network()
{
	/* Make socket */
	if( (g_ssock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		printf("socket error!!\n");
		exit(-1);
	}

	memset(&sendAddr, 0, sizeof(sendAddr));
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_addr.s_addr = inet_addr(sendIP);
	sendAddr.sin_port = htons(sport);

	if( (g_rsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		printf("socket error!!\n");
		exit(-1);
	}

	memset(&recvAddr, 0, sizeof(recvAddr));
	recvAddr.sin_family = AF_INET;
	recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	recvAddr.sin_port = htons(rport);

	if( bind(g_rsock, (struct sockaddr*)&recvAddr, sizeof(recvAddr)) < 0 ) {
		printf("bind() failed.\n");
		exit(-1);
	}
}

static void setting_spi_board_0()
{
	/* ADC Channel 1 */
	llrf_wr(ADDR_ADC1_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x800010ed);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 2 */
	llrf_wr(ADDR_ADC2_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x80001019);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x8000ff01);
	usleep(1);
	
	/* ADC Channel 3 */
	llrf_wr(ADDR_ADC3_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x800010e0);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x8000ff01);
	usleep(1);


	/* ADC Channel 4 */
	llrf_wr(ADDR_ADC4_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x80001015);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 5 */
	llrf_wr(ADDR_ADC5_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x800010eb);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 6 */
	llrf_wr(ADDR_ADC6_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x800010c8);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 7 */
	llrf_wr(ADDR_ADC7_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x800010ec);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 8 */
	llrf_wr(ADDR_ADC8_SPI, 0x80001600);
        usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x80001009);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 9 */
	llrf_wr(ADDR_ADC9_SPI, 0x80001600);
	sleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x800010fe);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 10 */
	llrf_wr(ADDR_ADC10_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x800010ef);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x8000ff01);
	usleep(1);
}

static void setting_spi_board_1()
{
	/* ADC Channel 1 */
	llrf_wr(ADDR_ADC1_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x800010e8);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 2 */
	llrf_wr(ADDR_ADC2_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x800010ff);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x8000ff01);
	usleep(1);
	
	/* ADC Channel 3 */
	llrf_wr(ADDR_ADC3_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x800010fc);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x8000ff01);
	usleep(1);


	/* ADC Channel 4 */
	llrf_wr(ADDR_ADC4_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x800010eb);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 5 */
	llrf_wr(ADDR_ADC5_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x800010e0);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 6 */
	llrf_wr(ADDR_ADC6_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x800010ee);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 7 */
	llrf_wr(ADDR_ADC7_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x800010fb);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 8 */
	llrf_wr(ADDR_ADC8_SPI, 0x80001600);
        usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x800010a6);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 9 */
	llrf_wr(ADDR_ADC9_SPI, 0x80001600);
	sleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x800010db);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 10 */
	llrf_wr(ADDR_ADC10_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x800010e3);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x8000ff01);
	usleep(1);
}

static void setting_spi_board_2()
{
	/* ADC Channel 1 */
	llrf_wr(ADDR_ADC1_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x800010e8);
	usleep(1);
	llrf_wr(ADDR_ADC1_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 2 */
	llrf_wr(ADDR_ADC2_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x800010ff);
	usleep(1);
	llrf_wr(ADDR_ADC2_SPI, 0x8000ff01);
	usleep(1);
	
	/* ADC Channel 3 */
	llrf_wr(ADDR_ADC3_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x800010fc);
	usleep(1);
	llrf_wr(ADDR_ADC3_SPI, 0x8000ff01);
	usleep(1);


	/* ADC Channel 4 */
	llrf_wr(ADDR_ADC4_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x800010eb);
	usleep(1);
	llrf_wr(ADDR_ADC4_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 5 */
	llrf_wr(ADDR_ADC5_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x800010e0);
	usleep(1);
	llrf_wr(ADDR_ADC5_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 6 */
	llrf_wr(ADDR_ADC6_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x800010ee);
	usleep(1);
	llrf_wr(ADDR_ADC6_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 7 */
	llrf_wr(ADDR_ADC7_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x800010fb);
	usleep(1);
	llrf_wr(ADDR_ADC7_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 8 */
	llrf_wr(ADDR_ADC8_SPI, 0x80001600);
        usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x800010a6);
	usleep(1);
	llrf_wr(ADDR_ADC8_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 9 */
	llrf_wr(ADDR_ADC9_SPI, 0x80001600);
	sleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x800010db);
	usleep(1);
	llrf_wr(ADDR_ADC9_SPI, 0x8000ff01);
	usleep(1);

	/* ADC Channel 10 */
	llrf_wr(ADDR_ADC10_SPI, 0x80001600);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x80001790);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x800010e3);
	usleep(1);
	llrf_wr(ADDR_ADC10_SPI, 0x8000ff01);
	usleep(1);
}

void init_llrf_register(int board_num)
{
	unsigned int spi, misc1, misc2, dac_ctrl, dac_shift, adc_ctrl;
	// SPI setting
	// MISC Control initialize

	switch(board_num) {
		case 0:
			printf("Pass 1\n");
			setting_spi_board_0();
			break;
		case 1:
			printf("Pass 2\n");
			setting_spi_board_1();
			break;
		case 2:
			printf("Pass 3\n");
			setting_spi_board_2();
			break;
		default:
			printf("Undifiend board number...program will exit...\n");
			exit(-1);
	}
	
	/* KBV SPI */
	llrf_wr(ADDR_KBV_SPI, 0x80000b01);
	usleep(1);
	llrf_wr(ADDR_KBV_SPI, 0x8000ff01);
	usleep(1);

	/* KBC SPI */
	llrf_wr(ADDR_KBC_SPI, 0x80000b01);
	usleep(1);
	llrf_wr(ADDR_KBC_SPI, 0x8000ff01);
	usleep(1);

	/* Temperature */
	llrf_wr(ADDR_TEMP_SPI, 0x20000000);
	usleep(1);

	/* TRIG OUT, TRIG IN SW */
	llrf_up_b(ADDR_MISC_CONTROL1, 0x7000, 0x0);
	usleep(1);

	/* DAC INVERT DELAY */
	llrf_up_b(ADDR_DAC_CTRL, 0x30000000, 0x0);
	llrf_up_b(ADDR_DAC_SHIFT_SIZE, 0x7ff, 0x39);
	usleep(1);

	/* DAC INV/ TRIG SHIFT */
	llrf_up_b(ADDR_DAC_CTRL, 0x0ffff000, 0xee25000);
	llrf_up_b(ADDR_DAC_SHIFT_SIZE, 0x000ff000, 0x70000);
	usleep(1000);

	/* DAC */
	llrf_wr(ADDR_DAC_SPI, 0x80000280);
	printf("#\n");
	llrf_wr(ADDR_DAC_SPI, 0x80000bf9);
	printf("##\n");
	llrf_wr(ADDR_DAC_SPI, 0x80000c01);
	printf("###\n");
	llrf_wr(ADDR_DAC_SPI, 0x80000ff9);
	printf("####\n");
	llrf_wr(ADDR_DAC_SPI, 0x80001001);
	printf("#####\n");

	llrf_up_b(ADDR_MISC_CONTROL2, 0x80000, 0x80000);
#if 1
	llrf_up_b(ADDR_ADC_CTRL, 0x1, 0x1);
#endif	
	idata[0] = 0xFFFF;
	qdata[0] = 0xFFFF;

	g_interlock[0] = 0;
	g_interlock[1] = 0;
	g_interlock[2] = 0;
}

static void restore_table()
{
	int fd;
	int size = 0;
	int i, j;
	STO_CALIB restore_calib;

	if( (fd = open("llrf_table.txt", O_RDONLY, 0644)) == -1) {
		printf("%s:%s [%d], open error\n", __FILE__, __func__, __LINE__);
		return;
	}

	if( (size = read(fd, &restore_calib, sizeof(STO_CALIB))) == -1 ) {
		printf("%s:%s [%d], read error\n", __FILE__, __func__, __LINE__);
		return;
	}

	close(fd);

	if( size ) {
		for(i = 0; i < LLRF_CHANNEL-2; i++) {
			memcpy(calib_vars.aon[i], restore_calib.aon[i], sizeof(restore_calib.aon[i]));
			memcpy(calib_vars.aoff[i], restore_calib.aoff[i], sizeof(restore_calib.aoff[i]));
			memcpy(calib_vars.pon[i], restore_calib.pon[i], sizeof(restore_calib.pon[i]));
			memcpy(calib_vars.poff[i], restore_calib.poff[i], sizeof(restore_calib.poff[i]));
			memcpy(calib_vars.onrt[i], restore_calib.onrt[i], sizeof(restore_calib.onrt[i]));
			memcpy(calib_vars.ofrt[i], restore_calib.ofrt[i], sizeof(restore_calib.ofrt[i]));
			memcpy(calib_vars.agin[i], restore_calib.agin[i], sizeof(restore_calib.agin[i]));
		}

		memcpy(calib_vars.atbl[1], restore_calib.atbl, sizeof(restore_calib.atbl));
		memcpy(calib_vars.ptbl[1], restore_calib.ptbl, sizeof(restore_calib.ptbl));
		memcpy(calib_vars.patbl[1], restore_calib.patbl, sizeof(restore_calib.patbl));
		memcpy(calib_vars.pptbl[1], restore_calib.pptbl, sizeof(restore_calib.pptbl));
		memcpy(calib_vars.pvoff[1], restore_calib.pvoff, sizeof(restore_calib.pvoff));
		memcpy(calib_vars.ppoff[1], restore_calib.ppoff, sizeof(restore_calib.ppoff));
	}
/*
	for(i = 0; i < LLRF_CHANNEL-2; i++) {
		printf("=== Channel %d ===\n", i);
		for(j = 0; j < 10; j++) {
			printf("aon[%d] = %lf, aoff[%d] = %lf, pon[%d] = %lf, poff[%d] = %lf, onrt[%d] = %lf, ofrt[%d] = %lf, agin[%d] = %lf\n",
				j, calib_vars.aon[i][j], j, calib_vars.aoff[i][j], j, calib_vars.pon[i][j], j, calib_vars.poff[i][j], 
				j, calib_vars.onrt[i][j], j, calib_vars.ofrt[i][j], j, calib_vars.agin[i][j]);
			if(i == 1) {
				printf("atbl = %lf, ptbl = %lf, patbl = %lf. pptbl = %lf, pvoff = %lf, ppoff = %lf\n",
					calib_vars.atbl[i][j], calib_vars.ptbl[i][j], calib_vars.patbl[i][j], calib_vars.pptbl[i][j],
					calib_vars.pvoff[i][j], calib_vars.ppoff[i][j]);
			}
		}
	}
*/
}

void set_defaults()
{
	/* Set initial value */
	memset(&llrfInfo, 0, sizeof(LLRF_INFO));
	memset(&send_ioc, 0, sizeof(IOC_SEND));
	
	llrfInfo.startpoint[0] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[0] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[1] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[1] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[2] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[2] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[3] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[3] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[4] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[4] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[5] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[5] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[6] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[6] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[7] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[7] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[8] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[8] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.startpoint[9] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.endpoint[9] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.kbv_startpoint[0] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.kbv_endpoint[0] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.kbv_startpoint[1] = (unsigned short)(2.0 * 238*7/12);
	llrfInfo.kbv_endpoint[1] = (unsigned short)(2.2 * 238*7/12);

	llrfInfo.kbc_startpoint[0] = (unsigned short)(1.0 * 238*7/12);
	llrfInfo.kbc_endpoint[0] = (unsigned short)(1.2 * 238*7/12);

	llrfInfo.kbc_startpoint[1] = (unsigned short)(2.0 * 238*7/12);
	llrfInfo.kbc_endpoint[1] = (unsigned short)(2.2 * 238*7/12);

	llrfInfo.dis_sample = 1;
	llrfInfo.rms_sample = 100;

	llrfInfo.pac_power = 0;
	llrfPacPower(llrfInfo.pac_power);
	llrfInfo.trgfreq = ((unsigned int)((29750000/60)/4)*4-1) & 0x0fffffff;

	/* Setting misc value */
	step_const_power = -40;
	step_flag = 0;
	prev_pac_power = 1;
}

void start_llrf_application(int board_num)
{
	/* Ethernet setting */
	init_network();

	/* Receive LLRF data */
	sendReqData();

#if 1
	/* SPI, Register setting */
	init_llrf_register(board_num);
#endif
	/* Create calibration array */
	init_calib_table();

	/* Restore calibration table */
//	restore_table();

	set_defaults();
}

int main(int argc, char *argv[])
{
	int ret;
	int board_num;
	char str[100];
	pthread_t threadId[4];			// 0: normal, 1: receive, 2: graph, 3: storing
	struct sched_param param[4]; 
	pthread_attr_t threadAttr[4];
	
	g_fd = open("/dev/llrf", O_RDONLY);
	if (g_fd < 0) {
		printStr("Error: cannot open device.\n");
		return FALSE;
	}
	printStr("device handle = 0x%x\n", g_fd);

	if(argc == 1) {
		printf("You must input board number...\n");
		printf("Program will exit...\n");
		return(-1);
	} else {
		board_num = atoi(argv[1]);
		printf("Board number = [%d]\n", board_num);
	}

	start_llrf_application(board_num);
#if 1
	/* initialized with default attributes */
	/* Normal */
	ret = pthread_attr_init( &threadAttr[0] );
	ret = pthread_attr_setinheritsched( &threadAttr[0], PTHREAD_EXPLICIT_SCHED );
	if( ret != 0 )
		printf("pthread_attr_setinheritsched() failed(%d) .\n", ret);
	ret = pthread_attr_setschedpolicy( &threadAttr[0], SCHED_RR );
	if( ret != 0 )
		printf("pthread_attr_setschedpolicy() failed(%d) .\n", ret);
	param[0].sched_priority = 20;

	ret = pthread_attr_setschedparam ( &threadAttr[0], &param[0] );
	if( ret != 0 )
		printf("pthread_attr_setschedparam() failed(%d).\n", ret);
	ret = pthread_create( (pthread_t*)&threadId[0], &threadAttr[0], sendThread, NULL);
	if( ret != 0 )
		printf("pthread_create() failed(%d).\n", ret);

	/* Receive */
	ret = pthread_attr_init( &threadAttr[1] );
	ret = pthread_attr_setinheritsched( &threadAttr[1], PTHREAD_EXPLICIT_SCHED );
	if( ret != 0 )
		printf("pthread_attr_setinheritsched() failed(%d) .\n", ret);
	ret = pthread_attr_setschedpolicy( &threadAttr[1], SCHED_RR );
	if( ret != 0 )
		printf("pthread_attr_setschedpolicy() failed(%d) .\n", ret);
	param[1].sched_priority = 5;

	ret = pthread_attr_setschedparam ( &threadAttr[1], &param[1] );
	if( ret != 0 )
		printf("pthread_attr_setschedparam() failed(%d).\n", ret);
	ret = pthread_create( (pthread_t*)&threadId[1], &threadAttr[1], recvThread, NULL);
	if( ret != 0 )
		printf("pthread_create() failed(%d).\n", ret);

	/* Graph */
	ret = pthread_attr_init( &threadAttr[2] );
	ret = pthread_attr_setinheritsched( &threadAttr[2], PTHREAD_EXPLICIT_SCHED );
	if( ret != 0 )
		printf("pthread_attr_setinheritsched() failed(%d) .\n", ret);
	ret = pthread_attr_setschedpolicy( &threadAttr[2], SCHED_RR );
	if( ret != 0 )
		printf("pthread_attr_setschedpolicy() failed(%d) .\n", ret);
	param[2].sched_priority = 5;

	ret = pthread_attr_setschedparam ( &threadAttr[2], &param[2] );
	if( ret != 0 )
		printf("pthread_attr_setschedparam() failed(%d).\n", ret);
	ret = pthread_create( (pthread_t*)&threadId[2], &threadAttr[2], grpThread, NULL);
	if( ret != 0 )
		printf("pthread_create() failed(%d).\n", ret);

	/* Data storing */
	ret = pthread_attr_init( &threadAttr[3] );
	ret = pthread_attr_setinheritsched( &threadAttr[3], PTHREAD_EXPLICIT_SCHED );
	if( ret != 0 )
		printf("pthread_attr_setinheritsched() failed(%d) .\n", ret);
	ret = pthread_attr_setschedpolicy( &threadAttr[3], SCHED_RR );
	if( ret != 0 )
		printf("pthread_attr_setschedpolicy() failed(%d) .\n", ret);
	param[3].sched_priority = 5;

	ret = pthread_attr_setschedparam ( &threadAttr[3], &param[3] );
	if( ret != 0 )
		printf("pthread_attr_setschedparam() failed(%d).\n", ret);
	ret = pthread_create( (pthread_t*)&threadId[3], &threadAttr[3], stoThread, NULL);
	if( ret != 0 )
		printf("pthread_create() failed(%d).\n", ret);
#endif
   
	printStr("Type \"help\" to get a list of commands.\r\n");
	do{
		printStr("llrf>> ");
		fgets(str, 100, stdin);
		str[strlen(str)-1] = '\0';
		if(!strcmp(str, "exit")) gbExit = 1;
	}while (!gbExit);

	close(g_fd);
#if 1
	pthread_attr_destroy( &threadAttr[0] );
//	pthread_attr_destroy( &threadAttr[1] );
	pthread_attr_destroy( &threadAttr[2] );
	pthread_attr_destroy( &threadAttr[3] );
	if( threadId[0] ) pthread_join( threadId[0], NULL );
//	if( threadId[1] ) pthread_join( threadId[1], NULL );
	if( threadId[2] ) pthread_join( threadId[2], NULL );
	if( threadId[3] ) pthread_join( threadId[3], NULL );
	
	printf("PollCmdThread end\n");
#endif
	return 0;
}

