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
#include <stdio.h>
#include <inttypes.h>

#include "plot.h"	
#include "FERSlib.h"
#include "FERSutils.h"
#include "Statistics.h"
#include "JanusC.h"

FILE *plotpipe = NULL;
int LastPlotType = -1; 
int LastPlotName = -1;
const int debug = 0;

#ifdef _WIN32
#define GNUPLOT_COMMAND  "..\\gnuplot\\gnuplot.exe"
#define NULL_PATH		 "nul"
#else
#define GNUPLOT_COMMAND  "/usr/bin/gnuplot"	// anyway .exe does not work for linux
#define NULL_PATH		 "/dev/null"
#endif

#define PLOT_TYPE_SPECTRUM		0
#define PLOT_TYPE_WAVE			1
#define PLOT_TYPE_HISTO			2
#define PLOT_TYPE_2D_MAP		3
#define PLOT_TYPE_SCAN_THR		4
#define PLOT_TYPE_SCAN_DELAY	5
#define PLOT_TYPE_SCAT_TIME		6

#define LEG_VSPACE	0.0226

// --------------------------------------------------------------------------------
// Open Gnuplot
// --------------------------------------------------------------------------------
int OpenPlotter()
{
	char str[200];
	//strcpy(str, ConfigVar->GnuPlotPath);
	strcpy(str, "");
	strcat(str, GNUPLOT_COMMAND);
	strcat(str, " 2> ");  
	strcat(str, NULL_PATH); // redirect stderr to nul (to hide messages showing up in the console output)
	if ((plotpipe = popen(str, "w")) == NULL) return -1;

	fprintf(plotpipe, "set terminal wxt noraise title 'FERS Readout' size 1200,800 position 700,10\n");
	fprintf(plotpipe, "set grid\n");
	fprintf(plotpipe, "set mouse\n");
	fflush(plotpipe);
	return 0;
}

// --------------------------------------------------------------------------------
// Close Gnuplot
// --------------------------------------------------------------------------------
int ClosePlotter()
{
	LastPlotType = -1;
	LastPlotName = -1;
	if (plotpipe != NULL)
#ifdef _WIN32
		_pclose(plotpipe);
#else
		pclose(plotpipe);
#endif
	return 0;
}

// --------------------------------------------------------------------------------
// Plot 1D Histogram (spectrum)
// --------------------------------------------------------------------------------

int PlotSpectrum()
{
	FILE* debbuff;
	if (debug) debbuff = fopen("gnuplot_debug_PHA.txt", "w");

	int i, t, nt, xcalib, Nbin, max_yval=1;
	int en[8], brd[8], ch[8]; // enable and Board/channel assigned to each trace
	char SorBF[8];
	int rn[8]; // run number for off line spectrum
	char cmd[1000];
	double mean[8], rms[8], A0[8] = {}, A1[8] = {}, la0=0, la1=0;
	char xunit[50];
	char fftitle[50]; //ffxunit[50];
	char description[8][50];
	static int LastNbin=0, LastXcalib=0;
	static double LastA0 = 0, LastA1 = 0;
	Histogram1D_t *Histo[8];

	if (plotpipe == NULL) return -1;

	xcalib = RunVars.Xcalib;
	Nbin = 0;
	if (RunVars.PlotType == PLOT_LEAD_SPEC) {
		strcpy(xunit, Stats.H1_Lead[0][0].x_unit);
		sprintf(fftitle, "T-Lead (BinSize = %.3fns)", Stats.H1_Lead[0]->A[1]);
		fprintf(plotpipe, "set title '%s'\n", fftitle);
		Nbin = WDcfg.LeadTrailHistoNbin;
		la0 = WDcfg.LeadHistoMin;
		la1 = WDcfg.LeadTrail_LSB_ns * WDcfg.LeadTrailRebin;
	} else if (RunVars.PlotType == PLOT_TRAIL_SPEC) {
		strcpy(xunit, Stats.H1_Trail[0][0].x_unit);
		sprintf(fftitle, "T-Trail (BinSize = %.3fns)", Stats.H1_Trail[0]->A[1]);
		fprintf(plotpipe, "set title '%s'\n", fftitle);
		Nbin = WDcfg.LeadTrailHistoNbin;
		la0 = WDcfg.TrailHistoMin;
		la1 = WDcfg.LeadTrail_LSB_ns * WDcfg.LeadTrailRebin;
	} else if (RunVars.PlotType == PLOT_TOT_SPEC) {
		strcpy(xunit, Stats.H1_ToT[0][0].x_unit);
		sprintf(fftitle, "ToT (BinSize = %.3fns)", Stats.H1_ToT[0]->A[1]);
		fprintf(plotpipe, "set title '%s'\n", fftitle);
		Nbin = WDcfg.ToTHistoNbin;
		la0 = WDcfg.ToTHistoMin;
		la1 = WDcfg.ToT_LSB_ns * WDcfg.ToTRebin;
	}

	nt = 0;
	//char tmp_rn[8][100];
	for(t=0; t<8; t++) {
		if (strlen(RunVars.PlotTraces[t]) == 0) {
			en[t] = 0;
		} else {
			en[t] = 1;
			nt++;
			char tmp_rn[100];
			sscanf(RunVars.PlotTraces[t], "%d %d %s", &brd[t], &ch[t], tmp_rn);
			SorBF[t] = tmp_rn[0];
			if (tmp_rn[0] == 'F'){	//offline
				//Nbin = Stats.offline_bin;
				sscanf(tmp_rn + 1, "%d", &rn[t]);
				// H1_File, histogram reserved for reading from file
				Histo[t] = &Stats.H1_File[t];
				//strcpy(Histo[t]->x_unit, ffxunit);
				//strcpy(Histo[t]->title, fftitle);
				sprintf(Histo[t]->spectrum_name, "From_File_%s[%d][%d]", fftitle, brd[t], ch[t]);

				mean[t] = 0;
				rms[t] = 0;
				if (Histo[t]->H_cnt > 0) {
					mean[t] = Histo[t]->mean / Histo[t]->H_cnt;
					rms[t] = sqrt((Histo[t]->rms / Histo[t]->H_cnt) - mean[t] * mean[t]);
				}
				//set description
				sprintf(description[t], "T%d Run%d", t, rn[t]);
			}
			else if (tmp_rn[0] == 'S'){
				//Nbin = Stats.offline_bin;
				char ext_file[100] = "";
				sscanf(tmp_rn + 1, "%s", ext_file);

				Histo[t] = &Stats.H1_File[t];
				//strcpy(Histo[t]->x_unit, ffxunit);	//why here are needed?
				strcpy(Histo[t]->title, fftitle);
				sprintf(Histo[t]->spectrum_name, "ExtFile_%s", fftitle);

				mean[t] = 0;
				rms[t] = 0;
				if (Histo[t]->H_cnt > 0) {
					mean[t] = Histo[t]->mean / Histo[t]->H_cnt;
					rms[t] = sqrt((Histo[t]->rms / Histo[t]->H_cnt) - mean[t] * mean[t]);
				}
				//set description
				//sprintf(description[t], "T%d File:%s", t, ext_file);
				sprintf(description[t], "%s", ext_file);

			} else {	// online
				if (brd[t] >= WDcfg.NumBrd) brd[t] = 0;
				if (ch[t] >= MAX_NCH) ch[t] = 0;
				if (RunVars.PlotType == PLOT_LEAD_SPEC) {
					Histo[t] = &Stats.H1_Lead[brd[t]][ch[t]];
					//Nbin = Stats.H1_Lead[0][0].Nbin;
				}
				if (RunVars.PlotType == PLOT_TRAIL_SPEC) {
					Histo[t] = &Stats.H1_Trail[brd[t]][ch[t]];
					//Nbin = Stats.H1_Trail[0][0].Nbin;
				}
				if (RunVars.PlotType == PLOT_TOT_SPEC) {
					Histo[t] = &Stats.H1_ToT[brd[t]][ch[t]];
					//Nbin = Stats.H1_ToT[0][0].Nbin;
				}
				//strcpy(xunit, Histo[t]->x_unit);  // is the same for all the traces
				mean[t] = 0;
				rms[t] = 0;
				if (Histo[t]->H_cnt > 0) {
					mean[t] = Histo[t]->mean / Histo[t]->H_cnt;
					rms[t] = sqrt((Histo[t]->rms / Histo[t]->H_cnt) - mean[t] * mean[t]); // DNIN: Statistically speaking, shouldn't be rms/(cnt-1)
				}

				sprintf(description[t], "T%d Run%d", t, RunVars.RunNumber);
			}
		}
	}
	if (nt == 0) return 0;

	if ((LastPlotType != PLOT_TYPE_SPECTRUM) || (LastNbin != Nbin) || (LastXcalib != xcalib) || (LastPlotName != RunVars.PlotType)
		|| (LastA0 != la0) || (LastA1 != la1)) {
		fprintf(plotpipe, "clear\n");
		fprintf(plotpipe, "set terminal wxt noraise title 'FERS Readout' size 1200,800 position 700,10\n");
		fprintf(plotpipe, "set ylabel 'Counts'\n");
		fprintf(plotpipe, "set autoscale y\n");
//		fprintf(plotpipe, "set yrange [0:100]");
//		fprintf(plotpipe, "set yrange [0:]\n");
		fprintf(plotpipe, "bind y 'set yrange [0:]'\n");
		fprintf(plotpipe, "set style fill solid\n");
		fprintf(plotpipe, "bind y 'set autoscale y'\n");
		fprintf(plotpipe, "set xtics auto\n");
		fprintf(plotpipe, "set ytics auto\n");
		fprintf(plotpipe, "unset logscale\n");
		fprintf(plotpipe, "unset label\n");
		fprintf(plotpipe, "set grid\n");
		fprintf(plotpipe, "set key font 'courier new,10'\n");
		fprintf(plotpipe, "set label font 'courier new,10'\n");

		fprintf(plotpipe, "set key title ' Mean    RMS                '\n");
		//if (WDcfg.NumBrd == 1)
		//	fprintf(plotpipe, "set key title ' Mean    RMS      '\n");
		//else
		//	fprintf(plotpipe, "set key title '        Mean    RMS      '\n");
		if (xcalib) {
			fprintf(plotpipe, "set xlabel '%s'\n", xunit);
			fprintf(plotpipe, "set xrange [%f:%f]\n", la0, la0 + Nbin * la1);
			fprintf(plotpipe, "bind x 'set xrange [%f:%f]'\n", la0, la0 + Nbin * la1);
			//int initxm = 1;
			//double xmin=1e15, xmax=0;
			//for(t=0; t<8; t++) {
			//	if (en[t]==0) continue;
			//	if (initxm) {
			//		xmin = A0[t];
			//		xmax = A0[t] + Nbin * A1[t];
			//		initxm = 0;
			//	} else {
			//		xmin = min(xmin, A0[t]);
			//		xmax = max(xmax, A0[t] + Nbin * A1[t]);
			//	}
			//}
			//fprintf(plotpipe, "set xlabel '%s'\n", xunit);
			//fprintf(plotpipe, "set xrange [%f:%f]\n", xmin, xmax);
			//fprintf(plotpipe, "bind x 'set xrange [%f:%f]'\n", xmin, xmax);
		} else {
			fprintf(plotpipe, "set xlabel 'Channels'\n");
			fprintf(plotpipe, "set xrange [0:%d]\n", Nbin);
			fprintf(plotpipe, "bind x 'set xrange [0:%d]'\n", Nbin);
		}
		LastNbin = Nbin;
		LastXcalib = xcalib;
		LastPlotType = PLOT_TYPE_SPECTRUM;
		LastPlotName = RunVars.PlotType;
		LastA0 = la0;
		LastA1 = la1;
	}

	if (debug && xcalib) {
		fprintf(debbuff, "set xlabel '%s'\n", xunit);
		fprintf(debbuff, "set xrange [%f:%f]\n", la0, la0 + Nbin * la1);
		fprintf(debbuff, "bind x 'set xrange [%f:%f]'\n", la0, la0 + Nbin * la1);
	}
	
	if (debug) 	fprintf(debbuff, "$PlotData << EOD\n");
	fprintf(plotpipe, "$PlotData << EOD\n");
	for(i=0; i < Nbin; i++) {
		for (t = 0; t < 8; ++t) {
			if (!en[t]) continue;
			int ind = i;
			if (i > (int)(Histo[t]->Nbin-1)) continue; // Offline histogram can have lower number of bins
			fprintf(plotpipe, "%" PRIu32 " ", Histo[t]->H_data[ind]);
			if (debug) fprintf(debbuff, "%" PRIu32 " ", Histo[t]->H_data[ind]);
		}
		fprintf(plotpipe, "\n");
		if (debug) fprintf(debbuff, "\n");
	}
	fprintf(plotpipe, "EOD\n");
	if (debug) fprintf(debbuff, "EOD\n");

	sprintf(cmd, "plot ");
	int nt_written = 0;
	double a0 = xcalib ? la0 : 0;
	double a1 = xcalib ? la1 : 1;
	for(t=0; t<8; t++) {
		char smeas[200];
		if (!en[t]) continue;
		char title[200] = "", tmpc[500];
		double m = a0 + a1 * mean[t];
		double r = a1 * rms[t];
		if (Histo[t]->H_cnt == 0)
			sprintf(smeas, "    ---     ---");
		else
			sprintf(smeas, "%7.3f  %6.3f", m, r);
		if (SorBF[t] == 'S') {
			sprintf(title, "%s: %s - T%d File", description[t], smeas, t);
			fprintf(plotpipe, "set label %d '%s' font 'courier new, 10' at graph 0.95,%f right textcolor rgb '#006A4E' noenhanced\n", nt_written + 1, title, 0.96 - (nt_written * LEG_VSPACE));

			sprintf(tmpc, " $PlotData u ($0*%lf+%lf):($%d) title ' ' noenhanced w step", a1, a0, nt_written + 1);
		}
		else {
			//if (SorBF[t] == 'F') sprintf(title, "Off ");
			if (WDcfg.NumBrd > 1)
				sprintf(title, "Brd[%d] Ch[%d]: %s - %s", brd[t], ch[t], smeas, description[t]);
			else
				sprintf(title, "Ch[%d]: %s - %s", ch[t], smeas, description[t]);
			
			char leg_color[] = "#000000";  // Offline and Online histograms differs by legend color
			if (SorBF[t] == 'F') sprintf(leg_color, "#2B65EC");
			else sprintf(leg_color, "#000000");
			fprintf(plotpipe, "set label %d '%s' font 'courier new, 10' at graph 0.95,%f right textcolor rgb '%s'\n", nt_written + 1, title, 0.958 - (nt_written * LEG_VSPACE), leg_color);
			sprintf(tmpc, " $PlotData u ($0*%lf+%lf):($%d) title '                      ' w step", a1, a0, nt_written + 1);
		}
		strcat(cmd, tmpc);
		++nt_written;
		
		if (nt_written < nt) strcat(cmd, ", ");
		else strcat(cmd, "\n");
	}
	if (debug) fprintf(debbuff, "%s", cmd);
	fprintf(plotpipe, "%s", cmd);
	fflush(plotpipe);
	if (debug) fflush(debbuff);
	if (debug) fclose(debbuff);
	return 0;
}


// --------------------------------------------------------------------------------
// Plot Count Rate vs Channel
// --------------------------------------------------------------------------------
int PlotCntHisto()
{
	int i, ch, brd;
	FILE *pd;

	brd = 0;
	ch = 0;
	// Take the first trace active or brd/ch 0 0
	for (int m = 0; m < 8; m++) {
		if (strlen(RunVars.PlotTraces[m]) != 0) {
			sscanf(RunVars.PlotTraces[m], "%d %d", &brd, &ch);
			break;
		}
	}

	if (plotpipe == NULL) return -1;
	if (LastPlotType != PLOT_TYPE_HISTO) {
		fprintf(plotpipe, "set terminal wxt noraise title 'FERS Readout' size 1200,800 position 700,10\n");
		fprintf(plotpipe, "set xlabel 'Channel'\n");
		fprintf(plotpipe, "set ylabel 'cps'\n");
		fprintf(plotpipe, "set xrange [0:64]\n");
		fprintf(plotpipe, "bind x 'set xrange [0:68]'\n");
		fprintf(plotpipe, "unset yrange\n");
		fprintf(plotpipe, "set yrange [0:]\n");
		fprintf(plotpipe, "bind y 'set yrange [0:]'\n");
		fprintf(plotpipe, "set style fill solid\n");
		fprintf(plotpipe, "set ytics auto\n");
		fprintf(plotpipe, "set xtics 2\n");
		fprintf(plotpipe, "unset logscale\n");
		fprintf(plotpipe, "unset label\n");
		fprintf(plotpipe, "set grid\n");
		LastPlotType = PLOT_TYPE_HISTO;
	}
	pd = fopen("PlotData.txt", "w");
	if (pd == NULL) return -1;
	fprintf(plotpipe, "set title 'Cps vs Channel (Board %d)'\n", brd);
	for(i=0; i<MAX_NCH; i++) 
		fprintf(pd, "%lf\n", Stats.ReadHitCnt[brd][i].rate);
	fclose(pd);
	fprintf(plotpipe, "plot 'PlotData.txt' title '' w histo linecolor 'orange'\n");
	fflush(plotpipe);
	return 0;
}

// --------------------------------------------------------------------------------
// Plot 2D (matrix)
// --------------------------------------------------------------------------------
int Plot2Dmap(int StatIntegral)
{
	int brd, ch, i, x, y;
	double zmax = 0;
	double zval[MAX_NCH];  // Z-axes in 2D plots
	char title[100];
	FILE* pd;
	brd = 0;
	ch = 0;
	// take the first trace active or brd/ch 0 0
	for (int m = 0; m < 8; m++) {
		if (strlen(RunVars.PlotTraces[m]) != 0) {
			sscanf(RunVars.PlotTraces[m], "%d %d", &brd, &ch);
			break;
		}
	}

	if (RunVars.PlotType == PLOT_2D_HIT_RATE) {
		for (i = 0; i < MAX_NCH; i++)	zval[i] = Stats.ReadHitCnt[brd][i].rate;
		sprintf(title, "Channel Hit Rate (Board %d)", brd);
	}

	if (plotpipe == NULL) return -1;
	pd = fopen("PlotData.txt", "w");
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			double pixz = zval[xy2ch(x, y)];
			fprintf(pd, "%d %d %lf\n", x, y, pixz);
			if (zmax < pixz) zmax = pixz;
		}
		fprintf(pd, "\n");
	}
	fclose(pd);
	if (zmax == 0) zmax = 1;

	if (LastPlotType != PLOT_TYPE_2D_MAP) {
		fprintf(plotpipe, "clear\n");
		fprintf(plotpipe, "set terminal wxt noraise title 'FERS Readout' size 400,400 position 700,10\n");
		fprintf(plotpipe, "set xlabel 'X'\n");
		fprintf(plotpipe, "set ylabel 'Y'\n");
		fprintf(plotpipe, "set xrange [-0.5:7.5]\n");
		fprintf(plotpipe, "set yrange [-0.5:7.5]\n");
		fprintf(plotpipe, "set xtics 1\n");
		fprintf(plotpipe, "set ytics 1\n");
		// Pixel Mapping: gnuplot has pixel (0,0) = A1 in the lower left corner 
		fprintf(plotpipe, "set xtics ('A' 0, 'B' 1, 'C' 2, 'D' 3, 'E' 4, 'F' 5, 'G' 6, 'H' 7)\n");
		fprintf(plotpipe, "set ytics ('1' 0, '2' 1, '3' 2, '4' 3, '5' 4, '6' 5, '7' 6, '8' 7)\n");
		fprintf(plotpipe, "unset logscale\n");
		fprintf(plotpipe, "unset label\n");
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				fprintf(plotpipe, "set label front center '%d' at graph %f, %f\n", xy2ch(x, y), 0.0625 + x / 8., 0.0625 + y / 8.);	// may create a function to float the value ...
			}
		}
		LastPlotType = PLOT_TYPE_2D_MAP;
	}
	fprintf(plotpipe, "bind z 'set cbrange [0:%f]'\n", zmax);
	fprintf(plotpipe, "set cbrange [0:%f]\n", zmax);
	fprintf(plotpipe, "set title '%s'\n", title);
	fprintf(plotpipe, "unset grid; set palette model CMY rgbformulae 15,7,3\n");
	fprintf(plotpipe, "plot 'PlotData.txt' with image\n");
	fflush(plotpipe);
	return 0;
}

// DO SCATTER PLOT ToA vs ToT. It is needed to further fitting the curve and improving the 
// resolution of ToA. This is meant to decoupling the time of crossing the threshold from
// the signal amplitude
int PlotToA_ToT() {
	int brd, ch, i;
	double zmax = 0;
	//double zval[MAX_NCH];  // Z-axes in 2D plots
	char title[100];
	FILE* pd;
	brd = 0;
	ch = 0;
	
	// take the first trace active or brd/ch 0 0
	for (int m = 0; m < 8; m++) {
		if (strlen(RunVars.PlotTraces[m]) != 0) {
			sscanf(RunVars.PlotTraces[m], "%d %d", &brd, &ch);
			break;
		}
	}

	if (plotpipe == NULL) return -1;
	sprintf(title, "ToA vs ToT (Board %d - Ch %d)", brd, ch);
	uint32_t min = Stats.H1_Lead[brd]->H_cnt;
	if (Stats.H1_ToT[brd]->H_cnt < min) min = Stats.H1_ToT[brd]->H_cnt;
	if (LastPlotName != PLOT_TYPE_SCAT_TIME) {
		pd = fopen("PlotData.txt", "w");
		for (i = 0; i < (int)min; ++i) {
			fprintf(pd, "%" PRIu32 " %" PRIu32 "\n", Stats.H1_Lead[brd][ch].H_data[i], Stats.H1_ToT[brd][ch].H_data[i]);
		}
	}
	else {
		pd = fopen("PlotData.txt", "a");
		fprintf(pd, "%" PRIu32 " %" PRIu32 "\n", Stats.H1_Lead[brd][ch].H_data[min-1], Stats.H1_ToT[brd][ch].H_data[min-1]);
	}
	if (LastPlotType != PLOT_TYPE_SCAT_TIME) {
		fprintf(plotpipe, "clear\n");
		fprintf(plotpipe, "set terminal wxt noraise title 'FERS Readout' size 400,400 position 700,10\n");
		fprintf(plotpipe, "set xlabel 'ToT'\n");
		fprintf(plotpipe, "set ylabel 'ToA'\n");
		fprintf(plotpipe, "set xrange [0:500]\n");	// DNIN: how to set the range? Is fixed, can be dynamic, there is an optimal range?
		fprintf(plotpipe, "set yrange [0:500]\n");
		fprintf(plotpipe, "set xtics auto\n");
		fprintf(plotpipe, "set ytics auto\n");
		// Pixel Mapping: gnuplot has pixel (0,0) = A1 in the lower left corner 
		fprintf(plotpipe, "unset logscale\n");
		fprintf(plotpipe, "unset label\n");
		fprintf(plotpipe, "set grid\n");

		fprintf(plotpipe, "bind x 'set xrange [0:500]\n");
		fprintf(plotpipe, "bind y 'set yrange [0:500]\n");
		// DNIN: does the plot visualization in bin make sense?
		// Does 'if (xcalib)' make sense? Does binding make sense

		LastPlotType = PLOT_TYPE_SCAT_TIME;
	}

	fprintf(plotpipe, "set title '%s'\n", title);
	fprintf(plotpipe, "set palette model CMY rgbformulae 15,7,3\n");	// DNIN: Check which palette should be used
	fprintf(plotpipe, "plot 'PlotData.txt' with image\n");
	fflush(plotpipe);


	return 0;
}
