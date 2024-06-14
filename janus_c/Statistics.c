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

#include <stdio.h>
#include <stdlib.h>
#include "JanusC.h"
#include "Statistics.h"
#include "FERSlib.h"
#include "console.h"
#include "MultiPlatform.h"

// ****************************************************************************************
// Global Variables
// ****************************************************************************************
Stats_t Stats;			// Statistics (histograms and counters)

//int HistoCreated[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedE[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedT[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedM[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoFileCreated = 0;

// ****************************************************************************************
// Create/destroy/initialize histograms
// ****************************************************************************************
int CreateHistogram1D(int Nbin, char *Title, char *Name, char *Xunit, char *Yunit, Histogram1D_t *Histo) {
	Histo->H_data = (uint32_t *)malloc(Nbin*sizeof(uint32_t));
	strcpy(Histo->title, Title);
	strcpy(Histo->spectrum_name, Name);
	strcpy(Histo->x_unit, Xunit);
	strcpy(Histo->y_unit, Yunit);
	Histo->Nbin = Nbin;
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	Histo->Bin_set = 0;
	Histo->mean = 0;
	Histo->rms = 0;
	Histo->real_time = 0;
	Histo->live_time = 0;
	Histo->n_ROI = 0;
	Histo->n_calpnt = 0;
	Histo->A[0] = 0;
	Histo->A[1] = 1;
	Histo->A[2] = 0;
	Histo->A[3] = 0;
	return 0;
}

int CreateHistogram2D(int NbinX, int NbinY, char *Title, char *Name, char *Xunit, char *Yunit, Histogram2D_t *Histo) {
	Histo->H_data = (uint32_t *)malloc(NbinX * NbinY * sizeof(uint32_t));
	strcpy(Histo->title, Title);
	strcpy(Histo->spectrum_name, Name);
	strcpy(Histo->x_unit, Xunit);
	strcpy(Histo->y_unit, Yunit);
	Histo->NbinX = NbinX;
	Histo->NbinY = NbinY;
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	return 0;
}


int DestroyHistogram2D(Histogram2D_t Histo) {
	if (Histo.H_data != NULL)
		free(Histo.H_data);
	return 0;
}

int DestroyHistogram1D(Histogram1D_t Histo) {
	if (Histo.H_data != NULL)
		free(Histo.H_data);	// DNIN error in memory access
	return 0;
}

int ResetHistogram1D(Histogram1D_t *Histo) {
	memset(Histo->H_data, 0, Histo->Nbin * sizeof(uint32_t));
	Histo->H_cnt = 0;
	Histo->H_p_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	Histo->Bin_set = 0;
	Histo->rms = 0;
	Histo->p_rms = 0;
	Histo->mean = 0;
	Histo->p_mean = 0;
	Histo->real_time = 0;
	Histo->live_time = 0;
	return 0;
}

int ResetHistogram2D(Histogram2D_t *Histo) {
	memset(Histo->H_data, 0, Histo->NbinX * Histo->NbinY * sizeof(uint32_t));
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Add one count to the histogram 1D
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_AddCount(Histogram1D_t *Histo, uint32_t Bin)
{
	if (Histo->H_data == NULL || Histo->Nbin == 0) 
		return -1;
	if (Bin < 0) {
		//Histo->Unf_cnt++;
		return -1;
	} else if (Bin >= (uint32_t)(Histo->Nbin-1)) {
		//Histo->Ovf_cnt++;
		return -1;
	}
	Histo->H_data[Bin]++;
	Histo->H_cnt++;
	Histo->mean += (double)Bin;
	Histo->rms += (double)Bin * (double)Bin;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set number of counts in a bin to the histogram 1D
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_SetBinsCount(Histogram1D_t* Histo, int Bin, int counts)
{
	if (Histo->H_data == NULL) 
		return -1;
	if (Bin < 0) {
		//Histo->Unf_cnt++;
		return -1;
	}
	else if (Bin >= (int)(Histo->Nbin - 1)) {
		//Histo->Ovf_cnt++;
		return -1;
	}
	Histo->H_data[Bin] = counts;
	Histo->H_cnt += counts;
	Histo->mean += (double)Bin * (double)counts;
	if (counts != 0)
		Histo->rms += (double)Bin * (double)Bin * (double)counts;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set number of counts by increasing the bin number to the histogram 1D [MultiChannel Scaler]
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_SetCount(Histogram1D_t* Histo, int counts)
{
	if (Histo->H_data == NULL) 
		return -1;
	// mean and rms are computed on Y-axis
	uint32_t Bin = Histo->Bin_set % (Histo->Nbin-1);
	Histo->H_data[Bin] = counts;
	Histo->Bin_set = Bin + 1;	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read header while loading an offline run
// Return:		0=OK, -1=unknown key or value
// --------------------------------------------------------------------------------------------------------- 
int read_offline_header(char* header, int trace)
{
	char name[50];
	sscanf(header, "#%s", &name);
	if (strcmp(name, "NBIN") == 0) {
		int nbin;
		sscanf(header, "#%s %d", &name, &nbin); //hrer should be set the ret value to raise an error in data structure
		Stats.offline_bin[trace] = nbin;
	} else if (strcmp(name, "A0") == 0) {
		float a0;
		sscanf(header, "#%s %f", &name, &a0);
		Stats.H1_File[trace].A[0] = a0;
	} else if (strcmp(name, "A1") == 0) {
		float a1;
		sscanf(header, "#%s %f", &name, &a1);
		Stats.H1_File[trace].A[1] = a1;
	} 
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set bin contents for staircase
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int GetNumLine(char* filename, int trace) {	// To initialize Histogram
	FILE* hfile;
	if (!(hfile = fopen(filename, "r"))) {
		Con_printf("LCSW", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: The file %s does not exists\n" COLOR_RESET, filename);
		return -1;
	}
	char tmp_str[100];
	int mline = 0;
	fgets(tmp_str, 100, hfile);
	if (tmp_str[0] == '#') {	// New version of histogram data
		while (tmp_str[0] == '#') {
			int ret = read_offline_header(tmp_str, trace);
			fgets(tmp_str, 100, hfile);
		}
	} else {
		while (!feof(hfile)) {
			fgets(tmp_str, 100, hfile);
			if (tmp_str[0] == '#') continue; // Line with comment is not a bin
			++mline;
		}
	}
	fclose(hfile);

	return (mline - 1);
};

// --------------------------------------------------------------------------------------------------------- 
// Description: Set bin contents in the histogram 1D - Browse from PlotTraces
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int H1_File_Fill_FromFile(int num_of_trace, char* type, char* name_of_file)
{
	ResetHistogram1D(&Stats.H1_File[num_of_trace]);
	FILE* savedtrace;
	if (!(savedtrace = fopen(name_of_file, "r")))
	{
		// DNIN: send an error message
		return -1;
	}

	int sbin = 0;
	int scounts = 0;
	float value_th = 0;
	while (!feof(savedtrace))
	{
		char tmpstr[50];
		fgets(tmpstr, 50, savedtrace);	// Since in the offline visualization you can have whatever file
		if (tmpstr[0] == '#') {
			read_offline_header(tmpstr, num_of_trace);
			continue;
		}
		sscanf(tmpstr, "%d", &scounts);
		Histo1D_SetBinsCount(&Stats.H1_File[num_of_trace], sbin, scounts);
		//Stats.H1_File[num_of_trace].Nbin = sbin;
		sbin++;
	}
	//if (strcmp(type, "Staircase") != 0) Stats.H1_File[num_of_trace].Nbin = sbin;
	fclose(savedtrace);

	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Manage offline histograms (F & S) filling
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_Offline(int num_of_trace, int num_brd, int num_ch, char *infos, int type_p) {

	char xunit_off[20];
	char yunit_off[20];
	char plot_name[50];
	if (type_p == PLOT_LEAD_SPEC) {
		sprintf(plot_name, "Lead");
		sprintf(xunit_off, "%s", "ns");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_TRAIL_SPEC) {
		sprintf(plot_name, "Trail");
		sprintf(xunit_off, "%s", "ns");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_TOT_SPEC) {
		sprintf(plot_name, "ToT");
		sprintf(xunit_off, "%s", "ns");
		sprintf(yunit_off, "%s", "Cnt");
	}

	char filename[200];
	if (infos[0] == 'F') {
		int num_r;
		sscanf(infos + 1, "%d", &num_r);
		sprintf(filename, "%s\\Run%d_%sHisto_%d_%d.txt", WDcfg.DataFilePath, num_r, plot_name, num_brd, num_ch);
	}
	else if (infos[0] == 'S') {
		if (infos[1] == '\0') return -1;
		sscanf(infos + 1, "%s", filename);
	}

	//if (Stats.offline_bin[num_of_trace] <= 0) {
	GetNumLine(filename, num_of_trace);
	if (Stats.offline_bin[num_of_trace] <= 0) {
		memset(Stats.H1_File[num_of_trace].H_data, 0, sizeof(uint32_t) * Stats.H1_File[num_of_trace].Nbin);
		return -1;
	}
	char h_name[50];
	sprintf(h_name, "%s[%d][%d]", plot_name, num_brd, num_ch);
	DestroyHistogram1D(Stats.H1_File[num_of_trace]);
	CreateHistogram1D(Stats.offline_bin[num_of_trace], plot_name, h_name, xunit_off, yunit_off, &Stats.H1_File[num_of_trace]);
	H1_File_Fill_FromFile(num_of_trace, plot_name, filename);

	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Add one count to the histogram 1D
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int Histo2D_AddCount(Histogram2D_t *Histo, int BinX, int BinY)
{
	if (Histo->H_data == NULL) 
		return -1;
	if ((BinX >= (int)(Histo->NbinX-1)) || (BinY >= (int)(Histo->NbinY-1))) {
		Histo->Ovf_cnt++;
		return -1;
	} else if ((BinX < 0) || (BinY < 0)) {
		Histo->Unf_cnt++;
		return -1;
	}
	Histo->H_data[BinX + HISTO2D_NBINY * BinY]++;
	Histo->H_cnt++;
	return 0;
}




// ****************************************************************************************
// Create Structures and histograms for the specific application
// ****************************************************************************************

// --------------------------------------------------------------------------------------------------------- 
// Description: Create all histograms and allocate the memory
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int CreateStatistics(int nb, int nch, int *AllocatedSize)
{
	int b, ch, i;

	for (i = 0; i < MAX_NTRACES; ++i) Stats.offline_bin[i] = -1;

	*AllocatedSize = sizeof(Stats_t);
	for(b=0; b < nb; b++) {
		for(ch=0; ch < nch; ch++) {
			char hname[100];
			
			if (WDcfg.LeadTrailHistoNbin > -1) {
				sprintf(hname, "Lead[%d][%d]", b, ch);
				CreateHistogram1D(WDcfg.LeadTrailHistoNbin, "Lead", hname, "ns", "Cnt", &Stats.H1_Lead[b][ch]);
				*AllocatedSize += WDcfg.LeadTrailHistoNbin * sizeof(uint32_t);
				Stats.H1_Lead[b][ch].A[0] = WDcfg.LeadHistoMin;
				Stats.H1_Lead[b][ch].A[1] = WDcfg.LeadTrail_LSB_ns * WDcfg.LeadTrailRebin;
				HistoCreatedT[b][ch] = 1;

				sprintf(hname, "Trail[%d][%d]", b, ch);
				CreateHistogram1D(WDcfg.LeadTrailHistoNbin, "Trail", hname, "ns", "Cnt", &Stats.H1_Trail[b][ch]);
				*AllocatedSize += WDcfg.LeadTrailHistoNbin * sizeof(uint32_t);
				Stats.H1_Trail[b][ch].A[0] = WDcfg.TrailHistoMin;
				Stats.H1_Trail[b][ch].A[1] = WDcfg.LeadTrail_LSB_ns * WDcfg.LeadTrailRebin;
				HistoCreatedT[b][ch] = 1;

			}
			if (WDcfg.ToTHistoNbin > -1) {
				sprintf(hname, "ToT[%d][%d]", b, ch);
				CreateHistogram1D(WDcfg.ToTHistoNbin, "ToT", hname, "ns", "Cnt", &Stats.H1_ToT[b][ch]);
				*AllocatedSize += WDcfg.ToTHistoNbin * sizeof(uint32_t);
				Stats.H1_ToT[b][ch].A[0] = WDcfg.ToTHistoMin;
				Stats.H1_ToT[b][ch].A[1] = WDcfg.ToT_LSB_ns * WDcfg.ToTRebin;
				HistoCreatedT[b][ch] = 1;
			}
		}
	}

	for(i=0; i<MAX_NTRACES; i++) {
		
		// Allocate histograms form file 
		int maxnbin = WDcfg.LeadTrailHistoNbin;
		maxnbin = max(maxnbin, WDcfg.ToTHistoNbin);
		CreateHistogram1D(maxnbin, "", "", "", "Cnt", &Stats.H1_File[i]);
		*AllocatedSize += maxnbin*sizeof(uint32_t);
	}

	HistoFileCreated = 1;
	ResetStatistics();	// DNIN: Is it needed here? Too Many Reset Histograms at the begin

	for (i = 0; i < MAX_NTRACES; i++) {
		char tmpchar[100] = "";
		int tmp0, tmp1; // , rn;
		sscanf(RunVars.PlotTraces[i], "%d %d %s", &tmp0, &tmp1, tmpchar);
		if (tmpchar[0] == 'F' || tmpchar[0] == 'S')
			Histo1D_Offline(i, tmp0, tmp1, tmpchar, RunVars.PlotType);
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Destroy (free memory) all histograms
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int DestroyStatistics()
{
	int b, ch, i;
	for(b=0; b<FERSLIB_MAX_NBRD; b++) {
		for(ch=0; ch<FERSLIB_MAX_NCH; ch++) {
			if (HistoCreatedT[b][ch]) {
				DestroyHistogram1D(Stats.H1_Lead[b][ch]);
				DestroyHistogram1D(Stats.H1_Trail[b][ch]);
				DestroyHistogram1D(Stats.H1_ToT[b][ch]);
				HistoCreatedT[b][ch] = 0;
			}
		}
	}
	if (HistoFileCreated) {
		for (i = 0; i < MAX_NTRACES; i++) {
			DestroyHistogram1D(Stats.H1_File[i]);
		}
		HistoFileCreated = 0;
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Reset all statistics and histograms
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int ResetStatistics()	// Have H1_File to be reset?
{
	int b, ch;

	Stats.start_time = get_time(); 
	Stats.current_time = Stats.start_time; 
	Stats.previous_time = Stats.start_time; 
	memset(&Stats.BuiltEventCnt, 0, sizeof(Counter_t));
	for(b=0; b<FERSLIB_MAX_NBRD; b++) {
		Stats.current_trgid[b] = 0;
		Stats.previous_trgid[b] = 0;
		Stats.current_tstamp_us[b] = 0;
		Stats.previous_tstamp_us[b] = 0;
		Stats.LostTrgPerc[b] = 0;
		Stats.ZeroSupprTrgPerc[b] = 0;
		Stats.ProcTrgPerc[b] = 0;
		Stats.BuildPerc[b] = 0;
		memset(&Stats.TotTrgCnt[b], 0, sizeof(Counter_t));
		memset(&Stats.LostTrgCnt[b], 0, sizeof(Counter_t));
		memset(&Stats.SupprTrgCnt[b], 0, sizeof(Counter_t));
		memset(&Stats.ReadTrgCnt[b], 0, sizeof(Counter_t));
		memset(&Stats.ByteCnt[b], 0, sizeof(Counter_t));
		for(ch=0; ch<FERSLIB_MAX_NCH; ch++) {
			if (HistoCreatedT[b][ch]) {
				ResetHistogram1D(&Stats.H1_Lead[b][ch]);
				ResetHistogram1D(&Stats.H1_Trail[b][ch]);
				ResetHistogram1D(&Stats.H1_ToT[b][ch]);
			}
			
			memset(&Stats.ReadHitCnt[b][ch], 0, sizeof(Counter_t));
			memset(&Stats.MatchHitCnt[b][ch], 0, sizeof(Counter_t));
			memset(&Stats.LeadMeas[b][ch], 0, sizeof(MeasStat_t));
			memset(&Stats.TrailMeas[b][ch], 0, sizeof(MeasStat_t));
			memset(&Stats.ToTMeas[b][ch], 0, sizeof(MeasStat_t));
		}
	}
	//for (int i = 0; i < 8; i++) {q
	//	ResetHistogram1D(&Stats.H1_File[i]);	// DNIN: they are built from scratch each times are changed
	//}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Update statistics 
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
#define MEAN(m, ns)		((ns) > 0 ? (m)/(ns) : 0)
#define RMS(r, m, ns)	((ns) > 0 ? sqrt((r)/(ns) - (m)*(m)) : 0)

void UpdateCounter(Counter_t *Counter, double elapsed_time_us, int RateMode) {
	if (elapsed_time_us <= 0) 
		Counter->rate = 0;
	else if (RateMode == 1)
		Counter->rate = Counter->cnt / (elapsed_time_us * 1e-6);
	else 
		Counter->rate = (Counter->cnt - Counter->pcnt) / (elapsed_time_us * 1e-6);
	Counter->pcnt = Counter->cnt;
}

void AddMeasure(MeasStat_t *mstat, double value) {
	mstat->sum += value;
	mstat->ssum += value * value;
	mstat->ncnt++;
}

void UpdateMeasure(MeasStat_t *mstat) {
	mstat->psum = mstat->sum;
	mstat->pssum = mstat->ssum;
	mstat->pncnt = mstat->ncnt;
	mstat->mean = MEAN(mstat->sum, mstat->ncnt);
	mstat->rms = RMS(mstat->ssum, mstat->mean, mstat->ncnt);
}

int UpdateStatistics(int RateMode)
{
	int b, ch;
	double pc_elapstime = (RateMode == 1) ? 1e3 * (Stats.current_time - Stats.start_time) : 1e3 * (Stats.current_time - Stats.previous_time);  // us
	Stats.previous_time = Stats.current_time;

	for (b = 0; b < FERSLIB_MAX_NBRD; b++) {
		double brd_elapstime = (RateMode == 1) ? Stats.current_tstamp_us[b] : Stats.current_tstamp_us[b] - Stats.previous_tstamp_us[b];
		double elapstime_serv = (RateMode == 1) ? Stats.last_serv_tstamp_us[b] : Stats.last_serv_tstamp_us[b] - Stats.prev_serv_tstamp_us[b];
		double elapstime = (brd_elapstime > 0) ? brd_elapstime : pc_elapstime;
		Stats.previous_tstamp_us[b] = Stats.current_tstamp_us[b];
		Stats.prev_serv_tstamp_us[b] = Stats.last_serv_tstamp_us[b];
		for (ch = 0; ch < FERSLIB_MAX_NCH; ch++) {
			UpdateMeasure(&Stats.LeadMeas[b][ch]);
			UpdateMeasure(&Stats.TrailMeas[b][ch]);
			UpdateMeasure(&Stats.ToTMeas[b][ch]);
			UpdateCounter(&Stats.ReadHitCnt[b][ch], elapstime, RateMode);
			UpdateCounter(&Stats.MatchHitCnt[b][ch], elapstime, RateMode);
		}
		if (Stats.TotTrgCnt[b].cnt == 0) {
			Stats.LostTrgPerc[b] = 0;
			Stats.ProcTrgPerc[b] = 0;
			Stats.ZeroSupprTrgPerc[b] = 0;
		} else if (RateMode == 1) {
			Stats.LostTrgPerc[b] = (float)(100.0 * Stats.LostTrgCnt[b].cnt / Stats.TotTrgCnt[b].cnt);
			Stats.ProcTrgPerc[b] = (float)(100.0 * Stats.ReadTrgCnt[b].cnt / Stats.TotTrgCnt[b].cnt);
			Stats.ZeroSupprTrgPerc[b] = (float)(100.0 * Stats.SupprTrgCnt[b].cnt / Stats.TotTrgCnt[b].cnt);
		} else if (Stats.TotTrgCnt[b].cnt > Stats.TotTrgCnt[b].pcnt) {
			Stats.LostTrgPerc[b] = (float)(100.0 * (Stats.LostTrgCnt[b].cnt - Stats.LostTrgCnt[b].pcnt) / (Stats.TotTrgCnt[b].cnt - Stats.TotTrgCnt[b].pcnt));
			Stats.ProcTrgPerc[b] = (float)(100.0 * (Stats.ReadTrgCnt[b].cnt - Stats.ReadTrgCnt[b].pcnt) / (Stats.TotTrgCnt[b].cnt - Stats.TotTrgCnt[b].pcnt));
			Stats.ZeroSupprTrgPerc[b] = (float)(100.0 * (Stats.SupprTrgCnt[b].cnt - Stats.SupprTrgCnt[b].pcnt) / (Stats.TotTrgCnt[b].cnt - Stats.TotTrgCnt[b].pcnt));
		} else {
			Stats.LostTrgPerc[b] = 0;
			Stats.ProcTrgPerc[b] = 0;
			Stats.ZeroSupprTrgPerc[b] = 0;
		}
		if (Stats.LostTrgPerc[b] > 100) Stats.LostTrgPerc[b] = 100;
		if (Stats.ProcTrgPerc[b] > 100) Stats.ProcTrgPerc[b] = 100;
		if (Stats.ZeroSupprTrgPerc[b] > 100) Stats.ZeroSupprTrgPerc[b] = 100;
		Stats.previous_trgid_p[b] = Stats.current_trgid[b];

		if (Stats.BuiltEventCnt.cnt == 0) 
			Stats.BuildPerc[b] = 0;
		else if (RateMode == 1) 
			Stats.BuildPerc[b] = (float)(100.0 * Stats.ReadTrgCnt[b].cnt / Stats.BuiltEventCnt.cnt);
		else if (Stats.BuiltEventCnt.cnt > Stats.BuiltEventCnt.pcnt) 
			Stats.BuildPerc[b] = (float)(100.0 * (Stats.ReadTrgCnt[b].cnt - Stats.ReadTrgCnt[b].pcnt) / (Stats.BuiltEventCnt.cnt - Stats.BuiltEventCnt.pcnt));
		else 
			Stats.BuildPerc[b] = 0;
		if (Stats.BuildPerc[b] > 100) 
			Stats.BuildPerc[b] = 100;
		double etime = elapstime_serv > 0 ? elapstime_serv : elapstime;
		UpdateCounter(&Stats.TotTrgCnt[b], etime, RateMode);
		UpdateCounter(&Stats.LostTrgCnt[b], etime, RateMode);
		UpdateCounter(&Stats.SupprTrgCnt[b], etime, RateMode);
		UpdateCounter(&Stats.ReadTrgCnt[b], elapstime, RateMode);
		UpdateCounter(&Stats.BoardHitCnt[b], elapstime, RateMode);
		UpdateCounter(&Stats.ByteCnt[b], elapstime, RateMode);
	}
	Stats.BuiltEventCnt.pcnt = Stats.BuiltEventCnt.cnt;
	return 0;
}

