#ifndef __ATTR_RECORDS__
#define __ATTR_RECORDS__

/*#####################################################################################
 ## FPGA Address
 ######################################################################################*/
#define LLRF_BASE_ADDR				0x0000

#define ADDR_ADC1_SPI				(LLRF_BASE_ADDR + 0x0100)
#define ADDR_ADC2_SPI				(LLRF_BASE_ADDR + 0x0140)
#define ADDR_ADC3_SPI				(LLRF_BASE_ADDR + 0x0180)
#define ADDR_ADC4_SPI				(LLRF_BASE_ADDR + 0x01c0)
#define ADDR_ADC5_SPI				(LLRF_BASE_ADDR + 0x0200)
#define ADDR_ADC6_SPI				(LLRF_BASE_ADDR + 0x0240)
#define ADDR_ADC7_SPI				(LLRF_BASE_ADDR + 0x0280)
#define ADDR_ADC8_SPI				(LLRF_BASE_ADDR + 0x02c0)
#define ADDR_ADC9_SPI				(LLRF_BASE_ADDR + 0x0300)
#define ADDR_ADC10_SPI				(LLRF_BASE_ADDR + 0x0340)

#define ADDR_DAC_SPI				(LLRF_BASE_ADDR + 0x0380)
#define ADDR_TEMP_SPI				(LLRF_BASE_ADDR + 0x03c0)
#define ADDR_KBV_SPI				(LLRF_BASE_ADDR + 0x0400)
#define ADDR_KBC_SPI				(LLRF_BASE_ADDR + 0x0440)

#define ADDR_ADC_CTRL				(LLRF_BASE_ADDR + 0x0700)
#define ADDR_DAC_CTRL				(LLRF_BASE_ADDR + 0x0740)
#define ADDR_DAC_SHIFT_SIZE			(LLRF_BASE_ADDR + 0x0744)
#define ADDR_DAC_TRANS_SIZE			(LLRF_BASE_ADDR + 0x0748)
#define ADDR_DAC_INVERT_DELAY		(LLRF_BASE_ADDR + 0x074c)
#define ADDR_MISC_CONTROL1			(LLRF_BASE_ADDR + 0x0750)
#define ADDR_MISC_CONTROL2			(LLRF_BASE_ADDR + 0x0754)
#define ADDR_MISC_CONTROL3			(LLRF_BASE_ADDR + 0x0758)

#define ADDR_DAC_I_DATA				(LLRF_BASE_ADDR + 0x1000)
#define ADDR_DAC_Q_DATA			(LLRF_BASE_ADDR + 0x1800)

#define ADDR_ADC1_DATA				(LLRF_BASE_ADDR + 0x2000)
#define ADDR_ADC2_DATA				(LLRF_BASE_ADDR + 0x2800)
#define ADDR_ADC3_DATA				(LLRF_BASE_ADDR + 0x3000)
#define ADDR_ADC4_DATA				(LLRF_BASE_ADDR + 0x3800)
#define ADDR_ADC5_DATA				(LLRF_BASE_ADDR + 0x4000)
#define ADDR_ADC6_DATA				(LLRF_BASE_ADDR + 0x4800)
#define ADDR_ADC7_DATA				(LLRF_BASE_ADDR + 0x5000)
#define ADDR_ADC8_DATA				(LLRF_BASE_ADDR + 0x5800)
#define ADDR_ADC9_DATA				(LLRF_BASE_ADDR + 0x6000)
#define ADDR_ADC10_DATA				(LLRF_BASE_ADDR + 0x6800)
#define ADDR_KBV_ADC_DATA			(LLRF_BASE_ADDR + 0x7000)
#define ADDR_KBC_ADC_DATA			(LLRF_BASE_ADDR + 0x7800)

#define NON_IQ
#define POWER_TEST

#define READ_SIZE		6144
#define LLRF_CHANNEL		12	// 10 channel + 2 kbv channel

#define TABLE_SIZE		101
#define TABLE_HORI_SIZE	10
#define TABLE_AMP_STEP	0.0125

#define INVALID_TBL_SIZE	10
#define MAX_RMS_SAMPLE	501
#define PAD_CTRL_DATA_SIZE	0

#define MAX_ST_PULSE	36000

#define PI	3.1415926535897932384626433832795
#define PI_DIVIDE_180	0.0174532

#ifndef NON_IQ
#define LLRF_SAMPLING	848
#define LLRF_PERIOD		4
#define MAX_ST_SIZE		366336000		// 36000 * 848 * 12
#else
#define LLRF_SAMPLING	1000
#define LLRF_PERIOD		14
#define MAX_ST_SIZE		432000000		// 36000 * 1000 * 12
#define DELTA_DEG		3/14*(2*PI)
#endif

/* Common functions -> define preprocessor */
#define START_POINT(x)	((x)+((LLRF_PERIOD-((x)%LLRF_PERIOD))%LLRF_PERIOD))
#define END_POINT(x)		((((x)+1)/LLRF_PERIOD)*LLRF_PERIOD-1)
#define NSAMPLES(x, y)	(((y)-(x)+1)/LLRF_PERIOD)
#define RANGE(x, y)		((y)-(x)+1)
#define NSAMPLE_I(x)		(x)*6
#define NSAMPLE_Q(x)		(x)*8

#define CONVERT_R_TO_V(x, y)	(0.0000381475547417411*((x) - (y)))
/* #define CONVERT_R_TO_V(x, y)	(0.000030518043793392843518730449378195*((x) - (y))) */
#define CONVERT_R_TO_A(I, Q)	sqrt((double)(pow((I),2)+pow((Q),2)))
#define CONVERT_R_TO_P(I, Q)	atan2((Q), (I))*57.295779513082320876798154814105
#define CONVERT_V_TO_D(v)	10*log10(((v)*(v))*20)
#define CONVERT_D_TO_V(d)	 sqrt(0.05*pow(10, (d)/10));
#define CONVERT_D_TO_W(w)	pow(10, (w)/10)
#define CONVERT_W_TO_D(w)	10*log10((w))

#define PERCENTAGE(x, y)		((x)/(y))*100
#define CONVERT_DAC_I(a, p, d)		(((a)*cos(((d)+(p))*PI_DIVIDE_180))+1.25)*26214
#define CONVERT_DAC_Q(a, p, d)	(((a)*sin(((d)+(p))*PI_DIVIDE_180))+1.25)*26214

/* structures */
typedef enum {
	TEMPER_SYS = 0,
	TEMPER_RFM,
	MAX_TEMPER
} LLRF_TEMPER;

typedef enum {
	ATTR_PAC_POWER,
	ATTR_PULSE_WIDTH,
	ATTR_PSK_TIME,
	ATTR_TRG_TIME,
	ATTR_TRG_SYNC_POL,
	ATTR_SLED_SWITCH_1,
	ATTR_SLED_SWITCH_2,
	ATTR_PSK_MODE,
	ATTR_AMPLIFIER,
	ATTR_TRG_FREQ,
	ATTR_DEFAULT,
	MAX_ATTR
} LLRF_ATTR;

typedef enum {
	LLRF_CMD_FILTER = 0,
	LLRF_CMD_ATTR,
	LLRF_CMD_REQ_INIT,
	LLRF_CMD_ACK_INIT,
	LLRF_CMD_NORMAL,
	LLRF_CMD_RAW_GRAPH,
	LLRF_CMD_AMP_GRAPH,
	LLRF_CMD_PHA_GRAPH,
	MAX_LLRF_CMD
} LLRF_CMD;

enum {
	OFF = 0,
	ON
};

enum {
	PID = 0,
	PAD_CAL,
	PAC_CAL,
	PSK_CAL
};

enum {
	AMP_OFF = 0,
	AMP_ON,
	AMP_OPERATION,
	AMP_RELOAD,
	PSK_CAL_STEP_1,
	PSK_CAL_STEP_2,
	PSK_CAL_STEP_3,
	MAX_AMP
};

enum {
	PSK_OFF = 0,
	PSK_HWON = 2,
	PSK_SWON = 3,
	MAX_PSK_MODE
};

typedef struct _LLRF_INFO_ {
	/* Received and information structure */
	int cmd;						// Command
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
	double aki;					// PID control(amplitude)
	double pki;					// PID control(phase)
	double psk_const;				// psk constant(hofs)
	double psk_offset;				// psk offset
	double ppmxr[4];				// peak power max range in channel 6, 7, 8, 9
	double ref_amp_stb_xr;		// reference channel power stability range
	double ref_pha_stb_xr;			// reference channel phase stability range
	double kbv_stb_xr[2];			// KBV stability range
	double kbc_stb_xr[2];			// KBC stability range
	double ivid;					// invalid diff of amplitude(dBm)
	double ipid;					// invalid diff of phase
	unsigned int ivmi;				// invalid value range
	unsigned int dis_sample;		// display samples
	unsigned int rms_sample;		// rms samples
	unsigned int trgfreq;			// trigger frequency
	unsigned int userStPulse;		// user stored buffer size (# of pulse)
	unsigned short startpoint[10];	// startpoint
	unsigned short endpoint[10];		// endpoint
	unsigned short kbv_startpoint[2];	// kbv startpoint
	unsigned short kbv_endpoint[2];	// kbv endpoint
	unsigned short kbc_startpoint[2];	// kbc startpoint
	unsigned short kbc_endpoint[2];	// kbc endpoint
	unsigned short pulse_width;		// pulse width
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
	unsigned short feedback_mode;	// Feedback mode
	unsigned short userStSwitch;	// user stored buffer switch (on/off)
} LLRF_INFO;

typedef struct _IOC_SEND_ {
	/* Send data structure */
	int cmd;
	double peakpower[10];				// peak power(10 channel)
	double disp_amp[10];				// display value(amplitude)
	double disp_pha[10];				// display value(phase)
	double mean_amp[10];				// Average of amplitude
	double mean_pha[10];				// Average of phase
	double rms_perc[10];				// Stability of amplitude
	double rms_psdev[10];				// Stability of phase
	double outputAmp;					// Output value of amplitude
	double outputPha;					// Output value of phase
	double mean_i[10];					// Average of raw data
	double sdev_i[10];					// Stability of raw data
	double kbv_mean[2];				// Average of klystron beam voltage
	double kbv_sdev[2];				// Stability of klystron beam voltage
	double kbv_perc[2];				// Stability of klystron beam voltage(percentage)
	double kbc_mean[2];				// Average of klystron beam current
	double kbc_sdev[2];				// Stability of klystron beam current
	double kbc_perc[2];				// Stability of klystron beam current(percentage)
	double temper[2];					// Temperature
	unsigned short ext_interlock[3];		// External interlock
	unsigned short adc_over[10];			// ADC overflow
	unsigned short ref_range_over;		// REF. range overflow
	unsigned short ref_amp_stb_over;	// REF. stability overflow(amplitude)
	unsigned short ref_pha_stb_over;	// REF. stability overflow(phase)
	unsigned short sled_interlock;		// SLED mismatching
	unsigned short pp_interlock[4];		// Peak power overflow
	unsigned short temper_over[2];		// Temperature overflow
	unsigned short kbv_stb_over[2];		// Stability of klystron beam voltage overflow
	unsigned short kbc_stb_over[2];		// Stability of klystron beam current overflow
	unsigned short il_monitoring[2];		// 0: internal, 1: external
} IOC_SEND;

/* Graph structuer : Raw data */
typedef struct _GRP_RAW_SEND_ {
	int cmd;
	float raw_data[LLRF_CHANNEL][LLRF_SAMPLING];
} GRP_RAW_SEND;

/* Graph structuer : Amplitude */
typedef struct _GRP_AMP_SEND_ {
	int cmd;
	float amp_grp[LLRF_CHANNEL-2][LLRF_SAMPLING];
} GRP_AMP_SEND;

/* Graph structuer : Phase */
typedef struct _GRP_PHA_SEND_ {
	int cmd;
	float pha_grp[LLRF_CHANNEL-2][LLRF_SAMPLING];
} GRP_PHA_SEND;

typedef struct _AMP_VAR_ {
	/* Amplitude info structure */
	double g_amplitude[LLRF_CHANNEL-2];
	double g_damplitude[LLRF_CHANNEL-2];
	double g_oamplitude[LLRF_CHANNEL-2];
	double g_pamplitude[LLRF_CHANNEL-2];
	double g_peak[LLRF_CHANNEL-2];
} AMP_VAR;

typedef struct _PHA_VAR_ {
	/* Phase info structuer */
	double g_phase[LLRF_CHANNEL-2];
	double g_cpha[LLRF_CHANNEL-2];
} PHA_VAR;

typedef struct _AMP_PHA_COMMON_ {
	double i_data;
	double q_data;
} AMP_PHA_COMMON;

typedef struct _CALIB_VAR_ {
	/* Calibration table structure */
	double ptblv[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double atblv[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double ptbl[LLRF_CHANNEL-2][TABLE_SIZE];
	double atbl[LLRF_CHANNEL-2][TABLE_SIZE];
	double aont[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double pont[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double aofft[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double pofft[LLRF_CHANNEL-2][TABLE_SIZE * TABLE_HORI_SIZE];
	double aon[LLRF_CHANNEL-2][TABLE_SIZE];
	double pon[LLRF_CHANNEL-2][TABLE_SIZE];
	double onrt[LLRF_CHANNEL-2][TABLE_SIZE];
	double agin[LLRF_CHANNEL-2][TABLE_SIZE];
	double aoff[LLRF_CHANNEL-2][TABLE_SIZE];
	double poff[LLRF_CHANNEL-2][TABLE_SIZE];
	double ofrt[LLRF_CHANNEL-2][TABLE_SIZE];
	double patbl[LLRF_CHANNEL-2][TABLE_SIZE];
	double pptbl[LLRF_CHANNEL-2][TABLE_SIZE];
	double pvoff[LLRF_CHANNEL-2][TABLE_SIZE];
	double ppoff[LLRF_CHANNEL-2][TABLE_SIZE];
} CALIB_VAR;

typedef struct _STO_CALIB_ {
	/* Storage structure */
	double aon[LLRF_CHANNEL-2][TABLE_SIZE];
	double aoff[LLRF_CHANNEL-2][TABLE_SIZE];
	double pon[LLRF_CHANNEL-2][TABLE_SIZE];
	double poff[LLRF_CHANNEL-2][TABLE_SIZE];
	double onrt[LLRF_CHANNEL-2][TABLE_SIZE];
	double ofrt[LLRF_CHANNEL-2][TABLE_SIZE];
	double agin[LLRF_CHANNEL-2][TABLE_SIZE];
	double atbl[TABLE_SIZE];
	double ptbl[TABLE_SIZE];
	double patbl[TABLE_SIZE];
	double pptbl[TABLE_SIZE];
	double pvoff[TABLE_SIZE];
	double ppoff[TABLE_SIZE];
}  STO_CALIB;

/* Interlock data in VME_LLRF_DATA */
typedef struct _INTERLOCK_ {
	unsigned long ext_intr		: 3;
	unsigned long adc_over		: 10;
	unsigned long kbv_kbc		: 4;		/* 0, 1: KBV / 2, 3: KBC */
	unsigned long reference		: 3;		/* 0: REF over, 1: REF AMP STB, 2: REF PHA STB */
	unsigned long peak_over	: 4;	
	unsigned long sled_error	: 1;
	unsigned long temp_over	: 2;		/* 0: SYS, 1: RFM */
	unsigned long ssa_intr		: 5;		/* SSA: not modifed yet */
} INTERLOCK;

/* VME BSA data */
typedef struct _VME_LLRF_DATA_ {
	/* Phase, Amplitude information */
	/* One pulse */
	float phase[10];						/* Phase mean value of one pulse */
	float amplitude[10];					/* Amplitude mean value of one pulse */
	/* Number of samples */
	float phase_mean[10];					/* Phase mean value of samples */
	float amplitude_mean[10];				/* Amplitude mean value of samples */
	float phase_sdev[10];					/* Standard deviation of phase */
	float amplitude_perc[10];				/* Percentage of amplitude */

	/* KBV information */
	/* KBV */
	float kbv_power[2];				/* KBV mean value of one pulse */
	float kbv_sdev[2];					/* KBV standard deviation value */
	float kbv_perc[2];					/* Percentage of KBV */
	/* KBC */
	float kbc_power[2];				/* KBC mean value of one pulse */
	float kbc_sdev[2];					/* KBC standard deviation value */
	float kbc_perc[2];					/* Percentage of KBC */

	INTERLOCK intr_field;				/* interlock bit-field */
}VME_LLRF_DATA;

typedef struct _DATA_STORAGE_ {
	unsigned short *buf;
	VME_LLRF_DATA *bsa;
	unsigned int pos;
}  DATA_STORAGE;

extern LLRF_INFO llrfInfo;
extern IOC_SEND send_ioc;

extern AMP_VAR amp_vars;
extern PHA_VAR pha_vars;
extern AMP_PHA_COMMON apcom_vars;
extern CALIB_VAR calib_vars;

extern int cal_index;

extern unsigned short idata[LLRF_SAMPLING];
extern unsigned short qdata[LLRF_SAMPLING];
extern unsigned short icont[LLRF_SAMPLING];
extern unsigned short qcont[LLRF_SAMPLING];

extern double dac_aval;
extern double dac_pval;

extern unsigned short ampon;
extern unsigned int misc2_container;
extern int g_fd;

extern unsigned short g_raw[LLRF_CHANNEL][LLRF_SAMPLING];

extern double step_const_power;
extern int step_flag;

#endif
