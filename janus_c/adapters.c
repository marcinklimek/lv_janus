/*******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/

#include "MultiPlatform.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "FERSlib.h"	
#include "JanusC.h"	
#include "console.h"
#include "adapters.h"
#include "configure.h"

// #################################################################################
// Global Variables
// #################################################################################
float CalibratedThrOffset[MAX_NBRD][MAX_NCH];
int ThrCalibLoaded[MAX_NBRD] = { 0 };
int ThrCalibIsDisabled[MAX_NBRD] = { 0 };


// #################################################################################
// Functions
// #################################################################################

// ---------------------------------------------------------------------------------
// Description: Return the number of channels of the adapter
// Return:		num of channels (0 = no adapter)
// ---------------------------------------------------------------------------------
int AdapterNch()
{
	if (WDcfg.AdapterType == ADAPTER_A5256)
		return 17;
	else if ((WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG) || (WDcfg.AdapterType == ADAPTER_A5256_REV0_POS))
		return 9;
	else return 0;
}

// ---------------------------------------------------------------------------------
// Description: Remap TDC channel into Adapter channel 
// Inputs:		TDC_ch: picoTDC channel number 
// Outputs:		Adapter_ch: Adapter channel number
// Return:		0: map OK, -1: map error
// ---------------------------------------------------------------------------------
int ChIndex_tdc2ada(int TDC_ch, int *Adapter_ch)
{
	int ich, ret = 0;
	*Adapter_ch = 0;
	if (WDcfg.AdapterType == ADAPTER_NONE) {
		*Adapter_ch = TDC_ch;
	} else if (WDcfg.AdapterType == ADAPTER_A5256) {
		ich = A5256_TDC2ADA[TDC_ch];
		if (ich == -1) ret = -1;
		else *Adapter_ch = ich;
	} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_POS) {
		ich = A5256_REV0_TDC2ADA_POS[TDC_ch];
		if (ich == -1) ret = -1;
		else *Adapter_ch = ich;
	} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG) {
		ich = A5256_REV0_TDC2ADA_NEG[TDC_ch];
		if (ich == -1) ret = -1;
		else *Adapter_ch = ich;
	}
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Remap Adapter channel into TDC channel 
// Inputs:		Adapter_ch: Adapter channel number
// Outputs:		TDC_ch: TDC channel number
// Return:		0: map OK, -1: map error
// ---------------------------------------------------------------------------------
int ChIndex_ada2tdc(int Adapter_ch, int *TDC_ch)
{
	*TDC_ch = -1;
	if (WDcfg.AdapterType == ADAPTER_NONE) {
		*TDC_ch = Adapter_ch;
	} else if (WDcfg.AdapterType == ADAPTER_A5256) {
		if ((Adapter_ch == 0) && (WDcfg.A5256_Ch0Polarity == A5256_CH0_POSITIVE))			*TDC_ch = 1;
		else if ((Adapter_ch == 0) && (WDcfg.A5256_Ch0Polarity == A5256_CH0_NEGATIVE))		*TDC_ch = 0;
		else *TDC_ch = A5256_ADA2TDC[Adapter_ch];
	} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_POS) {
		*TDC_ch = A5256_REV0_ADA2TDC_POS[Adapter_ch];
	} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG) {
		*TDC_ch = A5256_REV0_ADA2TDC_NEG[Adapter_ch];
	}
	return 0;
}

// ---------------------------------------------------------------------------------
// Description: Remap bits of the channel mask
// Inputs:		AdapterMask: Adapter channel mask
// Outputs		ChMask0: TDC channel mask for ch 0..31
//				ChMask1: TDC channel mask for ch 32..63
// Return:		0: OK; -1: error
// ---------------------------------------------------------------------------------
int ChMask_ada2tdc(uint32_t AdapterMask, uint32_t *ChMask0, uint32_t *ChMask1)
{
	int i, TDCch;
	uint64_t TDCmask = 0;
	int nch = AdapterNch();
	if (WDcfg.AdapterType == ADAPTER_NONE) 
		return -1;
	for(i=0; i<nch; i++) {
			if (AdapterMask & (1 << i)) {
			if (WDcfg.AdapterType == ADAPTER_A5256)
				if ((i == 0) && (WDcfg.A5256_Ch0Polarity == A5256_CH0_POSITIVE))		TDCch = 1;
				else if ((i == 0) && (WDcfg.A5256_Ch0Polarity == A5256_CH0_NEGATIVE))	TDCch = 0;
				else TDCch = A5256_ADA2TDC[i];
			else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_POS)
				TDCch = A5256_REV0_ADA2TDC_POS[i];
			else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG)
				TDCch = A5256_REV0_ADA2TDC_NEG[i];
				TDCmask |= (uint64_t)1 << TDCch;
			}
		}
	*ChMask0 = (uint32_t)(TDCmask & 0xFFFFFFFF);
	*ChMask1 = (uint32_t)((TDCmask >> 32) & 0xFFFFFFFF);
	return 0;
}



// ---------------------------------------------------------------------------------
// Description: Set discriminator threshold
// Inputs:		handle: board handle
//				Adapter_ch: adapter channel number
//				thr_mv: threshold in mv
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int Set_DiscrThreshold(int handle, int Adapter_ch, float thr_mv)
{
	int dac_ch, paired_ch = -1, ret = 0, b;
	float thr;
	uint32_t dev_addr, dac_set, paired_dac_set, data, addr, dac16;
	int nch = AdapterNch();

	if (WDcfg.AdapterType == ADAPTER_NONE) 
		return 0;
	if (Adapter_ch >= nch) 
		return -1;
	b = FERS_INDEX(handle);
		thr = (float)max(min(thr_mv, 1250), -1250);  
	if (ThrCalibLoaded[b] && !ThrCalibIsDisabled[b]) {
		thr += CalibratedThrOffset[b][Adapter_ch];
		if (thr > 1249) thr = 1249;
		if (thr < -1249) thr = -1249;
	}
	dac_set = A5256_mV_to_DAC(thr);  // Convert Threshold from mV to DAC LSB
		if (Adapter_ch == 0) {
		if ((WDcfg.AdapterType == ADAPTER_A5256) && (WDcfg.A5256_Ch0Polarity == A5256_CH0_POSITIVE)) {
			dac_ch = 1;
			paired_ch = 0;
			paired_dac_set = dac_set;
		} else {
			dac_ch = 0;
			paired_ch = 1;
			paired_dac_set = dac_set;
		}
			dev_addr = 0x10;
		} else {
		if (WDcfg.AdapterType == ADAPTER_A5256) {
			dac_ch = A5256_ADA2DAC[Adapter_ch];
			paired_ch = -1;
		} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_POS) {
			dac_ch = A5256_REV0_ADA2DAC_POS[Adapter_ch];
			paired_ch = A5256_REV0_ADA2DAC_NEG[Adapter_ch];
			paired_dac_set = 0xFFF; // Set threhsold of unused channel to full scale 
		} else if (WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG) {
			dac_ch = A5256_REV0_ADA2DAC_NEG[Adapter_ch];
			paired_ch = A5256_REV0_ADA2DAC_POS[Adapter_ch];
			paired_dac_set = 0xFFF; // Set threhsold of unused channel to full scale 
		}
			dev_addr = (dac_ch >= 16) ? 0x0D : 0x0C;
		}
		addr = 0x30 | (dac_ch & 0xF);
	dac16 = dac_set << 4;  // DAC uses 16 bit format (4 LSBs = 0)
	data = ((dac16 & 0xFF) << 24) | (((dac16 >> 8) & 0xFF) << 16);
		ret = FERS_I2C_WriteRegister(handle, dev_addr, addr, data);
	if (paired_ch >= 0) {
		addr = 0x30 | (paired_ch & 0xF);
		dac16 = paired_dac_set << 4;
		data = ((dac16 & 0xFF) << 24) | (((dac16 >> 8) & 0xFF) << 16);
		ret = FERS_I2C_WriteRegister(handle, dev_addr, addr, data);
	}
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Calibrate the threshold offset (zero)
// Inputs:		handle: board handle
//				min_thr: minimun threshold for the scan
//				max_thr: maximum threshold for the scan
// Outputs:		ThrOffset: threshold offsets
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
#define THRCALIB_MAX_NSTEP	100
int CalibThresholdOffset(int handle, float min_thr, float max_thr, int *done, float *ThrOffset, float *RMSnoise)
{
	int ret = 0, b, bb, size, Quit = 0, nb, ch, dtq, ntrg, niter, step, nstep;
	int nch;
	ListEvent_t* Event;
	double tstamp_us;
	uint32_t hit_cnt[THRCALIB_MAX_NSTEP][MAX_NCH], hit_cnt_max[MAX_NCH] = { 0 }, tot_hit, dac_start;
	uint64_t st;
	float thr[THRCALIB_MAX_NSTEP];
	Config_t *WDcfg_tmp;
	FILE* calib;

	b = FERS_INDEX(handle);
	if (WDcfg.AdapterType == ADAPTER_NONE) return -1;
	nch = AdapterNch();
	nstep = (int)((max_thr - min_thr) / A5256_DAC_LSB) + 1;
	if (nstep > THRCALIB_MAX_NSTEP) {
		Con_printf("LSCm", "The scan range from %.1f to %.1f is too high\n", min_thr, max_thr);
		return -1;
	}
	dac_start = A5256_mV_to_DAC(min_thr);
	if (A5256_DAC_to_mV(dac_start) > min_thr) {
		dac_start++;
		nstep++;
}

	// Reset current calibration
	memset(CalibratedThrOffset[b], 0, nch * sizeof(float));
	ThrCalibLoaded[b] = 0;

	WDcfg_tmp = (Config_t *)malloc(sizeof(Config_t));
	memcpy(WDcfg_tmp, &WDcfg, sizeof(Config_t));  // Save configuration
	WDcfg.AcquisitionMode = ACQMODE_TRG_MATCHING;
	WDcfg.TriggerMask = 0x20; // Trigger = Ptrg
	WDcfg.PtrgPeriod = 100000;  // 100 us
	WDcfg.TrgWindowWidth = 50000; // 50 us
	WDcfg.TrgWindowOffset = -50000; // -50 us
	WDcfg.ChEnableMask0[b] = (1 << nch) - 1; // enable all channels of the adapter
	WDcfg.ChEnableMask1[b] = 0;
	WDcfg.TDC_ChBufferSize = 5;
	WDcfg.En_Empty_Ev_Suppr = 0;
	WDcfg.Tref_Mask = 0;
	WDcfg.TestMode = 0;
	WDcfg.GlitchFilterMode = GLITCHFILTERMODE_DISABLED;
	WDcfg.StartRunMode = STARTRUN_ASYNC;

	FERS_InitReadout(handle, 0, &size);
	ConfigureFERS(handle, CFG_HARD);

	ClearScreen();
	Con_printf("Sa", "%02dRunning Thresholds Calibration (0 %%)", ACQSTATUS_CALIBTHR);
	Con_printf("LCSm", "Starting Thresholds Calibration (%d steps)\n\n", nstep);
	Con_printf("LCSm", "thr(mV)   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16\n");

	char thrVal[500] = "";
	for (step = 0; step < nstep; step++) {
		
		float xx = (1250 - (float)(dac_start * ((float)2500 / 4095)));				// Convert Threshold from DAC LSB to mV

		thr[step] = A5256_DAC_to_mV(dac_start - step);
		Con_printf("Sa", "%02dRunning Adapter Thresholds Calibration (%d %%)", ACQSTATUS_CALIBTHR, 100*step/(nstep));
		sprintf(thrVal, "%6.2f: ", thr[step]); // Con_printf("LCSm", "%6.2f: ", thr[step]);
		for (ch = 0; ch < nch; ch++)
			Set_DiscrThreshold(handle, ch, thr[step]);
		Sleep(10);
		memset(hit_cnt[step], 0, nch * sizeof(uint32_t));
		ntrg = 0;
		tot_hit = 0;
		niter = 0;

		FERS_StartAcquisition(&handle, 1, STARTRUN_ASYNC);
		st = get_time();
		while (ntrg < 1000) {
			nb = 0;
			ret = FERS_GetEvent(&handle, &bb, &dtq, &tstamp_us, (void**)(&Event), &nb); // Read Data from the board
			if (ret < 0)
				break;
			if (nb > 0) {
				ntrg++;
				if ((dtq & 0xF) == DTQ_TIMING) {
					for (int i = 0; i < Event->nhits; i++) {
						ChIndex_tdc2ada(Event->channel[i], &ch);
						hit_cnt[step][ch]++;
						tot_hit++;
					}
				}
			} else {  // no data; quit after 1 s
				niter++;
				if ((get_time() - st) > 1000)
					break;
			}
		}
		FERS_StopAcquisition(&handle, 1, STARTRUN_ASYNC);
		for (ch = 0; ch < nch; ch++)	// In case of console, a single line should be sent
			if (hit_cnt[step][ch] == 0) sprintf(thrVal, "%s  .", thrVal);  //  Con_printf("LCSm", "  .");
			else sprintf(thrVal, "%s  |", thrVal); // Con_printf("LCSm", "  |");
		Con_printf("LCSm", "%s\n", thrVal);//Con_printf("LCSm", "\n");
	}
	Con_printf("Sa", "%02dRunning Adapter Thresholds Calibration (%d %%)", ACQSTATUS_CALIBTHR, 100);
	// find centroids
	float centr[MAX_NCH];
	float rms[MAX_NCH];
	int ntot[MAX_NCH];
	Con_printf("LSCm", "\n");
	for (ch = 0; ch < nch; ch++) {
		centr[ch] = 0;
		ntot[ch] = 0;
		rms[ch] = 0;
		for (step = 0; step < nstep; step++) {
			centr[ch] += hit_cnt[step][ch] * thr[step];
			rms[ch] += hit_cnt[step][ch] * (thr[step] * thr[step]);
			ntot[ch] += hit_cnt[step][ch];
		}
		if (ntot[ch] > 0) {
			centr[ch] = centr[ch] / ntot[ch];
			rms[ch] = (float)sqrt(rms[ch] / ntot[ch] - centr[ch] * centr[ch]);
		}
		if (ntot[ch] > 0) {
			Con_printf("LCSm", "Ch %2d: Offset=%7.2f, RMS=%7.2f\n", ch, centr[ch], rms[ch]);
			done[ch] = 1;
		} else {
			Con_printf("LCSm", "Ch %2d: UNKNOWN\n", ch);
			done[ch] = 0;
		}
		if (ThrOffset != NULL) ThrOffset[ch] = centr[ch];
		if (RMSnoise != NULL) RMSnoise[ch] = rms[ch];
	}


	// save data to file (for debug)
	calib = fopen("ThrCalib.txt", "w");
	for (step = 0; step < nstep; step++) {
		fprintf(calib, "%6.1f : ", thr[step]);
		for (ch = 0; ch < nch; ch++) {
			fprintf(calib, "%10d ", hit_cnt[step][ch]);
		}
		fprintf(calib, "\n");
	}
	fprintf(calib, "\n\nCalibrated Offsets:\n");
	for (int ch = 0; ch < nch; ch++) {
		fprintf(calib, "%2d : %7.2f  (RMS=%.2f)\n", ch, ThrOffset[ch], rms[ch]);
	}
	fclose(calib);

	memcpy(CalibratedThrOffset[b], ThrOffset, nch * sizeof(float));
	ThrCalibLoaded[b] = 1;

	memcpy(&WDcfg, WDcfg_tmp, sizeof(Config_t)); // Restore configuration
	free(WDcfg_tmp);
	ConfigureFERS(handle, CFG_HARD);
	return 0;
}



// --------------------------------------------------------------------------------------------------------- 
// Description: Write Threshold offset calibration to flash
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
//				npts = number of values to write 
//				ThrOffset = Threshold offsets (use NULL pointer to keep old values)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int WriteThrCalibToFlash(int handle, int npts, float *ThrOffset)
{
	int ret, sz = npts * sizeof(float);
	uint8_t cal[FLASH_PAGE_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int calpage = FLASH_THR_OFFS_PAGE;

	ret = FERS_ReadFlashPage(handle, calpage, 16 + sz * 2, cal);
	if (ret < 0) return ret;
	cal[0] = 'T';	// Tag
	cal[1] = 0;		// Format
	*(uint16_t*)(cal + 2) = (uint16_t)(tm.tm_year + 1900);
	cal[4] = tm.tm_mon + 1;
	cal[5] = tm.tm_mday;
	cal[6] = npts;
	if (ThrOffset != NULL) memcpy(cal + 7, ThrOffset, sz); 
	ret = FERS_WriteFlashPage(handle, calpage, 7 + sz, cal);

	// Update local variables
	memcpy(CalibratedThrOffset, ThrOffset, sz);
	ThrCalibLoaded[FERS_INDEX(handle)] = 1;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read calibrated threshold offsets from flash
// Inputs:		handle = board handle 
//				npts = num of points to read 
// Outputs:		date = calibration date	(DD/MM/YYYY)
//				ThrOffset = calibrated offsets
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int ReadThrCalibFromFlash(int handle, int npts, char *date, float *ThrOffset)
{
	int ret, np;
	int year, month, day;
	uint8_t cal[FLASH_PAGE_SIZE];
	float calthr[MAX_NCH];
	int calpage = FLASH_THR_OFFS_PAGE;

	if (date != NULL) strcpy(date, "");
	ret = FERS_ReadFlashPage(handle, calpage, 256, cal);
	if ((cal[0] != 'T') || (cal[1] != 0)) {
		return FERSLIB_ERR_CALIB_NOT_FOUND;
	}
	year = *(uint16_t*)(cal + 2);
	month = cal[4];
	day = cal[5];
	np = cal[6];
	if (np > npts) return FERSLIB_ERR_CALIB_NOT_FOUND;
	if (date != NULL) sprintf(date, "%02d/%02d/%04d", day, month, year);
	memset(calthr, 0, npts * sizeof(float));
	memcpy(calthr, cal + 7, np * sizeof(float));
	if (ThrOffset != NULL) memcpy(ThrOffset, calthr, npts * sizeof(float));
	memcpy(CalibratedThrOffset[FERS_INDEX(handle)], calthr, npts * sizeof(float));
	ThrCalibLoaded[FERS_INDEX(handle)] = 1;
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Disable Threshold calibration
// Inputs:		handle: board handle
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int DisableThrCalib(int handle) {
	ThrCalibIsDisabled[FERS_INDEX(handle)] = 1;
	return 0;
}

// ---------------------------------------------------------------------------------
// Description: Enable Threshold calibration
// Inputs:		handle: board handle
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int EnableThrCalib(int handle) {
	ThrCalibIsDisabled[FERS_INDEX(handle)] = 0;
	return 0;
}
