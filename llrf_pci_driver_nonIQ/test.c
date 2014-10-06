#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _LLRF_INFO_ {
	int cmd;
	int attr;
	double module_loss[10];			// internal loss(dBm)
	double external_loss[10];		// external loss(dBm)
	double constant_loss;			// constant loss(dBm)
	double pac_loss;				// pac loss(not definition)(dBm)
	double interAtten[2];			// extra internal loss of two channel(dBm)
	double amp_weight[5];			// amplitude weight
	double pha_weight[5];			// phase weight
	double ata[5];					// amplitude target value(dBm)
	double pta[5];					// phase target value
	double target_max;			// target max amplitude(dBm)
	double ploa;					// low feedback amplitude
	double phia;					// high feedback amplitude
	double initpower;				// Used power when do not feedback(constant mode)
	double initphase;				// Used phase when do not feedback(constant mode)
	double aki;
	double pki;
	double psk_const;				// psk constant(hofs)
	double psk_offset;				// psk offset
	unsigned int kbv_startpoint[2];	// kbv startpoint
	unsigned int kbv_endpoint[2];	// kbv endpoint
	unsigned int kbc_startpoint[2];	// kbc startpoint
	unsigned int kbc_endpoint[2];	// kbc endpoint
	unsigned int ivmi;				// invalid value range
	unsigned int ivid;				// invalid diff of amplitude(dBm)
	unsigned int ipid;				// invalid diff of phase
	unsigned int dis_sample[10];		// display samples
	unsigned int rms_sample;		// rms samples
	unsigned int trgfreq;			// trigger frequency
	unsigned short startpoint[10];	// startpoint
	unsigned short endpoint[10];		// endpoint
	unsigned short pulse_width;
	unsigned short psk_time;		// psk time
	unsigned short trgtime;			// trigger on time
	unsigned short amp[10];			// amplifier on/off information
	unsigned short calib_mode;		// calibration mode
	unsigned short pfb[5];			// phase feedback on/off
	unsigned short afb[5];			// amplitude feedback on/off
	unsigned short doubled_loop;	// doubled loop mode
	unsigned short selected_ch;		// selected switch(org. in mux)
	unsigned short pac_power;		// pac power
	unsigned short trg_sync_pol;		// trigger sync pol
	unsigned short sledswitch1;		// sled1 switch
	unsigned short sledswitch2;		// sled2 switch
	unsigned short psk_mode;		// psk mode(HW on or SW on or OFF)
} LLRF_INFO;

typedef struct _IOC_SEND_ {
	int cmd;
	double peakpower[10];
	double mean_amp[10];
	double mean_pha[10];
	double disp_amp[10];
	double disp_pha[10];
	double rms_perc[10];
	double rms_psdev[10];
	double outputAmp;
	double outputPha;
	double mean_i;
	double mean_q;
	double sdev_i;
	double sdev_q;
	double temper[2];
	unsigned short ext_interlock[3];
	unsigned short adc_over[10];
	unsigned short ref_range_over;
	unsigned short sled_interlock;
	unsigned short temper_over[2];
} IOC_SEND;

int main(int argc, char *argv[])
{
	struct sockaddr_in drvAddr;
	struct sockaddr_in hostAddr;
	struct sockaddr_in Addr;
	int sock;
	int readlen;
	int clientlen = sizeof(Addr);
	int status;
	LLRF_INFO buffer;
	IOC_SEND recv_buf;

	if( (sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		printf("socket error\n");
		exit(-1);
	}

	memset(&hostAddr, 0, sizeof(hostAddr));
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	hostAddr.sin_port = htons(3600);

	if( bind(sock, (struct sockaddr *)&hostAddr, sizeof(hostAddr)) < 0 ) {
		printf("bind error\n");
		exit(-1);
	}

	memset(&drvAddr, 0, sizeof(drvAddr));
	drvAddr.sin_family = AF_INET;
	drvAddr.sin_addr.s_addr = inet_addr("192.168.0.68");
	drvAddr.sin_port = htons(3600);

	memset(&buffer, 0, sizeof(buffer));
	memset(&recv_buf, 0, sizeof(recv_buf));
	while(1) {
		readlen = recvfrom(sock, &recv_buf, sizeof(IOC_SEND), 0, (struct sockaddr *)&Addr, &clientlen);
		if( recv_buf.cmd == 1 ) {
			printf("Request Packet!!\n");
			buffer.cmd = 2;
			status = sendto(sock, &buffer, sizeof(LLRF_INFO), 0, (struct sockaddr*)&Addr, sizeof(Addr));
			printf("%d\n", status);
		}
	}

	return 0;
}
