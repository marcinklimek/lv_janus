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

#ifndef _REGISTERS_H
#define _REGISTERS_H				// Protect against multiple inclusion


// *****************************************************************
// Individual Channel and Broadcast address converter
// *****************************************************************
#define INDIV_ADDR(addr, ch)		(0x02000000 | ((addr) & 0xFFFF) | ((ch)<<16))
#define BCAST_ADDR(addr)			(0x03000000 | ((addr) & 0xFFFF))

// *****************************************************************
// FPGA Register Address Map
// *****************************************************************
#define a_acq_ctrl         0x01000000   //  Acquisition Control Register
#define a_run_mask         0x01000004   //  Run mask: bit[0]=SW, bit[1]=T0-IN
#define a_trg_mask         0x01000008   //  Global trigger mask: bit[0]=SW, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=Maj, bit[4]=Periodic
#define a_tref_mask        0x0100000C   //  Tref mask: bit[0]=T0-IN, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=T-OR, bit[5]=Periodic
#define a_t0_out_mask      0x01000014   //  T0 out mask
#define a_t1_out_mask      0x01000018   //  T1 out mask
#define a_io_ctrl          0x01000020   //  I/O control
#define a_veto_mask        0x0100001C   //  Veto mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN, 
#define a_tref_delay       0x01000048   //  Delay of the time reference for the coincidences
#define a_tref_window      0x0100004C   //  Tref coincidence window (for list mode)
#define a_dwell_time       0x01000050   //  Dwell time (periodic trigger) in clk cyclces. 0 => OFF
#define a_list_size        0x01000054   //  Number of 32 bit words in a packet in timing mode (list mode)
#define a_trg_delay        0x01000060   //  Trigger delay
#define a_pck_maxcnt       0x01000064   //  Max num of packets transmitted to the TDL (in one train)
#define a_lsof_almfull_bsy 0x01000068	//  Almost Full level of LSOF (Event Data FIFO) to assert Busy
#define a_lsof_almfull_skp 0x0100006C   //  Almost Full level of LSOF (Event Data FIFO) to skip data
#define a_trgf_almfull     0x01000070   //  Almost Full level of Trigger FIFO
#define a_tot_rej_lthr	   0x01000074	//  ToT Reject Lower Threshold (0=disabled)
#define a_tot_rej_hthr	   0x01000078	//  ToT Reject Higer Threshold (0=disabled)
#define a_trg_hold_off     0x0100007A   //	Retrigger protection time. Set the busy active for N clock cycles (0=disabled)
#define a_channel_mask_0   0x01000100   //  Input Channel Mask (ch  0 to 31)
#define a_channel_mask_1   0x01000104   //  Input Channel Mask (ch 32 to 63)
#define a_dprobe		   0x01000110   //  Digital probe selection	
#define a_i2c_addr         0x01000200   //  I2C Addr
#define a_i2c_data         0x01000204   //  I2C Data
#define a_spi_data         0x01000224   //  SPI R/W data (for Flash Memory access)
#define a_test_led         0x01000228   //  Test Mode for LEDs
#define a_fw_rev           0x01000300   //  Firmware Revision 
#define a_acq_status       0x01000304   //  Acquisition Status
#define a_real_time        0x01000308   //  Real Time in ms
#define a_dead_time        0x01000310   //  Dead Time in ms
#define a_trg_cnt          0x01000318   //  Trigger counter
#define a_rej_trg_cnt      0x0100031C   //  Rejected Trigger counter
#define a_zs_trg_cnt       0x01000320   //  Zero Suppressed Trigger counter
#define a_fpga_temp        0x01000348   //  FPGA die Temperature 
#define a_board_temp       0x01000350	//  Board Temp
#define a_tdc0_temp		   0x01000354	//  TDC0 Temperature
#define a_tdc1_temp		   0x01000358	//  TDC1 Temperature
#define a_pid              0x01000400	//  PID
#define a_pcb_rev          0x01000404	//  PCB revision
#define a_fers_code        0x01000408	//  Fers CODE (5202)
#define a_rebootfpga       0x0100FFF0	//  reboot FPGA from FW uploader
#define a_test_led         0x01000228   //  LED test mode
#define a_tdc_mode         0x0100022C   //  TDC Mode
#define a_tdc_data         0x01000230   //  TDC Data

#define a_commands         0x01008000   //  Send Commands (for Eth and USB)

// *****************************************************************
// FPGA Register Bit Fields 
// *****************************************************************
// Status Register 
#define STATUS_READY				(1 <<  0)
#define STATUS_FAIL					(1 <<  1)
#define STATUS_RUNNING				(1 <<  2)
#define STATUS_TDL_SYNC				(1 <<  3)
#define STATUS_FPGA_OVERTEMP		(1 <<  4)
#define STATUS_TDC_RO_ERR			(1 <<  5)
#define STATUS_TDLINK_LOL			(1 <<  6)
#define STATUS_TDC0_LOL				(1 <<  7)
#define STATUS_TDC1_LOL				(1 <<  8)
#define STATUS_RO_CLK_LOL			(1 <<  9)
#define STATUS_TDL_DISABLED			(1 << 10)
#define STATUS_TDC0_OVERTEMP		(1 << 11)
#define STATUS_TDC1_OVERTEMP		(1 << 12)
#define STATUS_BOARD_OVERTEMP		(1 << 13)
#define STATUS_UNUSED_14			(1 << 14)
#define STATUS_UNUSED_15			(1 << 15)
#define STATUS_SPI_BUSY				(1 << 16)
#define STATUS_I2C_BUSY				(1 << 17)
#define STATUS_I2C_FAIL				(1 << 18)



// *****************************************************************
// Commands
// *****************************************************************
#define CMD_TIME_RESET     0x11  // Absolute Time reset
#define CMD_ACQ_START      0x12  // Start acquisition
#define CMD_ACQ_STOP       0x13  // Stop acquisition
#define CMD_TRG            0x14  // Send software trigger
#define CMD_RESET          0x15  // Global Reset (clear data, set all regs to default)
#define CMD_TPULSE         0x16  // Send a test pulse
#define CMD_RES_PTRG       0x17  // Reset periodic trigger counter (and rearm PTRG in single pulse mode)
#define CMD_CLEAR	       0x18  // Clear Data
#define CMD_SET_VETO	   0x1A  // Set Veto
#define CMD_CLEAR_VETO	   0x1B  // Clear Veto
#define CMD_TDL_SYNC	   0x1C  // Sync signal from TDL


// *****************************************************************
// picoTDC registers
// *****************************************************************
#define a_pTDC_Control			0x0004				// R/W - Magic word + reset bits
#define a_pTDC_Enable 			0x0008				// R/W - Enable bits
#define a_pTDC_Header			0x000C				// R/W - Event data format (header)
#define a_pTDC_TrgWindow		0x0010				// R/W - Trg Window Latency + Width
#define a_pTDC_Trg0Del_ToT		0x0014				// R/W - Trg_Ch0 Delay + ToT
#define a_pTDC_BunchCount		0x0018				// R/W - Bunch Count Overflow + Offset
#define a_pTDC_EventID			0x001C				// R/W - EventID 
#define a_pTDC_Ch_Control(i)	(0x0020 + (i)*4)	// R/W - Channel offset + some Ctrl bits
#define a_pTDC_Buffers			0x0120				// R/W - Buffer settings
#define a_pTDC_Hit_RX_TX		0x0124				// R/W - Hit RX/TX
#define a_pTDC_DLL_TG			0x0128				// R/W - DLL/TG
#define a_pTDC_PLL1				0x0130				// R/W - PLL1
#define a_pTDC_PLL2				0x0134				// R/W - PLL2
#define a_pTDC_Clocks			0x0138				// R/W - 
#define a_pTDC_ClockShift		0x013C				// R/W - 
#define a_pTDC_Hit_RXen_T		0x0140				// R/W - 
#define a_pTDC_Hit_RXen_B		0x0144				// R/W - 
#define a_pTDC_PulseGen1		0x0148				// R/W - 
#define a_pTDC_PulseGen2		0x014C				// R/W - 
#define a_pTDC_PulseGen3		0x0150				// R/W - 
#define a_pTDC_ErrorFlagCtrl	0x0154				// R/W - 
#define a_pTDC_Ch_Status(i)		(0x0160 + (i)*4)	// R   - 
#define a_pTDC_Trg_Status(i)	(0x0260 + (i)*4)	// R   - 
#define a_pTDC_RO_Status(i)		(0x0270 + (i)*4)	// R   - 
#define a_pTDC_Cfg_Parity(i)	(0x0280 + (i)*4)	// R   - 
#define a_pTDC_PLL_Caps			0x28C				// R   - 
#define a_pTDC_DelayAdjust		0xFFFC				// W   - 


typedef struct {
	// ------------------------------------------------ Control
	union {
		struct {
			uint32_t magic_word              :  8; 
			uint32_t digital_reset           :  1;
			uint32_t bunchcount_reset        :  1;
			uint32_t eventid_reset           :  1;
			uint32_t dropcnts_reset          :  1;
			uint32_t errorcnts_reset         :  1;
			uint32_t trigger_create          :  1;
			uint32_t trigger_create_single   :  1;
			uint32_t reserved1               : 17;
		} bits;
		uint32_t reg;
	} Control;
	// ------------------------------------------------ Enable
	union {
		struct {
			uint32_t reserved1               :  1;
			uint32_t falling_en              :  1;
			uint32_t single_readout_en       :  1;
			uint32_t reserved2               :  2;
			uint32_t highres_en              :  1;
			uint32_t dig_rst_ext_en          :  1;
			uint32_t bx_rst_ext_en           :  1;
			uint32_t eid_rst_ext_en          :  1;
			uint32_t reserved3               :  8;
			uint32_t crossing_en             :  1;
			uint32_t rx_en_extref            :  1;
			uint32_t rx_en_bxrst             :  1;
			uint32_t rx_en_eidrst            :  1;
			uint32_t rx_en_trigger           :  1;
			uint32_t rx_en_reset             :  1;
			uint32_t left_suppress           :  1;
			uint32_t channel_split2          :  1;
			uint32_t channel_split4          :  1;
			uint32_t reserved4               :  6;
		} bits;
		uint32_t reg;
	} Enable;
	// ------------------------------------------------ Header
	union {
		struct {
			uint32_t untriggered             :  1;
			uint32_t reserved1               :  1;
			uint32_t relative                :  1;
			uint32_t second_header           :  1;
			uint32_t full_events             :  1;
			uint32_t trigger_ch0_en          :  1;
			uint32_t reserved2               :  2;
			uint32_t header_fields0          :  3;
			uint32_t reserved3               :  1;
			uint32_t header_fields1          :  3;
			uint32_t reserved4               :  1;
			uint32_t header_fields2          :  3;
			uint32_t reserved5               :  1;
			uint32_t header_fields3          :  3;
			uint32_t reserved6               :  1;
			uint32_t max_eventsize           :  8;
		} bits;
		uint32_t reg;
	} Header;
	// ------------------------------------------------ TrgWindow
	union {
		struct {
			uint32_t trigger_latency         : 13;
			uint32_t reserved1               :  3;
			uint32_t trigger_window          : 13;
			uint32_t reserved2               :  3;
		} bits;
		uint32_t reg;
	} TrgWindow;

	// ------------------------------------------------ Trg0Del_ToT
	union {
		struct {
			uint32_t trg_ch0_delay           : 13;
			uint32_t reserved1               :  3;
			uint32_t tot                     :  1;
			uint32_t tot_8bit                :  1;
			uint32_t tot_startbit            :  5;
			uint32_t tot_saturate            :  1;
			uint32_t tot_leadingstartbit     :  4;
			uint32_t reserved2               :  4;
		} bits;
		uint32_t reg;
	} Trg0Del_ToT;
	// ------------------------------------------------ BunchCount
	union {
		struct {
			uint32_t bunchcount_overflow     : 13;
			uint32_t reserved1               :  3;
			uint32_t bunchcount_offset       : 13;
			uint32_t reserved2               :  3;
		} bits;
		uint32_t reg;
	} BunchCount;
	// ------------------------------------------------ EventID
	union {
		struct {
			uint32_t eventid_overflow        : 13;
			uint32_t reserved1               :  3;
			uint32_t eventid_offset          : 13;
			uint32_t reserved2               :  3;
		} bits;
		uint32_t reg;
	} EventID;
	// ------------------------------------------------ Ch_Control[64]
	union {
		struct {
			uint32_t channel_offset          : 26;
			uint32_t falling_en_tm           :  1;
			uint32_t highres_en_tm           :  1;
			uint32_t scan_en_tm              :  1;
			uint32_t scan_in_tm              :  1;
			uint32_t reserved1               :  1;
			uint32_t channel_enable          :  1;
		} bits;
		uint32_t reg;
	} Ch_Control[64];
	// ------------------------------------------------ Buffers
	union {
		struct {
			uint32_t channel_buffer_size     :  4;
			uint32_t readout_buffer_size     :  4;
			uint32_t trigger_buffer_size     :  4;
			uint32_t disable_ro_reject       :  1;
			uint32_t errorcnts_saturate      :  1;
			uint32_t reserved1               :  2;
			uint32_t max_grouphits           :  8;
			uint32_t reserved2               :  8;
		} bits;
		uint32_t reg;
	} Buffers;
	// ------------------------------------------------ Hit_RX_TX
	union {
		struct {
			uint32_t hrx_top_delay           :  4;
			uint32_t hrx_top_bias            :  4;
			uint32_t hrx_top_filter_trailing :  1;
			uint32_t hrx_top_filter_leading  :  1;
			uint32_t hrx_top_en_r            :  1;
			uint32_t hrx_top_en              :  1;
			uint32_t hrx_bot_delay           :  4;
			uint32_t hrx_bot_bias            :  4;
			uint32_t hrx_bot_filter_trailing :  1;
			uint32_t hrx_bot_filter_leading  :  1;
			uint32_t hrx_bot_en_r            :  1;
			uint32_t hrx_bot_en              :  1;
			uint32_t tx_strenght             :  2;
			uint32_t reserved1               :  6;
		} bits;
		uint32_t reg;
	} Hit_RX_TX;
	// ------------------------------------------------ DLL_TG
	union {
		struct {
			uint32_t dll_bias_val            :  7;
			uint32_t dll_bias_cal            :  5;
			uint32_t dll_cp_comp             :  3;
			uint32_t dll_ctrlval             :  4;
			uint32_t dll_fixctrl             :  1;
			uint32_t dll_extctrl             :  1;
			uint32_t dll_cp_comp_dir         :  1;
			uint32_t dll_en_bias_cal         :  1;
			uint32_t tg_bot_nen_fine         :  1;
			uint32_t tg_bot_nen_coarse       :  1;
			uint32_t tg_top_nen_fine         :  1;
			uint32_t tg_top_nen_coarse       :  1;
			uint32_t tg_cal_nrst             :  1;
			uint32_t tg_cal_parity_in        :  1;
			uint32_t reserved1               :  3;
		} bits;
		uint32_t reg;
	} DLL_TG;
	// ------------------------------------------------ PLL1
	union {
		struct {
			uint32_t pll_cp_ce               :  1;
			uint32_t pll_cp_dacset           :  7;
			uint32_t pll_cp_irefcpset        :  5;
			uint32_t pll_cp_mmcomp           :  3;
			uint32_t pll_cp_mmdir            :  1;
			uint32_t pll_pfd_enspfd          :  1;
			uint32_t pll_resistor            :  5;
			uint32_t pll_sw_ext              :  1;
			uint32_t pll_vco_dacsel          :  1;
			uint32_t pll_vco_dacset          :  7;
		} bits;
		uint32_t reg;
	} PLL1;
	// ------------------------------------------------ PLL2
	union {
		struct {
			uint32_t pll_vco_dac_ce          :  1;
			uint32_t pll_vco_igen_start      :  1;
			uint32_t pll_railmode_vco        :  1;
			uint32_t pll_abuffdacset         :  5;
			uint32_t pll_buffce              :  1;
			uint32_t pll_afcvcal             :  4;
			uint32_t pll_afcstart            :  1;
			uint32_t pll_afcrst              :  1;
			uint32_t pll_afc_override        :  1;
			uint32_t pll_bt0                 :  1;
			uint32_t pll_afc_overrideval     :  4;
			uint32_t pll_afc_overridesig     :  1;
			uint32_t reserved1               : 10;
		} bits;
		uint32_t reg;
	} PLL2;

	// ------------------------------------------------ Clocks
	union {
		struct {
			uint32_t hit_phase               :  2;
			uint32_t reserved1               :  2;
			uint32_t trig_phase              :  3;
			uint32_t reserved2               :  1;
			uint32_t bx_rst_phase            :  3;
			uint32_t reserved3               :  1;
			uint32_t eid_rst_phase           :  3;
			uint32_t reserved4               :  1;
			uint32_t par_speed               :  2;
			uint32_t reserved5               :  2;
			uint32_t par_phase               :  3;
			uint32_t sync_clock              :  1;
			uint32_t ext_clk_dll             :  1;
			uint32_t reserved6               :  7;
		} bits;
		uint32_t reg;
	} Clocks;
	// ------------------------------------------------ ClockShift
	union {
		struct {
			uint32_t reserved1               :  4;
			uint32_t shift_clk1G28           :  4;
			uint32_t shift_clk640M           :  4;
			uint32_t shift_clk320M           :  4;
			uint32_t shift_clk320M_ref       :  4;
			uint32_t shift_clk160M           :  4;
			uint32_t shift_clk80M            :  4;
			uint32_t shift_clk40M            :  4;
		} bits;
		uint32_t reg;
	} ClockShift;
	// ------------------------------------------------ Hit_RXen_T 
	union {
		struct {
			uint32_t hrx_enable_t            : 32;
		} bits;
		uint32_t reg;
	} Hit_RXen_T;
	// ------------------------------------------------ Hit_RXen_B
	union {
		struct {
			uint32_t hrx_enable_b            : 32;
		} bits;
		uint32_t reg;
	} Hit_RXen_B;
	// ------------------------------------------------ PulseGen1
	union {
		struct {
			uint32_t pg_run                  :  1;
			uint32_t pg_rep                  :  1;
			uint32_t pg_direct               :  1;
			uint32_t pg_ini                  :  1;
			uint32_t pg_en                   :  1;
			uint32_t pg_strength             :  2;
			uint32_t reserved1               :  1;
			uint32_t pg_rising               : 18;
			uint32_t reserved2               :  6;
		} bits;
		uint32_t reg;
	} PulseGen1;
	// ------------------------------------------------ PulseGen2
	union {
		struct {
			uint32_t pg_phase                :  8;
			uint32_t pg_falling              : 18;
			uint32_t reserved1               :  6;
		} bits;
		uint32_t reg;
	} PulseGen2;
	// ------------------------------------------------ PulseGen3
	union {
		struct {
			uint32_t pg_reload               : 18;
			uint32_t reserved2               : 14;
		} bits;
		uint32_t reg;
	} PulseGen3;
	// ------------------------------------------------ ErrorFlagCtrl
	union {
		struct {
			uint32_t error_flags_1           : 10;
			uint32_t error_flags_2           : 10;
			uint32_t error_flags_3           : 10;
			uint32_t reserved1               :  2;
		} bits;
		uint32_t reg;
	} ErrorFlagCtrl;
	// ------------------------------------------------ DelayAdjust
	union {
		struct {
			uint32_t tg_delay_taps           :  8;
			uint32_t reserved1               : 24;
		} bits;
		uint32_t reg;
	} DelayAdjust;

	// ************************************************
	// Status Regs (read only)
	// ************************************************
	// ------------------------------------------------ Ch_Status[64]
	union {
		struct {
			uint32_t ch_fillgrade            :  8;
			uint32_t ch_dropped              :  8;
			uint32_t ch_parity               :  4;
			uint32_t scan_out                :  1;
			uint32_t reserved1               :  2;
			uint32_t ch_stateerr             :  1;
			uint32_t reserved2               :  8;
		} bits;
		uint32_t reg;
	} Ch_Status[64];
	// ------------------------------------------------ Trg_Status[4]
	union {
		struct {
			uint32_t tr_fillgrade            :  8;
			uint32_t tr_dropped              :  8;
			uint32_t tr_parity               :  4;
			uint32_t reserved1               :  3;
			uint32_t tr_stateerr             :  1;
			uint32_t reserved2               :  8;

		} bits;
		uint32_t reg;
	} Trg_Status[4];
	// ------------------------------------------------ RO_Status[4]
	union {
		struct {
			uint32_t ro_fillgrade            :  8;
			uint32_t ro_dropped              :  8;
			uint32_t ro_corrected            :  4;
			uint32_t reserved1               :  3;
			uint32_t ro_multibiterr          :  4;
			uint32_t reserved2               :  5;

		} bits;
		uint32_t reg;
	} RO_Status[4];
	// ------------------------------------------------ Cfg_Parity[3]
	union {
		struct {
			uint32_t parity                  : 32;
		} bits;
		uint32_t reg;
	} Cfg_Parity[3];
	// ------------------------------------------------ PLL_Caps
	union {
		struct {
			uint32_t pll_selectedcap         : 24;
			uint32_t reserved1               :  8;
		} bits;
		uint32_t reg;
	} PLL_Caps;

} picoTDC_Cfg_t;

#define I2C_ADDR_TDC(i)					(0x63 - (i)) // FBER: this is true only for i < 4 and depends to the hardware configuration of pull-down resistors 
#define I2C_ADDR_PLL0					0x68
#define I2C_ADDR_PLL1					0x69
#define I2C_ADDR_PLL2					0x70


// *****************************************************************
// Virtual Registers of the concentrator
// *****************************************************************
// Register virtual address map
#define VR_IO_STANDARD				0	// IO standard (NIM or TTL). Applies to all I/Os
#define VR_IO_FA_DIR				1	// Direction of FA
#define VR_IO_FB_DIR				2
#define VR_IO_RA_DIR				3
#define VR_IO_RB_DIR				4
#define VR_IO_FA_FN					5	// Function of FA when used as output
#define VR_IO_FB_FN					6
#define VR_IO_RA_FN					7
#define VR_IO_RB_FN					8
#define VR_IO_FOUT_FN				9	// Funtion of the eigth F_OUT
#define VR_IO_VETO_FN				10	// Veto source
#define VR_IO_FIN_MASK				11	// F_IN enable mask
#define VR_IO_FOUT_MASK				12	// F_OUT enable mask
#define VR_IO_REG_VALUE				13	// Register controlled I/O
#define VR_IO_TP_PERIOD				14	// Internal Test Pulse period
#define VR_IO_TP_WIDTH				15	// Internal Test Pulse width
#define VR_IO_CLK_SOURCE			16	// Clock source
#define VR_IO_SYNC_OUT_A_FN			17	// Function of SYNC_OUT (RJ45)
#define VR_IO_SYNC_OUT_B_FN			18
#define VR_IO_SYNC_OUT_C_FN			19
#define VR_IO_MASTER_SALVE			20	// Board is master or slave
#define VR_IO_SYNC_PULSE_WIDTH		21	// Sync pulse width
#define VR_IO_SYNC_SEND				22	// Send a sync pulse
#define VR_SYNC_DELAY				23	// node to node delay in the daisy chain (used to fine tune the sync skew)


// Register values
#define VR_IO_SYNCSOURCE_ZERO		0
#define VR_IO_SYNCSOURCE_SW_PULSE	1
#define VR_IO_SYNCSOURCE_SW_REG		2
#define VR_IO_SYNCSOURCE_FA			3
#define VR_IO_SYNCSOURCE_FB			4
#define VR_IO_SYNCSOURCE_RA			5
#define VR_IO_SYNCSOURCE_RB			6
#define VR_IO_SYNCSOURCE_GPS_PPS	7
#define VR_IO_SYNCSOURCE_CLK_REF	8

#define VR_IO_BOARD_MODE_MASTER		0
#define VR_IO_BOARD_MODE_SLAVE		1

#define VR_IO_STANDARD_IO_NIM		0
#define VR_IO_STANDARD_IO_TLL		1

#define VR_IO_DIRECTION_OUT			0
#define VR_IO_DIRECTION_IN_R50		1
#define VR_IO_DIRECTION_IN_HIZ		2

#define VR_IO_FUNCTION_REGISTER		0
#define VR_IO_FUNCTION_LOGIC_OR		1
#define VR_IO_FUNCTION_LOGIC_AND	2
#define VR_IO_FUNCTION_MAJORITY		3
#define VR_IO_FUNCTION_TEST_PULSE	4
#define VR_IO_FUNCTION_SYNC			5
#define VR_IO_FUNCTION_FA_IN		6
#define VR_IO_FUNCTION_FB_IN		7
#define VR_IO_FUNCTION_RA_IN		8
#define VR_IO_FUNCTION_RB_IN		9
#define VR_IO_FUNCTION_ZERO			15

#define VR_IO_CLKSOURCE_INTERNAL	0
#define VR_IO_CLKSOURCE_LEMO		1
#define VR_IO_CLKSOURCE_SYNC		2
#define VR_IO_CLKSOURCE_RA			3

#endif