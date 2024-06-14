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

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "FERSlib.h"	
#include "FERSutils.h"	
#include "adapters.h"
#include "JanusC.h"	
#include "configure.h"
#include "console.h"

// ---------------------------------------------------------------------------------
// Description: Program the registers of the FERS unit with the relevant parameters
// Inputs:		handle = board handle
//				mode: 0=hard cfg (includes reset), 1=soft cfg (no acq restart)
// Outputs:		-
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int ConfigureFERS(int handle, int mode)
{
	int i, ret = 0;  
	int numtdc = 0;
	picoTDC_Cfg_t pcfg;

	uint32_t d32;
	char CfgStep[100];
	int brd = FERS_INDEX(handle);

	int rundelay = 0;
	int deskew = -5;

	// ########################################################################################################
	sprintf(CfgStep, "General System Configuration and Acquisition Mode");
	// ########################################################################################################
	// Reset the unit 
	if (mode == CFG_HARD) {
		ret = FERS_SendCommand(handle, CMD_RESET);  // Reset
		if (ret != 0) goto abortcfg;
	}

	// Set USB or Eth communication mode
	if ((FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) || (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB))
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 20, 20, 1); // Force disable of TDlink

	// Acq Mode
	if (mode == CFG_HARD) ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 0, 7, WDcfg.AcquisitionMode);

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 11, 11, 1); // use new data qualifier format 


	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 26, 26, WDcfg.TrgIdMode); // Trigger ID: 0=trigger counter, 1=validation counter
	if (WDcfg.NumCh == 128) {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 22, 22, 1); // Enable 128 ch
	} else {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 22, 22, 0); // Enable 64 ch
	}

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 21, 21, WDcfg.En_service_event); // Enable board info: 0=disabled, 1=enabled
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 12, 13, WDcfg.En_Head_Trail); // 0=Header and trailer enabled, 1=one word trailer, 2=header and trailer suppressed
	if (WDcfg.AcquisitionMode == ACQMODE_STREAMING)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 14, 14, 1);
	else
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 14, 14, WDcfg.En_Empty_Ev_Suppr);
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 20, 20, WDcfg.Dis_tdl);

	if (WDcfg.TestMode) {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 23, 23, 1); // Enable Test Mode (create events with fixed pattern)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 0, 3, WDcfg.TestMode); // 1 = 1 hit per channel, 2 = 8 hits per channel
	}

	// Set ToT reject threshold
	if (WDcfg.MeasMode == MEASMODE_LEAD_TOT8 || WDcfg.MeasMode == MEASMODE_LEAD_TOT11) {
		ret |= FERS_WriteRegister(handle, a_tot_rej_lthr, WDcfg.ToT_reject_low_thr);
		ret |= FERS_WriteRegister(handle, a_tot_rej_hthr, WDcfg.ToT_reject_high_thr);
	}

	// Set Clock Source
	if (WDcfg.HighResClock == HRCLK_DAISY_CHAIN) {
		if (brd == 0) {
			ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 0); // TDC clock sel = internal
			ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 0); // Clock Out Sel = internal
		} else {
			ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 1); // TDC clock sel = external
			ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 1); // Clock Out Sel = external
		}
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 4, 4, 1); // Enable Clock Out
	} else if (WDcfg.HighResClock == HRCLK_FAN_OUT) {
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 1); // TDC clock sel = external
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 1); // Clock Out Sel = external
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 4, 4, 0); // Disable Clock Out
	} else {
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 0); // TDC clock sel = internal
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 0); // Clock Out Sel = internal
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 4, 4, 0); // Disable Clock Out
	}

	// Set LEMO I/O mode
	ret |= FERS_WriteRegister(handle, a_t0_out_mask, WDcfg.T0_outMask);  
	ret |= FERS_WriteRegister(handle, a_t1_out_mask, WDcfg.T1_outMask);  

	// Set Digital Probe
	ret |= FERS_WriteRegister(handle, a_dprobe, (WDcfg.DigitalProbe[1] << 8) | WDcfg.DigitalProbe[0]);

	// Set Periodic Trigger
	if ((WDcfg.AcquisitionMode == ACQMODE_STREAMING) && (FERS_FPGA_FW_MajorRev(handle) < 1)) {
		ret |= FERS_WriteRegister(handle, a_dwell_time, 1562);  // about 20 us
		//ret |= FERS_WriteRegister(handle, a_trg_mask, 0x21);	// Trigger Source as PTRG
		//ret |= FERS_WriteRegister(handle, a_tref_mask, 0x0);	// TRef Source as ZERO
	} else {
		ret |= FERS_WriteRegister(handle, a_dwell_time, (uint32_t)(WDcfg.PtrgPeriod / CLK_PERIOD));
	}

	// Set Trigger Source
	ret |= FERS_WriteRegister(handle, a_trg_mask, WDcfg.TriggerMask);
	// Set Tref Source
	ret |= FERS_WriteRegister(handle, a_tref_mask, WDcfg.Tref_Mask);

	// Set Trigger Hold Off (Retrigger Protection Time)
	if (WDcfg.AcquisitionMode != ACQMODE_STREAMING)
		ret |= FERS_WriteRegister(handle, a_trg_hold_off, WDcfg.TrgHoldOff);
	else
		ret |= FERS_WriteRegister(handle, a_trg_hold_off, 0);

	// Set Run Mask
	rundelay = (WDcfg.NumBrd - FERS_INDEX(handle) - 1) * 4;
	if (WDcfg.StartRunMode == STARTRUN_ASYNC) {
		ret |= FERS_WriteRegister(handle, a_run_mask, 0x01); 
	} else if (WDcfg.StartRunMode == STARTARUN_CHAIN_T0) {
		if (FERS_INDEX(handle) == 0) {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x01); 
			ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1 << 10);  // set T0-OUT = RUN_SYNC
		} else {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x82); 
			//ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1);  // set T0-OUT = T0-IN
			ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1 << 10);  // set T0-OUT = RUN_SYNC
		}
		//if (WDcfg.T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T0-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (WDcfg.StartRunMode == STARTRUN_CHAIN_T1) {
		if (FERS_INDEX(handle) == 0) {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x01); 
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1 << 10);  // set T1-OUT = RUN_SYNC
		} else {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x84); 
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1);  // set T1-OUT = T1-IN
		}
		//if (WDcfg.T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T1-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (WDcfg.StartRunMode == STARTRUN_TDL) {
		ret |= FERS_WriteRegister(handle, a_run_mask, 0x01); 
	}

	// Set Veto mask
	FERS_WriteRegister(handle, a_veto_mask, WDcfg.Veto_Mask);

	// Trigger buffer size and FIFO almost full levels
	if ((WDcfg.TriggerBufferSize > 0) && (WDcfg.TriggerBufferSize < 512)) {
		int af_bsy, af_skp, max_evsize;
		const int lsof_size = 16 * 1024;
		FERS_WriteRegister(handle, a_trgf_almfull, WDcfg.TriggerBufferSize);
		max_evsize = (WDcfg.TDC_ChBufferSize + 2) * (WDcfg.NumCh / 16); // One Ch Buffer takes data of 16 channels (+2 is for header and trailer)
		af_bsy = lsof_size - (2 * max_evsize + 100);  // keep space for at least 2 events; +100 is for extra margin. In case of overflow (more than 2 big events...), hits will be discarded by the FPGA when the second almost full (af_skp) is reached
		FERS_WriteRegister(handle, a_lsof_almfull_bsy, af_bsy);
		af_skp = lsof_size - (WDcfg.TriggerBufferSize * 2 * (WDcfg.NumCh / 16) + 10);  // keep space at least for header+trailers (one per group of 16 channels) of all pending triggers; +10 is for extra margin. 
		FERS_WriteRegister(handle, a_lsof_almfull_skp, af_skp);
	}
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_CNC) {
		if (WDcfg.MaxPck_Train > 0) FERS_WriteRegister(handle, a_pck_maxcnt, WDcfg.MaxPck_Train);
		//if (WDcfg.MaxPck_Block > 0) LLtdl_CncWriteRegister(handle, ???, WDcfg.MaxPck_Block);  // CTIN: wait for reg mapping
		//if (WDcfg.CncBufferSize > 0) LLtdl_CncWriteRegister(handle, ???, WDcfg.CncBufferSize);  // CTIN: wait for reg mapping
	}
	
	if (ret) goto abortcfg;
	
	numtdc = (WDcfg.NumCh == 64) ? 1 : 2;
	int tdc;
	for (tdc = 0; tdc < numtdc; tdc++) {

		// ########################################################################################################
		sprintf(CfgStep, "picoTDC power-up");
		// ########################################################################################################
		if (mode == CFG_HARD) {
			ret = 0;

			// Write default cfg
			Set_picoTDC_Default(&pcfg);
			Write_picoTDC_Cfg(handle, tdc, pcfg);

			// Initialize PLL with AFC
			pcfg.PLL2.bits.pll_afcrst = 1;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);
			pcfg.PLL2.bits.pll_afcrst = 0;
			pcfg.PLL2.bits.pll_afcstart = 1;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);
			Sleep(10); // Wait for lock 
			pcfg.PLL2.bits.pll_afcstart = 0;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);

			// Initialize DLL
			pcfg.DLL_TG.bits.dll_fixctrl = 1;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);
			Sleep(10); // Wait for lock 
			pcfg.DLL_TG.bits.dll_fixctrl = 0;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);

			// Turn everything ON
			pcfg.Control.bits.magic_word = 0x5C;
			pcfg.Control.bits.digital_reset = 1;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, pcfg.Control.reg);
			pcfg.Control.bits.digital_reset = 0;
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, pcfg.Control.reg);

		} else {
			Read_picoTDC_Cfg(handle, tdc, &pcfg);
		}
		if (ret < 0) goto abortcfg;

		// ########################################################################################################
		sprintf(CfgStep, "picoTDC setting");
		// ########################################################################################################
		// Header 
		pcfg.Header.bits.relative = 1;			// Time stamp relative to trigger window
		pcfg.Header.bits.header_fields0 = WDcfg.HeaderField0;	// Header field0 = 4 = near full flags (ch 0..7 of the port) - 00000CCCCCCCC
		pcfg.Header.bits.header_fields1 = WDcfg.HeaderField1;	// Header field1 = 5 = near full flags (ch 8..15 of the port + RO_buff, TrgBuff) - RT000CCCCCCCC
		pcfg.Header.bits.untriggered = 0;

		// Acquisition Mode
		double trgwin;
		if (WDcfg.AcquisitionMode == ACQMODE_COMMON_START || WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP) {
			if (WDcfg.GateWidth < 50000-7*TDC_CLK_PERIOD) {	// the max TrgWindow is 50 us: GATE + extra clock (5+2)
				trgwin = (WDcfg.GateWidth / TDC_CLK_PERIOD) + 2;
			} else {
				trgwin = ((50000 / TDC_CLK_PERIOD)-7 + 2);
				WDcfg.GateWidth = (uint32_t)((50000 / TDC_CLK_PERIOD) - 7);			}
		} else {
			if (WDcfg.TrgWindowWidth < 50000) {	//the max TrgWindow is 50 us
				trgwin = WDcfg.TrgWindowWidth / TDC_CLK_PERIOD;
			} else {
				trgwin = 50000 / TDC_CLK_PERIOD;
				WDcfg.TrgWindowWidth = 50000;
			}
		}

		pcfg.TrgWindow.bits.trigger_window = (uint32_t)trgwin;
		if (WDcfg.AcquisitionMode == ACQMODE_COMMON_START) {
			ret |= FERS_WriteRegister(handle, a_trg_delay, (uint32_t)(2*trgwin + 2));
			pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(trgwin + 5);
		} else if (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP) {
			ret |= FERS_WriteRegister(handle, a_trg_delay, 0);
			pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(trgwin + 2);
		} else if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING) {
			if ((WDcfg.TrgWindowOffset + WDcfg.TrgWindowWidth) > 0) {  // trg window ends after the trigger => need to delay the trigger (in the FPGA) and shift it to the end of the window
				ret |= FERS_WriteRegister(handle, a_trg_delay, (uint32_t)(2*(WDcfg.TrgWindowOffset + WDcfg.TrgWindowWidth) / TDC_CLK_PERIOD) + 1);
				pcfg.TrgWindow.bits.trigger_latency = (uint32_t)trgwin;
			} else {
				ret |= FERS_WriteRegister(handle, a_trg_delay, 0);
				pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(-WDcfg.TrgWindowOffset / TDC_CLK_PERIOD);
			}
		} else if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
			pcfg.TrgWindow.bits.trigger_window = 1562 / 2 - 1;
			pcfg.TrgWindow.bits.trigger_latency = 1562 / 2 + 1;
		}

		// TDC pulser
		if (WDcfg.TDCpulser_Period > 0) {
			pcfg.PulseGen1.bits.pg_run = 1;
			pcfg.PulseGen1.bits.pg_en = 1;
			pcfg.PulseGen1.bits.pg_strength = 3;
			pcfg.PulseGen1.bits.pg_direct = 0;
			pcfg.PulseGen1.bits.pg_rep = 1;
			pcfg.PulseGen1.bits.pg_rising = 1;
			pcfg.PulseGen2.bits.pg_phase = 3;
			pcfg.PulseGen2.bits.pg_falling = (uint32_t)(WDcfg.TDCpulser_Width / TDC_PULSER_CLK_PERIOD);
			pcfg.PulseGen3.bits.pg_reload = (uint32_t)(WDcfg.TDCpulser_Period / TDC_PULSER_CLK_PERIOD);
			FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1, pcfg.PulseGen1.reg);
		}

		// Hit RX filters (Glitch Filter)
		if (WDcfg.GlitchFilterMode != GLITCHFILTERMODE_DISABLED) {
			pcfg.Hit_RX_TX.bits.hrx_top_delay = WDcfg.GlitchFilterDelay & 0xF;
			pcfg.Hit_RX_TX.bits.hrx_bot_delay = WDcfg.GlitchFilterDelay & 0xF;
			pcfg.Hit_RX_TX.bits.hrx_top_filter_leading = (WDcfg.GlitchFilterMode >> 1) & 1;
			pcfg.Hit_RX_TX.bits.hrx_top_filter_trailing = WDcfg.GlitchFilterMode & 1;
			pcfg.Hit_RX_TX.bits.hrx_bot_filter_leading = (WDcfg.GlitchFilterMode >> 1) & 1;
			pcfg.Hit_RX_TX.bits.hrx_bot_filter_trailing = WDcfg.GlitchFilterMode & 1;
		}

		//pcfg.Buffers.bits.channel_buffer_size = 2;

		// Set Measurement mode (Enable ToT or leading edge)
		if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
			pcfg.Enable.bits.falling_en = 1;
			for (i = 0; i < PICOTDC_NCH; i++) {
				pcfg.Ch_Control[i].bits.falling_en_tm = 1;
			}
			if (MEASMODE_OWLT(WDcfg.MeasMode)) {
				pcfg.Trg0Del_ToT.bits.tot = 1;
				pcfg.Trg0Del_ToT.bits.tot_8bit = (WDcfg.MeasMode == MEASMODE_LEAD_TOT8) ? 1 : 0;
				pcfg.Trg0Del_ToT.bits.tot_saturate = 1;  //TOT will saturate if value bigger than dynamic range (otherwise overflow occurs)
				pcfg.Trg0Del_ToT.bits.tot_startbit = WDcfg.ToT_LSB;
				pcfg.Trg0Del_ToT.bits.tot_leadingstartbit = WDcfg.LeadTrail_LSB;
			}
		} else {
			pcfg.Enable.bits.falling_en = 0;
			for (i = 0; i < PICOTDC_NCH; i++) {
				pcfg.Ch_Control[i].bits.falling_en_tm = 0;
			}
		}

		// Channel offset 
		if (WDcfg.AdapterType == ADAPTER_NONE) {
			for (i = 0; i < PICOTDC_NCH; i++) 
				pcfg.Ch_Control[i].bits.channel_offset = WDcfg.Ch_Offset[brd][i];
		} else if ((WDcfg.AdapterType == ADAPTER_A5256_REV0_POS) || (WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG)) {
			for (i = 0; i < 17; i++) {
				int ch;
				ChIndex_ada2tdc(i, &ch);
				pcfg.Ch_Control[ch].bits.channel_offset = WDcfg.Ch_Offset[brd][i];
			}
		}

		// Channel RX Enable Mask
		uint32_t ChMask0, ChMask1, ChMask2, ChMask3;
		if (WDcfg.AdapterType == ADAPTER_NONE) {
			ChMask0 = WDcfg.ChEnableMask0[brd];
			ChMask1 = WDcfg.ChEnableMask1[brd];
			ChMask2 = WDcfg.ChEnableMask2[brd];
			ChMask3 = WDcfg.ChEnableMask3[brd];
		} else {
			ChMask_ada2tdc(WDcfg.ChEnableMask0[brd], &ChMask0, &ChMask1);
			ChMask2 = 0;
			ChMask3 = 0;
		}
		if (tdc == 0) {
			pcfg.Hit_RXen_T.bits.hrx_enable_t = ChMask0; //for TDC0
			pcfg.Hit_RXen_B.bits.hrx_enable_b = ChMask1; //for TDC0
			for (i = 0; i < PICOTDC_NCH; i++) {
				uint32_t mask = (i < 32) ? ChMask0 : ChMask1;
				pcfg.Ch_Control[i].bits.channel_enable = (mask >> (i % 32)) & 1;
			}
		} else if (tdc == 1) {
			pcfg.Hit_RXen_T.bits.hrx_enable_t = ChMask2; //for TDC1
			pcfg.Hit_RXen_B.bits.hrx_enable_b = ChMask3; //for TDC1
			for (i = 0; i < PICOTDC_NCH; i++) {
				uint32_t mask = (i < 32) ? ChMask2 : ChMask3;
				pcfg.Ch_Control[i].bits.channel_enable = (mask >> (i % 32)) & 1;
			}
		} 

		// Write final configuration
		ret = Write_picoTDC_Cfg(handle, tdc, pcfg);
		if (ret) goto abortcfg;
	}


	// ########################################################################################################
	sprintf(CfgStep, "Input Adapter");
	// ########################################################################################################
	if (WDcfg.AdapterType != ADAPTER_NONE) {
		int nch = AdapterNch();
		if (WDcfg.DisableThresholdCalib[brd])
			DisableThrCalib(handle);
		for (i = 0; i < nch; i++) {
			ret |= Set_DiscrThreshold(handle, i, WDcfg.DiscrThreshold[brd][i]);
		}
		if ((WDcfg.AdapterType == ADAPTER_A5256_REV0_NEG) || (WDcfg.AdapterType == ADAPTER_A5256))
			FERS_WriteRegisterSlice(handle, a_io_ctrl, 0, 1, 3); // invert polarity of the edge_conn trigger
	}
	if (ret) goto abortcfg;


	// ########################################################################################################
	sprintf(CfgStep, "Generic Write accesses with mask");
	// ########################################################################################################
	for(i=0; i<WDcfg.GWn; i++) {
		if (((int)WDcfg.GWbrd[i] < 0) || ((int)WDcfg.GWbrd[i] == brd)) {
			ret |= FERS_ReadRegister(handle, WDcfg.GWaddr[i], &d32);
			d32 = (d32 & ~WDcfg.GWmask[i]) | (WDcfg.GWdata[i] & WDcfg.GWmask[i]);
			ret |= FERS_WriteRegister(handle, WDcfg.GWaddr[i], d32);
		}
	}
	if (ret) goto abortcfg;

	// Save reg image to output file
	if (WDcfg.DebugLogMask & 0x10000) 
		Save_picoTDC_Cfg(handle, 0, "picoTDC_RegImg.txt");
	Sleep(10);
	return 0;

	abortcfg:
	sprintf(ErrorMsg, "Error at: %s. Exit Code = %d\n", CfgStep, ret);
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Set default value of picoTDC register (don't write in TDC, just in the struct)
// Outpus:		pfcg: struct with picoTDC regs
// --------------------------------------------------------------------------------------------------------- 
void Set_picoTDC_Default(picoTDC_Cfg_t *pcfg) {
	memset (pcfg, 0, sizeof(picoTDC_Cfg_t));  // set all parames to 0 

	pcfg->Enable.bits.highres_en = 1;
	pcfg->Enable.bits.crossing_en = 1;

	// Set High Res mode
	pcfg->Enable.bits.rx_en_trigger = 1;
	for (int i = 0; i < PICOTDC_NCH; i++) 
		pcfg->Ch_Control[i].bits.highres_en_tm = 1;
	pcfg->DLL_TG.bits.tg_bot_nen_fine = 0;
	pcfg->DLL_TG.bits.tg_bot_nen_coarse = 0;
	pcfg->DLL_TG.bits.tg_top_nen_fine = 0;
	pcfg->DLL_TG.bits.tg_top_nen_coarse = 0;

	pcfg->BunchCount.bits.bunchcount_overflow = 0x1FFF;
	pcfg->EventID.bits.eventid_overflow = 0x1FFF;

	pcfg->Buffers.bits.channel_buffer_size = WDcfg.TDC_ChBufferSize & 0x7;
	pcfg->Buffers.bits.readout_buffer_size = 0x7;
	pcfg->Buffers.bits.trigger_buffer_size = 0x7;
	pcfg->Buffers.bits.disable_ro_reject = 1;
	pcfg->Buffers.bits.errorcnts_saturate = 1;

	pcfg->Hit_RX_TX.bits.hrx_bot_bias = 0xF;
	pcfg->Hit_RX_TX.bits.hrx_top_bias = 0xF;
	pcfg->Hit_RX_TX.bits.hrx_top_en   = 1;
	pcfg->Hit_RX_TX.bits.hrx_top_en_r = 1;
	pcfg->Hit_RX_TX.bits.hrx_bot_en   = 1;
	pcfg->Hit_RX_TX.bits.hrx_bot_en_r = 1;
	pcfg->Hit_RX_TX.bits.tx_strenght = 0x3;

	pcfg->DLL_TG.bits.dll_bias_val = 2;

	pcfg->PLL1.bits.pll_cp_dacset = 0x53;
	pcfg->PLL1.bits.pll_resistor = 0x1E;

	pcfg->PLL2.bits.pll_afcvcal = 0x6;

	pcfg->Clocks.bits.hit_phase = 1;  // must be 1
	pcfg->Clocks.bits.par_speed = 2;  // 00=320, 01=160, 10=80, 11=40
	pcfg->Clocks.bits.par_phase = 4;  // 01=@rising edge
	pcfg->Clocks.bits.sync_clock = 1; // Sync Out; 0=byte sync, 1=half freq clock

	pcfg->ClockShift.bits.shift_clk1G28 = 2;

	pcfg->Enable.bits.falling_en = 0;
	for (int i = 0; i < PICOTDC_NCH; i++) {
		pcfg->Ch_Control[i].bits.falling_en_tm = 0;
	}


	
}

// --------------------------------------------------------------------------------------------------------- 
// Description: write cfg struct with regs into picoTDC
// Inputs:		handle = device handle
//				tdc: tdc index (0 or 1)
//				pcfg: struct with picoTDC regs
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int Write_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t pcfg) {
	int ret = 0, i;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control,		pcfg.Control.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Enable,			pcfg.Enable.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Header,			pcfg.Header.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_TrgWindow,		pcfg.TrgWindow.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg0Del_ToT,	pcfg.Trg0Del_ToT.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_BunchCount,		pcfg.BunchCount.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_EventID,		pcfg.EventID.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Buffers,		pcfg.Buffers.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RX_TX,		pcfg.Hit_RX_TX.reg);  
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG,			pcfg.DLL_TG.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL1,			pcfg.PLL1.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2,			pcfg.PLL2.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Clocks,			pcfg.Clocks.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ClockShift,		pcfg.ClockShift.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_T,		pcfg.Hit_RXen_T.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_B,		pcfg.Hit_RXen_B.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1,		pcfg.PulseGen1.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen2,		pcfg.PulseGen2.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen3,		pcfg.PulseGen3.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ErrorFlagCtrl,	pcfg.ErrorFlagCtrl.reg);
	for(i=0; i< PICOTDC_NCH; i++)
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Control(i),	pcfg.Ch_Control[i].reg);
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: read picoTDC regs and update the cfg struct
// Inputs:		handle = device handle
//				tdc: tdc index (0 or 1)
// Outputs		pcfg: struct with picoTDC regs
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int Read_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t *pcfg) {
	int ret = 0, i;
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control,			&pcfg->Control.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Enable,			&pcfg->Enable.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Header,			&pcfg->Header.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_TrgWindow,		&pcfg->TrgWindow.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg0Del_ToT,		&pcfg->Trg0Del_ToT.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_BunchCount,		&pcfg->BunchCount.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_EventID,			&pcfg->EventID.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Buffers,			&pcfg->Buffers.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RX_TX,		&pcfg->Hit_RX_TX.reg);  
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG,			&pcfg->DLL_TG.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL1,			&pcfg->PLL1.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2,			&pcfg->PLL2.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Clocks,			&pcfg->Clocks.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ClockShift,		&pcfg->ClockShift.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_T,		&pcfg->Hit_RXen_T.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_B,		&pcfg->Hit_RXen_B.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1,		&pcfg->PulseGen1.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen2,		&pcfg->PulseGen2.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen3,		&pcfg->PulseGen3.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ErrorFlagCtrl,	&pcfg->ErrorFlagCtrl.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL_Caps,		&pcfg->PLL_Caps.reg);
	for(i=0; i< PICOTDC_NCH; i++) {
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Control(i),	&pcfg->Ch_Control[i].reg);
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Status(i),	&pcfg->Ch_Status[i].reg);
	}
	for(i=0; i<4; i++) {
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg_Status(i),	&pcfg->Trg_Status[i].reg);
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_RO_Status(i),	&pcfg->RO_Status[i].reg);
	}
	for(i=0; i<3; i++) 
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Cfg_Parity(i),	&pcfg->Cfg_Parity[i].reg);
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Write picoTDC Reg Image into file
// Inputs:		handle = board handle
//				tdc: tdc index (0 or 1) 
//				fname = output file name
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int Save_picoTDC_Cfg(int handle, int tdc, char *fname) {
	picoTDC_Cfg_t pcfg;
	int i, ret;
	FILE *img;

	img = fopen(fname, "w");
	if (img == NULL) return -1;
	ret = Read_picoTDC_Cfg(handle, 0, &pcfg);
	if (ret < 0) return ret;
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Control,		pcfg.Control.reg,		"Control");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Enable,		pcfg.Enable.reg,		"Enable");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Header,		pcfg.Header.reg,		"Header");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_TrgWindow,	pcfg.TrgWindow.reg,		"TrgWindow");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Trg0Del_ToT,	pcfg.Trg0Del_ToT.reg,	"Trg0Del_ToT");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_BunchCount,	pcfg.BunchCount.reg,	"BunchCount");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_EventID,		pcfg.EventID.reg,		"EventID");
	for(i=0; i<PICOTDC_NCH; i++) 
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Ch_Control(i), pcfg.Ch_Control[i].reg, "Ch_Control", i);
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Buffers,		pcfg.Buffers.reg,		"Buffers");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RX_TX,	pcfg.Hit_RX_TX.reg,		"Hit_RX_TX");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_DLL_TG,		pcfg.DLL_TG.reg,		"DLL_TG");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL1,			pcfg.PLL1.reg,			"PLL1");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL2,			pcfg.PLL2.reg,			"PLL2");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Clocks,		pcfg.Clocks.reg,		"Clocks");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_ClockShift,	pcfg.ClockShift.reg,	"ClockShift");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RXen_T,	pcfg.Hit_RXen_T.reg,	"Hit_RXen_L");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RXen_B,	pcfg.Hit_RXen_B.reg,	"Hit_RXen_H");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen1,	pcfg.PulseGen1.reg,		"PulseGen1");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen2,	pcfg.PulseGen2.reg,		"PulseGen2");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen3	,	pcfg.PulseGen3.reg,		"PulseGen3");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_ErrorFlagCtrl,pcfg.ErrorFlagCtrl.reg,	"ErrorFlagCtrl");
	for(i=0; i<PICOTDC_NCH; i++) 
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Ch_Status(i),		pcfg.Ch_Status[i].reg, "Ch_Status", i);
	for(i=0; i<4; i++) 
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Trg_Status(i),	pcfg.Trg_Status[i].reg, "Trg_Status", i);
	for(i=0; i<4; i++) 
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_RO_Status(i),		pcfg.RO_Status[i].reg, "RO_Status", i);
	for(i=0; i<3; i++) 
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Cfg_Parity(i),	pcfg.Cfg_Parity[i].reg, "CfgParity", i);
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL_Caps,		pcfg.PLL_Caps.reg,	"PLL_Caps");
	fclose(img);
	return 0;
}


