/*
 *   llrf_pci.c (c) 2014 Euijae Lee (ejlee@mobiis.com) All Rights Reserved
 *
 *  Linux driver for LLRF PCIe Interface 
 *
 *  This software may be used and distributed according to the terms
 *  of the GNU General Public License, incorporated herein by reference.
 * 
 *
 *  To Do:
 */
#ifndef LLRF_ACCESS_H
#define LLRF_ACCESS_H

#define XL_PCIE_VENDOR_ID	0x10EE
#define XL_PCIE_DEVICE_ID	0x6011

/* define structure for using between USER and Driver */
typedef struct{
	union{
		struct{
			unsigned int calc[512];
			unsigned int adc[10][512];
			unsigned int dac[512];
			unsigned int config[512];
			unsigned int reserved[3][512];
		}llrf_data;
		struct{
			unsigned int llrf_all[16][512];
		}llrf_data_all;
	}u;
	
}LLRF_ACCESS_T;

typedef struct{
	unsigned int offset;
	unsigned int len;	// length in DWORD
	unsigned int *buf;
}__attribute__((packed)) llrf_ioctl_t;


#define IOCTL_LLRF_MAGIC		'l'

#define LLRF_IOCTL_CMD_GET_DATA	    _IOR(IOCTL_LLRF_MAGIC, 0, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_GET_DATA_DMA	_IOR(IOCTL_LLRF_MAGIC, 1, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_GET_CONFIG	_IOR(IOCTL_LLRF_MAGIC, 2, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_GET_D		    _IOR(IOCTL_LLRF_MAGIC, 3, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_DMA_GET_D_R	_IOR(IOCTL_LLRF_MAGIC, 4, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_DMA_GET_D_W	_IOR(IOCTL_LLRF_MAGIC, 5, llrf_ioctl_t)

#define LLRF_IOCTL_CMD_PUT_DATA	    _IOW(IOCTL_LLRF_MAGIC, 6, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_PUT_DATA_DMA	_IOW(IOCTL_LLRF_MAGIC, 7, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_PUT_CONFIG	_IOW(IOCTL_LLRF_MAGIC, 8, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_PUT_D		    _IOW(IOCTL_LLRF_MAGIC, 9, llrf_ioctl_t)

#define LLRF_IOCTL_CMD_DMA_MAP	    _IOW(IOCTL_LLRF_MAGIC, 10, int)
#define LLRF_IOCTL_CMD_DMA_UNMAP	    _IOW(IOCTL_LLRF_MAGIC, 11, int)
#define LLRF_IOCTL_CMD_DMA_START	    _IOW(IOCTL_LLRF_MAGIC, 12, int)
#define LLRF_IOCTL_CMD_DMA_CLEAR	    _IO (IOCTL_LLRF_MAGIC, 13)
#define LLRF_IOCTL_CMD_DMA_REGION_R	_IOW(IOCTL_LLRF_MAGIC, 14, llrf_ioctl_t)
#define LLRF_IOCTL_CMD_DMA_REGION_W	_IOW(IOCTL_LLRF_MAGIC, 15, llrf_ioctl_t)

#define IOCTL_LLRF_MAXNR		16

#endif
