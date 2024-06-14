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

#include "FERSlib.h"
#include "console.h"
#include "configure.h"
#include "JanusC.h"
#include "Statistics.h"
#include "adapters.h"


// ****************************************************
// Global Variables
// ****************************************************
FILE *of_raw_b = NULL, *of_raw_a = NULL;
FILE *of_list_b = NULL, *of_list_a = NULL;
FILE *of_sync = NULL;
uint8_t fnumFVer = 0;
uint8_t snumFVer = 0;
uint8_t datatype = 0x0;	
uint8_t fnumSW = 0;
uint8_t snumSW = 0;
uint8_t tnumSW = 0;
uint8_t WriteHeaderBin = 0;
uint8_t WriteHeaderAscii = 0;

// ****************************************************
// Local functions
// ****************************************************
static void CreateOutFileName(char *radix, int RunNumber, int binary, char *filename) {
	if (RunNumber >= 0) sprintf(filename, "%sRun%d_%s", WDcfg.DataFilePath, RunNumber, radix);
	else sprintf(filename, "%s%s", WDcfg.DataFilePath, radix);
	if (binary) strcat(filename, ".dat");
	else strcat(filename, ".txt");
}

// ****************************************************
// Open/Close Output Files
// ****************************************************
int OpenOutputFiles(int RunNumber)
{
	char filename[500];

	if ((WDcfg.OutFileEnableMask & OUTFILE_RAW_DATA_BIN) && (of_raw_b == NULL)) {
		CreateOutFileName("raw_data", RunNumber, 1, filename);
		of_raw_b = fopen(filename, "wb");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_RAW_DATA_ASCII) && (of_raw_a == NULL)) {
		CreateOutFileName("raw_data", RunNumber, 0, filename);
		of_raw_a = fopen(filename, "w");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_BIN) && (of_list_b == NULL)) {
		CreateOutFileName("list", RunNumber, 1, filename);
		of_list_b = fopen(filename, "wb");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_ASCII) && (of_list_a == NULL)) {
		CreateOutFileName("list", RunNumber, 0, filename);
		of_list_a = fopen(filename, "w");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_SYNC) && (of_sync == NULL)) {
		CreateOutFileName("sync", RunNumber, 0, filename);
		of_sync = fopen(filename, "w");
	}
	return 0;
}

int CloseOutputFiles()
{
	// char filename[500];
	if (of_raw_b != NULL) fclose(of_raw_b);
	if (of_raw_a != NULL) fclose(of_raw_a);
	if (of_list_b != NULL) fclose(of_list_b);
	if (of_list_a != NULL) fclose(of_list_a);
	if (of_sync != NULL) fclose(of_sync);
	of_raw_b = NULL;
	of_raw_a = NULL;
	of_list_b = NULL;
	of_list_a = NULL;
	of_sync = NULL;
	return 0;
}

// ****************************************************
// Save Raw data and Lists to Output Files
// ****************************************************

// CTIN: add check on file size and stop saving when the programmed limit is reached

int SaveRawData(uint32_t *buff, int nw)
{
	int i;
	if (of_raw_b != NULL) {
		fwrite(buff, sizeof(uint32_t), nw, of_raw_b);
	}
	if (of_raw_a != NULL) {
		for(i=0; i<nw; i++)
			fprintf(of_raw_a, "%3d %08X\n", i, buff[i]);
		fprintf(of_raw_a, "----------------------------------\n");
	}
	return 0;
}

// ****************************************************
// Write Header of list files
// ****************************************************
int WriteListfileHeader() {
	// Get software, data file and board version	
	sscanf(SW_RELEASE_NUM, "%" SCNu8 ".%" SCNu8 ".%" SCNu8, &fnumSW, &snumSW, &tnumSW);
	sscanf(FILE_LIST_VER, "%" SCNu8 ".%" SCNu8, &fnumFVer, &snumFVer);
	uint16_t brdVer;
#ifdef FERS_5203	
	//sscanf("52.03", "%" SCNu8 ".%" SCNu8, &fbrdVer, &sbrdVer);
	sscanf("5203", "%" SCNu16, &brdVer);
#else
	sscanf("52.02", "%" SCNu8 ".%" SCNu8, &fbrdVer, &sbrdVer);
#endif	

	// write headers, common for all the list files
	uint16_t acq_mode = 0;
	if (WDcfg.TestMode)
		acq_mode = (uint16_t)ACQMODE_TEST_MODE;
	else
		acq_mode = uint16_t(WDcfg.AcquisitionMode);
	uint8_t meas_mode = uint8_t(WDcfg.MeasMode);
	uint8_t t_unit = WDcfg.OutFileUnit;	// LSB or ns
	int16_t rn = (int16_t)RunVars.RunNumber;
	if (of_list_b != NULL) {
		uint8_t header_size = sizeof(uint8_t) + 2 * sizeof(fnumFVer) + 3 * sizeof(fnumSW)	// headersize + DataFormat (2*unt8) + SwVersion (3*uint8)
			+ sizeof(brdVer) + sizeof(rn)													// Brd version, Run number
			+ sizeof(acq_mode) + sizeof(meas_mode) + sizeof(t_unit)							// AcqMode, MeasMode, time unit
			+ 3 * sizeof(float) + sizeof(Stats.start_time);									// LSB value for ToA, ToT, Tstamp, StartTime
			
		//uint32_t tmask = WDcfg.ChEnableMask1[brd];	// see below	
		// In Data format 3.3 we will add the size of the header
		// fwrite(&header_size, sizeof(header_size), 1, of_list_b);
		fwrite(&fnumFVer, sizeof(fnumFVer), 1, of_list_b);
		fwrite(&snumFVer, sizeof(snumFVer), 1, of_list_b);
		
		fwrite(&fnumSW, sizeof(fnumSW), 1, of_list_b);
		fwrite(&snumSW, sizeof(snumSW), 1, of_list_b);
		fwrite(&tnumSW, sizeof(tnumSW), 1, of_list_b);
		

		fwrite(&brdVer, sizeof(brdVer), 1, of_list_b);
		
		fwrite(&rn, sizeof(int16_t), 1, of_list_b);
		
		fwrite(&acq_mode, sizeof(acq_mode), 1, of_list_b);
		fwrite(&meas_mode, sizeof(meas_mode), 1, of_list_b);
		fwrite(&t_unit, sizeof(t_unit), 1, of_list_b);
		
		float tmpf;
		tmpf = 1000 * WDcfg.LeadTrail_LSB_ns;
		fwrite(&tmpf, sizeof(tmpf), 1, of_list_b);
		tmpf = 1000 * WDcfg.ToT_LSB_ns;
		fwrite(&tmpf, sizeof(tmpf), 1, of_list_b);
		tmpf = float(CLK_PERIOD * 1000);
		fwrite(&tmpf, sizeof(float), 1, of_list_b); 

		fwrite(&Stats.start_time, sizeof(Stats.start_time), 1, of_list_b);
	}
	if (of_list_a != NULL) {
		char sm0[20] = "", sm1[20] = "", sm2[20] = "";
		fprintf(of_list_a, "//************************************************\n");
		fprintf(of_list_a, "// File Format Version %s\n", FILE_LIST_VER);
		fprintf(of_list_a, "// Janus_%" PRIu16 " Release %s\n", brdVer, SW_RELEASE_NUM);
		// Board Info
		if (WDcfg.AcquisitionMode == ACQMODE_COMMON_START && !WDcfg.TestMode)		fprintf(of_list_a, "// Acquisition Mode: Common Start\n");
		else if (WDcfg.AcquisitionMode == ACQMODE_COMMON_START && WDcfg.TestMode)	fprintf(of_list_a, "// Acquisition Mode: Test Mode");
		if (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)							fprintf(of_list_a, "// Acquisition Mode: Common Stop\n");
		if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING)							fprintf(of_list_a, "// Acquisition Mode: Trigger Matching\n");
		if (WDcfg.AcquisitionMode == ACQMODE_STREAMING)								fprintf(of_list_a, "// Acquisition Mode: Streaming\n");
		// Meas Mode
		if (WDcfg.MeasMode == MEASMODE_LEAD_ONLY)	fprintf(of_list_a, "// Measurement Mode: Lead Only\n");
		if (WDcfg.MeasMode == MEASMODE_LEAD_TRAIL)	fprintf(of_list_a, "// Measurement Mode: Lead Trail\n");
		if (WDcfg.MeasMode == MEASMODE_LEAD_TOT8)	fprintf(of_list_a, "// Measurement Mode: Lead TOT8\n");
		if (WDcfg.MeasMode == MEASMODE_LEAD_TOT11)	fprintf(of_list_a, "// Measurement Mode: Lead TOT11\n");

		if ((WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)) {
			if (WDcfg.OutFileUnit == OF_UNIT_NS) strcpy(sm1, " deltaT_ns");
			else strcpy(sm1, "deltaT_LSB");
			fprintf(of_list_a, "// deltaT_LSB = %.3f ps", 1000 * WDcfg.LeadTrail_LSB_ns);
		} else {
			if (WDcfg.OutFileUnit == OF_UNIT_NS) strcpy(sm1, "    ToA_ns");
			else strcpy(sm1, "   ToA_LSB");
			fprintf(of_list_a, "// ToA_LSB = %.3f ps", 1000 * WDcfg.LeadTrail_LSB_ns);
		}
		if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
			fprintf(of_list_a, "; ToT_LSB = %.3f ps", 1000 * WDcfg.ToT_LSB_ns);
			if (WDcfg.OutFileUnit == OF_UNIT_NS) strcpy(sm2, " ToT_ns");
			else strcpy(sm2, "ToT_LSB");
		}
		if (WDcfg.OutFileUnit == OF_UNIT_NS)  strcpy(sm0, "       Tstamp_us");
		else {
			fprintf(of_list_a, "; Tstamp_LSB = %.1f ns", CLK_PERIOD);
			strcpy(sm0, "      Tstamp_LSB");
		}

		fprintf(of_list_a, "\n");

		char mytime[100];
		strcpy(mytime, asctime(gmtime(&Stats.time_of_start)));
		mytime[strlen(mytime) - 1] = 0;
		fprintf(of_list_a, "// Run%d start time: %s UTC\n", rn, mytime);
		fprintf(of_list_a, "//************************************************\n");
		if (WDcfg.AcquisitionMode == ACQMODE_STREAMING)
			fprintf(of_list_a, "Brd  Ch E %12s", sm1);
		else if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING)
			fprintf(of_list_a, "%s        TrgID   Brd  Ch E %12s", sm0, sm1);  //       Tstamp_us
		else
			fprintf(of_list_a, "%s        TrgID   Brd  Ch %12s", sm0, sm1);
		if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) fprintf(of_list_a, " %12s", sm2);
		fprintf(of_list_a, "\n");
	}
	if (of_sync != NULL) {
		if (WDcfg.OutFileUnit == OF_UNIT_NS) fprintf(of_sync, "Brd    Tstamp_us      TrgID \n");
		else fprintf(of_sync, "Brd    Tstamp_LSB      TrgID \n");
	}
	return 0;
}

// ****************************************************
// Save List
// ****************************************************
int SaveList(int brd, double ts, uint64_t trgid, void* generic_ev, uint64_t* of_ToA, uint16_t* of_ToT, int dtq) {
	// Append event data to list file
	ListEvent_t* ev = (ListEvent_t*)generic_ev;
	if (of_list_b != NULL) {
		datatype = 0x0;
		uint32_t i;
		uint8_t b8 = brd, ch, edge;
		uint16_t size, size_hit, nh = 0;

		// Hit size :
		// Brd + Ch + ToA(64bit in Streaming, 32bit for the other) + ToT(uint16_t or float)
		size_hit = sizeof(b8) + sizeof(ch);
		if (WDcfg.OutFileUnit == OF_UNIT_LSB) {		// Not so nice
			if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) size_hit += sizeof(uint64_t); // ToA 64 bit
			else size_hit += sizeof(uint32_t); // ToA 32 bit
			if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) size_hit += sizeof(uint16_t);	// ToT 16 bit
		} else {
			if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) size_hit += sizeof(double); // ToA double
			else size_hit += sizeof(float); // ToA float
			if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) size_hit += sizeof(float);	// ToT float
		}

		if ((WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)) {
			uint64_t tmp_ToA[MAX_NCH] = {};	// Variables to write binary files  
			uint32_t tmp_ToT[MAX_NCH] = {};	// with the same structures
			uint8_t tmp_ch[MAX_NCH] = {};

			for (i = 0; i < MAX_NCH; ++i) {	// To get an array with no empty positions
				if (of_ToA[i] > 0) {
					tmp_ToA[nh] = of_ToA[i];
					tmp_ToT[nh] = of_ToT[i];
					tmp_ch[nh] = i;
					++nh;
				}
			}

			if (nh == 0) return 1;
			size = sizeof(size) + sizeof(ev->tstamp_clk) + sizeof(trgid) + sizeof(ev->nhits) + nh * size_hit;	// TstampClk and tstampUs have the same size

			fwrite(&size, sizeof(size), 1, of_list_b);
			//fwrite(&ts, sizeof(ts), 1, of_list_b);
			if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&ev->tstamp_clk, sizeof(ev->tstamp_clk), 1, of_list_b);
			else fwrite(&ev->tstamp_us, sizeof(ev->tstamp_us), 1, of_list_b);
			fwrite(&trgid, sizeof(trgid), 1, of_list_b);
			fwrite(&nh, sizeof(nh), 1, of_list_b);
			for (i = 0; i < nh; ++i) {
				uint32_t toa_lsb;
				uint16_t tot_lsb;
				float toa_ns, tot_ns;

				if (WDcfg.OutFileUnit == OF_UNIT_LSB) {
					toa_lsb = (uint32_t)tmp_ToA[i];
					tot_lsb = (uint16_t)tmp_ToT[i];
				} else {
					toa_ns = tmp_ToA[i] * WDcfg.LeadTrail_LSB_ns;
					tot_ns = tmp_ToT[i] * WDcfg.ToT_LSB_ns;
				}
				ch = tmp_ch[i];
				fwrite(&b8, sizeof(b8), 1, of_list_b);
				fwrite(&ch, sizeof(uint8_t), 1, of_list_b);
				if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&toa_lsb, sizeof(toa_lsb), 1, of_list_b);
				else fwrite(&toa_ns, sizeof(toa_ns), 1, of_list_b);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&tot_lsb, sizeof(tot_lsb), 1, of_list_b);
					else fwrite(&tot_ns, sizeof(tot_ns), 1, of_list_b);
				}
			}

		} else if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING) {
			size_hit += sizeof(edge);
			nh = ev->nhits;
			if (nh == 0) return 1;

			size = sizeof(size) + sizeof(ev->tstamp_us) + sizeof(trgid) + sizeof(ev->nhits) + nh * size_hit;

			fwrite(&size, sizeof(size), 1, of_list_b);
			if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&ev->tstamp_clk, sizeof(ev->tstamp_clk), 1, of_list_b);
			else fwrite(&ev->tstamp_us, sizeof(ev->tstamp_us), 1, of_list_b);
			fwrite(&trgid, sizeof(trgid), 1, of_list_b);
			fwrite(&nh, sizeof(nh), 1, of_list_b);
			for (i = 0; i < nh; ++i) {
				uint32_t toa_lsb;
				uint16_t tot_lsb;
				float toa_ns, tot_ns;
				int tmp_c;
				ChIndex_tdc2ada(ev->channel[i], &tmp_c);
				ch = (uint8_t)tmp_c;
				edge = (ev->edge[ch] & 1) ^ WDcfg.InvertEdgePolarity[brd][ch];	

				if (WDcfg.OutFileUnit == OF_UNIT_LSB) {
					toa_lsb = (uint32_t)of_ToA[i];
					tot_lsb = (uint16_t)of_ToT[i];
				} else {
					toa_ns = of_ToA[i] * WDcfg.LeadTrail_LSB_ns;
					tot_ns = of_ToT[i] * WDcfg.ToT_LSB_ns;
				}
				fwrite(&b8, sizeof(b8), 1, of_list_b);
				fwrite(&ch, sizeof(ch), 1, of_list_b);
				fwrite(&edge, sizeof(edge), 1, of_list_b);
				if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&toa_lsb, sizeof(toa_lsb), 1, of_list_b);
				else fwrite(&toa_ns, sizeof(toa_ns), 1, of_list_b);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&tot_lsb, sizeof(tot_lsb), 1, of_list_b);
					else fwrite(&tot_ns, sizeof(tot_ns), 1, of_list_b);
				}
			}
		} else if (WDcfg.AcquisitionMode == ACQMODE_STREAMING) {
			size_hit += sizeof(edge);
			nh = ev->nhits;
			if (nh == 0) return 1;

			size = sizeof(size) + sizeof(ev->tstamp_us) + sizeof(ev->nhits) + nh * size_hit;

			fwrite(&size, sizeof(size), 1, of_list_b);
			if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&ev->tstamp_clk, sizeof(ev->tstamp_clk), 1, of_list_b);
			else fwrite(&ev->tstamp_us, sizeof(ev->tstamp_us), 1, of_list_b);
			fwrite(&nh, sizeof(nh), 1, of_list_b);
			for (i = 0; i < nh; ++i) {
				uint64_t toa_lsb;
				uint16_t tot_lsb;
				double toa_ns;
				float tot_ns;

				int tmp_c;
				ChIndex_tdc2ada(ev->channel[i], &tmp_c);
				ch = (uint8_t)tmp_c;
				edge = (ev->edge[ch] & 1) ^ WDcfg.InvertEdgePolarity[brd][ch];

				if (WDcfg.OutFileUnit == OF_UNIT_LSB) {
					toa_lsb = of_ToA[i];
					tot_lsb = (uint16_t)of_ToT[i];
				} else {
					toa_ns = (double)of_ToA[i] * WDcfg.LeadTrail_LSB_ns;
					tot_ns = of_ToT[i] * WDcfg.ToT_LSB_ns;
				}
				fwrite(&b8, sizeof(b8), 1, of_list_b);
				fwrite(&ch, sizeof(ch), 1, of_list_b);
				fwrite(&edge, sizeof(edge), 1, of_list_b);
				if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&toa_lsb, sizeof(toa_lsb), 1, of_list_b);
				else fwrite(&toa_ns, sizeof(toa_ns), 1, of_list_b);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (WDcfg.OutFileUnit == OF_UNIT_LSB) fwrite(&tot_lsb, sizeof(tot_lsb), 1, of_list_b);
					else fwrite(&tot_ns, sizeof(tot_ns), 1, of_list_b);
				}
			}
		}
	}

	if (of_list_a != NULL) {
		uint32_t i;
		char m1[20] = "", m2[20] = "";
		uint8_t ch;

		if ((WDcfg.AcquisitionMode == ACQMODE_COMMON_START) || (WDcfg.AcquisitionMode == ACQMODE_COMMON_STOP)) {
			if (WDcfg.OutFileUnit == OF_UNIT_LSB) fprintf(of_list_a, "%16" PRIu64 "  %12" PRIu64, ev->tstamp_clk, ev->trigger_id);
			else fprintf(of_list_a, "%16.4lf %12" PRIu64 " ", ev->tstamp_us, ev->trigger_id);
			for (i = 0; i < MAX_NCH; i++) {
				if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m1, "%12.3f", (uint32_t)of_ToA[i] * WDcfg.LeadTrail_LSB_ns);
				else sprintf(m1, "%12d", (uint32_t)of_ToA[i]);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (of_ToT[i] > 0) {
						if (of_ToT[i] == 0xFFFF) sprintf(m2, "%12s", "OVF");
						else if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m2, "%12.3f", of_ToT[i] * WDcfg.ToT_LSB_ns);
						else sprintf(m2, "%12" PRIu32, of_ToT[i]);
					} else sprintf(m2, "           - ");
				}
				if (i == 0) {
					if (of_ToA[i] == 0) fprintf(of_list_a, "\n");
					else fprintf(of_list_a, "   %02d  %02d %s %s \n", brd, 0, m1, m2);
				} else {
					if (of_ToA[i] == 0) continue;
					fprintf(of_list_a, "                                 %02d  %02d %s %s \n", brd, i, m1, m2);
				}
			}
		} else if (WDcfg.AcquisitionMode == ACQMODE_TRG_MATCHING) {
			if (WDcfg.OutFileUnit == OF_UNIT_LSB) fprintf(of_list_a, "%16" PRIu64 "  %12" PRIu64, ev->tstamp_clk, ev->trigger_id);
			else fprintf(of_list_a, "%16.4lf %12" PRIu64 " ", ev->tstamp_us , ev->trigger_id);
			if (ev->nhits == 0) fprintf(of_list_a, "\n");
			for (i = 0; i < ev->nhits; i++) {
				int tmp_c;
				ChIndex_tdc2ada(ev->channel[i], &tmp_c);
				ch = (uint8_t)tmp_c;
				char edge = ((ev->edge[i] & 1) ^ WDcfg.InvertEdgePolarity[brd][ch]) == EDGE_LEAD ? 'L' : 'T';
				if (i > 0) fprintf(of_list_a, "                              ");	
				if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m1, "%12.3f", of_ToA[i] * WDcfg.LeadTrail_LSB_ns);
				else sprintf(m1, "%12" PRIu64, of_ToA[i]);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (of_ToT[i] > 0) {
						if (of_ToT[i] == 0xFFFF) sprintf(m2, "%12s", "OVF");
						else if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m2, "%12.3f", of_ToT[i] * WDcfg.ToT_LSB_ns);
						else sprintf(m2, "%12" PRIu16, of_ToT[i]);
					} else sprintf(m2, "             - ");
				}
				fprintf(of_list_a, "   %02d  %02" PRIu8 " %c %s %s \n", brd, ch, edge, m1, m2);
			}
		} else { // Streaming
			if (ev->nhits == 0) fprintf(of_list_a, "\n");
			for (i = 0; i < ev->nhits; i++) {
				int tmp_c;
				ChIndex_tdc2ada(ev->channel[i], &tmp_c);
				ch = (uint8_t)tmp_c;
				char edge = ((ev->edge[i] & 1) ^ WDcfg.InvertEdgePolarity[brd][ch]) == EDGE_LEAD ? 'L' : 'T';
				if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m1, "%12.3f", of_ToA[i] * WDcfg.LeadTrail_LSB_ns);
				else sprintf(m1, "%12" PRIu64, of_ToA[i]);
				if (WDcfg.MeasMode != MEASMODE_LEAD_ONLY) {
					if (of_ToT[i] > 0) {
						if (of_ToT[i] == 0xFFFF) sprintf(m2, "%12s", "OVF");
						else if (WDcfg.OutFileUnit == OF_UNIT_NS) sprintf(m2, "%12.3f", of_ToT[i] * WDcfg.ToT_LSB_ns);
						else sprintf(m2, "%12" PRIu16, of_ToT[i]);
					} else sprintf(m2, "           - ");
				}
				fprintf(of_list_a, " %02d  %02" PRIu8 " %c %s %s \n", brd, ch, edge, m1, m2);
			}
		}
	}
	
	if (of_sync != NULL) {
		if (WDcfg.OutFileUnit == OF_UNIT_NS) fprintf(of_sync, "%3d %12.4lf %10" PRIu64 "\n", brd, ev->tstamp_us, trgid);
		else fprintf(of_sync, "%3d %12" PRIu64 " %10" PRIu64 "\n", brd, ev->tstamp_clk, trgid);
	}

	return 0;
}


// ****************************************************
// Save Histograms
// ****************************************************
int SaveHistos()
{
	//kv 0
	//	#SPECTRUM_NAME Spettro_Prova
	//	#REAL_TIME 123.000000
	//	#LIVE_TIME 122.000000
	//	#CALIB_COEFF 0.000000 1.000000 0.000000 0.000000
	//	#UNITS keV
	//	#NUM_CHANNELS 16384
	//	#DATA
	int ch, b, i;
	char fname[500];
	char header_Lead[1000], header_ToT[1000];
	FILE *hf;

	// Create header for offline reprocessing
	sprintf(header_Lead, "#VER 1\n");  // Version
	strcpy(header_ToT, header_Lead);
	if (WDcfg.OutFileEnableMask & OUTFILE_LEAD_HISTO) {
		sprintf(header_Lead, "%s#TYPE %x\n", header_Lead, OUTFILE_LEAD_HISTO);
		sprintf(header_Lead, "%s#NBIN %d\n", header_Lead, WDcfg.LeadTrailHistoNbin);
		sprintf(header_Lead, "%s#A0 %f\n", header_Lead, WDcfg.LeadHistoMin);
		sprintf(header_Lead, "%s#A1 %f\n", header_Lead, WDcfg.LeadTrail_LSB_ns * WDcfg.LeadTrailRebin);
		sprintf(header_Lead, "%s#DATA\n", header_Lead);
	} 
	if (WDcfg.OutFileEnableMask & OUTFILE_TOT_HISTO) {
		sprintf(header_Lead, "%s#TYPE %x\n", header_Lead, OUTFILE_TOT_HISTO);
		sprintf(header_ToT, "%s#NBIN %d\n", header_ToT, WDcfg.ToTHistoNbin);
		sprintf(header_ToT, "%s#A0 %f\n", header_ToT, WDcfg.ToTHistoMin);
		sprintf(header_ToT, "%s#A1 %f\n", header_ToT, WDcfg.ToT_LSB_ns * WDcfg.ToTRebin);
		sprintf(header_ToT, "%s#DATA\n", header_ToT);
	}

	for(b=0; b<WDcfg.NumBrd; b++) {
		for(ch=0; ch<WDcfg.NumCh; ch++) {
			if (WDcfg.OutFileEnableMask & OUTFILE_LEAD_HISTO) {
				if (Stats.H1_Lead[b][ch].H_cnt > 0) {

					sprintf(fname, "%sRun%d_LeadHisto_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						fprintf(hf, "%s", header_Lead);
						for (i=0; i<(int)Stats.H1_Lead[b][ch].Nbin; i++)	// WDcfg.ToTHistoNbin
							fprintf(hf, "%" PRId32 "\n", Stats.H1_Lead[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
			if (WDcfg.OutFileEnableMask & OUTFILE_TRAIL_HISTO) {
				if (Stats.H1_Trail[b][ch].H_cnt > 0) {
					sprintf(fname, "%sRun%d_TrailHisto_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						fprintf(hf, "%s", header_Lead);
						for (i=0; i<(int)Stats.H1_Trail[b][ch].Nbin; i++)	// WDcfg.ToTHistoNbin
							fprintf(hf, "%" PRId32 "\n", Stats.H1_Trail[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
			if (WDcfg.OutFileEnableMask & OUTFILE_TOT_HISTO) {
				if (Stats.H1_ToT[b][ch].H_cnt > 0) {

					sprintf(fname, "%sRun%d_ToTHisto_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						fprintf(hf, "%s", header_ToT);
						for (i=0; i<(int)Stats.H1_ToT[b][ch].Nbin; i++)
							fprintf(hf, "%" PRId32 "\n", Stats.H1_ToT[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
		}
	}
	return 0;
}


/******************************************************
* Save Measurements
******************************************************/
int SaveMeasurements()
{
	int b, ch;
	char fname[500];
	FILE *mf;

	if (WDcfg.OutFileEnableMask & OUTFILE_LEAD_MEAS) {
		sprintf(fname, "%sRun%d_LeadMeas.txt", WDcfg.DataFilePath, RunVars.RunNumber);
		mf = fopen(fname, "w");
		if (mf != NULL) {
			fprintf(mf, "BRD  CH    MEAN(ns)    RMS(ns)\n");
			for(b=0; b<WDcfg.NumBrd; b++) {
				for(ch=0; ch<WDcfg.NumCh; ch++) {
					fprintf(mf, "%3d %3d  %10.3f %10.3f\n", b, ch, Stats.LeadMeas[b][ch].mean, Stats.LeadMeas[b][ch].rms);
				}
			}
			fclose(mf);
		}
	}
	if (WDcfg.OutFileEnableMask & OUTFILE_TRAIL_MEAS) {
		sprintf(fname, "%sRun%d_TrailMeas.txt", WDcfg.DataFilePath, RunVars.RunNumber);
		mf = fopen(fname, "w");
		if (mf != NULL) {
			fprintf(mf, "BRD  CH    MEAN(ns)    RMS(ns)\n");
			for(b=0; b<WDcfg.NumBrd; b++) {
				for(ch=0; ch<WDcfg.NumCh; ch++) {
					fprintf(mf, "%3d %3d  %10.3f %10.3f\n", b, ch, Stats.TrailMeas[b][ch].mean, Stats.TrailMeas[b][ch].rms);
				}
			}
			fclose(mf);
		}
	}
	if (WDcfg.OutFileEnableMask & OUTFILE_TOT_MEAS) {
		sprintf(fname, "%sRun%d_ToTMeas.txt", WDcfg.DataFilePath, RunVars.RunNumber);
		mf = fopen(fname, "w");
		if (mf != NULL) {
			fprintf(mf, "BRD  CH    MEAN(ns)    RMS(ns)\n");
			for(b=0; b<WDcfg.NumBrd; b++) {
				for(ch=0; ch<WDcfg.NumCh; ch++) {
					fprintf(mf, "%3d %3d  %10.3f %10.3f\n", b, ch, Stats.ToTMeas[b][ch].mean, Stats.ToTMeas[b][ch].rms);
				}
			}
			fclose(mf);
		}
	}
	return 0;
}


/******************************************************
* Save Run Info
******************************************************/
int SaveRunInfo()
{
	char str[200];
	char fname[500];
	struct tm* t;
	time_t tt;
	int b;
	FILE* cfg;
	FILE* iof;
	
	sprintf(fname, "%sRun%d_Info.txt", WDcfg.DataFilePath, RunVars.RunNumber);
	iof = fopen(fname, "w");

	uint8_t rr;

	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "Run n. %d\n\n", RunVars.RunNumber);
	tt = (time_t)(Stats.start_time / 1000); //   RO_Stats.StartTime_ms / 1000);
	t = localtime(&tt);
	strftime(str, sizeof(str) - 1, "%d/%m/%Y %H:%M:%S", t);
	fprintf(iof, "Start Time: %s\n", str);
	tt = (time_t)(Stats.stop_time / 1000); //   RO_Stats.StopTime_ms / 1000);
	t = localtime(&tt);
	strftime(str, sizeof(str) - 1, "%d/%m/%Y %H:%M:%S", t);
	fprintf(iof, "Stop Time:  %s\n", str);
	fprintf(iof, "Elapsed time = %.3f s\n", Stats.current_tstamp_us[0] / 1e6); //     RO_Stats.CurrentTimeStamp_us / 1e6);
	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Setup:\n");
	fprintf(iof, "********************************************************************* \n");
	FERS_BoardInfo_t BoardInfo;
	fprintf(iof, "Software Version: Janus %s\n", SW_RELEASE_NUM);
	for (b = 0; b < WDcfg.NumBrd; b++) {
		rr = FERS_ReadBoardInfo(handle[b], &BoardInfo); // Read Board info
		if (rr != 0)
			return -1;
		char fver[100];
		//if (FPGArev == 0) sprintf(fver, "BootLoader"); DNIN: mixed with an old version checker with register.
		//else 
		sprintf(fver, "%d.%d (Build = %04X)", (BoardInfo.FPGA_FWrev >> 8) & 0xFF, (BoardInfo.FPGA_FWrev) & 0xFF, (BoardInfo.FPGA_FWrev >> 16) & 0xFFFF);
		fprintf(iof, "Board %d:", b);
		fprintf(iof, "\tModel = %s\n", BoardInfo.ModelName);
		fprintf(iof, "\t\tPID = %" PRIu32 "\n", BoardInfo.pid);
		fprintf(iof, "\t\tFPGA FW revision = %s\n", fver);
		fprintf(iof, "\t\tuC FW revision = %08X\n", BoardInfo.uC_FWrev);
	}
	// CTIN: save event statistics
	/*
	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Statistics:\n");
	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "Total Acquired Events: %lld (Rate = %.3f Kcps)\n", (long long)RO_Stats.EventCnt, (float)RO_Stats.EventCnt/(RO_Stats.CurrentTimeStamp_us/1000));
	for (b = 0; b < WDcfg.NumBrd; b++) {
		fprintf(iof, "\nBoard %d (s.n. %d)\n", b, DGTZ_SerialNumber(handle[b]));
		fprintf(iof, "Lost Events: %lld (%.3f %%)\n", (long long)RO_Stats.LostEventCnt[b], PERCENT(RO_Stats.LostEventCnt[b], RO_Stats.LostEventCnt[b] + RO_Stats.EventCnt));
	}
	*/
	if (WDcfg.EnableJobs) {
		sprintf(fname, "%sJanus_Config_Run%d.txt", WDcfg.DataFilePath, RunVars.RunNumber);
		cfg = fopen(fname, "r");
		if (cfg == NULL) 
			sprintf(fname, "%s", CONFIG_FILENAME);
	} else 
		sprintf(fname, "%s", CONFIG_FILENAME);

	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Config file: %s\n", fname);
	fprintf(iof, "********************************************************************* \n");
	cfg = fopen(fname, "r");
	if (cfg != NULL) {
		while (!feof(cfg)) {
			char line[500];
			fgets(line, 500, cfg);
			fputs(line, iof);
		}
	}

	fclose(iof);
	return 0;
}

