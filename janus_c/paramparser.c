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
#include <ctype.h>
#include "paramparser.h"
#include "JanusC.h"
#include "console.h"
#include "FERSlib.h"
#include "adapters.h"


#define SETBIT(r, m, b)  (((r) & ~(m)) | ((m) * (b)))

int ch=-1, brd=-1;
int ValidParameterName = 0;
int ValidParameterValue = 0;
int ValidUnits = 0;

// ---------------------------------------------------------------------------------
// Description: compare two strings
// Inputs:		str1, str2: strings to compare
// Outputs:		-
// Return:		1=equal, 0=not equal
// ---------------------------------------------------------------------------------
int streq(char *str1, char *str2)
{
	if (strcmp(str1, str2) == 0) {
		ValidParameterName = 1;
		return 1;
	} else {
		return 0;
	}
}

// ---------------------------------------------------------------------------------
// Description: check if the directory exists, if not it is created
// Inputs:		WDcfg, Cfg file
// Outputs:		-
// Return:		void
// ---------------------------------------------------------------------------------
int f_mkdir(const char* path) {	// taken from CAENMultiplatform.c (https://gitlab.caen.it/software/utility/-/blob/develop/src/CAENMultiplatform.c#L216)
	int32_t ret = 0;
#ifdef _WIN32
	DWORD r = (CreateDirectoryA(path, NULL) != 0) ? 0 : GetLastError();
	switch (r) {
	case 0:
	case ERROR_ALREADY_EXISTS:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
#else
	int r = mkdir(path, ACCESSPERMS) == 0 ? 0 : errno;
	switch (r) {
	case 0:
	case EEXIST:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
#endif
	return ret;
}

void GetDatapath(FILE* f_ini, Config_t* WDcfg) {

	char mchar[200];
	fscanf(f_ini, "%s", mchar);
	if (access(mchar, 0) == 0) { // taken from https://stackoverflow.com/questions/6218325/
		struct stat status;
		stat(mchar, &status);
		int myb = status.st_mode & S_IFDIR;
		if (myb == 0) {
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: DataFilePath: %s is not a valid directory. Default .DataFiles folder is used\n" COLOR_RESET, mchar);
			strcpy(WDcfg->DataFilePath, "DataFiles");
		}
		else
			strcpy(WDcfg->DataFilePath, mchar);
	}
	else {
		int ret = f_mkdir(mchar);
		if (ret == 0)
			strcpy(WDcfg->DataFilePath, mchar);
		else {
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: DataFilePath: %s cannot be created, default .DataFiles folder is used\n" COLOR_RESET, mchar);
			strcpy(WDcfg->DataFilePath, "DataFiles");
		}
	}
}

// ---------------------------------------------------------------------------------
// Description: Read an integer (decimal) from the conig file
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		integer value read from the file / Set ValidParameterValue=0 if the string is not an integer
// ---------------------------------------------------------------------------------
int GetInt(FILE *f_ini)
{
	int ret;
	char val[50];
	//fscanf(f_ini, "%d", &ret);
	fscanf(f_ini, "%s", val);
	//fgets(val, 100, f_ini);
	int num = sscanf(val, "%d", &ret);
	if (ret < 0 || num < 1) ValidParameterValue = 0;
	else ValidParameterValue = 1;
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Read number of bins for amn histogram
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		integer value read from the file
// ---------------------------------------------------------------------------------
int GetNbin(FILE *f_ini)
{
	int i;
	char str[100];
	fscanf(f_ini, "%s", str);
	for (i=0; i<(int)strlen(str); i++)
		str[i] = toupper(str[i]);
	if		((streq(str, "DISABLED")) || (streq(str, "0")))	return 0;
	else if  (streq(str, "256"))							return 256;
	else if  (streq(str, "512"))							return 512;	else if ((streq(str, "1024")  || streq(str, "1K")))		return 1024;
	else if ((streq(str, "2048")  || streq(str, "2K")))		return 2048;
	else if ((streq(str, "4096")  || streq(str, "4K")))		return 4096;
	else if ((streq(str, "8192")  || streq(str, "8K")))		return 8192;
	else if ((streq(str, "16384") || streq(str, "16K")))	return 16384;
	else { 												
		return 1024;  // assign a default value on error
		ValidParameterValue = 0;
		//Con_printf("LCSw", "WARNING: Einvalid Nbin value %s\n", str);
	}
}



// ---------------------------------------------------------------------------------
// Description: Read an integer (hexadecimal) from the conig file
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		integer value read from the file / Set ValidParameterValue = 0 if the value is not in HEX format
// ---------------------------------------------------------------------------------
int GetHex(FILE *f_ini)
{ 
	int ret;
	char str[100];
	ValidParameterValue = 1;
	fscanf(f_ini, "%s", str);
	if ((str[1] == 'x') || (str[1] == 'X')) {
		sscanf(str + 2, "%x", &ret);
		if (str[0] != '0') ValidParameterValue = 0;	// Rise a warning for wrong HEX format 0x
		for (uint8_t i = 2; i < strlen(str); ++i) {
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	}
	else {
		sscanf(str, "%x", &ret);
		for (uint8_t i = 0; i < strlen(str); ++i) {	// Rise a warning for wrong HEX format
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	}
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Read a float from the conig file
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		float value read from the file / 
// ---------------------------------------------------------------------------------
float GetFloat(FILE *f_ini)
{
	float ret;
	char str[1000];
	int i;
	ValidParameterValue = 1;
	fgets(str, 1000, f_ini);
	// replaces ',' with '.' (decimal separator)
	for(i=0; i<(int)strlen(str); i++)
		if (str[i] == ',') str[i] = '.';
	sscanf(str, "%f", &ret);
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Read a value from the conig file followed by an optional time unit (ps, ns, us, ms, s)
//              and convert it in a time expressed in ns as a float 
// Inputs:		f_ini: config file
//				tu: time unit of the returned time value
// Outputs:		-
// Return:		time value (in ns) read from the file / Set ValidParamName/Value=0 if the expected format is not matched
// ---------------------------------------------------------------------------------
float GetTime(FILE *f_ini, char *tu)
{
	double timev=-1;
	double ns;
	char val[500];
	long fp;
	char str[100];

	int element;

	//fscanf(f_ini, "%lf", &timev);
	//fp = ftell(f_ini);
	fgets(val, 500, f_ini);
	fp = ftell(f_ini);
	element = sscanf(val, "%lf %s", &timev, str);
	//fseek(f_ini, fp, SEEK_SET);
	/*if (timev < 0) ValidParameterValue = 0;
	else ValidParameterValue = 1;*/
	// try to read the unit from the config file (string)
	//fp = ftell(f_ini);  // save current pointer before "str"
	//fscanf(f_ini, "%s", str);  // read string "str"
	ValidUnits = 1;
	if (streq(str, "ps"))		ns = timev * 1e-3;
	else if (streq(str, "ns"))	ns = timev;
	else if (streq(str, "us"))	ns = timev * 1e3;
	else if (streq(str, "ms"))	ns = timev * 1e6;
	else if (streq(str, "s"))	ns = timev * 1e9;
	else if (streq(str, "m"))	ns = timev * 60e9;
	else if (streq(str, "h"))	ns = timev * 3600e9;
	else if (streq(str, "d"))	ns = timev * 24 * (3600e9);
	else if (element == 1 || streq(str, "#")) return (float)timev;
	else {
		ValidUnits = 0;
		fseek(f_ini, fp-2, SEEK_SET); // move pointer back to beginning of "str" and use it again for next parsing
		return (float)timev;  // no time unit specified in the config file; assuming equal to the requested one (ns of default)
	}

	if (streq(tu, "ps"))		return (float)(ns*1e3);
	else if (streq(tu, "ns"))	return (float)(ns);
	else if (streq(tu, "us"))	return (float)(ns/1e3);
	else if (streq(tu, "ms"))	return (float)(ns/1e6);
	else if (streq(tu, "s") )	return (float)(ns/1e9);
	else if (streq(tu, "m") )	return (float)(ns/60e9);
	else if (streq(tu, "h") )	return (float)(ns/3600e9);
	else if (streq(tu, "d") )	return (float)(ns/(24*3600e9));
	else return (float)timev;
}

// ---------------------------------------------------------------------------------
// Description: Read a value from the conig file followed by an optional time unit (V, mV, uV)
//              and convert it in a voltage expressed in volts 
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		voltage value expressed in volts
// ---------------------------------------------------------------------------------
float GetVoltage(FILE *f_ini)
{
	float var;
	long fp;
	char str[100];

	int val0 = fscanf(f_ini, "%f", &var);
	// try to read the unit from the config file (string)
	fp = ftell(f_ini);  // save current pointer before "str"
	int val1 = fscanf(f_ini, "%s", str);  // read string "str"
	ValidUnits = 1;
	if (streq(str, "uV"))		return var / 1000000;
	else if (streq(str, "mV"))	return var / 1000;
	else if (streq(str, "V"))	return var;
	else if (val1 != 1 || streq(str, "#")) {	// no units, assumed Voltage
		fseek(f_ini, fp, SEEK_SET); // move pointer back to beginning of "str" and use it again for next parsing
		return var;
	}
	else {	// wrong units, raise warning
		ValidUnits = 0;
		fseek(f_ini, fp, SEEK_SET); // move pointer back to beginning of "str" and use it again for next parsing
		return var;  // no voltage unit specified in the config file; assuming volts
	}
}

// ---------------------------------------------------------------------------------
// Description: Read a value from the conig file followed by an optional time unit (A, mA, uA)
//              and convert it in a current expressed in mA 
// Inputs:		f_ini: config file
// Outputs:		-
// Return:		current value expressed in mA  / Set ValidParamName/Value=0 if the expected format is not matched
// ---------------------------------------------------------------------------------
float GetCurrent(FILE *f_ini)
{
	char van[50];
	float var = 0;
	char val[500] = "";
	long fp = 0;
	char stra[100] = "";
	int element0 = 0;
	int element1 = 0;

	//fscanf(f_ini, "%f", &var);
	fgets(val, 500, f_ini);
	fp = ftell(f_ini);
	//fscanf(f_ini, "%s", val);
	//element0 = sscanf(val, "%f %s", &var, stra);
	element0 = sscanf(val, "%s %s", van, stra);
	element1 = sscanf(van, "%f", &var);
	if (element1 > 0) ValidParameterValue = 1;
	else ValidParameterValue = 0;
	
	// try to read the unit from the config file (string)
	//fp = ftell(f_ini);  // save current pointer before "str"
	//fscanf(f_ini, "%s", stra);  // read string "str"
	ValidUnits = 1;
	if (streq(stra, "uA"))		return var / 1000;
	else if (streq(stra, "mA"))	return var;
	else if (streq(stra, "A"))	return var * 1000;
	else if (element1*element0 == 1 || streq(stra, "#")) {	// No units, no warning raised
		fseek(f_ini, fp - 1, SEEK_SET); // move pointer back to beginning of "str" and use it again for next parsing
		return var;
	}
	else {	// Wrong units entered
		ValidUnits = 0;
		fseek(f_ini, fp - 1, SEEK_SET); // move pointer back to beginning of "str" and use it again for next parsing
		return var;  // wrong unit specified in the config file; assuming mA
	}
}


// ---------------------------------------------------------------------------------
// Description: Set a parameter (individual board or broadcast) to a given integer value 
// Inputs:		param: array of parameters 
//				val: value to set
// Outputs:		-
// Return:		-
// ---------------------------------------------------------------------------------
void SetBoardParam(int param[MAX_NBRD], int val)
{
	int b;
	if (brd == -1)
		for (b = 0; b < MAX_NBRD; b++)
			param[b] = val;
	else
		param[brd] = val;
}

// ---------------------------------------------------------------------------------
// Description: Set a parameter (individual board or broadcast) to a given integer value 
// Inputs:		param: array of parameters 
//				val: value to set
// Outputs:		-
// Return:		-
// ---------------------------------------------------------------------------------
void SetBoardParamFloat(float param[MAX_NBRD], float val)
{
	int b;
	if (brd == -1)
		for (b = 0; b < MAX_NBRD; b++)
			param[b] = val;
	else
		param[brd] = val;
}

// ---------------------------------------------------------------------------------
// Description: Set a parameter (individual channel or broadcast) to a given integer value 
// Inputs:		param: array of parameters 
//				val: value to set
// Outputs:		-
// Return:		-
// ---------------------------------------------------------------------------------
void SetChannelParam8(uint8_t param[MAX_NBRD][MAX_NCH], int val)
{
	int i, b;
    if (brd == -1) {
        for(b=0; b<MAX_NBRD; b++)
			for(i=0; i<MAX_NCH; i++)
				param[b][i] = val;
	} else if (ch == -1) {
		for(i=0; i<MAX_NCH; i++)
			param[brd][i] = val;
	} else {
        param[brd][ch] = val;
	}
}

void SetChannelParam16(uint16_t param[MAX_NBRD][MAX_NCH], int val)
{
	int i, b;
    if (brd == -1) {
        for(b=0; b<MAX_NBRD; b++)
			for(i=0; i<MAX_NCH; i++)
				param[b][i] = val;
	} else if (ch == -1) {
		for(i=0; i<MAX_NCH; i++)
			param[brd][i] = val;
	} else {
        param[brd][ch] = val;
	}
}

void SetChannelParam32(uint32_t param[MAX_NBRD][MAX_NCH], int val)
{
	int i, b;
    if (brd == -1) {
        for(b=0; b<MAX_NBRD; b++)
			for(i=0; i<MAX_NCH; i++)
				param[b][i] = val;
	} else if (ch == -1) {
		for(i=0; i<MAX_NCH; i++)
			param[brd][i] = val;
	} else {
        param[brd][ch] = val;
	}
}

// ---------------------------------------------------------------------------------
// Description: Set a parameter (individual channel or broadcast) to a given float value 
// Inputs:		param: array of parameters 
//				val: value to set
// Outputs:		-
// Return:		-
// ---------------------------------------------------------------------------------
void SetChannelParamFloat(float param[MAX_NBRD][MAX_NCH], float val)
{
	int i, b;
    if (brd == -1) {
        for(b=0; b<MAX_NBRD; b++)
			for(i=0; i<MAX_NCH; i++)
				param[b][i] = val;
	} else if (ch == -1) {
		for(i=0; i<MAX_NCH; i++)
			param[brd][i] = val;
	} else {
        param[brd][ch] = val;
	}
}

// ---------------------------------------------------------------------------------
// Description: Parse WDcfg with parameter of a new configuration file
// Inputs:		param: file name, Config_t 
//				val: value to set
// Outputs:		-
// Return:		-
// ---------------------------------------------------------------------------------
void LoadExtCfgFile(FILE* f_ini, Config_t* WDcfg) {	// DNIN: The first initialization should not be done, it must be inside and if block [NEED TO BE CHECKED]
	char nfile[500];
	int mf = 0;
	mf = fscanf(f_ini, "%s", nfile);
	if (mf == 0) ValidParameterValue = 0;
	else {
		FILE* n_cfg;
		n_cfg = fopen(nfile, "r");
		if (n_cfg != NULL) {
			Con_printf("LCSm", "Overwriting parameters from %s\n", nfile);
			ParseConfigFile(n_cfg, WDcfg, 0);
			fclose(n_cfg);
			ValidParameterValue = 1;
			ValidParameterName = 1;
		} else {
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: Loading Macro: Macro file \"%s\" not found\n" COLOR_RESET, nfile);
			ValidParameterValue = 0;
		}
	}


}

// ---------------------------------------------------------------------------------
// Description: Read a config file, parse the parameters and set the relevant fields in the WDcfg structure
// Inputs:		f_ini: config file pinter
// Outputs:		WDcfg: struct with all parameters
// Return:		0=OK, -1=error
// ---------------------------------------------------------------------------------
int ParseConfigFile(FILE* f_ini, Config_t* WDcfg, bool fcall)
{
	char str[1000], str1[1000];
	int i, b, val; // , tr = -1;
	int brd1, ch1;  // target board/ch defined as ParamName[b][ch]
	int brd2 = -1, ch2 = -1;  // target board/ch defined as Section ([BOARD b] [CHANNEL ch])

	if (fcall) { // initialize WDcfg when it is call by Janus. No when it is call for overwrite parameters 
		memset(WDcfg, 0, sizeof(Config_t));
		/* Default settings */
		strcpy(WDcfg->DataFilePath, "DataFiles");
		WDcfg->NumBrd = 0;	
		WDcfg->NumCh = 64;
		WDcfg->GWn = 0;
		WDcfg->LeadTrailHistoNbin = 4096;
		WDcfg->ToTHistoNbin = 4096;
		WDcfg->LeadTrailRebin = 1;
		WDcfg->ToTHistoMin = 0;
		WDcfg->ToTRebin = 1;
		WDcfg->AcquisitionMode = 0;
		WDcfg->TriggerMask = 0;
		WDcfg->TriggerLogic = 0;
		WDcfg->T0_outMask = 0;
		WDcfg->T1_outMask = 0;
		WDcfg->Tref_Mask = 0;
		WDcfg->TrgWindowWidth = 1000;
		WDcfg->GateWidth = -500;
		WDcfg->TrgWindowOffset = -500;
		WDcfg->PtrgPeriod = 0;
		WDcfg->DigitalProbe[0] = 0;
		WDcfg->DigitalProbe[1] = -1;  // not assigned
		WDcfg->En_Head_Trail = 1;      // One word chip trailer 
		WDcfg->HeaderField0 = 4;
		WDcfg->HeaderField1 = 5;
		WDcfg->En_Empty_Ev_Suppr = 0;  //Enable empty event suppression: 0=DISABLED, 1=ENABLED. Works only if En_Head_Trail != 2 (SUPPRESSED)
		for(i=0; i<6; i++) 
			WDcfg->WalkFitCoeff[i] = (i == 1) ? (float)1 : (float)0;
		WDcfg->En_128_ch = 0;
		WDcfg->En_service_event = 1;	
		WDcfg->TriggerBufferSize = 16;
		WDcfg->ToT_reject_low_thr = 0;
		WDcfg->ToT_reject_high_thr = 0;
		WDcfg->TrgHoldOff = 0;
		WDcfg->MaxPck_Train = 0; // 0=use default in FW
		WDcfg->MaxPck_Block = 0; // 0=use default in FW
		WDcfg->CncBufferSize = 0; // 0=use default in FW
		WDcfg->DataAnalysis = 0xFFFF; // enable all

		for (b = 0; b < MAX_NBRD; b++) {
			WDcfg->ChEnableMask0[b] = 0xFFFFFFFF;
			WDcfg->ChEnableMask1[b] = 0xFFFFFFFF;
			WDcfg->ChEnableMask2[b] = 0xFFFFFFFF;
			WDcfg->ChEnableMask3[b] = 0xFFFFFFFF;
			for (i = 0; i < PICOTDC_NCH; i++) {
				WDcfg->Ch_Offset[b][i] = 0;
			}
		}
	}
	
	//read config file and assign parameters 
	while(!feof(f_ini)) {
		int read;
		char *cb, *cc;

		brd1 = -1;
		ch1 = -1;
		ValidParameterName = 0;
        // read a word from the file
        read = fscanf(f_ini, "%s", str);
        if( !read || (read == EOF) || !strlen(str))
			continue;

        // skip comments
        if (str[0] == '#') {
			fgets(str, 1000, f_ini);
			continue;
		}

		ValidParameterValue = 1;
		ValidUnits = 1;
        // Section (COMMON or individual channel)
		if (str[0] == '[')	{
			ValidParameterName = 1;
			if (strstr(str, "COMMON")!=NULL) {
				brd2 = -1;
				ch2 = -1;
			} else if (strstr(str, "BOARD")!=NULL) {
				ch2 = -1;
				fscanf(f_ini, "%s", str1);
				sscanf(str1, "%d", &val);
				if (val < 0 || val >= MAX_NBRD) Con_printf("LCSm", "%s: Invalid board number\n", str);
				else brd2 = val;
			} else if (strstr(str, "CHANNEL") != NULL) {
				fscanf(f_ini, "%s", str1);
				sscanf(str1, "%d", &val);
				if (val < 0 || val >= MAX_NCH) Con_printf("LCSm", "%s: Invalid channel number\n", str);
				if ((brd2 == -1) && (WDcfg->NumBrd == 1)) brd2 = 0;
				else ch2 = val;
			}
		} else if ((cb = strstr(str, "[")) != NULL) {
			char *ei;
			sscanf(cb+1, "%d", &brd1); 
			if ((ei = strstr(cb, "]")) == NULL) {
				Con_printf("LCSm", "%s: Invalid board index\n", str);
				fgets(str1, 1000, f_ini);
			}
			if ((cc = strstr(ei, "[")) != NULL) {
				sscanf(cc+1, "%d", &ch1);
				if ((ei = strstr(cc, "]")) == NULL) {
					Con_printf("LCSm", "%s: Invalid channel index\n", str);
					fgets(str1, 1000, f_ini);
				}
			}
			*cb = 0;
		}
		if ((brd1 >= 0) && (brd1 < MAX_NBRD) && (ch1 < MAX_NCH)) {
			brd = brd1;
			ch = ch1;
		} else {
			brd = brd2;
			ch = ch2;
		}

		// Some name replacement for back compatibility
		if (streq(str, "BunchTrgSource"))		sprintf(str, "TriggerSource");
		if (streq(str, "DwellTime"))			sprintf(str, "PtrgPeriod");
		if (streq(str, "TrgTimeWindow"))		sprintf(str, "TstampCoincWindow");

 		if (streq(str, "Open"))	{
			if (brd==-1) {
				Con_printf("LCSm", "%s: cannot be a common setting (must be in a [BOARD] section)\n", str); 
				fgets(str1, 1000, f_ini);
			} else {
				fscanf(f_ini, "%s", str1);
				if (streq(WDcfg->ConnPath[brd], ""))
					WDcfg->NumBrd++;
				strcpy(WDcfg->ConnPath[brd], str1);
				//WDcfg->NumBrd++;
			}
		}
		if (streq(str, "WriteRegister")) {	
			if (WDcfg->GWn < MAX_GW) {
				WDcfg->GWbrd[WDcfg->GWn]=brd;
				fscanf(f_ini, "%x", (int *)&WDcfg->GWaddr[WDcfg->GWn]);
				fscanf(f_ini, "%x", (int *)&WDcfg->GWdata[WDcfg->GWn]);
				fscanf(f_ini, "%x", (int *)&WDcfg->GWmask[WDcfg->GWn]);
				WDcfg->GWn++;
			} else {
				Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: MAX_GW Generic Write exceeded (%d). Change MAX_GW and recompile\n" COLOR_RESET, MAX_GW);
			}
		}
		if (streq(str, "WriteRegisterBits")) {
			if (WDcfg->GWn < MAX_GW) {
				int start, stop, data;
				WDcfg->GWbrd[WDcfg->GWn]=brd;
				fscanf(f_ini, "%x", (int *)&WDcfg->GWaddr[WDcfg->GWn]);
				fscanf(f_ini, "%d", &start);
				fscanf(f_ini, "%d", &stop);
				fscanf(f_ini, "%d", &data);
				WDcfg->GWmask[WDcfg->GWn] = ((1<<(stop-start+1))-1) << start;
				WDcfg->GWdata[WDcfg->GWn] = ((uint32_t)data << start) & WDcfg->GWmask[WDcfg->GWn];
				WDcfg->GWn++;
			} else {
				Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: MAX_GW Generic Write exceeded (%d). Change MAX_GW and recompile\n" COLOR_RESET, MAX_GW);
			}
		}
		if (streq(str, "AcquisitionMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "COMMON_START"))	WDcfg->AcquisitionMode = ACQMODE_COMMON_START;
			else if	(streq(str1, "COMMON_STOP"))	WDcfg->AcquisitionMode = ACQMODE_COMMON_STOP;	
			else if (streq(str1, "STREAMING"))      WDcfg->AcquisitionMode = ACQMODE_STREAMING;  
			else if	(streq(str1, "TRG_MATCHING"))	WDcfg->AcquisitionMode = ACQMODE_TRG_MATCHING;
			else if (streq(str1, "TEST_MODE") || streq(str1, "TEST_MODE_1")) {
				WDcfg->AcquisitionMode = ACQMODE_COMMON_START;
				WDcfg->TestMode = 1;
			}
			else if (streq(str1, "TEST_MODE_2")) {
				WDcfg->AcquisitionMode = ACQMODE_COMMON_START;
				WDcfg->TestMode = 2;
			}
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "MeasMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "LEAD_ONLY"))		WDcfg->MeasMode = MEASMODE_LEAD_ONLY;
			else if	(streq(str1, "LEAD_TRAIL"))		WDcfg->MeasMode = MEASMODE_LEAD_TRAIL;	
			else if	(streq(str1, "LEAD_TOT8"))		WDcfg->MeasMode = MEASMODE_LEAD_TOT8;	
			else if	(streq(str1, "LEAD_TOT11"))		WDcfg->MeasMode = MEASMODE_LEAD_TOT11;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "StartRunMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "MANUAL"))			WDcfg->StartRunMode = STARTRUN_ASYNC;  // keep "MANUAL" option for backward compatibility
			else if	(streq(str1, "ASYNC"))			WDcfg->StartRunMode = STARTRUN_ASYNC;  
			else if	(streq(str1, "CHAIN_T0"))		WDcfg->StartRunMode = STARTARUN_CHAIN_T0;  
			else if	(streq(str1, "CHAIN_T1"))		WDcfg->StartRunMode = STARTRUN_CHAIN_T1;  
			else if	(streq(str1, "TDL"))			WDcfg->StartRunMode = STARTRUN_TDL;  
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "StopRunMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "MANUAL"))			WDcfg->StopRunMode = STOPRUN_MANUAL;
			else if	(streq(str1, "PRESET_TIME"))	WDcfg->StopRunMode = STOPRUN_PRESET_TIME;
			else if	(streq(str1, "PRESET_COUNTS"))	WDcfg->StopRunMode = STOPRUN_PRESET_COUNTS;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "TriggerSource")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "SW_ONLY"))			WDcfg->TriggerMask = 0x1;
			else if	(streq(str1, "T1-IN"))				WDcfg->TriggerMask = 0x3;
			else if	(streq(str1, "T0-IN"))				WDcfg->TriggerMask = 0x11;
			else if	(streq(str1, "PTRG"))				WDcfg->TriggerMask = 0x21;
			else if	(streq(str1, "EDGE_CONN"))			WDcfg->TriggerMask = 0x41;
			else if	(streq(str1, "EDGE_CONN_MB"))		WDcfg->TriggerMask = 0x41;
			else if	(streq(str1, "EDGE_CONN_PB"))		WDcfg->TriggerMask = 0x81;
			else if	(streq(str1, "MASK"))				fscanf(f_ini, "%x", &WDcfg->TriggerMask);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "TrefSource")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "ZERO") || streq(str1, "0") || streq(str1, "CH0"))	WDcfg->Tref_Mask = 0x0;
			else if	(streq(str1, "T0-IN"))			WDcfg->Tref_Mask = 0x1;
			else if	(streq(str1, "T1-IN"))			WDcfg->Tref_Mask = 0x2;
			else if	(streq(str1, "PTRG"))			WDcfg->Tref_Mask = 0x10;
			else if	(streq(str1, "MASK"))			fscanf(f_ini, "%x", &WDcfg->Tref_Mask);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "TrgIdMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "TRIGGER_CNT"))	WDcfg->TrgIdMode = 0;
			else if	(streq(str1, "VALIDATION_CNT"))	WDcfg->TrgIdMode = 1;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "A5256_Ch0Polarity")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "POSITIVE"))		WDcfg->A5256_Ch0Polarity = A5256_CH0_POSITIVE;
			else if (streq(str1, "NEGATIVE"))		WDcfg->A5256_Ch0Polarity = A5256_CH0_NEGATIVE;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n", str, str1);
		}
		if (streq(str, "En_Head_Trail")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "KEEP_ALL") || streq(str1, "0"))		WDcfg->En_Head_Trail = 0;
			else if (streq(str1, "ONE_WORD") || streq(str1, "1"))	WDcfg->En_Head_Trail = 1;
			else if (streq(str1, "SUPPRESSED") || streq(str1, "2"))	WDcfg->En_Head_Trail = 2;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "En_service_event")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "DISABLED") || streq(str1, "0"))		WDcfg->En_service_event = 0;
			else if (streq(str1, "ENABLED") || streq(str1, "1"))	WDcfg->En_service_event = 1;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "En_128_ch")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "DISABLED"))			WDcfg->En_128_ch = 0;
			else if (streq(str1, "ENABLED"))		WDcfg->En_128_ch = 1;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "En_Empty_Ev_Suppr")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "DISABLED") || streq(str1, "0"))			WDcfg->En_Empty_Ev_Suppr = 0;
			else if (streq(str1, "ENABLED") || streq(str1, "1"))		WDcfg->En_Empty_Ev_Suppr = 1;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "Dis_tdl")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "DISABLED"))			WDcfg->Dis_tdl = 0;
			else if (streq(str1, "ENABLED"))		WDcfg->Dis_tdl = 1;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "VetoSource")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "DISABLED"))		WDcfg->Veto_Mask = 0x0;
			else if	(streq(str1, "SW_CMD"))			WDcfg->Veto_Mask = 0x1;
			else if	(streq(str1, "T0-IN"))			WDcfg->Veto_Mask = 0x2;
			else if	(streq(str1, "T1-IN"))			WDcfg->Veto_Mask = 0x4;
			else if	(streq(str1, "MASK"))			fscanf(f_ini, "%x", &WDcfg->Veto_Mask);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "EventBuildingMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "DISABLED"))		WDcfg->EventBuildingMode = EVBLD_DISABLED;
			else if	(streq(str1, "TRGTIME_SORTING"))WDcfg->EventBuildingMode = EVBLD_TRGTIME_SORTING;
			else if	(streq(str1, "TRGID_SORTING"))	WDcfg->EventBuildingMode = EVBLD_TRGID_SORTING;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "DataAnalysis")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "NONE") || streq(str1, "DISABLED"))	WDcfg->DataAnalysis = 0;
			else if	(streq(str1, "CNT_ONLY"))		WDcfg->DataAnalysis = DATA_ANALYSIS_CNT;
			else if	(streq(str1, "CNT+MEAS"))		WDcfg->DataAnalysis = DATA_ANALYSIS_CNT | DATA_ANALYSIS_MEAS;
			else if	(streq(str1, "CNT+HISTO"))		WDcfg->DataAnalysis = DATA_ANALYSIS_CNT | DATA_ANALYSIS_HISTO;
			else if	(streq(str1, "ALL"))			WDcfg->DataAnalysis = DATA_ANALYSIS_CNT | DATA_ANALYSIS_HISTO | DATA_ANALYSIS_MEAS;
			else if	(streq(str1, "MASK"))			fscanf(f_ini, "%x", &WDcfg->DataAnalysis);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "T0_Out")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "TRG_SOURCE") || streq(str1, "BUNCHTRG")) sprintf(str1, "TRIGGER");  // backward compatibility
			if		(streq(str1, "T0-IN"))			WDcfg->T0_outMask = 0x001;
			else if	(streq(str1, "TRIGGER"))		WDcfg->T0_outMask = 0x002;
			else if	(streq(str1, "RUN"))			WDcfg->T0_outMask = 0x008;
			else if	(streq(str1, "PTRG"))			WDcfg->T0_outMask = 0x010;
			else if	(streq(str1, "BUSY"))			WDcfg->T0_outMask = 0x020;
			else if	(streq(str1, "DPROBE"))			WDcfg->T0_outMask = 0x040;
			else if	(streq(str1, "SQ_WAVE"))		WDcfg->T0_outMask = 0x100;
			else if	(streq(str1, "TDL_SYNC"))		WDcfg->T0_outMask = 0x200;
			else if	(streq(str1, "RUN_SYNC"))		WDcfg->T0_outMask = 0x400;
			else if	(streq(str1, "ZERO"))			WDcfg->T0_outMask = 0x000;
			else if	(streq(str1, "MASK"))			fscanf(f_ini, "%x", &WDcfg->T0_outMask);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "T1_Out")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "TRG_SOURCE") || streq(str1, "BUNCHTRG")) sprintf(str1, "TRIGGER");  // backward compatibility
			if		(streq(str1, "T1-IN"))			WDcfg->T1_outMask = 0x001;
			else if	(streq(str1, "TRIGGER"))		WDcfg->T1_outMask = 0x002;
			else if	(streq(str1, "RUN"))			WDcfg->T1_outMask = 0x008;
			else if	(streq(str1, "PTRG"))			WDcfg->T1_outMask = 0x010;
			else if	(streq(str1, "BUSY"))			WDcfg->T1_outMask = 0x020;
			else if	(streq(str1, "DPROBE"))			WDcfg->T1_outMask = 0x040;
			else if	(streq(str1, "SQ_WAVE"))		WDcfg->T1_outMask = 0x100;
			else if	(streq(str1, "TDL_SYNC"))		WDcfg->T1_outMask = 0x200;
			else if	(streq(str1, "RUN_SYNC"))		WDcfg->T1_outMask = 0x400;
			else if	(streq(str1, "ZERO"))			WDcfg->T1_outMask = 0x000;
			else if	(streq(str1, "MASK"))			fscanf(f_ini, "%x", &WDcfg->T1_outMask);
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "DigitalProbe") || streq(str, "DigitalProbe0") || streq(str, "DigitalProbe1")) {
			int dp = 0;
			if (streq(str, "DigitalProbe1")) dp = 1;
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "CLK_1024"))			WDcfg->DigitalProbe[dp] = DPROBE_CLK_1024;
			else if (streq(str1, "TRG_ACCEPTED"))		WDcfg->DigitalProbe[dp] = DPROBE_TRG_ACCEPTED;
			else if (streq(str1, "TRG_REJECTED"))		WDcfg->DigitalProbe[dp] = DPROBE_TRG_REJECTED;		
			else if (streq(str1, "TX_DATA_VALID"))		WDcfg->DigitalProbe[dp] = DPROBE_TX_DATA_VALID;
			else if (streq(str1, "TX_PCK_COMMIT"))		WDcfg->DigitalProbe[dp] = DPROBE_TX_PCK_COMMIT;
			else if (streq(str1, "TX_PCK_ACCEPTED"))	WDcfg->DigitalProbe[dp] = DPROBE_TX_PCK_ACCEPTED;
			else if (streq(str1, "TX_PCK_REJECTED"))	WDcfg->DigitalProbe[dp] = DPROBE_TX_PCK_REJECTED;
			else if (streq(str1, "TDC_DATA_VALID"))		WDcfg->DigitalProbe[dp] = DPROBE_TDC_DATA_VALID;
			else if (streq(str1, "TDC_DATA_COMMIT"))	WDcfg->DigitalProbe[dp] = DPROBE_TDC_DATA_COMMIT;
			else if	(strstr(str1, "ACQCTRL") != NULL) {
				char *c = strchr(str1, '_');
				sscanf(c+1, "%d", &val);
				WDcfg->DigitalProbe[dp] = val;
			} else if (strstr(str1, "TDC") != NULL) {
				char *c = strchr(str1, '_');
				sscanf(c+1, "%d", &val);
				WDcfg->DigitalProbe[dp] = 0x10 | val;
			} else if (strstr(str1, "DTBLD") != NULL) {
				char *c = strchr(str1, '_');
				sscanf(c+1, "%d", &val);
				WDcfg->DigitalProbe[dp] = 0x20 | val;
			} else if (strstr(str1, "TDL") != NULL) {
				char* c = strchr(str1, '_');
				sscanf(c + 1, "%d", &val);
				WDcfg->DigitalProbe[dp] = 0x30 | val;
			} else if (strstr(str1, "PMP") != NULL) {
				char* c = strchr(str1, '_');
				sscanf(c + 1, "%d", &val);
				WDcfg->DigitalProbe[dp] = 0x40 | val;
			} else if	((str1[0]=='0') && (tolower(str1[1])=='x')) {
				sscanf(str1+2, "%x", &val);
				WDcfg->DigitalProbe[dp] = val;
			} else Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
			if (streq(str, "DigitalProbe")) WDcfg->DigitalProbe[1] = WDcfg->DigitalProbe[0];
		}
		if (streq(str, "GlitchFilterMode")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "DISABLED"))		WDcfg->GlitchFilterMode = GLITCHFILTERMODE_DISABLED;
			else if	(streq(str1, "TRAILING"))		WDcfg->GlitchFilterMode = GLITCHFILTERMODE_TRAILING;
			else if	(streq(str1, "LEADING"))		WDcfg->GlitchFilterMode = GLITCHFILTERMODE_LEADING;
			else if	(streq(str1, "BOTH"))			WDcfg->GlitchFilterMode = GLITCHFILTERMODE_BOTH;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "TDC_ChannelBufferSize")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "4"))		WDcfg->TDC_ChBufferSize = 0x0;
			else if	(streq(str1, "8"))		WDcfg->TDC_ChBufferSize = 0x1;
			else if	(streq(str1, "16"))		WDcfg->TDC_ChBufferSize = 0x2;
			else if	(streq(str1, "32"))		WDcfg->TDC_ChBufferSize = 0x3;
			else if	(streq(str1, "64"))		WDcfg->TDC_ChBufferSize = 0x4;
			else if	(streq(str1, "128"))	WDcfg->TDC_ChBufferSize = 0x5;
			else if	(streq(str1, "256"))	WDcfg->TDC_ChBufferSize = 0x6;
			else if	(streq(str1, "512"))	WDcfg->TDC_ChBufferSize = 0x7;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "OF_OutFileUnit")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "LSB"))			WDcfg->OutFileUnit = OF_UNIT_LSB;
			else if	(streq(str1, "ns"))				WDcfg->OutFileUnit = OF_UNIT_NS;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "AdapterType")) {
			fscanf(f_ini, "%s", str1);
			if (streq(str1, "NONE"))				WDcfg->AdapterType = ADAPTER_NONE;
			else if (streq(str1, "A5256_REV0_POS"))	WDcfg->AdapterType = ADAPTER_A5256_REV0_POS;
			else if (streq(str1, "A5256_REV0_NEG"))	WDcfg->AdapterType = ADAPTER_A5256_REV0_NEG;
			else if (streq(str1, "A5256"))			WDcfg->AdapterType = ADAPTER_A5256;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "HighResClock")) {
			fscanf(f_ini, "%s", str1);
			if		(streq(str1, "DISABLED"))		WDcfg->HighResClock = HRCLK_DISABLED;
			else if	(streq(str1, "DAISY_CHAIN"))	WDcfg->HighResClock = HRCLK_DAISY_CHAIN;
			else if	(streq(str1, "FAN_OUT"))		WDcfg->HighResClock = HRCLK_FAN_OUT;
			else 	Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: invalid setting %s\n" COLOR_RESET, str, str1);
		}
		if (streq(str, "ChEnable")) {
			int en;
			fscanf(f_ini, "%d", &en);
			if ((ch >= 0) && (ch < 32))
				WDcfg->ChEnableMask0[brd] = (WDcfg->ChEnableMask0[brd] & ~(1 << ch)) | (en << ch);
			else if ((ch >= 32) && (ch < 64))
				WDcfg->ChEnableMask1[brd] = (WDcfg->ChEnableMask1[brd] & ~(1 << ch)) | (en << ch);
			else if ((ch >= 64) && (ch < 96))
				WDcfg->ChEnableMask2[brd] = (WDcfg->ChEnableMask2[brd] & ~(1 << ch)) | (en << ch);
			else if ((ch >= 96) && (ch < 128))
				WDcfg->ChEnableMask3[brd] = (WDcfg->ChEnableMask3[brd] & ~(1 << ch)) | (en << ch);
		}
		if (streq(str, "FiberDelayAdjust")) {
			int np, cnc, chain, node;
			float length; // length expressed in m
			np = fscanf(f_ini, "%d %d %d %f", &cnc, &chain, &node, &length);
			if ((np == 4) && (cnc >= 0) && (cnc < MAX_NCNC) && (chain >= 0) && (chain < 8) && (node >= 0) && (node < 16))
				WDcfg->FiberDelayAdjust[cnc][chain][node] = length;
		}

		if (streq(str, "DataFilePath"))				GetDatapath(f_ini, WDcfg);
		if (streq(str, "LeadTrailHistoNbin"))		WDcfg->LeadTrailHistoNbin	= GetNbin(f_ini);
		if (streq(str, "ToTHistoNbin"))				WDcfg->ToTHistoNbin			= GetNbin(f_ini);
		if (streq(str, "LeadHistoMin"))				WDcfg->LeadHistoMin			= GetTime(f_ini, "ns");
		if (streq(str, "TrailHistoMin"))			WDcfg->TrailHistoMin		= GetTime(f_ini, "ns");
		if (streq(str, "LeadTrailRebin"))			WDcfg->LeadTrailRebin		= GetInt(f_ini);
		if (streq(str, "ToTHistoMin"))				WDcfg->ToTHistoMin			= GetTime(f_ini, "ns");
		if (streq(str, "ToTRebin"))					WDcfg->ToTRebin				= GetInt(f_ini);
		if (streq(str, "ToT_LSB"))					WDcfg->ToT_LSB				= GetInt(f_ini);
		if (streq(str, "LeadTrail_LSB"))			WDcfg->LeadTrail_LSB		= GetInt(f_ini);
		if (streq(str, "EnableWalkCorrection"))		WDcfg->EnableWalkCorrection = GetInt(f_ini);
		if (streq(str, "GlitchFilterDelay"))		WDcfg->GlitchFilterDelay	= GetInt(f_ini);

		if (streq(str, "JobFirstRun"))				WDcfg->JobFirstRun			= GetInt(f_ini);
		if (streq(str, "JobLastRun"))				WDcfg->JobLastRun			= GetInt(f_ini);
		if (streq(str, "RunSleep"))					WDcfg->RunSleep				= GetTime(f_ini, "s");
		if (streq(str, "EnableJobs"))				WDcfg->EnableJobs			= GetInt(f_ini);
		if (streq(str, "DebugLogMask"))				WDcfg->DebugLogMask			= GetHex(f_ini);
		if (streq(str, "OutFileEnableMask"))		WDcfg->OutFileEnableMask	= GetHex(f_ini);
		if (streq(str, "OF_RawBin"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_RAW_DATA_BIN, GetInt(f_ini));
		if (streq(str, "OF_RawAscii"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_RAW_DATA_ASCII, GetInt(f_ini));
		if (streq(str, "OF_ListBin"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_LIST_BIN, GetInt(f_ini));
		if (streq(str, "OF_ListAscii"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_LIST_ASCII, GetInt(f_ini));
		if (streq(str, "OF_Sync"))					WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_SYNC, GetInt(f_ini));
		if (streq(str, "OF_LeadHisto"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_LEAD_HISTO, GetInt(f_ini));
		if (streq(str, "OF_TrailHisto"))			WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_TRAIL_HISTO, GetInt(f_ini));
		if (streq(str, "OF_ToTHisto"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_TOT_HISTO, GetInt(f_ini));
		if (streq(str, "OF_LeadMeas"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_LEAD_MEAS, GetInt(f_ini));
		//if (streq(str, "OF_TrailMeas"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_TRAIL_MEAS, GetInt(f_ini));
		if (streq(str, "OF_ToTMeas"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_TOT_MEAS, GetInt(f_ini));
		if (streq(str, "OF_RunInfo"))				WDcfg->OutFileEnableMask	= SETBIT(WDcfg->OutFileEnableMask, OUTFILE_RUN_INFO, GetInt(f_ini));
		if (streq(str, "TstampCoincWindow"))		WDcfg->TstampCoincWindow	= (uint32_t)GetTime(f_ini, "ns");
		if (streq(str, "TDCpulser_Width"))			WDcfg->TDCpulser_Width		= GetTime(f_ini, "ns");
		if (streq(str, "TDCpulser_Period"))			WDcfg->TDCpulser_Period		= GetTime(f_ini, "ns");
		if (streq(str, "ToT_reject_low_thr"))		WDcfg->ToT_reject_low_thr	= GetInt(f_ini);
		if (streq(str, "ToT_reject_high_thr"))		WDcfg->ToT_reject_high_thr	= GetInt(f_ini);
		if (streq(str, "TrgHoldOff"))				WDcfg->TrgHoldOff			= GetInt(f_ini);
		if (streq(str, "PresetTime"))				WDcfg->PresetTime			= GetTime(f_ini, "s");
		if (streq(str, "PresetCounts"))				WDcfg->PresetCounts			= GetInt(f_ini);
		if (streq(str, "GateWidth"))				WDcfg->GateWidth			= GetTime(f_ini, "ns");
		if (streq(str, "TrgWindowWidth"))			WDcfg->TrgWindowWidth		= GetTime(f_ini, "ns");
		if (streq(str, "TrgWindowOffset"))			WDcfg->TrgWindowOffset		= GetTime(f_ini, "ns");
		if (streq(str, "PtrgPeriod"))				WDcfg->PtrgPeriod			= GetTime(f_ini, "ns");
		if (streq(str, "RunNumber_AutoIncr"))		WDcfg->RunNumber_AutoIncr	= GetInt(f_ini);
		if (streq(str, "TestMode"))					WDcfg->TestMode				= GetInt(f_ini);
		if (streq(str, "TriggerBufferSize"))		WDcfg->TriggerBufferSize	= GetInt(f_ini);
		if (streq(str, "MaxPck_Train"))				WDcfg->MaxPck_Train			= GetInt(f_ini);
		if (streq(str, "MaxPck_Block"))				WDcfg->MaxPck_Block			= GetInt(f_ini);
		if (streq(str, "CncBufferSize"))			WDcfg->CncBufferSize		= GetInt(f_ini);

		if (streq(str, "DiscrThreshold"))			SetChannelParamFloat(WDcfg->DiscrThreshold,		GetFloat(f_ini));
		if (streq(str, "DiscrThreshold2"))			SetChannelParamFloat(WDcfg->DiscrThreshold2,	GetFloat(f_ini));
		if (streq(str, "Ch_Offset"))				SetChannelParam32(WDcfg->Ch_Offset,		(uint16_t)GetTime(f_ini, "ns"));
		if (streq(str, "InvertEdgePolarity"))		SetChannelParam8(WDcfg->InvertEdgePolarity,	(uint8_t)GetInt(f_ini));

		if (streq(str, "ChEnableMask0"))			SetBoardParam((int *)WDcfg->ChEnableMask0,			GetHex(f_ini));
		if (streq(str, "ChEnableMask1"))			SetBoardParam((int *)WDcfg->ChEnableMask1,			GetHex(f_ini));
		if (streq(str, "ChEnableMask2"))			SetBoardParam((int*)WDcfg->ChEnableMask2,		    GetHex(f_ini));
		if (streq(str, "ChEnableMask3"))			SetBoardParam((int*)WDcfg->ChEnableMask3,           GetHex(f_ini));
		if (streq(str, "DisableThresholdCalib"))	SetBoardParam((int*)WDcfg->DisableThresholdCalib,   GetInt(f_ini));
	
		if (streq(str, "Load"))						LoadExtCfgFile(f_ini, WDcfg);

		if (streq(str, "WalkFitCoeff")) {
			float c[6] = { 0 };
			int nc;
			fgets(str1, 500, f_ini);
			nc = sscanf(str1, "%f %f %f %f %f %f", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5]);
			for(i=0; (i<nc) && (i<6); i++) 
				WDcfg->WalkFitCoeff[i] = c[i];
		}

		if (!ValidParameterName || !ValidParameterValue || !ValidUnits) {
			if (!ValidUnits && ValidParameterValue)
				Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: unkwown units. Janus use as default V, mA, ns\n" COLOR_RESET, str);
			else
				Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: unkwown parameter\n" COLOR_RESET, str);
			fgets(str, (int)strlen(str)-1, f_ini);
		}
	}
	if (!fcall) return 0; // The following code have to be run just once, in the first call of paramparser.c

	// Use  in CStart/Stop
	//if (WDcfg->AcquisitionMode == ACQMODE_COMMON_START || WDcfg->AcquisitionMode == ACQMODE_COMMON_STOP)
	//	WDcfg->TrgWindowWidth = WDcfg->GateWindowWidth;

	// Enable/Disable Histograms - implemented for back compatibility of Janus code
	if (!(WDcfg->DataAnalysis & DATA_ANALYSIS_HISTO)) {
		WDcfg->LeadTrailHistoNbin = 0;
		WDcfg->ToTHistoNbin = 0;
	}

	if (MEASMODE_OWLT(WDcfg->MeasMode)) {
		WDcfg->LeadTrail_Rescale = 0; 
		WDcfg->ToT_Rescale = 0; 
	} else {
		WDcfg->LeadTrail_Rescale = WDcfg->LeadTrail_LSB; 
		WDcfg->ToT_Rescale = WDcfg->ToT_LSB; 
	}
	WDcfg->LeadTrail_LSB_ns = (float)(TDC_LSB_ps * (1 << WDcfg->LeadTrail_LSB) / 1000);
	WDcfg->ToT_LSB_ns = (float)(TDC_LSB_ps * (1 << WDcfg->ToT_LSB) / 1000);

	// Warning that LEAD_TOT cannot be used in STREAMING
	if (WDcfg->AcquisitionMode == ACQMODE_STREAMING) {
		if (WDcfg->MeasMode == MEASMODE_LEAD_TOT8 || WDcfg->MeasMode == MEASMODE_LEAD_TOT11) {
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: Meas Mode: LEAD+TOT modes cannot be used in STREAMING. MeasMode switched to LEAD_ONLY\n" COLOR_RESET);
			WDcfg->MeasMode = MEASMODE_LEAD_ONLY;
		}
	}

	// Warning if TrgWindow(GateWindow) > DynamicRange ToA, to avoid ToA rollOver
	int two_bit = 0;
	float GateTrgWindow = 0;
	char pName[100];
	if (WDcfg->En_Head_Trail == 1 || WDcfg->En_Head_Trail == 2) two_bit = 0;
	else two_bit = 1;
	if (WDcfg->AcquisitionMode == ACQMODE_COMMON_START || WDcfg->AcquisitionMode == ACQMODE_COMMON_STOP) {
		GateTrgWindow = WDcfg->GateWidth;
		sprintf(pName, "GateWidth");
	} else {
		GateTrgWindow = WDcfg->TrgWindowWidth;
		sprintf(pName, "TrgWindowWidth");
	}

	if (WDcfg->MeasMode == MEASMODE_LEAD_ONLY || WDcfg->MeasMode == MEASMODE_LEAD_TRAIL) {
		if (GateTrgWindow > ((1 << (24 + 2*two_bit)) * (WDcfg->LeadTrail_LSB_ns))) {
			GateTrgWindow = (1 << (24 + 2 * two_bit)) * WDcfg->LeadTrail_LSB_ns;
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: larger than Lead dynamic range. Value set to %.0f ns\n" COLOR_RESET, pName, (1 << (24 + 2 * two_bit))* (WDcfg->LeadTrail_LSB_ns));
		}
	} else if (WDcfg->MeasMode == MEASMODE_LEAD_TOT8) {
		if (GateTrgWindow > ((1 << (17 + 2 * two_bit)) * (WDcfg->LeadTrail_LSB_ns))) {
			GateTrgWindow = (1 << (17 + 2 * two_bit)) * WDcfg->LeadTrail_LSB_ns;
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: larger than Lead dynamic range. Value set to %.0f ns\n" COLOR_RESET, pName, (1 << (17 + 2 * two_bit))* (WDcfg->LeadTrail_LSB_ns));
		}
	} else if (WDcfg->MeasMode == MEASMODE_LEAD_TOT11) {
		if (GateTrgWindow > ((1 << (14 + 2 * two_bit)) * (WDcfg->LeadTrail_LSB_ns))) {
			GateTrgWindow = (1 << (14 + 2 * two_bit)) * WDcfg->LeadTrail_LSB_ns;
			Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: %s: larger than Lead dynamic range. Value set to %.0f ns\n" COLOR_RESET, pName, (1 << (14 + 2 * two_bit))* (WDcfg->LeadTrail_LSB_ns));
		}
	}

	// Warning that StopRun cannot be MANUAL if Job are enabled
	if (WDcfg->EnableJobs && (WDcfg->StopRunMode == STOPRUN_MANUAL)) {
		Con_printf("LCSw", FONT_STYLE_BOLD COLOR_YELLOW "WARNING: StopRunMode: Cannot configure STOPRUN MANUAL during jobs. Switching to PRESET COUNT\n" COLOR_RESET);
		WDcfg->StopRunMode = STOPRUN_PRESET_COUNTS;
	}

	// Avoid plots to show beyond the histogramMax
	//if (WDcfg->LeadTrailHistoNbin > (1 << TOA_NBIT))	WDcfg->LeadTrailHistoNbin = (1 << TOA_NBIT);
	//int totnbin = MEASMODE_OWLT(WDcfg->MeasMode) ? TOT_NBIT : TOA_NBIT;
	//if (WDcfg->ToTHistoNbin > (1 << totnbin))	WDcfg->ToTHistoNbin = (1 << totnbin);
	// DNIN: Would histMax change in case of Streaming?
	// In CommonS Rebin=LSB, in Streaming there are two of them
	WDcfg->LeadHistoMax = (float)(WDcfg->LeadHistoMin + TDC_LSB_ps/1000 * WDcfg->LeadTrailRebin * WDcfg->LeadTrailHistoNbin);
	WDcfg->TrailHistoMax = (float)(WDcfg->TrailHistoMin + TDC_LSB_ps/1000 * WDcfg->LeadTrailRebin * WDcfg->LeadTrailHistoNbin);
	WDcfg->ToTHistoMax = (float)(WDcfg->ToTHistoMin + TDC_LSB_ps/1000 * WDcfg->ToTRebin * WDcfg->ToTHistoNbin);

	// If the board is already connected, retrieve the number of channels from the BoardInfo
	// CTIN: can be different from board to board... For the moment, take NumCh from board 0 and extend it to all boards
	if (handle[0] != -1) {
		WDcfg->NumCh = FERS_NumChannels(handle[0]);
	}


	// Check: Gate


#ifdef linux
	if (WDcfg->DataFilePath[strlen(WDcfg->DataFilePath)-1] != '/')	sprintf(WDcfg->DataFilePath, "%s/", WDcfg->DataFilePath);
#else
	if (WDcfg->DataFilePath[strlen(WDcfg->DataFilePath)-1] != '\\')	sprintf(WDcfg->DataFilePath, "%s\\", WDcfg->DataFilePath);
#endif

	return 0;
}

