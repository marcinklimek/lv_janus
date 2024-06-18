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

#ifndef _JanusC_H
#define _JanusC_H                    // Protect against multiple inclusion

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


#include "MultiPlatform.h"
#include "FERSutils.h"



#ifdef _WINDOWS
    #define popen _popen
    #define DEFAULT_GNUPLOT_PATH "..\\gnuplot\\gnuplot.exe"
    #define GNUPLOT_TERMINAL_TYPE "wxt"
    #define PATH_SEPARATOR "\\"
    #define CONFIG_FILE_PATH ""
    #define DATA_FILE_PATH "."
    #define WORKING_DIR ""
    #define EXE_NAME "JanusC.exe"
#else
    #include <unistd.h>
    #include <stdint.h>   /* C99 compliant compilers: uint64_t */
    #include <ctype.h>    /* toupper() */
    #include <sys/time.h>
    #define DEFAULT_GNUPLOT_PATH	"gnuplot"
    #define GNUPLOT_TERMINAL_TYPE "x11"
    #define PATH_SEPARATOR "/"
    #define EXE_NAME "JanusC"
    #ifndef Sleep
        #define Sleep(x) usleep((x)*1000)
    #endif
    #ifdef _ROOT_
        #define CONFIG_FILE_PATH _ROOT_"/Janus/config/"
        #define DATA_FILE_PATH _ROOT_"/Janus/"
        #define WORKING_DIR _ROOT_"/Janus/"
    #else
        #define CONFIG_FILE_PATH ""
        #define DATA_FILE_PATH "./DataFiles"
        #define WORKING_DIR ""
    #endif
#endif

#define SW_RELEASE_NUM			"2.5.1"
#define SW_RELEASE_DATE			"02/11/2023"
#define FILE_LIST_VER			"3.2"

#ifdef _WINDOWS
#define CONFIG_FILENAME			"Janus_Config.txt"
#define RUNVARS_FILENAME		"RunVars.txt"
#define PIXMAP_FILENAME			"pixel_map.txt"
#else	// in Linux the Debug run from Janus folder, and the executable in Janus/Debug
#define CONFIG_FILENAME			"Janus_Config.txt"
#define RUNVARS_FILENAME		"RunVars.txt"
#define PIXMAP_FILENAME			"pixel_map.txt"
#endif

//****************************************************************************
// Definition of limits and sizes
//****************************************************************************
#define MAX_NBRD						16	// max. number of boards 
#define MAX_NCNC						8	// max. number of concentrators
#define MAX_NCH							128	// max. number of channels 
#define PICOTDC_NCH  					64	// number of picoTDC channels 
#define MAX_GW							20	// max. number of generic write commads
#define MAX_NTRACES						8	// Max num traces in the plot

// *****************************************************************
// Parameter options
// *****************************************************************
#define OUTFILE_RAW_DATA_BIN			0x0001
#define OUTFILE_RAW_DATA_ASCII			0x0002
#define OUTFILE_LIST_BIN				0x0004
#define OUTFILE_LIST_ASCII				0x0008
#define OUTFILE_LEAD_HISTO				0x0040
#define OUTFILE_TRAIL_HISTO				0x0080
#define OUTFILE_TOT_HISTO				0x0100
#define OUTFILE_RUN_INFO				0x0200
#define OUTFILE_SYNC					0x0400
#define OUTFILE_LEAD_MEAS				0x0800
#define OUTFILE_TRAIL_MEAS				0x1000
#define OUTFILE_TOT_MEAS				0x2000

#define OF_UNIT_LSB						0
#define OF_UNIT_NS						1

#define PLOT_LEAD_SPEC					0
#define PLOT_TOT_SPEC					1
#define PLOT_TOT_COUNTS					2
#define PLOT_2D_HIT_RATE				3
#define PLOT_TRAIL_SPEC					4

#define SMON_HIT_RATE					0
#define SMON_HIT_CNT					1
#define SMON_TOT_RATE					2	// Takes into account all the events read by TDC, no software 
#define SMON_TOT_CNT					3	
#define SMON_LEAD_MEAN					4	
#define SMON_LEAD_RMS					5	
#define SMON_TOT_MEAN					6	
#define SMON_TOT_RMS					7	

#define STOPRUN_MANUAL					0
#define STOPRUN_PRESET_TIME				1
#define STOPRUN_PRESET_COUNTS			2

#define EVBLD_DISABLED					0
#define EVBLD_TRGTIME_SORTING			1
#define EVBLD_TRGID_SORTING				2

#define DATA_ANALYSIS_CNT				0x0001
#define DATA_ANALYSIS_MEAS				0x0002
#define DATA_ANALYSIS_HISTO				0x0004

#define GLITCHFILTERMODE_DISABLED		0
#define GLITCHFILTERMODE_TRAILING		1
#define GLITCHFILTERMODE_LEADING		2
#define GLITCHFILTERMODE_BOTH			3

#define HRCLK_DISABLED					0
#define HRCLK_DAISY_CHAIN				1
#define HRCLK_FAN_OUT					2

#define DPROBE_CLK_1024					0x00
#define DPROBE_TRG_ACCEPTED				0x04
#define DPROBE_TRG_REJECTED				0x05
#define DPROBE_TX_DATA_VALID			0x20
#define DPROBE_TX_PCK_COMMIT			0x21
#define DPROBE_TX_PCK_ACCEPTED			0x23
#define DPROBE_TX_PCK_REJECTED			0x24
#define DPROBE_TDC_DATA_VALID			0x12
#define DPROBE_TDC_DATA_COMMIT			0x15
#define DPROBE_TDL_SYNC					0x30

#define A5256_CH0_POSITIVE				0
#define A5256_CH0_NEGATIVE				1
#define A5256_CH0_DUAL					2


// Acquisition Status Bits
#define ACQSTATUS_SOCK_CONNECTED		1	// GUI connected through socket
#define ACQSTATUS_HW_CONNECTED			2	// Hardware connected
#define ACQSTATUS_READY					3	// ready to start (HW connected, memory allocated and initialized)
#define ACQSTATUS_RUNNING				4	// acquisition running (data taking)
#define ACQSTATUS_RESTARTING			5	// Restarting acquisition
#define ACQSTATUS_CALIBTHR				9	// Calibrate discriminator thresholds for A5256
#define ACQSTATUS_UPGRADING_FW			12	// Upgrading the FW
#define ACQSTATUS_ERROR					-1	// Error

// Almost Full buffers Bits
#define PORT0_AF						0x01
#define PORT1_AF						0x02
#define PORT2_AF						0x04
#define PORT3_AF						0x08
#define MEB_AF							0x10
#define ROB_AF							0x20
#define TRG_Buff_AF						0x40

// Temperatures
#define TEMP_BOARD						0
#define TEMP_FPGA						1
#define TEMP_TDC0						2
#define TEMP_TDC1						3

#define DBLOG_TEMP_STATUS				0x00010000		// Enable Temperature and Status Log

class lv_logger;
const char AFBuffName[7][10] = { "Port0", "Port1", "Port2", "Port3", "LOSF", "Trg_Buff", "SZF" };

//****************************************************************************
// struct that contains the configuration parameters (HW and SW)
//****************************************************************************
typedef struct Config_t {

    // System info 
    char ConnPath[MAX_NBRD][40];	// IP address of the board
    int FERSmodel;					// Type of FERS board
    int NumBrd;                     // Tot number of connected boards
    int NumCh;						// Number of channels 
    //int SerialNumber;				// Board serial number

    int DebugLogMask;				// Enable debug messages or log files
    int OutFileEnableMask;			// Enable/Disable output files 
    char DataFilePath[500];			// Output file data path
    int OutFileUnit;				// Unit for time measurement in output files (0 = LSB, 1 = ns)
    int EnableJobs;					// Enable Jobs
    int JobFirstRun;				// First Run Number of the job
    int JobLastRun;					// Last Run Number of the job
    float RunSleep;					// Wait time between runs of one job
    int StartRunMode;				// Start Mode (this is a HW setting that defines how to start/stop the acquisition in the boards)
    int StopRunMode;				// Stop Mode (unlike the start mode, this is a SW setting that deicdes the stop criteria)
    int RunNumber_AutoIncr;			// auto increment run number after stop
    float PresetTime;				// Preset Time (Real Time in s) for auto stop
    int PresetCounts;				// Preset Number of counts (triggers)
    int EventBuildingMode;			// Event Building Mode
    int TstampCoincWindow;			// Time window (ns) in event buiding based on trigger Tstamp
    int DataAnalysis;				// Data Analysis Enable/disable mask

    // Histogram Settings
    int LeadTrailHistoNbin;			// Number of channels (bins) in the Leading/Trailing Histogram
    int LeadTrailRebin;				// Rebin factor (Histo_Bin = Orig_Value_LSB / BinSize); The BinSize will be calculated (not read from Config File). Must be a power of 2
    float LeadHistoMin;				// Min value in ns for Leading Histogram
    float TrailHistoMin;			// Min value in ns for Trailing Histogram
    float LeadHistoMax;				// Max value in ns for Leading Histogram (will be rounded to accomodate rebinning with a power of 2)
    float TrailHistoMax;			// Max value in ns for Trailing Histogram (will be rounded to accomodate rebinning with a power of 2)
    int ToTHistoNbin;				// Same as Lead/Trail
    float ToTHistoMin;				// Same as Lead/Trail
    float ToTHistoMax;				// Same as Lead/Trail
    int ToTRebin;					// Same as Lead/Trail
    
    float WalkFitCoeff[6];			// Walk vs ToT calibration curve (up to 5th order polynomial fit)
    int EnableWalkCorrection;		// Enable Walk correction by ToT

    //                                                                       
    // Acquisition Setup (HW settings)
    //                                                                       
    // Board Settings
    uint32_t AcquisitionMode;						// ACQMODE_COMMON_START, ACQMODE_COMMON_STOP, ACQMODE_STREAMING, ACQMODE_TRG_MATCHING
    uint32_t MeasMode;								// LEAD_ONLY, LEAD_TRAIL, LEAD_TOT8, LEAD_TOT11
    uint32_t HighResClock;							// High Res clock distribution (MCX connectors)
    uint32_t TestMode;								// Run with fixed data patterns generated by the FPGA (emulating a common start with 64 channels)
    uint32_t LeadTrail_LSB;							// Leading/Trailing LSB required by the user: 0: LSB = ~3ps, N: LSB = 3ps * 2^N, (Max N=10; LSB = ~3.125 ns)
    uint32_t ToT_LSB;								// ToT LSB required by the user: 0: LSB = 3.125ps, N: LSB = 3.125ps * 2^N, (Max N=18; LSB = ~800 ns)
    uint32_t LeadTrail_Rescale;						// Rescaling factor for the Leading/Trailing LSB (this variable is calculated by SW)
    uint32_t ToT_Rescale;							// Rescaling factor for the ToT LSB (this variable is calculated by SW)
    float LeadTrail_LSB_ns;							// Leading/Trailing LSB in ns, after rescale (this variable is calculated by SW)
    float ToT_LSB_ns;								// ToT LSB in ns, after rescale (this variable is calculated by SW)
    uint32_t GlitchFilterDelay;						// Delay of the glitch filter (~800 ps to ~10 ns with 16 steps)
    uint32_t GlitchFilterMode;						// DISABLED, TRAILING, LEADING, BOTH
    uint16_t ToT_reject_low_thr;					// The FPGA suppresses the Hits with ToT > low_threshold  (0 disabled)
    uint16_t ToT_reject_high_thr;					// The FPGA suppresses the Hits with ToT < high_threshold  (0 disabled)
    uint16_t TrgHoldOff;							// Retrigger protection time. Set the busy active for N clock cycles (0=disabled)
    uint32_t TriggerMask;							// Bunch Trigger mask
    uint32_t TriggerLogic;							// Trigger Logic Definition
    uint32_t T0_outMask;							// T0-OUT mask
    uint32_t T1_outMask;							// T1-OUT mask
    uint32_t Tref_Mask;								// Tref mask
    uint32_t Veto_Mask;								// Veto mask
    uint32_t HeaderField0;							// Header Field0 (for test only, default=4=near full flags (ch 0..7 of the port) - 00000CCCCCCCC)
    uint32_t HeaderField1;							// Header Field1 (for test only, default=5=near full flags (ch 8..15 of the port + RO_buff, TrgBuff) - RT000CCCCCCCC)
    uint32_t TDC_ChBufferSize;						// Channel buffer size in the picoTDC (set a limit to the max number of hits acquired by the channel)
    uint32_t TriggerBufferSize;						// Size of the trigger buffer in the FPGA (limits the number or pending triggers, already sent to the TDC but not written in the output FIFO yet)
    uint32_t MaxPck_Train;							// Max. number of packets (events) that the A5203 can transmit to the concentrator in a single data train (0 = use default in FW)
    uint32_t MaxPck_Block;							// Max. number of packets (events) that the concentrator can aggregate in one Data Block (0 = use default in FW)
    uint32_t CncBufferSize;							// Data buffer size (in 32 bit words) in the concentrator; stop trains when this level is reached (0 = use default in FW)
    uint32_t En_Head_Trail;							// Enable Header and Trailer: 0=Keep all (group header+trail), 1=One word, 2=Header and trailer suppressed
    uint32_t En_Empty_Ev_Suppr;						// Enable event suppression (only in Custom header mode)
    uint8_t  En_service_event;						// Enable service event
    uint8_t  En_128_ch;								// Enable 128 channels
    uint8_t  Dis_tdl;								// Enable the TDL switching off when not used (DNIN: why is a parameter?)
    float TrgWindowWidth;							// Trigger window width in ns (will be rounded to steps of 25 ns)
    float TrgWindowOffset;							// Trigger window offset in ns; can be negative (will be rounded to steps of 25 ns)
    float GateWidth;								// Gate window width in ns (will be rounded to steps of 25 ns)
    float PtrgPeriod;								// period in ns of the internal periodic trigger (dwell time)
    uint32_t DigitalProbe[2];						// Digital Probe (internal signals of FPGA)
    uint32_t TrgIdMode;								// Trigger ID: 0 = trigger counter, 1 = validation counter
    float TDCpulser_Width;							// picoTDC Pulser Output (width in ns)
    float TDCpulser_Period;							// picoTDC Pulser Output (period in ns)
    float FiberDelayAdjust[MAX_NCNC][8][16];		// Fiber length (in meters) for individual tuning of the propagation delay along the TDL daisy chains

    uint32_t ChEnableMask0[MAX_NBRD];				// Channel enable mask ch 0..31 (TDC0)
    uint32_t ChEnableMask1[MAX_NBRD];				// Channel enable mask ch 32..63 (TDC0)
    uint32_t ChEnableMask2[MAX_NBRD];				// Channel enable mask ch 0..31 (TDC1)
    uint32_t ChEnableMask3[MAX_NBRD];				// Channel enable mask ch 32..63 (TDC1)

    // Channel Settings 
    uint32_t Ch_Offset[MAX_NBRD][MAX_NCH];			// Channel Offset
    uint8_t InvertEdgePolarity[MAX_NBRD][MAX_NCH];	// Invert edge polarity. NOTE: can be used in LEAD_TRAIL mode only

    // Settings for the input adapters
    int AdapterType;								// Adapter Type
    float DiscrThreshold[MAX_NBRD][MAX_NCH];		// Discriminator Threshold
    float DiscrThreshold2[MAX_NBRD][MAX_NCH];		// Discriminator 2nd Threshold (double thershold mode only)
    int DisableThresholdCalib[MAX_NBRD];			// Disable threshold calibration
    int A5256_Ch0Polarity;							// Polarity of Ch0 in A5256 (POS, NEG)

    // Generic write accesses 
    int GWn;
    uint32_t GWbrd[MAX_GW];						// Board Number (-1 = all)
    uint32_t GWaddr[MAX_GW];					// Register Address
    uint32_t GWdata[MAX_GW];					// Data to write
    uint32_t GWmask[MAX_GW];					// Bit Mask
    
} Config_t;


typedef struct RunVars_t {
    int ActiveBrd;				// Active board
    int ActiveCh;				// Active channel
    int PlotType;				// Plot Type
    int SMonType;				// Statistics Monitor Type
    int Xcalib;					// X-axis calibration
    int RunNumber;				// Run Number (used in output file name; -1 => no run number)
    char PlotTraces[8][100];	// Plot Traces (format: "0 3 X" => Board 0, Channel 3, From X[B board - F offline - S from file)
    int StaircaseCfg[4];		// Staircase Params: MinThr Maxthr Step Dwell
    int HoldDelayScanCfg[4];	// Hold Delay Scan Params: MinDelay MaxDelay Step Nmean
} RunVars_t;




//****************************************************************************
// Global Variables
//****************************************************************************
extern Config_t	WDcfg;				// struct containing all acquisition parameters
extern RunVars_t RunVars;			// struct containing run time variables
extern int handle[MAX_NBRD];		// board handles
//extern int ActiveCh, ActiveBrd;		// Board and Channel active in the plot
extern int AcqRun;					// Acquisition running
extern int AcqStatus;				// Acquisition Status
extern int SockConsole;				// 0: use stdio console, 1: use socket console
extern char ErrorMsg[250];			// Error Message


// forward declarations
int janusc_init();
void janusc_destroy();


#endif

