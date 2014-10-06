/*
  * Applying result Program
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

unsigned short idata[LLRF_SAMPLING];
unsigned short qdata[LLRF_SAMPLING];

unsigned int dac_i[LLRF_SAMPLING/2];
unsigned int dac_q[LLRF_SAMPLING/2];

extern unsigned int llrf_up_b(unsigned int pAddr, unsigned int mask, unsigned int data);
extern unsigned int llrf_wr(unsigned int pAddr, unsigned int data);
extern unsigned int llrf_rd(unsigned int pAddr);
extern void set_reg_misc_control2(unsigned int val);
extern int sendReqData();

int loopback()
{
	llrf_ioctl_t llrf_ioctl_dac_i;
	llrf_ioctl_t llrf_ioctl_dac_q;
	unsigned int reg;
	unsigned int dac_trans_size, dac_ctrl;
	unsigned int val;
	int i, j, ret;

	/* Ready to write in memory */
	llrf_ioctl_dac_i.offset = ADDR_DAC_I_DATA;
	llrf_ioctl_dac_i.len = LLRF_SAMPLING/2;
	llrf_ioctl_dac_i.buf = dac_i;

	llrf_ioctl_dac_q.offset = ADDR_DAC_Q_DATA;
	llrf_ioctl_dac_q.len = LLRF_SAMPLING/2;
	llrf_ioctl_dac_q.buf = dac_q;

	if( (misc2_container & 0x3) == 0x0 ) {
		if( (idata[0] != 0) && (qdata[0] != 0) ) {
			for(i = 0; i < LLRF_SAMPLING/2; i++)
				dac_i[i] = 0x7fff7fff;
			ret = ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl_dac_i);

			for(i = 0; i < LLRF_SAMPLING; i++)
				dac_q[i] = 0x80008000;
			ret = ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl_dac_q);

			idata[0] = 0;
			qdata[0] = 0;
		}
	} else {
		if (memcmp((void *)icont, (void *)idata, sizeof(idata))
			|| memcmp((void *)qcont, (void *)qdata, sizeof(qdata)) ) {

			for(i = 0, j = 0; i < LLRF_SAMPLING/2; i++, j+=2) {
				dac_i[i] = (icont[j+1] << 16) | icont[j];
			}
			ret = ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl_dac_i);

			for(i = 0, j = 0; i < LLRF_SAMPLING/2; i++, j+=2) {
				dac_q[i] = (qcont[j+1] << 16) | qcont[j];
			}
			ret = ioctl(g_fd, LLRF_IOCTL_CMD_PUT_DATA, (unsigned long)&llrf_ioctl_dac_q);

			memcpy((void *)idata, (void *)icont, sizeof(idata));
			memcpy((void *)qdata, (void *)qcont, sizeof(qdata));
		}
	}
#if 1
	switch(ampon) {
		case AMP_OFF:
			val = misc2_container & ~(0x1 << (llrfInfo.selected_ch + 4));
			set_reg_misc_control2(val);
			break;
		case AMP_ON:
			val = misc2_container;
			val &= ~(0x1 << (llrfInfo.selected_ch + 4));
			val |= (0x1 << (llrfInfo.selected_ch + 4));
			set_reg_misc_control2(val);
			break;
		case AMP_RELOAD:
			sendReqData();
			break;
		case PSK_CAL_STEP_1:
			reg = llrf_rd(ADDR_MISC_CONTROL2);
			/* RF type: PM, PSK ON : HW ON */
			reg = (reg & 0xfffffff0) | (0x2 << 2) | 0x3;

			/* Amp off */
			reg = (reg & 0xffffffdf);
			set_reg_misc_control2(reg);

			llrfInfo.startpoint[0] = 0xf8;
			llrfInfo.endpoint[0] = 0x687;

			llrfInfo.startpoint[1] = 0xf8;
			llrfInfo.endpoint[1] = 0x687;

			llrfInfo.startpoint[2] = 0xf8;
			llrfInfo.endpoint[2] = 0x687;

			llrfInfo.startpoint[3] = 0xf8;
			llrfInfo.endpoint[3] = 0x687;

			llrfInfo.startpoint[4] = 0xf8;
			llrfInfo.endpoint[4] = 0x687;

			llrfInfo.startpoint[5] = 0xf8;
			llrfInfo.endpoint[5] = 0x687;

			llrfInfo.startpoint[6] = 0xf8;
			llrfInfo.endpoint[6] = 0x687;

			llrfInfo.startpoint[7] = 0xf8;
			llrfInfo.endpoint[7] = 0x687;

			llrfInfo.startpoint[8] = 0xf8;
			llrfInfo.endpoint[8] = 0x687;

			llrfInfo.startpoint[9] = 0xf8;
			llrfInfo.endpoint[9] = 0x687;

			llrf_up_b(ADDR_DAC_TRANS_SIZE, 0x7ff, 0x7d0);
			llrf_up_b(ADDR_DAC_CTRL, 0x7ff, 0x7d0);
			break;
		case PSK_CAL_STEP_2:
			llrf_up_b(ADDR_DAC_CTRL, 0x7ff, 0x0);
			break;
		case PSK_CAL_STEP_3:
			break;
	}
#endif
//	printf("End of loopback\n");
	return 0;
}
