/******************************************************************************
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

#ifndef _ADAPTERS_H
#define _ADAPTERS_H				// Protect against multiple inclusion

// Input Adapters
#define ADAPTER_NONE					0x0000	// No adapter
#define ADAPTER_A5255					0x0000	// A5255 is just a mechanical adapter (same as NONE)
#define ADAPTER_A5256					0x0002	// 16+1 ch discriminator with programmable thresholds
#define ADAPTER_A5256_REV0_POS			0x0100	// Proto0 of A5256 used with positive signals (discontinued)
#define ADAPTER_A5256_REV0_NEG			0x0101	// Proto0 of A5256 used with negative signals (discontinued)


#define NC -1

#define A5256_DAC_LSB				((float)2500/4095) 									// LSB of the threshold in mV
#define A5256_mV_to_DAC(mV)			(uint32_t)round((1250 - (mV)) / A5256_DAC_LSB)		// Convert Threshold from mV into DAC LSB
#define A5256_DAC_to_mV(lsb)		(1250 - (float)((lsb) * A5256_DAC_LSB))				// Convert Threshold from DAC LSB to mV

//										0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 
const int A5256_ADA2DAC[17]			= {	1,  0,  1,  3,  2,  9,  8, 11, 10,  6,  7,  5,  4, 14, 15, 13, 12 };
const int A5256_ADA2TDC[17]			= {	1,  2,  4, 20, 22, 38, 40, 58, 62,  3,  5, 23, 25, 39, 41, 59, 61 };
const int A5256_TDC2ADA[64]			= {  0,  0,  1,  9,  2, 10, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC,  3, NC,  4, 11,
										NC, 12, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC,  5, 13,
										 6, 14, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC,  7, 15, NC, 16,  8, NC };


//                                    0   1   2   3   4   5   6   7   8 
const int A5256_REV0_ADA2DAC_POS[9] =	{	0,  1,  9,  6, 14, 16, 25, 22, 30}; 
const int A5256_REV0_ADA2TDC_POS[9] =	{	0,  4,  20, 38, 58, 5, 23, 39, 59};
const int A5256_REV0_TDC2ADA_POS[64] =	{	0,  NC, NC, NC,  1,  5, NC, NC, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC,  2, NC, NC,  6, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC, NC, NC,  3,  7, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										NC, NC,  4,  8, NC, NC, NC, NC };

//                                    0   1   2   3   4   5   6   7   8 
const int A5256_REV0_ADA2DAC_NEG[9] =	{	0,  0,  8,  7, 15, 17, 24, 23, 31}; 
const int A5256_REV0_ADA2TDC_NEG[9] =	{	1,  2, 16, 32, 52, 3,  17, 33, 55};
const int A5256_REV0_TDC2ADA_NEG[64] =	{	NC,  0,  1,  5, NC, NC, NC, NC, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										 2,  6, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										 3,  7, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC, NC, NC, NC, NC, 
										NC, NC, NC, NC,  4, NC, NC,  8, 
										NC, NC, NC, NC, NC, NC, NC, NC };

int AdapterNch();
int ChIndex_tdc2ada(int TDC_ch, int *Adapter_ch);
int ChIndex_ada2tdc(int Adapter_ch, int *TDC_ch);
int ChMask_ada2tdc(uint32_t AdapterMask, uint32_t *ChMask0, uint32_t *ChMask1);
int Set_DiscrThreshold(int handle, int Adapter_ch, float thr_mv);
int CalibThresholdOffset(int handle, float min_thr, float max_thr, int *done, float* ThrOffset, float* RMSnoise);
int WriteThrCalibToFlash(int handle, int npts, float* ThrOffset);
int ReadThrCalibFromFlash(int handle, int npts, char* date, float* ThrOffset);
int DisableThrCalib(int handle);
int EnableThrCalib(int handle);

#endif