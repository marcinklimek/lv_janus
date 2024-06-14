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

#ifndef _CONFIGURE_H
#define _CONFIGURE_H                    // Protect against multiple inclusion

#include "FERSlib.h"

#define CFG_HARD	0	// reset + configure (acq must be restarted)
#define CFG_SOFT	1	// runtime reconfigure params (no restart required)


//****************************************************************************
// Function prototypes
//****************************************************************************
int ConfigureFERS(int handle, int mode);
void Set_picoTDC_Default(picoTDC_Cfg_t *pcfg);
int Write_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t pcfg);
int Read_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t *pcfg);
int Save_picoTDC_Cfg(int handle, int tdc, char *fname);

#endif