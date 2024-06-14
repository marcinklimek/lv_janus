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

#include "JanusC.h"
#include "plot.h"
#include "console.h"
#include "adapters.h"
#include "paramparser.h"
#include "configure.h"
#include "outputfiles.h"
#include "Statistics.h"
#include "FERSlib.h"

#include "../src/lv_clogger.h"

// ******************************************************************************************
// Global Variables
// ******************************************************************************************
Config_t WDcfg;					// struct with all parameters
RunVars_t RunVars;				// struct containing run time variables
ServEvent_t sEvt[MAX_NBRD];		// struct containing the service information of each boards
int SockConsole = 0;			// 0=stdio, 1=socket
int AcqStatus = 0;				// Acquisition Status (running, stopped, fail, etc...)
int HoldScan_newrun = 0;
int handle[MAX_NBRD];			// Board handles
int cnc_handle[MAX_NCNC];		// Concentrator handles
int Freeze = 0;					// stop plot
int OneShot = 0;				// Single Plot 
int StatMode = 0;				// Stats monitor: 0=per board, 1=all baords
int StatIntegral = 0;			// Stats caclulation: 0=istantaneous, 1=integral
int Quit = 0;					// Quit readout loop
int RestartAcq = 0;				// Force restart acq (reconfigure HW)
int RestartAll = 0;				// Restart from the beginning (recovery after error)
int SkipConfig = 0;				// Skip board configuration
int grp_ch64 = 0;				// 64 channel group (0:0-63, 1:64-127)
double build_time_us = 0;		// Time of the event building (in us)
char ErrorMsg[250];				// Error Message
float BrdTemp[MAX_NBRD][4];		// Board Temperatures (near PIC/FPGA)
uint32_t StatusReg[MAX_NBRD];	// Status Register
FILE* MsgLog = NULL;			// message log output file

int jobrun = 0;
uint8_t stop_sw = 0; // Used to disable automatic start during jobs
int ServerDead = 0;

// ******************************************************************************************
// Save/Load run time variables, utility functions...
// ******************************************************************************************
int LoadRunVariables(RunVars_t* RunVars)
{
	char str[100];
	int i;
	FILE* ps = fopen(RUNVARS_FILENAME, "r");
	//char plot_name[50];	// name of histofile.

	// set defaults
	RunVars->PlotType = PLOT_LEAD_SPEC;
	RunVars->SMonType = SMON_HIT_RATE;
	RunVars->RunNumber = 0;
	RunVars->ActiveCh = 0;
	RunVars->ActiveBrd = 0;
	for (i = 0; i < 4; i++) RunVars->StaircaseCfg[i] = 0;
	for (i = 0; i < 4; i++) RunVars->HoldDelayScanCfg[i] = 0;
	for (int i = 0; i < 8; i++)
		sprintf(RunVars->PlotTraces[i], "");
	if (ps == NULL) return -1;
	for (i = 0; i < MAX_NTRACES; ++i)  Stats.offline_bin[i] = -1; // Reset offline binning
	while (!feof(ps)) {
		fscanf(ps, "%s", str);

		if (strcmp(str, "ActiveBrd") == 0)	fscanf(ps, "%d", &RunVars->ActiveBrd);
		else if (strcmp(str, "ActiveCh") == 0)	fscanf(ps, "%d", &RunVars->ActiveCh);
		else if (strcmp(str, "PlotType") == 0)	fscanf(ps, "%d", &RunVars->PlotType);
		else if (strcmp(str, "SMonType") == 0)	fscanf(ps, "%d", &RunVars->SMonType);
		else if (strcmp(str, "RunNumber") == 0)	fscanf(ps, "%d", &RunVars->RunNumber);
		else if (strcmp(str, "Xcalib") == 0)	fscanf(ps, "%d", &RunVars->Xcalib);
		else if (strcmp(str, "PlotTraces") == 0) {	// DNIN: if you set board > connectedBoard you see the trace of the last connected board
			int v1, v2, v3, nr;
			char c1[100];
			nr = fscanf(ps, "%d", &v1);
			if (nr < 1) break;
			fscanf(ps, "%d %d %s", &v2, &v3, c1);
			if ((v1 >= 0) && (v1 < 8) && (v2 >= 0) && (v2 < MAX_NBRD) && (v3 >= 0) && (v3 < MAX_NCH) && (c1[0] == 'B' || c1[0] == 'F' || c1[0] == 'S'))
				sprintf(RunVars->PlotTraces[v1], "%d %d %s", v2, v3, c1);
			else
				continue;
			//char tmp_var[50];
			//strcpy(tmp_var, Stats.H1_PHA_LG[v2][v3].title);
			if (Stats.H1_File[0].H_data == NULL) // Check if statistics have been created
				continue;

			if ((c1[0] == 'F' || c1[0] == 'S') && (RunVars->PlotType < 4 || RunVars->PlotType == 9))
				Histo1D_Offline(v1, v2, v3, c1, RunVars->PlotType);
			else
				continue;
		}
	}
	if (RunVars->ActiveBrd >= WDcfg.NumBrd) RunVars->ActiveBrd = 0;
	fclose(ps);
	return 0;
}

int SaveRunVariables(RunVars_t RunVars)
{
	bool no_traces = 1;
	FILE* ps = fopen(RUNVARS_FILENAME, "w");
	if (ps == NULL) return -1;
	fprintf(ps, "ActiveBrd     %d\n", RunVars.ActiveBrd);
	fprintf(ps, "ActiveCh      %d\n", RunVars.ActiveCh);
	fprintf(ps, "PlotType      %d\n", RunVars.PlotType);
	fprintf(ps, "SMonType      %d\n", RunVars.SMonType);
	fprintf(ps, "RunNumber     %d\n", RunVars.RunNumber);
	fprintf(ps, "Xcalib        %d\n", RunVars.Xcalib);
	for (int i = 0; i < 8; i++) {
		if (strlen(RunVars.PlotTraces[i]) > 0) {
			no_traces = 0;
			fprintf(ps, "PlotTraces		%d %s  \n", i, RunVars.PlotTraces[i]);
		}
	}
	if (no_traces) {	// default 
		fprintf(ps, "PlotTraces		0 0 0 B\n");
	}

	fclose(ps);
	return 0;
}

// Parse the config file of a run during a job. If the corresponding cfg file is not found, the default config is parsed
void increase_job_run_number() {
	if (jobrun < WDcfg.JobLastRun) jobrun++;
	else jobrun = WDcfg.JobFirstRun;
	RunVars.RunNumber = jobrun;
	SaveRunVariables(RunVars);
	stop_sw = 1;
}

void job_read_parse() {
	FILE* cfg;
	// When running a job, load the specific config file for this run number
	int TmpJRun = WDcfg.JobFirstRun;
	char fname[200];
	sprintf(fname, "%sJanus_Config_Run%d.txt", WDcfg.DataFilePath, jobrun);
	cfg = fopen(fname, "r");
	if (cfg != NULL) {
		ParseConfigFile(cfg, &WDcfg, 1);
		fclose(cfg);
		WDcfg.JobFirstRun = TmpJRun;	// The first run cannot be overwrite from the config file
	} else {
		sprintf(fname, "Janus_Config.txt");
		cfg = fopen(fname, "r");
		if (cfg != NULL) {
			ParseConfigFile(cfg, &WDcfg, 1);
			fclose(cfg);
		}
	}

}

// Convert a double in a string with unit (k, M, G)
void double2str(double f, int space, char* s)
{
	if (!space) {
		if (f <= 999.999)			sprintf(s, "%7.3f ", f);
		else if (f <= 999999)		sprintf(s, "%7.3fk", f / 1e3);
		else if (f <= 999999000)	sprintf(s, "%7.3fM", f / 1e6);
		else						sprintf(s, "%7.3fG", f / 1e9);
	}
	else {
		if (f <= 999.999)			sprintf(s, "%7.3f ", f);
		else if (f <= 999999)		sprintf(s, "%7.3fk", f / 1e3);
		else if (f <= 999999000)	sprintf(s, "%7.3fM", f / 1e6);
		else						sprintf(s, "%7.3fG", f / 1e9);
	}
}

void cnt2str(uint32_t c, char* s)
{
	if (c <= 9999999)			sprintf(s, "%7d ", c);
	else if (c <= 9999999999)	sprintf(s, "%7dk", c / 1000);
	else						sprintf(s, "%7dM", c / 1000000);
}

// convert temperature to string
void temp2str(float temp, char *s) 
{
	if (temp < 255) sprintf(s, "%4.1f", temp);
	else strcpy(s, "N.A.");
}

uint32_t ToTbin(uint32_t ToT) {
	float fbin = (ToT - (WDcfg.ToTHistoMin / WDcfg.ToT_LSB_ns)) / WDcfg.ToTRebin;
	uint32_t bin = (fbin > 0) ? (uint32_t)fbin : 0; // At the moment it will be discharged as overflow.
	return bin;
}
uint32_t Leadbin(uint32_t ToA) {
	float fbin = (ToA - (WDcfg.LeadHistoMin / WDcfg.LeadTrail_LSB_ns)) / WDcfg.LeadTrailRebin;
	uint32_t bin = (fbin > 0) ? (uint32_t)fbin : 0;
	return bin;
}
uint32_t Trailbin(uint32_t ToA) {
	float fbin = (ToA - (WDcfg.TrailHistoMin / WDcfg.LeadTrail_LSB_ns)) / WDcfg.LeadTrailRebin;
	uint32_t bin = (fbin > 0) ? (uint32_t)fbin : 0;
	return bin;
}
uint32_t Leadbin64(uint64_t ToA) {
	float fbin = (ToA - (WDcfg.LeadHistoMin / WDcfg.LeadTrail_LSB_ns)) / WDcfg.LeadTrailRebin;
	uint32_t bin = (fbin > 0) ? (uint32_t)fbin : 0;
	return bin;
}
uint32_t Trailbin64(uint64_t ToA) {
	float fbin = (ToA - (WDcfg.TrailHistoMin / WDcfg.LeadTrail_LSB_ns)) / WDcfg.LeadTrailRebin;
	uint32_t bin = (fbin > 0) ? (uint32_t)fbin : 0;
	return bin;
}


void SendAcqStatusMsg(char* fmt, ...)
{
	char msg[1000];
	va_list args;

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

	if (SockConsole) Con_printf("Sa", "%02d%s", AcqStatus, msg);
	else Con_printf("C", "%-70s\n", msg);
}


void reportProgress(char* msg, int progress)
{
	char fwmsg[100];
	if (progress > 0) sprintf(fwmsg, "Progress: %3d %%", progress);
	else strcpy(fwmsg, msg);
	SendAcqStatusMsg(fwmsg);
}

int Update_Service_Info(int handle) {
	int brd = FERS_INDEX(handle);
	int ret = 0, fail = 0;
	uint64_t now = get_time();

	if (sEvt[brd].update_time > (now - 2000)) {
		BrdTemp[brd][TEMP_TDC0] = sEvt[brd].tempTDC[0];
		BrdTemp[brd][TEMP_TDC1] = sEvt[brd].tempTDC[1];
		BrdTemp[brd][TEMP_BOARD] = sEvt[brd].tempBoard;
		BrdTemp[brd][TEMP_FPGA] = sEvt[brd].tempFPGA;
		StatusReg[brd] = sEvt[brd].Status;
	} else {
		uint32_t rejtrg_cnt, tottrg_cnt, supprtrg_cnt;
		ret |= FERS_Get_FPGA_Temp(handle, &BrdTemp[brd][TEMP_FPGA]);
		ret |= FERS_Get_TDC0_Temp(handle, &BrdTemp[brd][TEMP_TDC0]);
		if (FERS_NumChannels(handle) == 128)
			ret |= FERS_Get_TDC1_Temp(handle, &BrdTemp[brd][TEMP_TDC1]);
		ret |= FERS_Get_Board_Temp(handle, &BrdTemp[brd][TEMP_BOARD]);
		ret |= FERS_ReadRegister(handle, a_acq_status, &StatusReg[brd]);
		ret |= FERS_ReadRegister(handle, a_trg_cnt, &tottrg_cnt);
		Stats.TotTrgCnt[brd].cnt = tottrg_cnt;
		ret |= FERS_ReadRegister(handle, a_rej_trg_cnt, &rejtrg_cnt);
		Stats.LostTrgCnt[brd].cnt = rejtrg_cnt;
		if ((FERS_FPGA_FW_MajorRev(handle) > 1) || (FERS_FPGA_FW_MinorRev(handle) >= 4)) {
			ret |= FERS_ReadRegister(handle, a_zs_trg_cnt, &supprtrg_cnt);
			Stats.SupprTrgCnt[brd].cnt = supprtrg_cnt;
		}
	}
	if (ret < 0) 
		fail = 1;
	//Con_printf("ST", "%d %6.1f %6.1f %6.1f %6.1f %d", brd, BrdTemp[brd][TEMP_TDC0], BrdTemp[brd][TEMP_TDC1], BrdTemp[brd][TEMP_BOARD], BrdTemp[brd][TEMP_FPGA], fail);
	return ret;
}


// ******************************************************************************************
// Run Control functions
// ******************************************************************************************
// Start Run (starts acq in all boards)
int StartRun() {
	int ret = 0, b, tdl = 1;
	if (AcqStatus == ACQSTATUS_RUNNING) return 0;
	OpenOutputFiles(RunVars.RunNumber);

	for (b = 0; b < WDcfg.NumBrd; b++) {
		if (FERS_CONNECTIONTYPE(handle[b]) != FERS_CONNECTIONTYPE_TDL) tdl = 0;
	}
	if (!tdl && (WDcfg.StartRunMode == STARTRUN_TDL)) {
		WDcfg.StartRunMode = STARTRUN_ASYNC;
		Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: can't start in TDL mode; switched to Async mode\n" COLOR_RESET);
	}

	ret = FERS_StartAcquisition(handle, WDcfg.NumBrd, WDcfg.StartRunMode);

	Stats.start_time = get_time();
	time(&Stats.time_of_start);
	build_time_us = 0;
	Stats.stop_time = 0;
	if (AcqStatus != ACQSTATUS_RESTARTING) Con_printf("LCSm", "Run #%d started\n", RunVars.RunNumber);
	AcqStatus = ACQSTATUS_RUNNING;

	WriteListfileHeader(); // Write the file header anyway

	stop_sw = 0; // Set at 0, used to prevent automatic start during jobs after a sw stop

	return ret;
}

// Stop Run
int StopRun() {
	int ret = 0;

	ret = FERS_StopAcquisition(handle, WDcfg.NumBrd, WDcfg.StartRunMode);

	if (Stats.stop_time == 0) Stats.stop_time = get_time();
	SaveHistos();
	SaveMeasurements();
	if (WDcfg.OutFileEnableMask & OUTFILE_RUN_INFO && AcqStatus != ACQSTATUS_READY) SaveRunInfo();
	CloseOutputFiles();
	if (AcqStatus == ACQSTATUS_RUNNING) {
		Con_printf("LCSm", "Run #%d stopped. Elapsed Time = %.2f\n", RunVars.RunNumber, (float)(Stats.stop_time - Stats.start_time) / 1000);
		if ((WDcfg.RunNumber_AutoIncr) && (!WDcfg.EnableJobs)) {
			++RunVars.RunNumber;
			SaveRunVariables(RunVars);
		}
		AcqStatus = ACQSTATUS_READY;
	}
	return ret;
}

// Change stats monitor on the GUI
void change_monitor_stat_gui() {
	if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
		Con_printf("SSG0", "Time Stamp");
		Con_printf("SSG1", "Hit Rate");
		Con_printf("SSG2", "Dead Time");
		Con_printf("SSG3", "Event Build");
		Con_printf("SSG4", "Readout Rate");
	} else {
		Con_printf("SSG0", "Time Stamp");
		Con_printf("SSG1", "Trigger-ID");
		Con_printf("SSG2", "Trg Rate");
		Con_printf("SSG3", "Processed");
		Con_printf("SSG4", "Rejected");
		Con_printf("SSG5", "Emtpy Suppr");
		Con_printf("SSG6", "Brd Hit Rate");
		Con_printf("SSG7", "Event Build");
		Con_printf("SSG8", "Readout Rate");
	}
}


// Check if the config files have been changed; ret values: 0=no change, 1=changed (no acq restart needed), 2=changed (acq restart needed)
int CheckFileUpdate() {
	static uint64_t CfgUpdateTime, RunVarsUpdateTime;
	uint64_t CurrentTime;
	static int first = 1;
	int ret = 0;
	FILE* cfg;

	GetFileUpdateTime(CONFIG_FILENAME, &CurrentTime);
	if ((CurrentTime > CfgUpdateTime) && !first) {
		const Config_t WDcfg_1 = WDcfg;
		//memcpy(&WDcfg_1, &WDcfg, sizeof(Config_t));
		cfg = fopen(CONFIG_FILENAME, "r");
		ParseConfigFile(cfg, &WDcfg, 1);
		fclose(cfg);
		Con_printf("LCSm", "Config file reloaded\n");

		if ((WDcfg.AcquisitionMode == ACQMODE_STREAMING && WDcfg_1.AcquisitionMode != ACQMODE_STREAMING) ||
			(WDcfg.AcquisitionMode != ACQMODE_STREAMING && WDcfg_1.AcquisitionMode == ACQMODE_STREAMING)) {
			Con_printf("SSR", "Reset Stats Monitor");
			change_monitor_stat_gui();
		}

		if ((WDcfg_1.NumBrd != WDcfg.NumBrd) ||
			(WDcfg_1.NumCh != WDcfg.NumCh) ||
			(WDcfg_1.EnableJobs != WDcfg.EnableJobs) ||
			(WDcfg_1.JobFirstRun != WDcfg.JobFirstRun) ||
			(WDcfg_1.JobLastRun != WDcfg.JobLastRun))		ret = 3;
		else if ((WDcfg_1.AcquisitionMode != WDcfg.AcquisitionMode) ||
			(WDcfg_1.OutFileEnableMask != WDcfg.OutFileEnableMask) ||
			(WDcfg_1.LeadTrailHistoNbin != WDcfg.LeadTrailHistoNbin) ||
			(WDcfg_1.ToTHistoNbin != WDcfg.ToTHistoNbin) ||
			(WDcfg_1.TrailHistoMin != WDcfg.TrailHistoMin) ||
			(WDcfg_1.TrailHistoMax != WDcfg.TrailHistoMax) ||
			(WDcfg_1.LeadHistoMin != WDcfg.LeadHistoMin) ||
			(WDcfg_1.LeadHistoMax != WDcfg.LeadHistoMax) ||
			(WDcfg_1.ToTHistoMin != WDcfg.ToTHistoMin) ||
			(WDcfg_1.ToTHistoMax != WDcfg.ToTHistoMax) ||
			(WDcfg_1.LeadTrailRebin != WDcfg.LeadTrailRebin) ||
			(WDcfg_1.LeadTrail_LSB != WDcfg.LeadTrail_LSB) ||
			(WDcfg_1.ToTRebin != WDcfg.ToTRebin) ||
			(WDcfg_1.ToT_LSB != WDcfg.ToT_LSB) ||
			(WDcfg_1.PtrgPeriod != WDcfg.PtrgPeriod))		ret = 2;
		else												ret = 1;

		if (WDcfg_1.DebugLogMask != WDcfg.DebugLogMask) {
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "DebugLogMask cannot be changed while Janus is running. Please, set the mask up before launching Janus\n");
			WDcfg.DebugLogMask = WDcfg_1.DebugLogMask;
		}
	}

	CfgUpdateTime = CurrentTime;

	GetFileUpdateTime(RUNVARS_FILENAME, &CurrentTime);
	if ((CurrentTime > RunVarsUpdateTime) && !first) {
		RunVars_t RunVars1;
		memcpy(&RunVars1, &RunVars, sizeof(RunVars_t));
		LoadRunVariables(&RunVars);
		if (WDcfg.EnableJobs) {
			RunVars.RunNumber = RunVars1.RunNumber;
			SaveRunVariables(RunVars);
		}
		if (RunVars1.RunNumber != RunVars.RunNumber) ret = 1;
	}
	RunVarsUpdateTime = CurrentTime;
	first = 0;
	return ret;
}


// ******************************************************************************************
// RunTime commands menu
// ******************************************************************************************
int RunTimeCmd(int c)
{
	int b;  // reload_cfg = 0;
	static int CfgDataAnalysis = -1;
	int bb = 0;
	int cc = 0;
	for (int m = 0; m < 8; m++) {
		if (strlen(RunVars.PlotTraces[m]) != 0) {
			sscanf(RunVars.PlotTraces[m], "%d %d", &bb, &cc);
			break;
		}
	}
	//sscanf(RunVars.PlotTraces[0], "%d %d", &bb, &cc);

	if (c == 'q') {
		if (SockConsole) Con_GetInt(&ServerDead);
		Quit = 1;
	}
	if (c == 't') {
		for (b = 0; b < WDcfg.NumBrd; b++)
			FERS_SendCommand(handle[b], CMD_TRG);	// SW trg
	}
	if ((c == 's') && (AcqStatus == ACQSTATUS_READY)) {
		ResetStatistics();
		StartRun();
	}
	if ((c == 'S') && (AcqStatus == ACQSTATUS_RUNNING)) {
		StopRun();
		if (WDcfg.EnableJobs) {	// DNIN: In case of stop from keyboard the jobrun is increased
			increase_job_run_number();
			job_read_parse();
		}
	}
		
	if (c == 'b') {
		int new_brd;
		if (!SockConsole) {
			printf("Current Active Board = %d\n", RunVars.ActiveBrd);
			printf("New Active Board = ");
			scanf("%d", &new_brd);
		} else {
			Con_GetInt(&new_brd);
		}
		if ((new_brd >= 0) && (new_brd < WDcfg.NumBrd)) {
			RunVars.ActiveBrd = new_brd;
			if (!SockConsole) sprintf(RunVars.PlotTraces[0], "%d %d B", RunVars.ActiveBrd, cc);
			else Con_printf("Sm", "Active Board = %d\n", RunVars.ActiveBrd);
		}
	}
	if (c == 'c') {
		int new_ch;
		if (!SockConsole) {
			char chs[10];
			printf("Current Active Channel = %d\n", cc);
			printf("New Active Channel ");
			myscanf("%s", &chs);
			if (isdigit(chs[0])) sscanf(chs, "%d", &new_ch);
			else new_ch = xy2ch(toupper(chs[0]) - 'A', chs[1] - '1');
		} else {
			Con_GetInt(&new_ch);
		}
		if ((new_ch >= 0) && (new_ch < MAX_NCH)) {
			//RunVars.ActiveCh = new_ch;
			sprintf(RunVars.PlotTraces[0], "%d %d B", RunVars.ActiveBrd, new_ch);
		}
	}
	if (c == '+') {
		static int init = 1;
		int next_ch;
		if (init) {
			printf("Set Channel ");
			myscanf("%d", &next_ch);
			init = 0;
		} else {
			next_ch = cc + 1;
		}
		if (next_ch < WDcfg.NumCh) {
			printf("Current Active Channel = %d\n", next_ch);
			Con_getch();
			sprintf(RunVars.PlotTraces[0], "%d %d B", RunVars.ActiveBrd, next_ch);
			SaveRunVariables(RunVars);
			if (next_ch < 32) {
				WDcfg.ChEnableMask0[0] = 1 | (1 << next_ch);
				WDcfg.ChEnableMask1[0] = 0;
			} else {
				WDcfg.ChEnableMask0[0] = 1;
				WDcfg.ChEnableMask1[0] = (1 << (next_ch - 32));
			}
			AcqStatus = ACQSTATUS_RESTARTING;
			StopRun();
			ResetStatistics();
			ConfigureFERS(handle[0], CFG_HARD);
			StartRun();
		}
	}
	if ((c == 'm') && !SockConsole)
		ManualController(handle[RunVars.ActiveBrd]);

	if (c == 'r') {
		SaveRunVariables(RunVars);  
		ResetStatistics();
		SkipConfig = 1;
		RestartAcq = 1;
	}
	if (c == 'j') {
		if (WDcfg.EnableJobs) {
			if (AcqStatus == ACQSTATUS_RUNNING) StopRun();
			jobrun = WDcfg.JobFirstRun;
			RunVars.RunNumber = jobrun;
			SaveRunVariables(RunVars);
			return 100; // To let the main loop know that 'j' was pressed.
		}
	}
	if ((c == 'R') && (SockConsole)) {
		uint32_t addr, data;
		char str[10];
		char rw = Con_getch();
		if (rw == 'r') {
			Con_GetString(str, 8);
			sscanf(str, "%x", &addr);
			FERS_ReadRegister(handle[RunVars.ActiveBrd], addr, &data);
			Con_printf("Sm", "Read Reg: ADDR = %08X, DATA = %08X\n", addr, data);
			Con_printf("Sr", "%08X", data);
		}
		else if (rw == 'w') {
			Con_GetString(str, 8);
			sscanf(str, "%x", &addr);
			Con_GetString(str, 8);
			sscanf(str, "%x", &data);
			FERS_WriteRegister(handle[RunVars.ActiveBrd], addr, data);
			Con_printf("Sm", "Write Reg: ADDR = %08X, DATA = %08X\n", addr, data);
		}
	}
	if (c == 'f') {
		if (!SockConsole) Freeze ^= 1;
		else Freeze = (Con_getch() - '0') & 0x1;
		// OneShot = 1;
	}
	if (c == 'o') {
		Freeze = 1;
		OneShot = 1;
	}
	if (c == 'F') {
		if (CfgDataAnalysis == -1) {
			CfgDataAnalysis = WDcfg.DataAnalysis;
			WDcfg.DataAnalysis = 0;
			Con_printf("L", "Data Analysis disabled\n");
		} else {
			WDcfg.DataAnalysis = CfgDataAnalysis;
			CfgDataAnalysis = -1;
		}
	}
	if (c == 'C') {
		if (!SockConsole) {
			printf("\n\n");
			printf("Current stat monitor: %d\n", RunVars.SMonType);
			printf("0 = Hit Rate\n");
			printf("1 = Hit Cnt\n");
			printf("2 = Tot Rate\n");
			printf("3 = Tot Cnt\n");
			printf("4 = Lead Mean\n");
			printf("5 = Lead RMS\n");
			printf("6 = ToT Mean\n");
			printf("7 = ToT RMS\n");
			printf("[Other Keys] Return\n");
		}
		c = Con_getch() - '0';
		if ((c >= 0) && (c <= 7)) RunVars.SMonType = c;
	}
	if (c == 'P') {
		if (!SockConsole) {
			printf("\n\n");
			printf("Current plot type: %d\n", RunVars.PlotType);
			printf("0 = Spect Lead\n");
			printf("1 = Spect ToT\n");
			printf("2 = Counts\n");
			//printf("3 = 2D Count Rates\n");
			//printf("4 = Spect Trail\n");
			printf("[Other Keys] Return\n");
		}
		c = tolower(Con_getch());
		if ((c >= '0') && (c <= '9')) RunVars.PlotType = c - '0';
	}
	if (c == 'x') {
		if (SockConsole) RunVars.Xcalib = (Con_getch() - '0') & 0x1;
		else RunVars.Xcalib ^= 1;
		SaveRunVariables(RunVars);
	}
	if (c == '-') {
		if (bb > 0) {
			bb--;
			sprintf(RunVars.PlotTraces[0], "%d %d B", 0, bb);
		}
	}
	if (c == '+') {
		if (bb < (MAX_NCH - 1)) {
			bb++;
			sprintf(RunVars.PlotTraces[0], "%d %d B", 0, bb);
			// ConfigureProbe(handle[RunVars.ActiveBrd]);
		}
	}
	if (c == 'U') {	// Firmware upgrade
		int brd, as = AcqStatus;
		char fname[500] = "";
		FILE* fp;
		AcqStatus = ACQSTATUS_UPGRADING_FW;
		// if is in console mode use command input
		if (!SockConsole) printf("Insert the board num followed by firmware file name (2new_firmware.ffu): ");
		Con_GetInt(&brd);
		//if (!SockConsole) printf("Insert the firmware filename: ");
		Con_GetString(fname, 500);
		fname[strcspn(fname, "\n")] = 0; // In console mode GetString append \n at the end

		if ((FERS_CONNECTIONTYPE(handle[brd]) == FERS_CONNECTIONTYPE_TDL) && (FERS_FPGA_FW_MajorRev(handle[brd]) < 10)) { //
			AcqStatus = as;
			Sleep(1);
			Con_printf("LCSu", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: FERS Firwmare cannot be upgraded via TDLink. This function will be available with then next FW release.\n" COLOR_RESET);
			return 0;
		}

		fp = fopen(fname, "rb");
		if (!fp) {
			SendAcqStatusMsg("Failed to open file %s\n", fname);
			AcqStatus = as;
		} else {
			if ((brd >= 0) && (brd < WDcfg.NumBrd)) {
				int ret = FERS_FirmwareUpgrade(handle[brd], fp, reportProgress);
				if (ret == FERSLIB_ERR_INVALID_FWFILE) Con_printf("Sw", "Invalid Firmware Upgrade File");
				fclose(fp);
				RestartAll = 1;
			} else {
				Con_printf("Sw", "Invalid Board Number %d", brd);
				fclose(fp);
				AcqStatus = as;
			}
		}
	}

	if (c == '!') {
		int brd = 0, ret;
		if (WDcfg.NumBrd > 1) {
			if (!SockConsole) printf("Enter board index: ");
			Con_GetInt(&brd);
		}
		if ((brd >= 0) && (brd < WDcfg.NumBrd) && (FERS_CONNECTIONTYPE(handle[brd]) == FERS_CONNECTIONTYPE_USB)) {
			ret = FERS_Reset_IPaddress(handle[brd]);
			if (ret == 0) {
				Con_printf("LCSm", "Default IP address (192.168.50.3) has been restored\n");
			}
			else {
				Con_printf("CSw", "Failed to reset IP address\n");
			}
		} else {
			Con_printf("LCSm", "The IP address can be restored via USB connection only\n");
		}
	}
	if (c == '\t') {
		if (!SockConsole)
			StatMode ^= 1;
		else {
			Con_GetInt(&StatMode);
		}
	}
	if (c == 'g') {
		if (WDcfg.NumCh == 128) grp_ch64 ^= 1;
	}
	if (c == 'I') {
		if (!SockConsole)
			StatIntegral ^= 1;
		else 
			Con_GetInt(&StatIntegral);
		}
	if (c == 'T') {
		int ret;
		float ThrOffset[MAX_NCH], RMSnoise[MAX_NCH];
		int done[MAX_NCH];
		if (AcqStatus == ACQSTATUS_RUNNING) StopRun();
		ClearScreen();
		ret = CalibThresholdOffset(handle[RunVars.ActiveBrd], -10, +10, done, ThrOffset, RMSnoise);
		//if (!SockConsole) {
		if (ret == 0) {
			Con_printf("CSp", "Do you want to save the calibration to flash? [y/n]\n");
			if (Con_getch() == 'y')
				WriteThrCalibToFlash(handle[RunVars.ActiveBrd], AdapterNch(), ThrOffset);
		} else {
			Con_printf("CSw", "Threshold offset calibration failed! Press a key\n");
			Con_getch();
		}
		//}
		ClearScreen();
		RestartAll = 1;
	}

	if (c == '#') PrintMap();

	if ((c == ' ') && !SockConsole) {
		printf("[q] Quit\n");
		printf("[s] Start acquisition\n");
		printf("[S] Stop acquisition\n");
		printf("[t] SW trigger\n");
		printf("[C] Set stats monitor type\n");
		if (WDcfg.NumCh == 128) printf("[g] Change channel group 0..63 <-> 64..127\n");
		printf("[tab] Change statistics (channel/board)\n");
		printf("[i] Change statistics mode (integral/updating)\n");
		printf("[P] Set plot mode\n");
		printf("[x] Enable/Disable x-axis calibration\n");
		printf("[b] Change board\n");
		printf("[c] Change channel\n");
		printf("[f] Freeze plot\n");
		printf("[o] One shot plot\n");
		printf("[r] Reset histograms\n");
		printf("[j] Reset jobs (when enabled)\n");
		printf("[!] Reset IP address\n");
		printf("[U] Upgrade firmware\n");
		if (WDcfg.AdapterType != ADAPTER_NONE) printf("[T] Calibrate Discriminator Thresholds\n");
		printf("[m] Register manual controller\n");
		//printf("[#] Print Pixel Map\n");
		c = Con_getch();
		RunTimeCmd(c);
	}

	return 0;
}

int report_firmware_notfound(int b, int as) {
	int c;
	// sprintf(ErrorMsg, "The firmware of the FPGA in board %d cannot be loaded.\nPlease, try to re-load the firmware running on shell the command\n'./%s -u %s firmware_filename.ffu'\n", b, EXE_NAME, WDcfg.ConnPath[b]);
	Con_printf("LCSF", "Cannot read board info for board %d. Do you want to reload the FPGA ffu firmware? [y][n]\n", b);
	while (!(c = Con_getch()));
	if (c == 'y') {
		AcqStatus = ACQSTATUS_UPGRADING_FW;
		// Get file and call FERSFWUpgrade
		if (!SockConsole) {
			Con_printf("LCSm", "Please, insert the board ID followed by the path of the ffu firmware you want to load (i.e: Firmware\\a5203.ffu:\n");
			AcqStatus = ACQSTATUS_UPGRADING_FW;
		}
		char fname[500] = "";
		if (SockConsole) while (!(c = Con_getch()));  // Is not needed, but it maintains compatibility with Python
		Con_GetString(fname, 500);
		fname[strcspn(fname, "\n")] = 0; // In console mode GetString append \n at the end

		FILE* fp = fopen(fname, "rb");
		if (!fp) {
			Con_printf("CSm", "Failed to open file %s\n", fname);
			return -1;
		}
		int ret = FERS_FirmwareUpgrade(handle[b], fp, reportProgress);
		if (ret == 0)
			if (!SockConsole) Con_printf("LCSm", "Firmware upgrade completed, press any keys to quit\n");
			else Con_printf("L", "Firmware upgrade failed, press any keys to quit\n");
		c = Con_getch();
		AcqStatus = as;
		return ret;
	} else {
		return -2; // 'n' (or any other key a the moment) has been selected
	}
}

// ******************************************************************************************
// MAIN
// ******************************************************************************************



int orginal_main(int argc, char* argv[])
{
	int i = 0, jobrun = 0, ret = 0, clrscr = 0, dtq, ch, b, cnc, rdymsg, edge;
	int snd_fpga_warn = 0;
	int PresetReached = 0;
	int nb = 0;
	double tstamp_us, curr_tstamp_us = 0;
	int MinFWrev = 255; 
	uint64_t Tref;
	uint64_t kb_time, curr_time, print_time; // , wave_time;
	char ConfigFileName[500] = CONFIG_FILENAME;
	char fwupg_fname[500] = "", fwupg_conn_path[20] = "";
	char PixelMapFileName[500] = PIXMAP_FILENAME; //  CTIN: get name from config file
	char Temp2GUI[1024] = ""; // The temperatures are sent in a single send
	ListEvent_t *Event;
	float elapsedPC_s;
	float elapsedBRD_s;
	FILE* cfg;
	int a1, a2, AllocSize;
	int ROmode;

	// ToA (or deltaT) after event data processing (rescale + time difference). Data for output file
	// ToT after event data processing (rescale + time difference). Data for output file
	// The pointers are allocated once to avoid the overhead of the malloc
	// Common Start/Stop
	uint64_t* of_DeltaT = (uint64_t*)calloc(MAX_NCH, sizeof(uint64_t));
	uint16_t* of_ToTDeltaT = (uint16_t*)calloc(MAX_NCH, sizeof(uint16_t));
	// Trg Matching/Streaming
	uint64_t* of_ToA = (uint64_t*)calloc(MAX_LIST_SIZE, sizeof(uint64_t));
	uint16_t* of_ToT = (uint16_t*)calloc(MAX_LIST_SIZE, sizeof(uint16_t));

	// Get command line options
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i] + 1, "g") == 0) SockConsole = 1;
			if (argv[i][1] == 'c') strcpy(ConfigFileName, &argv[i][2]);
			if (argv[i][1] == 'u') {
				if (argc > (i + 2)) {
					strcpy(fwupg_conn_path, argv[i + 1]);
					strcpy(fwupg_fname, argv[i + 2]);
					i += 2;
				}
			}
		}
	}

	MsgLog = fopen("MsgLog.txt", "w");
	ret = InitConsole(SockConsole, MsgLog);
	if (ret) {
		printf(FONT_STYLE_BOLD COLOR_RED "ERROR: init console failed\n" COLOR_RESET);
		exit(0);
	}
	memset(handle, -1, MAX_NBRD * sizeof(int));	// DNIN: Is it needed?

	// Check if a FW upgrade has been required
	if ((strlen(fwupg_fname) > 0) && (strlen(fwupg_conn_path) > 0)) {
		FILE* fp = fopen(fwupg_fname, "rb");
		if (!fp) {
			Con_printf("CSm", "Failed to open file %s\n", fwupg_fname);
			return -1;
		}
		ret = FERS_OpenDevice(fwupg_conn_path, &handle[0]);
		if (ret < 0) {
			Con_printf("CSm", FONT_STYLE_BOLD COLOR_RED "ERROR: can't open FERS at %s\n" COLOR_RESET, fwupg_conn_path);
			return -1;
		}

		if ((FERS_CONNECTIONTYPE(handle[0]) == FERS_CONNECTIONTYPE_TDL) && (FERS_FPGA_FW_MajorRev(handle[0]) < 10)) {
			Con_printf("LCSu", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: FERS Firwmare cannot be upgraded via TDLink. This function will be available with then next FW release.\n" COLOR_RESET);
			Sleep(10000);
			return 0;
		}

		ret = FERS_FirmwareUpgrade(handle[0], fp, reportProgress);
		fclose(fp);
		return 0;
	}

	AcqStatus = ACQSTATUS_SOCK_CONNECTED;
	if (SockConsole) SendAcqStatusMsg("JanusC connected. Release %s (%s).", SW_RELEASE_NUM, SW_RELEASE_DATE);

	Con_printf("LCSm", "*************************************************\n");
	Con_printf("LCSm", "JanusC Rev %s (%s)\n", SW_RELEASE_NUM, SW_RELEASE_DATE);
	Con_printf("LCSm", "Readout Software for CAEN FERS-5200\n");
	Con_printf("LCSm", "*************************************************\n");

ReadCfg:
	// -----------------------------------------------------
	// Parse config file
	// -----------------------------------------------------
	cfg = fopen(ConfigFileName, "r");
	if (cfg == NULL) {
		sprintf(ErrorMsg, "Can't open configuration file %s\n", ConfigFileName);
		goto ManageError;
	}
	Con_printf("LCSm", "Reading configuration file %s\n", ConfigFileName);
	ret = ParseConfigFile(cfg, &WDcfg, 1);
	fclose(cfg);
	LoadRunVariables(&RunVars);
	if (WDcfg.EnableJobs) {
		jobrun = WDcfg.JobFirstRun;
		RunVars.RunNumber = jobrun;
		SaveRunVariables(RunVars);
	}
	FERS_SetDebugLogs(WDcfg.DebugLogMask);

	// -----------------------------------------------------
	// Read pixel map (channel (0:63) to pixel[x][y] 
	// -----------------------------------------------------
	Con_printf("LCSm", "Reading Pixel Map %s\n", PixelMapFileName);
	if (Read_ch2xy_Map(PixelMapFileName) < 0)
		Con_printf("LCSm", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: Map File not found. Sequential mapping will be used\n" COLOR_RESET);

	// -----------------------------------------------------
	// Connect To Boards
	// -----------------------------------------------------
	memset(handle, -1, sizeof(*handle) * MAX_NBRD);
	memset(cnc_handle, -1, sizeof(*handle) * MAX_NCNC);
	cnc = 0;
	for (b = 0; b < WDcfg.NumBrd; b++) {
		FERS_BoardInfo_t BoardInfo;
		char* cc, cpath[100];
		if (((cc = strstr(WDcfg.ConnPath[b], "tdl")) != NULL)) {  // TDlink used => Open connection to concentrator (this is not mandatory, it is done for reading information about the concentrator)
			FERS_Get_CncPath(WDcfg.ConnPath[b], cpath);			
			if (!FERS_IsOpen(cpath)) {
				FERS_CncInfo_t CncInfo;
				Con_printf("LCSm", "\n--------------- Concentrator %2d ----------------\n", cnc);
				Con_printf("LCSm", "Opening connection to %s\n", cpath);
				ret = FERS_OpenDevice(cpath, &cnc_handle[cnc]);
				if (ret == 0) {
					Con_printf("LCSm", "Connected to Concentrator %s\n", cpath);
				} else {
					sprintf(ErrorMsg, "Can't open concentrator at %s\n", cpath);
					goto ManageError;
				}
				if (!FERS_TDLchainsInitialized(cnc_handle[cnc])) {
					Con_printf("LCSm", "Initializing TDL chains. This will take a few seconds...\n", cpath);
					if (SockConsole) SendAcqStatusMsg("Initializing TDL chains. This may take a few seconds...");
				}
				ret = FERS_InitTDLchains(cnc_handle[cnc], WDcfg.FiberDelayAdjust[cnc]);
				if (ret != 0) {
					sprintf(ErrorMsg, "Failure in TDL chain init\n");
					goto ManageError;
				}
				ret |= FERS_ReadConcentratorInfo(cnc_handle[cnc], &CncInfo);
				if (ret == 0) {
					Con_printf("LCSm", "FPGA FW revision = %s\n", CncInfo.FPGA_FWrev);
					Con_printf("LCSm", "SW revision = %s\n", CncInfo.SW_rev);
					Con_printf("LCSm", "PID = %d\n", CncInfo.pid);
					if (CncInfo.ChainInfo[0].BoardCount == 0) { 	// Rising error if no board is connected to link 0
						sprintf(ErrorMsg, "No board connected to link 0\n");
						goto ManageError;
					}
					for (i = 0; i < 8; i++) {
						if (CncInfo.ChainInfo[i].BoardCount > 0)
							Con_printf("LCSm", "Found %d board(s) connected to TDlink n. %d\n", CncInfo.ChainInfo[i].BoardCount, i);
					}
				} else {
					sprintf(ErrorMsg, "Can't read concentrator info\n");
					goto ManageError;
				}
				cnc++;
			}
		}
		if ((WDcfg.NumBrd > 1) || (cnc > 0)) Con_printf("LCSm", "\n------------------ Board %2d --------------------\n", b);
		Con_printf("LCSm", "Opening connection to %s\n", WDcfg.ConnPath[b]);
		ret = FERS_OpenDevice(WDcfg.ConnPath[b], &handle[b]);
		if (ret == 0) {
			Con_printf("LCSm", "Connected to %s\n", WDcfg.ConnPath[b]);
			//int bootL;
			//uint32_t ver, rel;
			//FERS_CheckBootloaderVersion(handle[b], &bootL, &ver, &rel);
			//if (bootL) {
			//	report_firmware_notfound(b);
			//	goto ManageError;
			//}
		} else {
			sprintf(ErrorMsg, "Can't open board %d at %s\n", b, WDcfg.ConnPath[b]);
			//report_firmware_notfound(b);
			goto ManageError;
		}
		ret = FERS_ReadBoardInfo(handle[b], &BoardInfo);
		if (ret == 0) {
			char fver[100];
			if (BoardInfo.FPGA_FWrev == 0) sprintf(fver, "BootLoader");
			else sprintf(fver, "%d.%d (Build = %04X)", (BoardInfo.FPGA_FWrev >> 8) & 0xFF, BoardInfo.FPGA_FWrev & 0xFF, (BoardInfo.FPGA_FWrev >> 16) & 0xFFFF);
			WDcfg.NumCh = BoardInfo.NumCh;
			//WDcfg.NumCh = 64; // FBER: Only for debug
			MinFWrev = min((int)(BoardInfo.FPGA_FWrev >> 8) & 0xFF, MinFWrev);
			Con_printf("LCSm", "FPGA FW revision = %s\n", fver);
			if (strstr(WDcfg.ConnPath[b], "tdl") == NULL)
				Con_printf("LCSm", "uC FW revision = %08X\n", BoardInfo.uC_FWrev);
			Con_printf("LCSm", "PID = %d (Model = %s)\n", BoardInfo.pid, BoardInfo.ModelName);
			if (SockConsole) {
				if (strstr(WDcfg.ConnPath[b], "tdl") == NULL) Con_printf("Si", "%d;%d;%s;%s;%08X", b, BoardInfo.pid, BoardInfo.ModelName, fver, BoardInfo.uC_FWrev); // ModelName for firmware upgrade
				else Con_printf("Si", "%d;%d;%s;%s;N.A.", b, BoardInfo.pid, BoardInfo.ModelName, fver);
			}
			if ((BoardInfo.FPGA_FWrev > 0) && ((BoardInfo.FPGA_FWrev & 0xFF00) < 1) && (BoardInfo.FERSCode == 5202)) {
				sprintf(ErrorMsg, "Your FW revision is %d.%d; must be 1.X or higher\n", (BoardInfo.FPGA_FWrev >> 8) & 0xFF, BoardInfo.FPGA_FWrev & 0xFF);
				goto ManageError;
			}
		} else {
			sprintf(ErrorMsg, "Can't read board info\n");
			int ret_rep = report_firmware_notfound(b, AcqStatus);
			if (ret_rep == 0) { // Firmware updated succesfully
				Quit = 1;
				goto ExitPoint;
			} else {
				goto ManageError;
			}
		}

		// Load threshold calibration of the Adapter (if present)
		if (WDcfg.AdapterType != ADAPTER_NONE) {
			char date[100];
			if (ReadThrCalibFromFlash(handle[b], AdapterNch(), date, NULL) == 0) 
				Con_printf("LCSm", "Loaded Threshold calibration from flash (%s)\n", date);
		}
	}

	if ((WDcfg.NumBrd > 1) || (cnc > 0))  Con_printf("LCSm", "\n");
	if (AcqStatus != ACQSTATUS_RESTARTING) {
		AcqStatus = ACQSTATUS_HW_CONNECTED;
		SendAcqStatusMsg("Num of connected boards = %d", WDcfg.NumBrd);
	}

	// -----------------------------------------------------
	// Allocate memory buffers and histograms, open files
	// -----------------------------------------------------
	ROmode = (WDcfg.EventBuildingMode != 0) ? 1 : 0;
	for (b = 0; b < WDcfg.NumBrd; b++)
		FERS_InitReadout(handle[b], ROmode, &a1);
	CreateStatistics(WDcfg.NumBrd, FERSLIB_MAX_NCH, &a2);
	AllocSize = a2 + FERS_TotalAllocatedMemory();
	Con_printf("LCSm", "Total allocated memory = %.2f MB\n", (float)AllocSize / (1024 * 1024));

	// -----------------------------------------------------
	// Open plotter
	// -----------------------------------------------------
	OpenPlotter();

// +++++++++++++++++++++++++++++++++++++++++++++++++++++
Restart:  // when config file changes or a new run of the job is scheduled, the acquisition restarts here // BUG: it does not restart when job is enable, just when preset time or count is active
// +++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (!PresetReached)
		ResetStatistics(); // should no happened if Job enable is not on
	else
		PresetReached = 0;

	LoadRunVariables(&RunVars);
	if (WDcfg.EnableJobs) {
		//increase_job_run_number();
		job_read_parse();
	}

	// -----------------------------------------------------
	// Configure Boards
	// -----------------------------------------------------
	if (!SkipConfig) {
		Con_printf("LCSm", "Configuring %d boards.\n", WDcfg.NumBrd);
		for (b = 0; b < WDcfg.NumBrd; b++) {
			ret = ConfigureFERS(handle[b], CFG_HARD);
			if (ret < 0) {
				Con_printf("LCSm", "Failed!!!\n");
				Con_printf("LCSm", "%s", ErrorMsg);
				goto ManageError;
			} else {
				Con_printf("LCSm", "Board %d configured.\n", b);
			}
		}
		if (FERS_CONNECTIONTYPE(handle[0]) == FERS_CONNECTIONTYPE_TDL)
			FERS_SendCommandBroadcast(handle, CMD_RES_PTRG, 0);
		Con_printf("LCSm", "Done.\n");
	}
	SkipConfig = 0;
	
	// Send some info to GUI
	if (SockConsole) {
		for (int b = 0; b < WDcfg.NumBrd; ++b) {
			int retc = Update_Service_Info(handle[b]);
			sprintf(Temp2GUI, "%s %d %6.1f %6.1f %6.1f %6.1f %d", Temp2GUI, b, BrdTemp[b][TEMP_TDC0], BrdTemp[b][TEMP_TDC1], BrdTemp[b][TEMP_BOARD], BrdTemp[b][TEMP_FPGA], retc);
		}
		Con_printf("ST", "%s", Temp2GUI);
		change_monitor_stat_gui();
	}

	// ###########################################################################################
	// Readout Loop
	// ###########################################################################################
	// Start Acquisition
	if ((AcqStatus == ACQSTATUS_RESTARTING) || ((WDcfg.EnableJobs && (jobrun > WDcfg.JobFirstRun) && (jobrun <= WDcfg.JobLastRun)))) {
		if (WDcfg.EnableJobs) Sleep((int)(WDcfg.RunSleep * 1000));
		StartRun();
		if (!SockConsole) ClearScreen();
	}
	else if (AcqStatus != ACQSTATUS_RUNNING) AcqStatus = ACQSTATUS_READY;

	curr_time = get_time();
	print_time = curr_time - 2000;  // force 1st print with no delay
	//wave_time = curr_time;
	kb_time = curr_time;
	rdymsg = 1;
	PresetReached = 0;
	build_time_us = 0;
	Tref = 0;
	while (!Quit && !RestartAcq && !PresetReached && !RestartAll) {

		curr_time = get_time();
		Stats.current_time = curr_time;
		nb = 0;

		// ---------------------------------------------------
		// Check for commands from console or changes in cfg files
		// ---------------------------------------------------
		if ((curr_time - kb_time) > 200) {
			int upd = CheckFileUpdate();
			if (upd == 1) {
				if (!SockConsole) clrscr = 1;
				rdymsg = 1;
				int curr_state = AcqStatus;
				// Import form 5202 code the Enabling live PramaChange ?
				if (curr_state == ACQSTATUS_RUNNING) StopRun();
				Con_printf("LCSm", "Configuring Boards ... ");
				for (b = 0; b < WDcfg.NumBrd; b++) {
					ret = ConfigureFERS(handle[b], CFG_SOFT);
					if (ret < 0) {
						Con_printf("LCSm", FONT_STYLE_BOLD COLOR_RED "Failed!!!\n" COLOR_RESET);
						Con_printf("LCSm", FONT_STYLE_BOLD COLOR_RED "%s" COLOR_RESET, ErrorMsg);
						goto ManageError;
					}
					FERS_FlushData(handle[b]);
				}
				Con_printf("LCSm", "Done\n");
				if (curr_state == ACQSTATUS_RUNNING) StartRun();
			} else if (upd == 2) {
				int size;
				DestroyStatistics();
				CreateStatistics(WDcfg.NumBrd, FERSLIB_MAX_NCH, &size);
				RestartAcq = 1;
			} else if (upd == 3) {
				RestartAll = 1;
			}
			if (Con_kbhit()) {
				int tret = RunTimeCmd(Con_getch()); // reset job
				clrscr = 1;
				rdymsg = 1;
				if (tret == 100) {
					kb_time = curr_time;
					goto Restart;
				}			
			}
			kb_time = curr_time;
		}

		// ---------------------------------------------------
		// Read Data from the boards
		// ---------------------------------------------------
		if (AcqStatus == ACQSTATUS_RUNNING) {
			ret = FERS_GetEvent(handle, &b, &dtq, &tstamp_us, (void **)(&Event), &nb);
			if (nb > 0) curr_tstamp_us = tstamp_us;
			//else Sleep(1);
			if (ret < 0) {
				sprintf(ErrorMsg, "Readout failure (ret = %d)!\n", ret);
				goto ManageError;
			}
			elapsedPC_s = (Stats.current_time > Stats.start_time) ? ((float)(Stats.current_time - Stats.start_time)) / 1000 : 0;
			elapsedBRD_s = (float)(Stats.current_tstamp_us[0] * 1e-6);
			if ((WDcfg.StopRunMode == STOPRUN_PRESET_TIME) && ((elapsedBRD_s > WDcfg.PresetTime) || (elapsedPC_s > (WDcfg.PresetTime + 1)))) {
				Stats.stop_time = Stats.start_time + (uint64_t)(WDcfg.PresetTime * 1000);
				StopRun();
				PresetReached = 1; // Preset time reached; quit readout loop
			} else if ((WDcfg.StopRunMode == STOPRUN_PRESET_COUNTS) && (Stats.ReadTrgCnt[0].cnt >= (uint32_t)WDcfg.PresetCounts)) {  // Stop on board 0 counts
				StopRun();
				PresetReached = 1; // Preset counts reached; quit readout loop
			}
		}
		if ((nb > 0) && !PresetReached) {
			Stats.current_tstamp_us[b] = curr_tstamp_us;
			Stats.ByteCnt[b].cnt += (uint32_t)nb;
			if (dtq != DTQ_SERVICE) 
				Stats.ReadTrgCnt[b].cnt++;
			if ((curr_tstamp_us > (build_time_us + 0.001 * WDcfg.TstampCoincWindow)) && (WDcfg.EventBuildingMode != EVBLD_DISABLED)) {
				if (build_time_us > 0) Stats.BuiltEventCnt.cnt++;
				build_time_us = curr_tstamp_us;
			}

			// ---------------------------------------------------
			// update statistics and spectra and save list files 
			// ---------------------------------------------------
			if (((dtq & 0xF) == DTQ_TIMING) && (WDcfg.DataAnalysis != 0)) {
				uint32_t DTb=0, ToTb=0;

				Stats.current_trgid[b] = Event->trigger_id;
				Stats.BoardHitCnt[b].cnt += Event->nhits;
				if (!(WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS) && !(WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO)) {  // count only
					for (int i = 0; i < Event->nhits; i++) {
						if (WDcfg.AdapterType == ADAPTER_NONE) ch = Event->channel[i];
						else ChIndex_tdc2ada(Event->channel[i], &ch);
						Stats.ReadHitCnt[b][ch].cnt++;
					}
				} else {

				// ******** COMMON START/STOP ***********
					// Measurement of deltaT between Ch[n] and Tref=CH[0]. Only 1st leading edge is considered (no multi-hit).
					// Any hit outside the gate will be discarded. LeadHisto is populated with deltaT, TrailHisto is not populated.
					// ToTHisto is populated with either ToT from OWLT or Trail-Lead. NOTE: both edges must fall within the gate.
					if ((WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)) {
						int Lead[MAX_NCH], Trail[MAX_NCH];  // First (Cstart) or Last (Cstop) leading/traling edge; -1 = not found
						memset(Lead, -1, MAX_NCH * sizeof(int));
						memset(Trail, -1, MAX_NCH * sizeof(int));
						for (int s = 0; s < Event->nhits; s++) {
							int i = (WDcfg.AcquisitionMode == ACQMODE_COMMON_START) ? s : Event->nhits - s - 1;  // index (CSTART = ascending, CSTOP = descending)
							if (WDcfg.AdapterType == ADAPTER_NONE) ch = Event->channel[i];
							else ChIndex_tdc2ada(Event->channel[i], &ch);
							Stats.ReadHitCnt[b][ch].cnt++;
							if (Event->edge[i] == (EDGE_LEAD ^ WDcfg.InvertEdgePolarity[b][ch])) {
								if ((ch == 0) && (b == 0 || WDcfg.TestMode)) Tref = Event->tstamp_clk * CLK2LSB + Event->ToA[i];
								if (Lead[ch] == -1)	Lead[ch] = i;
							} else {
								if (Trail[ch] == -1) Trail[ch] = i;
							}
						}

						for (ch = 0; ch < MAX_NCH; ch++) {
							int IsChRef = ((ch == 0) && (b == 0));
							if (Lead[ch] != -1) {
								uint64_t abstime = Event->tstamp_clk * CLK2LSB + Event->ToA[Lead[ch]];
								int tmp_DT = (WDcfg.AcquisitionMode == ACQMODE_COMMON_START) ? int(abstime - Tref) : int(Tref - abstime);
								if (tmp_DT < 0) { // Stop/Start out of the begin of the gate - (abstime < Tref && WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (abstime > Tref && WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)
									of_DeltaT[ch] = 0;
									of_ToTDeltaT[ch] = 0;
									continue;
								}
								if (IsChRef) of_DeltaT[ch] = Event->ToA[Lead[ch]];
								else of_DeltaT[ch] = tmp_DT;
								of_DeltaT[ch] = of_DeltaT[ch] >> WDcfg.LeadTrail_Rescale;
								if (IsChRef || ((of_DeltaT[ch] * WDcfg.LeadTrail_LSB_ns) <= (uint32_t(WDcfg.GateWidth)))) {
									Stats.MatchHitCnt[b][ch].cnt++;
									if (Trail[ch] != -1) {
										uint32_t t_tot = (Event->ToA[Trail[ch]] - Event->ToA[Lead[ch]]) >> WDcfg.ToT_Rescale;
										of_ToTDeltaT[ch] = ((t_tot & 0xFFFF0000)>>16 == 0) ? t_tot : 0xFFFF;
										if (WDcfg.DataAnalysis & DATA_ANALYSIS_CNT) Stats.MatchHitCnt[b][ch].cnt++;
									} else {  //if (Event->ToT[Lead[ch]] > 0) {
										of_ToTDeltaT[ch] = Event->ToT[Lead[ch]] >> WDcfg.ToT_Rescale;
										if (((WDcfg.MeasMode == MEASMODE_LEAD_TOT8) && (of_ToTDeltaT[ch] == 0xFF)) || ((WDcfg.MeasMode == MEASMODE_LEAD_TOT11) && (of_ToTDeltaT[ch] == 0x7FFF)))
											of_ToTDeltaT[ch] = 0xFFFF;
									}
									// Apply walk correction 
									if ((WDcfg.EnableWalkCorrection) && !IsChRef && (of_ToTDeltaT[ch] != 0xFFFF)) {
										double ToT_ns, walk = 0;
										ToT_ns = of_ToTDeltaT[ch] * WDcfg.ToT_LSB_ns;
										for (int c = 0; c < 6; c++)
											walk += WDcfg.WalkFitCoeff[c] * pow(ToT_ns, c);
										of_DeltaT[ch] = of_DeltaT[ch] - (int32_t)(walk / WDcfg.LeadTrail_LSB_ns);
									}
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS) {
										if (!IsChRef && (of_DeltaT[ch] > 0)) AddMeasure(&Stats.LeadMeas[b][ch], of_DeltaT[ch] * WDcfg.LeadTrail_LSB_ns);
										if (of_ToTDeltaT[ch] > 0) AddMeasure(&Stats.ToTMeas[b][ch], of_ToTDeltaT[ch] * WDcfg.ToT_LSB_ns);
									}
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
										if (!IsChRef) {
											DTb = Leadbin((uint32_t)of_DeltaT[ch]);
											Histo1D_AddCount(&Stats.H1_Lead[b][ch], DTb);
										}
										if (of_ToTDeltaT[ch] > 0) {
											ToTb = ToTbin(of_ToTDeltaT[ch]);
											if (ToTb > 0) Histo1D_AddCount(&Stats.H1_ToT[b][ch], ToTb);
										}
									}
								}
							} else {
								of_DeltaT[ch] = 0;
								of_ToTDeltaT[ch] = 0;
							}
						}

					// ******** TRIGGER MATCHING ***********
					// Hit selection with trigger window. Multi-hit supported.
					// LeadHisto populated with deltaT between 1st edge following edges (if present). Same for Trail.
					// ToTHisto is populated with either ToT from OWLT or Trail-Lead. NOTE: both edges must fall within the trigger window.
					} else if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING) {
						uint32_t PrevL[MAX_NCH] = { 0 }, PrevT[MAX_NCH] = { 0 };
						for (i = 0; i < Event->nhits; i++) {
							if (WDcfg.AdapterType == ADAPTER_NONE) ch = Event->channel[i];
							else ChIndex_tdc2ada(Event->channel[i], &ch);
							edge = Event->edge[i] ^ WDcfg.InvertEdgePolarity[b][ch];
							Stats.ReadHitCnt[b][ch].cnt++;
							Stats.MatchHitCnt[b][ch].cnt++;
							if (edge == EDGE_LEAD) {
								if (PrevL[ch] > 0) {
									uint32_t lead = (Event->ToA[i] - PrevL[ch]) >> WDcfg.LeadTrail_Rescale;
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
										AddMeasure(&Stats.LeadMeas[b][ch], lead * WDcfg.LeadTrail_LSB_ns);
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
										DTb = Leadbin(lead);
										Histo1D_AddCount(&Stats.H1_Lead[b][ch], DTb);
									}
								} else PrevL[ch] = Event->ToA[i];
							} else {  // TRAIL
								if (PrevT[ch] > 0) {
									uint32_t trail = (Event->ToA[i] - PrevT[ch]) >> WDcfg.LeadTrail_Rescale;
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
										AddMeasure(&Stats.TrailMeas[b][ch], trail * WDcfg.LeadTrail_LSB_ns);
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
										DTb = Trailbin(trail);
										Histo1D_AddCount(&Stats.H1_Trail[b][ch], DTb);
									}
								}
								PrevT[ch] = Event->ToA[i];
							}
							if (MEASMODE_OWLT(WDcfg.MeasMode)) {
								of_ToT[i] = Event->ToT[i] >> WDcfg.ToT_Rescale;
								if (((WDcfg.MeasMode == MEASMODE_LEAD_TOT8) && (of_ToT[i] == 0xFF)) || ((WDcfg.MeasMode == MEASMODE_LEAD_TOT11) && (of_ToT[i] == 0x7FF)))
									of_ToT[i] = 0xFFFF;
							} else if ((edge == EDGE_TRAIL) && (PrevL[ch] > 0)) {
								uint32_t t_tot = (Event->ToA[i] - PrevL[ch]) >> WDcfg.ToT_Rescale;
								of_ToT[i] = ((t_tot & 0xFFFF0000) >> 16 == 0) ? t_tot : 0xFFFF;
							}
							if (of_ToT[i] > 0) {
								if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
									AddMeasure(&Stats.ToTMeas[b][ch], of_ToT[i] * WDcfg.ToT_LSB_ns);
								if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
									ToTb = ToTbin(of_ToT[i]);
									if (ToTb > 0) Histo1D_AddCount(&Stats.H1_ToT[b][ch], ToTb);
								}
							}
							of_ToA[i] = Event->ToA[i] >> WDcfg.LeadTrail_Rescale;
						}

					// ******** STREAMING ***********
					// No hit selection, continuous acquisition.
					// LeadHisto populated with deltaT between one edge and the following one. Same for Trail. 
					// ToTHisto is populated with either ToT from OWLT or Trail-Lead. 
					} else if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
						static uint64_t PrevL[MAX_NCH] = { 0 }, PrevT[MAX_NCH] = { 0 };
						for (i = 0; i < Event->nhits; i++) {
							uint64_t ToA_64 = Event->tstamp_clk * CLK2LSB + Event->ToA[i];
							if (WDcfg.AdapterType == ADAPTER_NONE) ch = Event->channel[i];
							else ChIndex_tdc2ada(Event->channel[i], &ch);
							edge = Event->edge[i] ^ WDcfg.InvertEdgePolarity[b][ch];
							Stats.ReadHitCnt[b][ch].cnt++;
							Stats.MatchHitCnt[b][ch].cnt++;
							if (edge == EDGE_LEAD) {
								if (PrevL[ch] > 0) {
									uint64_t lead = (ToA_64 - PrevL[ch]) >> WDcfg.LeadTrail_Rescale;
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
										AddMeasure(&Stats.LeadMeas[b][ch], lead * WDcfg.LeadTrail_LSB_ns);
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
										DTb = Leadbin64(lead);
										Histo1D_AddCount(&Stats.H1_Lead[b][ch], DTb);
									}
								}
								PrevL[ch] = ToA_64;
							} else {
								if (PrevT[ch] > 0) {
									uint64_t trail = (ToA_64 - PrevT[ch]) >> WDcfg.LeadTrail_Rescale;
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
										AddMeasure(&Stats.TrailMeas[b][ch], trail * WDcfg.LeadTrail_LSB_ns);
									if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
										DTb = Trailbin64(trail);
										Histo1D_AddCount(&Stats.H1_Trail[b][ch], DTb);
									}
								}
								PrevT[ch] = ToA_64;
							}
							if (!MEASMODE_OWLT(WDcfg.MeasMode) && (edge == EDGE_TRAIL) && (PrevL[ch] > 0)) {
								uint64_t t_tot = ((ToA_64 - PrevL[ch]) >> WDcfg.ToT_Rescale);
								of_ToT[i] = ((t_tot & 0xFFFFFFFFFFFF0000) >> 16 == 0) ? (uint16_t)t_tot : 0xFFFF;
							} else {
								of_ToT[i] = Event->ToT[i] >> WDcfg.ToT_Rescale;
								if (((WDcfg.MeasMode == MEASMODE_LEAD_TOT8) && (of_ToTDeltaT[i] == 0xFF)) || ((WDcfg.MeasMode == MEASMODE_LEAD_TOT11) && (of_ToTDeltaT[i] == 0x7FF)))
									of_ToT[i] = 0xFFFF;
							}
							if (of_ToT[i] > 0) {
								if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS)
									AddMeasure(&Stats.ToTMeas[b][ch], of_ToT[i] * WDcfg.ToT_LSB_ns);
								if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
									ToTb = ToTbin(of_ToT[i]);
									if (ToTb > 0) Histo1D_AddCount(&Stats.H1_ToT[b][ch], ToTb);
								}
							}
							of_ToA[i] = ToA_64 >> WDcfg.LeadTrail_Rescale;
						}
					}
					// Save List Files
					if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_ASCII) || (WDcfg.OutFileEnableMask & OUTFILE_LIST_BIN) || (WDcfg.OutFileEnableMask & OUTFILE_SYNC)) {
						if ((WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP))
							SaveList(b, Stats.current_tstamp_us[b], Stats.current_trgid[b], Event, of_DeltaT, of_ToTDeltaT, dtq);
						else
							SaveList(b, Stats.current_tstamp_us[b], Stats.current_trgid[b], Event, of_ToA, of_ToT, dtq);
					}
				}
			} else if (dtq == DTQ_SERVICE) {
				ServEvent_t* Ev = (ServEvent_t*)Event;
				memcpy(&sEvt[b], Ev, sizeof(ServEvent_t));
				Stats.last_serv_tstamp_us[b] = Ev->tstamp_us;
				Stats.TotTrgCnt[b].cnt = Ev->TotTrg_cnt;
				Stats.LostTrgCnt[b].cnt = Ev->RejTrg_cnt;
				Stats.SupprTrgCnt[b].cnt = Ev->SupprTrg_cnt;
			}

			// Count lost triggers (per board). NOTE: counting lost trigger by means of the gaps in trigger id doesn't work in case of trigger zero suppression
			/*if ((Stats.current_trgid[b] > 0) && (Stats.current_trgid[b] > (Stats.previous_trgid[b] + 1)))
				Stats.LostTrg[b].cnt += ((uint32_t)Stats.current_trgid[b] - (uint32_t)Stats.previous_trgid[b] - 1);
			Stats.previous_trgid[b] = Stats.current_trgid[b]; */
		}
		if (b == (WDcfg.NumBrd - 1)) Tref = 0;

		// ---------------------------------------------------
		// print stats to console
		// ---------------------------------------------------
		if (((curr_time - print_time) > 1000) || PresetReached) {
			char rinfo[100] = "", ror[20], totror[20], trr[20], tottrr[20], hitr[20], ss2gui[2048] = "", ss[MAX_NCH][10], stemp[4][10];
			//double lostp[MAX_NBRD], BldPerc[MAX_NBRD];
			float rtime, tp;
			static char stitle[8][30] = {
				"Matched Hit Rate (cps)", 
				"Matched Hit Cnt", 
				"Read Hit Rate (cps)", 
				"Read Hit Cnt",
				"Lead Mean",
				"Lead RMS",
				"ToT Mean",
				"ToT RMS"
			};
			// int ab = RunVars.ActiveBrd;

			// Check if the connection is lost and get service info (temperatures, etc...)
			sprintf(Temp2GUI, "");
			for (b = 0; b < WDcfg.NumBrd; b++) {
				ret = Update_Service_Info(handle[b]);
				sprintf(Temp2GUI, "%s %d %6.1f %6.1f %6.1f %6.1f %d", Temp2GUI, b, BrdTemp[b][TEMP_TDC0], BrdTemp[b][TEMP_TDC1], BrdTemp[b][TEMP_BOARD], BrdTemp[b][TEMP_FPGA], ret);
				if (ret < 0) {
					sprintf(ErrorMsg, "Lost Connection to brd %d", b);
					goto ManageError;
				} else if ((BrdTemp[b][TEMP_FPGA] > 83 && BrdTemp[b][TEMP_FPGA] < 200) || (BrdTemp[b][TEMP_TDC0] > 83 && BrdTemp[b][TEMP_TDC0] < 128)) {	// DNIN: more sofisticated actions can be taken
					char wmsg[200];
					sprintf(wmsg, "WARNING: Board %d is OVERHEATING. Please provide ventilation to prevent from permanent damages", b);
					if (snd_fpga_warn == 0) Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "%s\n" COLOR_RESET, wmsg);
					else Con_printf("LCSm", FONT_STYLE_BOLD COLOR_YELLOW "%s\n" COLOR_RESET, wmsg);
					snd_fpga_warn ^= 1;
				} else if (StatusReg[b] & STATUS_FAIL) {
					sprintf(ErrorMsg, "Board Failure (brd=%d, Status=%08X)", b, StatusReg[b]);
					goto ManageError;
				}
			}
			Con_printf("ST", "%s", Temp2GUI);

			if (WDcfg.StopRunMode == STOPRUN_PRESET_TIME) {
				if (WDcfg.EnableJobs) sprintf(rinfo, "(%d of %d, Preset = %.2f s)", jobrun - WDcfg.JobFirstRun + 1, WDcfg.JobLastRun - WDcfg.JobFirstRun + 1, WDcfg.PresetTime);
				else sprintf(rinfo, "(Preset = %.2f s)", WDcfg.PresetTime);
			} else if (WDcfg.StopRunMode == STOPRUN_PRESET_COUNTS) {
				if (WDcfg.EnableJobs) sprintf(rinfo, "(%d of %d, Preset = %d cnts)", jobrun - WDcfg.JobFirstRun + 1, WDcfg.JobLastRun - WDcfg.JobFirstRun + 1, WDcfg.PresetCounts);
				else sprintf(rinfo, "(Preset = %d cnts)", WDcfg.PresetCounts);
			} else {
				if (WDcfg.EnableJobs) sprintf(rinfo, "(%d of %d)", jobrun - WDcfg.JobFirstRun + 1, WDcfg.JobLastRun - WDcfg.JobFirstRun + 1);
				else sprintf(rinfo, "");
			}

			if ((AcqStatus == ACQSTATUS_READY) && rdymsg && !PresetReached) {
				SendAcqStatusMsg("Ready to start Run #%d %s", RunVars.RunNumber, rinfo);
				Con_printf("C", "Press [s] to start, [q] to quit, [SPACE] to enter the menu\n");
				rdymsg = 0;
			} else if (AcqStatus == ACQSTATUS_RUNNING) {
				if (!SockConsole) {
					if (clrscr) ClearScreen();
					clrscr = 0;
					gotoxy(1, 1);
				}

				UpdateStatistics(StatIntegral);

				// Calculate Total data throughput
				double totrate = 0;
				for (i = 0; i < WDcfg.NumBrd; ++i) {
					totrate += Stats.ByteCnt[i].rate;
					double2str(totrate, 1, totror);
				}

				if (PresetReached) {
					if (WDcfg.StopRunMode == STOPRUN_PRESET_TIME) rtime = WDcfg.PresetTime;
					tp = 100;
				} else {
					rtime = (float)(curr_time - Stats.start_time) / 1000;
					if (WDcfg.StopRunMode == STOPRUN_PRESET_TIME) tp = WDcfg.PresetTime > 0 ? 100 * rtime / WDcfg.PresetTime : 0;
					else tp = WDcfg.PresetCounts > 0 ? 100 * (float)Stats.ReadTrgCnt[0].cnt / WDcfg.PresetCounts : 0;
				}
				if (WDcfg.StopRunMode == STOPRUN_PRESET_TIME) SendAcqStatusMsg("Run #%d: Elapsed Time = %.2f = %.2f%% %s", RunVars.RunNumber, rtime, tp, rinfo);
				else if (WDcfg.StopRunMode == STOPRUN_PRESET_COUNTS) SendAcqStatusMsg("Run #%d: Elapsed Time = %.2f (%.2f%%) %s", RunVars.RunNumber, rtime, tp, rinfo);
				else {
					if (SockConsole) SendAcqStatusMsg("Run #%d: Elapsed Time = %.2f s. Readout = %sB/s", RunVars.RunNumber, rtime, totror); // Readout?
					else SendAcqStatusMsg("Run #%d: Elapsed Time = %.2f s", RunVars.RunNumber, rtime);
				}

				if (!Freeze || OneShot) {
					if (StatMode == 0) {  // Single boards statistics
						int NoServEvnt = 0;
						int ab = RunVars.ActiveBrd;

						if (WDcfg.En_service_event && (Stats.last_serv_tstamp_us[ab] < (Stats.current_tstamp_us[ab] - 2e6)))
							NoServEvnt = 1;
						double2str(Stats.ByteCnt[ab].rate, 1, ror);
						double2str(Stats.ReadTrgCnt[ab].rate, 1, trr);
						double2str(Stats.TotTrgCnt[ab].rate, 1, tottrr);
						double2str(Stats.BoardHitCnt[ab].rate, 1, hitr);
						for (i = 0; i < MAX_NCH; i++) {
							int acs; // available channel statistics: 0=none, 1=read_cnt, 2=all_cnt, 3=all_cnt+meas
							if (WDcfg.DataAnalysis & DATA_ANALYSIS_MEAS) acs = 3;
							else if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) acs = 2;
							else if (WDcfg.DataAnalysis & DATA_ANALYSIS_CNT) acs = 1;
							else acs = 0;
							sprintf(ss[i], "N/A     ");
							if ((RunVars.SMonType == SMON_HIT_RATE) && (acs >= 2)) {
								double2str(Stats.MatchHitCnt[ab][i].rate, 0, ss[i]);
							} else if ((RunVars.SMonType == SMON_HIT_CNT) && (acs >= 2)) {
								cnt2str(Stats.MatchHitCnt[ab][i].cnt, ss[i]);
							} else if ((RunVars.SMonType == SMON_TOT_RATE) && (acs >= 1)) {
								double2str(Stats.ReadHitCnt[ab][i].rate, 0, ss[i]);
							} else if ((RunVars.SMonType == SMON_TOT_CNT) && (acs >= 1)) {
								cnt2str(Stats.ReadHitCnt[ab][i].cnt, ss[i]);
							} else if ((RunVars.SMonType == SMON_LEAD_MEAN) && (acs >= 3)) {
								double2str(Stats.LeadMeas[ab][i].mean, 0, ss[i]);
							} else if ((RunVars.SMonType == SMON_LEAD_RMS) && (acs >= 3)) {
								double2str(Stats.LeadMeas[ab][i].rms, 0, ss[i]);
							} else if ((RunVars.SMonType == SMON_TOT_MEAN) && (acs >= 3)) {
								double2str(Stats.ToTMeas[ab][i].mean, 0, ss[i]);
							} else if ((RunVars.SMonType == SMON_TOT_RMS) && (acs >= 3)) {
								double2str(Stats.ToTMeas[ab][i].rms, 0, ss[i]);
							}
						}

						if (SockConsole) {
							Con_printf("CSSb", "%d", ab);						
							char sg2gui[1024];
							if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
								sprintf(sg2gui, "%s\t%.3lf s\t%scps\t%.2lf%%\t%.2lf\t%sB/s",
									stitle[RunVars.SMonType],
									Stats.current_tstamp_us[ab] / 1e6,
									hitr,
									Stats.LostTrgPerc[ab],
									Stats.BuildPerc[ab],
									ror);
							} else {
								sprintf(sg2gui, "%s\t%.3lf s\t%" PRIu64 "\t%scps\t%scps(%.2f%%)\t%.2lf%%(%" PRIu32 ")\t%.2lf\t%scps\t%.2lf%%\t%sB/s",
									stitle[RunVars.SMonType],
									Stats.current_tstamp_us[ab] / 1e6,
									Stats.current_trgid[ab],
									tottrr,
									trr, Stats.ProcTrgPerc[ab],
									Stats.LostTrgPerc[ab], Stats.LostTrgCnt[ab].cnt,
									Stats.ZeroSupprTrgPerc[ab],
									hitr,
									Stats.BuildPerc[ab],
									ror);	// DNIN: for all the events read by software, before any rejection
							}
							Con_printf("CSSg", "%s", sg2gui);
							//DEBUG *********************************
							//sEvt[ab].ChAlmFullFlags[0] = 0x35F4786A;
							//sEvt[ab].ReadoutFlags = 0x40C0609;
							//***************************************							Con_printf("CSSg", "%s", sg2gui);
							for (i = 0; i < MAX_NCH; i++)
								sprintf(ss2gui, "%s %s", ss2gui, ss[i]);
							Con_printf("CSSc", "%" PRIu64 " %" PRIu64 " %" PRIu32 " %s", sEvt[ab].ChAlmFullFlags[0], sEvt[ab].ChAlmFullFlags[1], sEvt[ab].ReadoutFlags, ss2gui);  // CTIN: ChAlmFull msut be 2 64 bit words; otherFlags is 32 bit
							//Con_printf("CSSc", "%s", ss2gui);
							//Con_printf("CSSFc", "%" PRIu32, ow_trailer[ab]); // Send one world trailer - JanusPy is taking care of decoding
						} else {
							if (WDcfg.NumBrd > 1) Con_printf("C", "Board n. %d (press [b] to change active board)\n", ab);
							Con_printf("C", "Time Stamp:   %10.3lf s                \n", Stats.current_tstamp_us[ab] / 1e6);
							if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
								Con_printf("C", "Hit Rate:        %scps                 \n", hitr);
								Con_printf("C", "Dead Time:    %10.2lf %%               \n", Stats.LostTrgPerc[ab]);
							} else {
								Con_printf("C", "Trigger-ID:   %10lld                   \n", Stats.current_trgid[ab]);
								Con_printf("C", "Trg Rate:        %scps           \n", tottrr);
								Con_printf("C", "  Processed:     %scps (%.2f %%)        \n", trr, Stats.ProcTrgPerc[ab]);
								Con_printf("C", "  Rejected:      %6.2lf %% (%" PRIu32 ")      \n", Stats.LostTrgPerc[ab], Stats.LostTrgCnt[ab].cnt);
								Con_printf("C", "  Empty Suppr:   %6.2lf %%             \n", Stats.ZeroSupprTrgPerc[ab]);
								Con_printf("C", "Brd Hit Rate:    %scps                 \n", hitr);
								if (WDcfg.EventBuildingMode != EVBLD_DISABLED) 
									Con_printf("C", "EvBuild:      %10.2lf %%               \n", Stats.BuildPerc[ab]);
							} 
							Con_printf("C", "Readout Rate:    %sB/s                 \n", ror);
							temp2str(BrdTemp[ab][TEMP_BOARD], stemp[TEMP_BOARD]);
							temp2str(BrdTemp[ab][TEMP_FPGA], stemp[TEMP_FPGA]);
							temp2str(BrdTemp[ab][TEMP_TDC0], stemp[TEMP_TDC0]);
							temp2str(BrdTemp[ab][TEMP_TDC1], stemp[TEMP_TDC1]);
							if (WDcfg.NumCh == 64)
								Con_printf("C", "Temp (degC): FPGA=%s TDC=%s Brd=%s \n", stemp[TEMP_FPGA], stemp[TEMP_TDC0], stemp[TEMP_BOARD]);
							else 
								Con_printf("C", "Temp (degC): FPGA=%s TDC0=%s TDC1=%s Brd=%s \n", stemp[TEMP_FPGA], stemp[TEMP_TDC0], stemp[TEMP_TDC1], stemp[TEMP_BOARD]);
							Con_printf("C", "Flags: ");
							char stat_msg[100] = "";
							if (sEvt[ab].ChAlmFullFlags[0] || sEvt[ab].ChAlmFullFlags[1]) strcat(stat_msg, "TDCCh "); // picoTDC Channel AlmFull
							if (sEvt[ab].ReadoutFlags & 0x000000FF) strcat(stat_msg, "TDCTrg ");
							if (sEvt[ab].ReadoutFlags & 0x0000FF00) strcat(stat_msg, "TDCBuff ");
							if (sEvt[ab].ReadoutFlags & 0x00FF0000) strcat(stat_msg, "RPF ");
							if (sEvt[ab].ReadoutFlags & 0x01000000) strcat(stat_msg, "LSOF ");
							if (sEvt[ab].ReadoutFlags & 0x06000000) strcat(stat_msg, "TRGF ");
							if (sEvt[ab].ReadoutFlags & 0x20000000) strcat(stat_msg, "HLOSS ");
							if (sEvt[ab].ReadoutFlags & 0xC0000000) strcat(stat_msg, "ERR ");
							if (NoServEvnt) strcat(stat_msg, "(NoServ) ");
							if (strlen(stat_msg) > 0) Con_printf("C", "%-50s", stat_msg);
							else Con_printf("C", "%-50s", "None");
							Con_printf("C", "\n\n");
							if (StatIntegral) Con_printf("C", "Statistics averaging: Integral (press [I] for Updating mode)\n");
							else Con_printf("C", "Statistics averaging: Updating (press [I] for Integral mode)\n");
							Con_printf("C", "Press [tab] to view statistics of all boards\n\n");
							for (i = 0 + grp_ch64 * 64; i < grp_ch64 * 64 + 64; i++) {
								if (i < 100) 
									Con_printf("C", "%02d:  %s     ", i, ss[i]);
								else
									Con_printf("C", "%02d: %s     ", i, ss[i]);
								if ((i & 0x3) == 0x3) Con_printf("C", "\n");
							}
							Con_printf("C", "\n");
							// Write temperature and status log file
							if (WDcfg.DebugLogMask & DBLOG_TEMP_STATUS) {
								static FILE *tlog=fopen("tlog.txt", "w");
								uint32_t status;
								for (i = 0; i < WDcfg.NumBrd; ++i) {
									ret = FERS_ReadRegister(handle[i], a_acq_status, &status);
									fprintf(tlog, "Brd%d: Time=%10.3lf s  FT=%4.1f TT=%4.1f BT=%4.1f - Stat=%08X\n",  i, Stats.current_tstamp_us[ab] / 1e6, BrdTemp[ab][TEMP_FPGA], BrdTemp[ab][TEMP_TDC0], BrdTemp[ab][TEMP_BOARD], status);
									fflush(tlog);
								}
							}						}
					} else { // Multi boards statistics
						if (SockConsole) {
							char tmp_brdstat[1024] = "";
							Con_printf("CSSt", "%s", stitle[RunVars.SMonType]);
							for (i = 0; i < WDcfg.NumBrd; ++i) {
								uint8_t afbit = 0;
								if (sEvt[i].ChAlmFullFlags[0] || sEvt[i].ChAlmFullFlags[1] || sEvt[i].ReadoutFlags & 0x7FFFFFF) afbit = 1;
								else afbit = 0;
								sprintf(tmp_brdstat, "%s %3d %12.2lf %12" PRIu64 " %12.5lf %12.5lf %12.2lf %12.5lf %" PRIu8, tmp_brdstat, i,
									Stats.current_tstamp_us[i] / 1e6, Stats.current_trgid[i], Stats.ReadTrgCnt[i].rate / 1000, Stats.LostTrgPerc[i], Stats.BuildPerc[i], Stats.ByteCnt[i].rate / (1024 * 1024), afbit);
							}
							Con_printf("CSSB", "%s", tmp_brdstat);
						} else {
							Con_printf("C", "\n");
							if (StatIntegral) Con_printf("C", "Statistics averaging: Integral (press [I] for Updating mode)\n");
							else Con_printf("C", "Statistics averaging: Updating (press [I] for Integral mode)\n");
							Con_printf("C", "Press [tab] to view statistics of single boards\n\n");
							if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
								Con_printf("C", "%3s %10s %10s %10s %10s\n", "Brd", "TStamp", "HitRate", "DeadTime", "DtRate");
								Con_printf("C", "%3s %10s %10s %10s %10s\n\n", "", "[s]", "[KHz]", "[%]", "[%]", "[MB/s]");
								for (i = 0; i < WDcfg.NumBrd; i++)
									Con_printf("C", "%3d %10.2lf %10.5lf %10.2lf\n", i, Stats.current_tstamp_us[i] / 1e6, Stats.BoardHitCnt[i].rate / 1000, Stats.LostTrgPerc[i], Stats.ByteCnt[i].rate / (1024 * 1024));
							} else {
								Con_printf("C", "%3s %10s %10s %10s %10s %10s %10s\n", "Brd", "TStamp", "Trg-ID", "TrgRate", "LostTrg", "Build", "DtRate");
								Con_printf("C", "%3s %10s %10s %10s %10s %10s %10s\n\n", "", "[s]", "[cnt]", "[KHz]", "[%]", "[%]", "[MB/s]");
								for (i = 0; i < WDcfg.NumBrd; i++)
									Con_printf("C", "%3d %10.2lf %10" PRIu64 " %10.5lf %10.5lf %10.2lf %10.5lf\n", i, Stats.current_tstamp_us[i] / 1e6, Stats.current_trgid[i], Stats.ReadTrgCnt[i].rate / 1000, Stats.LostTrgPerc[i], Stats.BuildPerc[i], Stats.ByteCnt[i].rate / (1024 * 1024));
							}

						}
					}
				}
			}
			// ---------------------------------------------------
			// plot histograms
			// ---------------------------------------------------
			if (WDcfg.DataAnalysis & DATA_ANALYSIS_HISTO) {
				if ((RunVars.PlotType == PLOT_LEAD_SPEC) || (RunVars.PlotType == PLOT_TRAIL_SPEC) || (RunVars.PlotType == PLOT_TOT_SPEC))  PlotSpectrum();
				if (RunVars.PlotType == PLOT_TOT_COUNTS) PlotCntHisto();
				if (RunVars.PlotType == PLOT_2D_HIT_RATE) Plot2Dmap(StatIntegral);
			}
			OneShot = 0;
			print_time = curr_time;
		}
	}

ExitPoint:
	if (RestartAll || Quit) {
		StopRun();
		//if (RestartAll) {  // Copied from 5202, to investigate
		//	for (b = 0; b < WDcfg.NumBrd; b++) {	// DNIN: configure the board after each run stop, to avoid block of the readout in the next run
		//		int ret = ConfigureFERS(handle[b], CFG_HARD);
		//		if (ret < 0) {
		//			Con_printf("LCSm", "Configuration Boards Failed!!!\n");
		//			Con_printf("LCSm", "%s", ErrorMsg);
		//			return -1;
		//		}
		//	}
		//}
		for (b = 0; b < WDcfg.NumBrd; b++) {
			if (handle[b] < 0) break;
			FERS_CloseDevice(handle[b]);
			FERS_CloseReadout(handle[b]);
		}
		for (b = 0; b < MAX_NCNC; b++) {
			if (cnc_handle[b] < 0) break;
			FERS_CloseDevice(cnc_handle[b]);
		}
		if (of_DeltaT != NULL && Quit) {
			free(of_DeltaT);
			free(of_ToTDeltaT);
		}
		if (of_ToA != NULL && Quit) {
			free(of_ToA);
			free(of_ToT);
		}
		DestroyStatistics();
		ClosePlotter();
		if (RestartAll) {
			if (AcqStatus == ACQSTATUS_RUNNING) AcqStatus = ACQSTATUS_RESTARTING;
			RestartAll = 0;
			Quit = 0;
			goto ReadCfg;
		}
		else {
			Con_printf("LCSm", "Quitting...\n");
			if (!SockConsole) SaveRunVariables(RunVars);
			fclose(MsgLog);
			return 0;
		}
	}
	else if (RestartAcq) {
		if (AcqStatus == ACQSTATUS_RUNNING) {
			StopRun();
			AcqStatus = ACQSTATUS_RESTARTING;
			Sleep(100);
		}
		RestartAcq = 0;
		goto Restart;
	}


	if (WDcfg.EnableJobs) {
		PresetReached = 0;
		Con_printf("LCSm", "Run %d closed\n", jobrun);
		if (jobrun < WDcfg.JobLastRun) {
			jobrun++;
			SendAcqStatusMsg("Getting ready for Run #%d of the Job...", jobrun);
		}
		else jobrun = WDcfg.JobFirstRun;
		RunVars.RunNumber = jobrun;
		SaveRunVariables(RunVars);
		if (!SockConsole) ClearScreen();
	}
	goto Restart;

ManageError:
	AcqStatus = ACQSTATUS_ERROR;
	Con_printf("LSm", FONT_STYLE_BOLD COLOR_RED "ERROR: %s\n" COLOR_RESET, ErrorMsg);
	SendAcqStatusMsg(FONT_STYLE_BOLD COLOR_RED "ERROR: %s\n" COLOR_RESET, ErrorMsg);
	if (SockConsole) {
		//Sleep(1);
		//goto ExitPoint;
		// This will be improved when there will be a serious error handling
		while (1) {
			if (Con_kbhit()) {
				int c = Con_getch();
				if (c == 'q') break;
			}
			int upd = CheckFileUpdate();
			if (upd > 0) {
				RestartAll = 1;
				goto ExitPoint;
			}
			Sleep(100);
		}
	}
	else {
		Quit = 1;
		Con_getch();
		goto ExitPoint;
	}

	return 0;
}



// for the lib

int janusc_init()
{
	lv_logger_info("call from JanusC lib - init");

	// work
	return 42;
}

void janusc_destroy()
{
	lv_logger_info("call from JanusC lib - destroy");
}

