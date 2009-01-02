/*
 * WMO format sounding access
 */
/*
 *      Copyright (C) 2008 by UCAR
 *  University Corporation for Atmospheric Research
 *         All rights reserved
 *
 * No part of this work covered by the copyrights herein may be reproduced
 * or used in any form or by any means -- graphic, electronic, or mechanical,
 * including photocopying, recording, taping, or information storage and
 * retrieval systems -- without permission of the copyright owner.
 * 
 * This software and any accompanying written materials are provided "as is"
 * without warranty of any kind.  UCAR expressly disclaims all warranties of
 * any kind, either express or implied, including but not limited to the
 * implied warranties of merchantibility and fitness for a particular purpose.
 * UCAR does not indemnify any infringement of copyright, patent, or trademark
 * through use or modification of this software.  UCAR does not provide 
 * maintenance or updates for its software.
 */

# include <cctype>
# include <cerrno>
# include <cstdio>
# include <cstring>
# include <map>
# include <sstream>
# include <string>

extern "C" {
#	include <ui.h>
#	include <ui_error.h>

#	include "sounding.h"
#	include "flags.h"
	/*
	 * Our C-accessible functions
	 */
	int wmo_snd_count(const char* fname, const char* filter);
	void wmo_get_sounding(const char* fname, const char* filter, int ndx, 
		struct snd* sounding);
} // end extern "C"

/*
 * TEMP report types and report parts
 */
typedef enum _ReportType {
	WMO_TEMP, WMO_TEMP_SHIP, WMO_TEMP_DROP, WMO_TEMP_MOBIL
} ReportType;

typedef enum _ReportPart {
	WMO_PART_A, WMO_PART_B, WMO_PART_C, WMO_PART_D
} ReportPart;

/*
 * Missing value indicator
 */
static const float MISSING_VAL = -99999.;

/*
 * Prototypes
 */
class wmoSounding;
static void wmo_unpackFile(const char* filename, const char* filter);
static int wmo_openFile(const char* filename);
static int wmo_nextLine();
static int wmo_nextTokenThisLineOnly();
static int wmo_nextToken(int required, int inReport, const char* desc);
static void wmo_readDataSection();
static void wmo_readHeading(char dataType[2], char country[2], 
	int* levelDesignator, char siteName[5], int* day, int* hour, 
	int* minute);
static wmoSounding* wmo_readSounding();
static int wmo_atEndOfReport();
static void wmo_findEndOfReport(int verbose);
static int wmo_dateCompare(const date d1, const date d2);
static void wmo_trimString(char* string);
static void wmo_unpack_PPHHH(const char* token, ReportPart part, float* pres, 
	float* height);
static void wmo_unpack_xxPPP(const char* token, ReportPart part, float* pres);
static void wmo_unpack_TTTDD(const char* token, float* temp, float* dpd);
static void wmo_unpack_dddff(const char* token, float* wdir, float* wspd,
	bool convertFromKnots);

/*
 * Struct to hold our open file and associated information
 */
class _openFile {
public:
	FILE* file;
	std::string filename;
	char line[1024];
	int lineNum;
	char token[80];
	char* nextTokenStart;
} OpenFile;


/*
 * One point in a wmoSounding
 */
class wmoPoint {
public:
	wmoPoint() : _pres(MISSING_VAL), _height(MISSING_VAL), 
		_temp(MISSING_VAL), _dp(MISSING_VAL),
		_wspd(MISSING_VAL), _wdir(MISSING_VAL) {}
	/*
	 * Merge good values from other into our values.  If we have two
	 * good values for something, favor the value from other.
	 */
	void merge(wmoPoint other) {
		if (other._pres != MISSING_VAL)
			_pres = other._pres;
		if (other._height != MISSING_VAL)
			_height = other._height;
		if (other._temp != MISSING_VAL)
			_temp = other._temp;
		if (other._dp != MISSING_VAL)
			_dp = other._dp;
		if (other._wspd != MISSING_VAL)
			_wspd = other._wspd;
		if (other._wdir != MISSING_VAL)
			_wdir = other._wdir;
	}
	float _pres;	// hPa
	float _height;	// geopotential height in m
	float _temp;	// C
	float _dp;	// C
	float _wspd;	// m/s
	float _wdir;	// deg
};

/*
 * A class to serve as a sounding identifier
 */
class sndId {
public:
	sndId(date d, std::string site) : _date(d), _site(site) {}
	/*
	 * Sort first by date, then by site number
	 */
	bool operator<(const sndId& right) const {
		int dateCmp = wmo_dateCompare(_date, right._date);
		if (dateCmp < 0)
			return true;
		else if (dateCmp == 0)
			return (_site < right._site);
		else
			return false;				
	}
	bool operator==(const sndId& right) const {
		return (_date.ds_yymmdd == right._date.ds_yymmdd && 
			_date.ds_hhmmss == right._date.ds_hhmmss && 
			_site == right._site);
	}
	bool operator!=(const sndId& right) const {
		return !operator==(right);
	}
	std::string asString() const {
		std::stringstream sstr;
		sstr << "sndId " << _date.ds_yymmdd << "/" << _date.ds_hhmmss <<
			"/" << _site;
		return sstr.str();
	}
protected:
	date _date;
	std::string _site;
};
	
/*
 * Our local class to hold a sounding
 */
class wmoSounding {
public:
	wmoSounding(date d, std::string site) : _date(d), _site(site) {}
	~wmoSounding() {
	}
	date time() { return _date; }
	std::string site() { return _site; }
	sndId id() { return sndId(_date, _site); }
	
	void addOrMergePoint(wmoPoint p) {
		_points[p._pres].merge(p);
	}
	
	int nPoints() { return _points.size(); }
	/*
	 * Return the ndx-th point, where points are indexed from highest
	 * to lowest pressure.
	 */
	wmoPoint getPoint(int ndx) {
		if (ndx >= _points.size())
			ui_error((char*)"wmoSounding::getPoint() index error!");
		std::map<float, wmoPoint>::const_reverse_iterator it = _points.rbegin();
		for (int i = 0; i < ndx; i++)
			it++;
		return it->second;
	}
	
	void merge(wmoSounding* ws) {
		if (id() != ws->id())
			ui_error((char*)"wmoSounding::merge() id error!");
		std::map<float, wmoPoint>::const_iterator it;
		//
		// loop through points in the other, adding or merging them in
		//
		for (it = ws->_points.begin(); it != ws->_points.end(); it++)
			addOrMergePoint(it->second);
	}
	
	date soundingDate() { return _date; }
	std::string soundingSite() { return _site; }
	void setSoundingLat(float lat) { _sitelat = lat; }
	float soundingLat() { return _sitelat; }
	void setSoundingLon(float lon) { _sitelon = lon; }
	float soundingLon() { return _sitelon; }
private:
	date _date;
	std::string _site;
	float _sitelat;
	float _sitelon;
	std::map<float, wmoPoint> _points;	// map pressures to wmoPoint-s
};

/*
 * A struct to hold everything unpacked from the last file we read
 */
class _unpackedFile {
public:
	_unpackedFile() {}
	
	int nSoundings() { return _soundings.size(); }
	
	void clear() {
		std::map<sndId, wmoSounding*>::iterator i;
		for (i = _soundings.begin(); i != _soundings.end(); i++)
			delete i->second;
		_soundings.clear();
		if (OpenFile.file) {
			fclose(OpenFile.file);
			OpenFile.file = 0;
		}
		_filename = "";
		_filter = "";
	}
	
	wmoSounding* find(sndId id) {
		std::map<sndId, wmoSounding*>::const_iterator i = _soundings.find(id);
		if (i != _soundings.end())
			return i->second;
		else
			return 0;
	}
	
	void addOrMergeSounding(wmoSounding* sounding) { 
		wmoSounding* snd = find(sounding->id());
		if (! _filter.empty() && _filter != sounding->site())
			return;
		/*
		 * @todo: assign correct year/month here
		 */
		if (snd) {
			snd->merge(sounding);
			delete sounding;
		}
		else 
			_soundings[sounding->id()] = sounding;
	}
	
	wmoSounding* getSounding(int ndx) {
		if (ndx >= _soundings.size())
			ui_error((char*)"_unpackedFile::getSounding() index error!");
		std::map<sndId, wmoSounding*>::const_iterator it = _soundings.begin();
		for (int i = 0; i < ndx; i++)
			it++;
		return it->second;
	}
	std::string _filename;
	std::string _filter;
private:
	std::map<sndId, wmoSounding*> _soundings;
} UnpackedFile;

/*
 * Return the count of separate soundings in the given WMO file.
 */
int
wmo_snd_count(const char* fname, const char* filter)
{
	wmo_unpackFile(fname, filter);
	int count = UnpackedFile.nSoundings();
	return count;
}


/*
 * Get the selected sounding from the given file
 */
void
wmo_get_sounding(const char* fname, const char* filter, int ndx, 
	struct snd* sounding)
{
	wmo_unpackFile(fname, filter);
	
	if (ndx >= 0 && ndx < UnpackedFile.nSoundings())
	{
		wmoSounding* ws = UnpackedFile.getSounding(ndx);
//		int	maxndx;
//		struct snd_datum	*dlists[MAXFLDS];
		sounding->site = (char*)malloc(ws->soundingSite().length() + 1);
		strcpy(sounding->site, ws->soundingSite().c_str());
		sounding->rls_time = ws->soundingDate();
		sounding->sitelat = ws->soundingLat();
		sounding->sitelon = ws->soundingLon();
		sounding->sitealt = 0.0;	// unknown...
		sounding->fields[0] = f_pres;
		sounding->fields[1] = f_alt;
		sounding->fields[2] = f_temp;
		sounding->fields[3] = f_dp;
		sounding->fields[4] = f_wdir;
		sounding->fields[5] = f_wspd;
		sounding->maxndx = ws->nPoints() - 1;
		
		struct snd_datum* prevPres = 0;
		struct snd_datum* prevHeight = 0;
		struct snd_datum* prevTemp = 0;
		struct snd_datum* prevDp = 0;
		struct snd_datum* prevWdir = 0;
		struct snd_datum* prevWspd = 0;
		
		for (int i = 0; i < ws->nPoints(); i++)
		{
			wmoPoint p = ws->getPoint(i);
			struct snd_datum* d;
			/*
			 * Pressure
			 */
			if (p._pres != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._pres;
				if (prevPres)
				{
					d->prev = prevPres;
					prevPres->next = d;
				}
				else
					sounding->dlists[0] = d;
				
				prevPres = d;
			}
			/*
			 * Height
			 */
			if (p._height != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._height;
				if (prevHeight)
				{
					d->prev = prevHeight;
					prevHeight->next = d;
				}
				else
					sounding->dlists[1] = d;
				
				prevHeight = d;
			}
			/*
			 * Temp
			 */
			if (p._temp != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._temp;
				if (prevTemp)
				{
					d->prev = prevTemp;
					prevTemp->next = d;
				}
				else
					sounding->dlists[2] = d;
				
				prevTemp = d;
			}
			/*
			 * dewpoint
			 */
			if (p._dp != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._dp;
				if (prevDp)
				{
					d->prev = prevDp;
					prevDp->next = d;
				}
				else
					sounding->dlists[3] = d;
				
				prevDp = d;
			}
			/*
			 * wdir
			 */
			if (p._wdir != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._wdir;
				if (prevWdir)
				{
					d->prev = prevWdir;
					prevWdir->next = d;
				}
				else
					sounding->dlists[4] = d;
				
				prevWdir = d;
			}
			/*
			 * wspd
			 */
			if (p._wspd != MISSING_VAL)
			{
				d = (struct snd_datum*)calloc(1, sizeof(struct snd_datum));
				d->index = i;
				d->value = p._wspd;
				if (prevWspd)
				{
					d->prev = prevWspd;
					prevWspd->next = d;
				}
				else
					sounding->dlists[5] = d;
				
				prevWspd = d;
			}
		}
		
	}
	else
		ui_error((char*)"Attempt to unpack sounding %d of %d from %s!", 
			ndx + 1, UnpackedFile.nSoundings(), fname);
	
	return;
}


/*
 * Open the named file and populate the OpenFile struct.  A call
 * to wmo_nextLine() will be executed to make the first line and
 * first token available immediately.  Return 1 on success, or 0 on error.
 */
static 
int
wmo_openFile(const char* filename)
{
	if (OpenFile.file)
		fclose(OpenFile.file);
	
	OpenFile.file = fopen(filename, "r");
	if (! OpenFile.file)
		ui_error((char*)"Error opening file '%s': %s", filename,
			strerror(errno));

	OpenFile.filename = filename;
	OpenFile.lineNum = 0; /* number of last line read */
	OpenFile.nextTokenStart = 0;
	OpenFile.token[0] = '\0';
	OpenFile.line[0] = '\0';
	if (! wmo_nextLine())
		ui_error((char*)"File %s is empty!", OpenFile.filename.c_str());
}

/*
 * Read line(s) from the file until we get one with non-whitespace content.
 * The OpenFile struct is left with the resulting line trimmed of leading
 * and trailing whitespace, and the first token from that line.  Return 1
 * on success, or 0 on end-of-file.
 */
static 
int
wmo_nextLine()
{
	int nconsumed;
	int sizeofLine = sizeof(OpenFile.line);
	
	while (1)
	{
		OpenFile.lineNum++;
		
		if (!fgets(OpenFile.line, sizeofLine, OpenFile.file))
		{
			if (!feof(OpenFile.file))
				ui_error((char*)"Reading line %d from %s: %s", 
					OpenFile.lineNum, OpenFile.filename.c_str(),
					strerror(errno));
			return 0;
		}
		/*
		 * If we read the maximum number of chars, there's trouble...
		 */
		if (strlen(OpenFile.line) == (sizeofLine - 1))
			ui_error((char*)"Line %d too long; max size %d!", 
				OpenFile.lineNum, sizeofLine);
		/*
		 * Trim leading and trailing whitespace from the line
		 */
		wmo_trimString(OpenFile.line);
		/* 
		 * Skip blank lines
		 */
		if (strlen(OpenFile.line) == 0)
			continue;
		/*
		 * Start the token read pointer at the beginning of the line
		 */
		OpenFile.nextTokenStart = OpenFile.line;
		/* 
		 * Parse out the first token of the line
		 */
		if (! wmo_nextTokenThisLineOnly())
			ui_error((char*)"BUG: first token in line %d is empty", 
				OpenFile.lineNum);
		return 1;
	}	
}

/*
 * Extract the next token from the current line.  Return 1 if a token
 * was found or 0 if we're at the end of the current line.
 */
static 
int
wmo_nextTokenThisLineOnly()
{
	int nconsumed = 0;
	static char* readFormat = 0;
	/*
	 * Return zero if we're at the end of the line
	 */
	if (strlen(OpenFile.nextTokenStart) == 0)
		return 0;
	/*
	 * Build our read format based on the size of our token.
	 * E.g., for an 80-byte token space, the format will be "%79s%n".
	 */
	if (! readFormat)
	{
		readFormat = (char*)malloc(16);
		sprintf(readFormat, "%%%ds%%n", sizeof(OpenFile.token) - 1);	
	}
	/*
	 * Read the next token from the line.  If tokens are longer than
	 * the size of OpenFile.token, the results here will be bad...
	 */
	sscanf(OpenFile.nextTokenStart, readFormat, OpenFile.token, &nconsumed);
	OpenFile.nextTokenStart += nconsumed;
	return 1;
}


/*
 * Extract the next token, moving to a new line if necessary.
 * Return 1 on success or 0 on end-of-file.
 */
static
int
wmo_nextToken(int required, int inReport, const char* desc)
{
	int success;
	/*
	 * If the next token is required to be from the same report,
	 * make sure we're not at end-of-report
	 */
	if (required && inReport && wmo_atEndOfReport())
		ui_error((char*)"End of report before %s at line %d",
			(desc ? desc : "required token"), OpenFile.lineNum);
	/*
	 * Get the next token from this line, if any, or get another line
	 * (which loads the first token of the line)
	 */
	success = (wmo_nextTokenThisLineOnly() || wmo_nextLine());
	/*
	 * If a token is required, but we got none, trigger an error.
	 */
	if (required && ! success)
		ui_error((char*)"End of file at line %d when expecting %s",
			OpenFile.lineNum, 
			(desc ? desc : "required token"));
	return 1;
}

/*
 * Unpack all soundings from the named file, putting the results into 
 * the UnpackedFile struct.
 */
static 
void
wmo_unpackFile(const char* filename, const char* filter)
/*
 * Unpack the WMO file, saving the results into the static struct "UnpackedFile"
 */
{
	if (UnpackedFile._filename == filename && 
		UnpackedFile._filter == filter)
		return;
/*
 * Clear out old contents in UnpackedFile
 */
	ui_printf((char*)"unpacking %s\n", filename);
	UnpackedFile.clear();
/*
 * Set the site filter
 */
	UnpackedFile._filename = filename;
	UnpackedFile._filter = filter;
/*
 * Extract all soundings in the file
 */
	wmo_openFile(filename);
	if (! OpenFile.file)
		ui_error((char*)"Error opening file '%s': %s", filename,
			strerror(errno));
	while (! feof(OpenFile.file))
		wmo_readDataSection();
}


/*
 * Read the next data section from the given file, where a data section is
 * delimited by "ZCZC" and "NNNN" lines, e.g.,
 * 
 *	ZCZC
 *	USCI40 BCLZ 010000 
 *	TTAA  31231 52983 99810 13859 00000 70115 12064 32505 50582
 *	08380 35007 40753 17126 25509 30963 30956 23517 25090 38169
 *	28017 20241 47168 27516 15426 58767 28510 10674 689// 36008
 *	88999 77999=
 *	
 *	
 *	
 *	
 *	NNNN
 * 
 * If the section contains a sounding, return a pointer to an associated
 * wmoSounding.  Return null if the data section is not a sounding 
 * section.
 */
static 
void
wmo_readDataSection()
{
	wmoSounding* sounding = 0;
	char dataType[3];	// 2 chars + terminating null
	char country[3];	// 2 chars + terminating null
	char siteName[5];	// 4 chars + terminating null
	int ignoreToEod;
	int borkStart = 0;
	int levelDesignator, insertDay, insertHour, insertMinute;
	
	/*
	 *  Loop through data sections in the file
	 */
	while (1) 
	{
		ignoreToEod = 0;
		/* 
		 * Right now we're between messages. 
		 * 
		 * Read the next line from the file.
		 */
		if (! wmo_nextLine())
			break;
		/* 
		 * If the first token is "NIL", continue to the next line.
		 */
		if (! strcmp(OpenFile.token, "NIL"))
			continue;
		/*
		 * Now we're expecting beginning-of-data ("ZCZC")
		 */
		if (strcmp(OpenFile.token, "ZCZC"))
		{
			ui_error((char*)"Got '%s' when expecting 'ZCZC' at line %d", 
				OpenFile.line, OpenFile.lineNum);
			break;
		}
		/*
		 * Get the abbreviated heading line (see "Manual on the Global
		 * Telecommunication System", WMO No. 386, Part II, 2.3.2)
		 */
		wmo_nextLine(); /* skip anything on current line after ZCZC */
		wmo_readHeading(dataType, country, &levelDesignator,
				siteName, &insertDay, &insertHour, 
				&insertMinute);
		/*
		 * We only care about sounding data sections, which have one
		 * of the following types: UE, UF, UK, UL, UM, US.  If the
		 * message is not one of these types, skip to the end of this
		 * message.
		 */
		if (dataType[0] == 'U' && strchr("EFKLMS", dataType[1]))
		{
			while (1)
			{
				wmo_nextToken(0, 0, 0);
				if (! strcmp(OpenFile.token, "NNNN"))
					break;
				if (! strncmp(OpenFile.token, "NIL", 3))
					continue;
				sounding = wmo_readSounding();
				if (sounding)
					UnpackedFile.addOrMergeSounding(sounding);
			}
			/* 
			 * Skip everything else up to the end-of-data marker.
			 */
			if (strcmp(OpenFile.token, "NNNN"))
			{
				/* 
				 * If we're not at EOD, we borked when trying 
				 * to read a sounding.  Skip to the EOD marker.
				 */
				ignoreToEod = 1;
				borkStart = OpenFile.lineNum;
			}
		}
		else
		{
			ignoreToEod = 1;
		}
		
		/*
		 * Look for the end-of-data line "NNNN", skipping blank
		 * lines.  We also skip non-blank lines if ignoreAllToEod
		 * is true.
		 */
		while (strcmp(OpenFile.token, "NNNN")) 
		{
			if (!ignoreToEod)
				ui_error((char*)"Got '%s' when expecting 'NNNN' at line %d",
					OpenFile.line, OpenFile.lineNum);
			if (!wmo_nextLine())
				break;
		}
		if (feof(OpenFile.file))
			break;
		if (borkStart)
		{
			ui_warning((char*)"Borked reading a sounding.  "
				"Lines %d-%d skipped.",	borkStart, OpenFile.lineNum);
			borkStart = 0;
		}
	}
}


/*
 * Read and parse an abbreviated heading.  See "Manual on the Global 
 * Telecommunication System", WMO No. 386, Part II, 2.3.2
 */
static 
void 
wmo_readHeading(char dataType[3], char country[3],
		int* levelDesignator, char siteName[5], int* insertDay, 
		int* insertHour,
		int* insertMinute)
{
	/*
	 * First token of the heading contains 2-character data type, 
	 * 2-character country, and 2-digit level designator
	 */
	memcpy(dataType, OpenFile.token, 2);
	dataType[2] = '\0';
	memcpy(country, OpenFile.token + 2, 2);
	country[2] = '\0';
	sscanf(OpenFile.token + 4, "%2d", levelDesignator);
	/*
	 * 2nd token of the heading is the 4-character site name
	 */
	if (! wmo_nextTokenThisLineOnly())
		ui_error((char*)"No 2nd heading token at line %d!", OpenFile.lineNum);
	strncpy(siteName, OpenFile.token, 5);
	/*
	 * 3rd token of the heading is time of insertion into GTS: day, hour,
	 * and minute (DDhhmm)
	 */
	if (! wmo_nextTokenThisLineOnly())
		ui_error((char*)"No 3rd heading token at line %d!", OpenFile.lineNum);
	sscanf(OpenFile.token, "%2d%2d%2d", insertDay, insertHour, insertMinute);
	/*
	 * There may be a 4th token on the line.  Skip it.
	 */
	wmo_nextTokenThisLineOnly();
}


/*
 * Read a sounding (TEMP report) from the file, unpacking it into a newly 
 * allocated wmoSounding, which is returned.  On entry, the next line in the 
 * open file should start with one of these tokens: TTAA, TTBB, TTCC, 
 * TTDD, XXAA, XXBB, XXCC, XXDD, IIAA, IIBB, IICC, IIDD.
 */
static
wmoSounding*
wmo_readSounding()
{
	wmoSounding* sounding = 0;
	const char* token = OpenFile.token;
	/* 
	 * If the token is MANxxx (just indicating that the upcoming report
	 * contains mandatory levels), skip the token.
	 * 
	 * @todo Where is this documented?
	 */
	if (! strncmp(token, "MAN", 3))
		wmo_nextToken(1, 0, "sounding code tag");
	/*
	 * The next token should be 4 characters, with the first two 
	 * representing the report type (TT, UU, XX, or II), and the second 
	 * two representing the report part (AA, BB, CC, or DD).
	 */
	char typestr[2];
	ReportType type;
	strncpy(typestr, token, 2);
	if (! strncmp(typestr, "TT", 2))
		type = WMO_TEMP;
	else if (! strncmp(typestr, "UU", 2))
		type = WMO_TEMP_SHIP;
	else if (! strncmp(typestr, "XX", 2))
		type = WMO_TEMP_DROP;
	else if (! strncmp(typestr, "II", 2))
		type = WMO_TEMP_MOBIL;
	else
	{
		if (! Flg_wmoQuiet)
			ui_warning((char*)"Unknown TEMP message type '%c%c' at line %d",
				typestr[0], typestr[1], OpenFile.lineNum);
		wmo_findEndOfReport(! Flg_wmoQuiet);
		return 0;
	}
	
	char partstr[2];
	ReportPart part;
	strncpy(partstr, token + 2, 2);
	if (! strncmp(partstr, "AA", 2))
		part = WMO_PART_A;
	else if (! strncmp(partstr, "BB", 2))
		part = WMO_PART_B;
	else if (! strncmp(partstr, "CC", 2))
		part = WMO_PART_C;
	else if (! strncmp(partstr, "DD", 2))
		part = WMO_PART_D;
	else
	{
		if (! Flg_wmoQuiet)
			ui_warning((char*)"Unknown TEMP message part '%c%c' at line %d",
				partstr[0], partstr[1], OpenFile.lineNum);
		wmo_findEndOfReport(! Flg_wmoQuiet);
		return 0;
	}

	/*
	 * For SHIP and MOBIL reports, the next token is either the call sign
	 * or if no call sign, either "SHIP" or "MOBIL".
	 */
	char siteId[32];
	if (type == WMO_TEMP_SHIP || type == WMO_TEMP_MOBIL)
	{
		wmo_nextToken(1, 0, "call sign");
		sprintf(siteId, "%s", token);
	}
	/*
	 * Next token has date and hour of the data, and an indication of
	 * the highest level with winds
	 */
	char topWindChar;
	int day, hour;
	wmo_nextToken(1, 0, "date/hour");
	if (! strcmp(token, "/////"))
	{
		day = -99;
		hour = -99;
		topWindChar = '/';
	}
	else if (sscanf(token, "%2d%2d%c", &day, &hour, &topWindChar) != 3)
	{
		ui_warning((char*)"Bad time code token '%s' at line %d", token, 
			OpenFile.lineNum);
		wmo_findEndOfReport(1);
		return 0;
	}
	/*
	 * Highest level with winds (only applies to part A and part C)
	 */
	int topWindPres;
	switch (topWindChar)
	{
	case '0':
		topWindPres = 1000;
		break;
	case '1':
		/* 
		 * This should actually be 100 (or 10) hPa, but a number of
		 * soundings actually include 70 (or 7) hPa and 50 (or 5) hPa
		 * as "mandatory" levels, and they include winds...
		 */
		if (part == WMO_PART_A)
			topWindPres = 50;
		else
			topWindPres = 5;
		break;
	case '2':
	case '3':
	case '4':
	case '5':
	case '7':
		if (part == WMO_PART_A)
			topWindPres = 100 * (topWindChar - '0');
		else
			topWindPres = 10 * (topWindChar - '0');
		break;
	case '8':
		topWindPres = 850;
		break;
	case '9':
		topWindPres = 925;
		break;
	case '/':
		topWindPres = 9999;	/* no winds */
		break;
	default:
		ui_warning((char*)"Bad top wind char '%c' at line %d",
			topWindChar, OpenFile.lineNum);
		wmo_findEndOfReport(1);
		return 0;
	}
	/*
	 * If day is > 50, it means winds are reported in knots, and
	 * 50 should be subtracted from the day.
	 */
	bool convertFromKnots = false;
	if (day > 50)
	{
		convertFromKnots = true;
		day -= 50;
	}
	/*
	 * Now site information
	 */
	float lat, lon;
	if (type == WMO_TEMP)
	{
		wmo_nextToken(1, 0, "station #");
		strncpy(siteId, token, sizeof(siteId) - 1);
	}
	else
	{
		int ilat, ilon, quadrant;
		/*
		 * latitude group
		 */
		wmo_nextToken(1, 0, "lat group");
		if (sscanf(token, "99%3d", &ilat) != 1)
		{
			ui_warning((char*)"Bad latitude token '%s' at line %d",
				token, OpenFile.lineNum);
			wmo_findEndOfReport(1);
			return 0;
		}
		lat = 0.1 * ilat;
		/*
		 * quadrant/longitude group
		 */
		wmo_nextToken(1, 0, "lon group");
		if (sscanf(token, "%1d%4d", &quadrant, &ilon) != 2)
		{
			ui_warning((char*)"Bad latitude token '%s' at line %d",
				token, OpenFile.lineNum);
			wmo_findEndOfReport(1);
			return 0;
		}
		lon = 0.1 * ilon;
		
		if (quadrant == 3 || quadrant == 5)
			lat *= -1;
		if (quadrant == 5 || quadrant == 7)
			lon *= -1;
		/*
		 * Marsden square group
		 * We read and skip this, since all information it provides
		 * is provided by the lat/lon we already decoded.
		 */
		wmo_nextToken(1, 0, "Marsden square group");
		/*
		 * Drop altitude (get the token and ignore it for now)
		 */
		if (type == WMO_TEMP_DROP)
			wmo_nextToken(1, 0, "drop altitude");
	}
	/*
	 * Unpack all the data for this report
	 */
	date d;
	d.ds_yymmdd = day;
	d.ds_hhmmss = hour * 10000;
	sounding = new wmoSounding(d, siteId);
	
	sounding->setSoundingLat(lat);
	sounding->setSoundingLon(lon);
	
	int index = 0;
	while (! wmo_atEndOfReport())
	{
		wmoPoint p;
		ERRORCATCH
			int windAvail = 0; /* will we read a wind token? */

			wmo_nextToken(1, 1, "first data");
				
			if (! strncmp(token, "NIL", 3))
				continue;
			/*
			 * In part A and C reports, it's special handling once we
			 * hit the tropopause level information (which starts with 88)
			 */
			if ((part == WMO_PART_A || part == WMO_PART_C) &&
				! strncmp(token, "88", 2))
			{
				/*
				 * The sounding may have more than one tropopause 
				 */
				while (! strncmp(token, "88", 2))
				{
					/*
					 * Tropopause section
					 */
					wmo_unpack_xxPPP(token, part, &p._pres);
					if (p._pres != MISSING_VAL)
					{
					    wmo_nextToken(1, 1, "trop temp group");
					    wmo_unpack_TTTDD(token, &p._temp, 
							    &p._dp);
					    if (p._pres >= topWindPres)
					    {
					        wmo_nextToken(1, 1, "trop wind group");
					        wmo_unpack_dddff(token, &p._wdir, 
					        	&p._wspd, convertFromKnots);
					    }
					}
					if (wmo_atEndOfReport())
						break;
					wmo_nextToken(1, 1, "trop or max wind");
					/*
					 * They sometimes include a "/////" trop
					 * wind group even winds are supposedly
					 * not reported...
					 */
					if (topWindPres == 9999 &&
						! strncmp(token, "/////", 5))
					{
						if (wmo_atEndOfReport())
							break;
						wmo_nextToken(1, 1, "trop or max wind");
					}
				}
				/*
				 * We may be done now
				 */
				if (wmo_atEndOfReport())
					break;
				/*
				 * Max wind section may be next
				 */
				if (!strncmp(token, "77", 2) || !strncmp(token, "66", 2))
				{
					wmo_unpack_xxPPP(token, part, &p._pres);
					if (p._pres != MISSING_VAL)
					{
						wmo_nextToken(1, 1, "max wind");
						wmo_unpack_dddff(token, &p._wdir, 
							&p._wspd, convertFromKnots);
					}
				}
				/*
				 * Skip the rest of the report
				 */
				wmo_findEndOfReport(0);
				break;
			}
			/*
			 * For part B and part D reports, we skip everything after
			 * 21212
			 */
			else if ((part == WMO_PART_B || part == WMO_PART_D) &&
				! strncmp(token, "21212", 5))
			{
				wmo_findEndOfReport(0);
				break;
			}
			/*
			 * Unpack pressure (and height for mandatory level parts)
			 */
			if ((part == WMO_PART_A || part == WMO_PART_C))
			{
				if (index == 0 && part == WMO_PART_A)
				{
					/*
					 * Surface group
					 */
					if (strncmp(token, "99", 2))
						ui_error((char*)"Bad surface group '%s' at line %d",
							token, OpenFile.lineNum);
					wmo_unpack_xxPPP(token, part, &p._pres);
					windAvail = 1; /* sfc wind always follows */
				}
				else
				{
					/*
					 * Next manadatory level
					 */
					wmo_unpack_PPHHH(token, part, &p._pres, &p._height);
					if (p._pres == MISSING_VAL)
						continue;
				}
			}
			else
				wmo_unpack_xxPPP(token, part, &p._pres);
			/*
			 * TTTDD: Temperature and dewpoint depression
			 */
			wmo_nextToken(1, 1, "TTTDD");
			if (! strncmp(token, "NIL", 3))
				continue;
			wmo_unpack_TTTDD(token, &p._temp, &p._dp);
			/*
			 * For parts A and C, wind speed and direction are next
			 * (if we're still at a level which includes winds)
			 */
			windAvail = windAvail || (p._pres >= topWindPres);
			if ((part == WMO_PART_A || part == WMO_PART_C) &&
				windAvail)
			{
				wmo_nextToken(1, 1, "dddff");
				if (! strncmp(token, "NIL", 3))
					continue;
				wmo_unpack_dddff(token, &p._wdir, &p._wspd,
					convertFromKnots);
			}
			index++;
		ON_ERROR
			/*
			 * Skip the rest of this sounding message
			 */
			wmo_findEndOfReport(! Flg_wmoQuiet);
			/* 
			 * clear sounding
			 */
			if (sounding)
			{
				delete sounding;
				sounding = 0;
			}
			return sounding;
		ENDCATCH
		sounding->addOrMergePoint(p);
	}
	
	if (sounding->nPoints() == 0)
	{
		delete sounding;
		sounding = 0;
	}
	return sounding;
}


/*
 * Return true iff the current token marks the end of a report (i.e., 
 * if the token contains a '=' character).
 */
static 
int 
wmo_atEndOfReport()
{
	return(strchr(OpenFile.token, '=') != 0);
}


/*
 * Read until we hit the end-of-report (i.e., a token that ends with '=')
 * or the end of file.
 */
static 
void 
wmo_findEndOfReport(int verbose)
{
	const char* token = OpenFile.token;
	int startLine = OpenFile.lineNum;
	int status = 1;
	/*
	 * Quick (and quiet) bailout if we're already at end-of-report
	 */
	if (wmo_atEndOfReport())
		return;
	
	while (! wmo_atEndOfReport())
	{
		if (! wmo_nextToken(0, 0, 0))
			break;
	}
	
	if (verbose)
		ui_warning((char*)"Skipped lines %d-%d to get to end-of-report",
			startLine, OpenFile.lineNum);
	return;
}


/*
 * Compare dates.
 * Return:
 *   -1 if (d1 < d2)
 *    0 if (d1 == d2)
 *    1 if (d1 > d2)
 */
static 
int
wmo_dateCompare(const date d1, const date d2)
{
	if (d1.ds_yymmdd < d2.ds_yymmdd)
		return -1;
	else if (d1.ds_yymmdd > d2.ds_yymmdd)
		return 1;
	/*
	 * Dates are the same so compare times
	 */
	if (d1.ds_hhmmss < d2.ds_hhmmss)
		return -1;
	else if (d1.ds_hhmmss > d2.ds_hhmmss)
		return 1;
	/*
	 * They're exactly the same
	 */
	return 0;
}

/* 
 * Trim leading and trailing whitespace from the given string.
 */
static 
void
wmo_trimString(char* string) 
{
	int i;
	int firstgood, lastgood, newlen;
	
	for (i = 0; i < strlen(string); i++)
	{
		if (! isspace(string[i]))
			break;
	}
	if (i == strlen(string))
	{
		string[0] = '\0';
		return;
	}
	else
		firstgood = i;
	
	for (i = strlen(string) - 1; i >= firstgood; i--)
	{
		if (! isspace(string[i]))
			break;
	}
	lastgood = i;
	newlen = lastgood - firstgood + 1;
	memmove(string, string + firstgood, newlen);
	string[newlen] = '\0';
}


/*
 * Unpack PPHHH token (pressure and height) from a part A or part C report
 */
static
void
wmo_unpack_PPHHH(const char* token, ReportPart part, float* pres, float* height)
{
	int ipres, iheight;
	if (! strncmp(token, "//", 2))
		*pres = MISSING_VAL;
	else
	{
		if (sscanf(token, "%2d", &ipres) != 1)
			ui_error((char*)"Bad pressure in PPHHH token '%s' at line %d", 
				token, OpenFile.lineNum);
		if (part == WMO_PART_A)
		{
			/*
			 * 0 means 1000 hPa, 92 means 925 hPa, the rest are
			 * (pressure in hPa / 10).
			 */
			if (ipres == 0)
				*pres = 1000;
			else if (ipres == 92)
				*pres = 925;
			else
				*pres = ipres * 10;
		}
		else
			*pres = ipres;	/* part C */
	}
	/*
	 * height
	 */
	if (! strncmp(token + 2, "///", 3))
		*height = MISSING_VAL;
	else
	{
		if (sscanf(token + 2, "%3d", &iheight) != 1)
			ui_error((char*)"Bad height in PPHHH token '%s' at line %d",
				token, OpenFile.lineNum);
		switch ((int)(*pres))
		{
		case 1000:
			if (iheight >= 500)
				*height = iheight - 500;
			break;
		case 925:
			break;
		case 850:
			*height = iheight + 1000;
			break;
		case 700:
			if (iheight < 500)
				*height = iheight + 3000;
			else
				*height = iheight + 2000;
			break;
		case 500:
		case 400:
			*height = 10 * iheight;
			break;
		case 300:
		case 250:
			if (iheight < 500)
				*height += 10000 + 10 * iheight;
			else
				*height = 10 * iheight;
			break;
		case 200:
		case 150:
		case 100:
		case 70:
			*height = 10000 + 10 * iheight;
			break;
		case 50:
			if (iheight < 500)
				*height = 20000 + 10 * iheight;
			else
				*height = 10000 + 10 * iheight;
			break;
		case 30:
		case 20:
			*height = 20000 + 10 * iheight;
			break;
		case 10:
			if (iheight < 500)
				*height = 30000 + 10 * iheight;
			else
				*height = 20000 + 10 * iheight;
			break;
		/*
		 * @todo No WMO reference for this, so it's a guess...
		 */
		case 7:
		case 5:
			*height = 30000 + 10 * iheight;
			break;
		default:
			ui_error((char*)"bad PPHHH token '%s' at line %d", 
				token, OpenFile.lineNum);
		}

	}
}


/*
 * Unpack xxPPP token (level and pressure)
 */
static
void
wmo_unpack_xxPPP(const char* token, ReportPart part, float* pres)
{
	int ipres;
	if (sscanf(token + 2, "%3d", &ipres) != 1)
		ui_error((char*)"Bad pressure in xxPPP token '%s' at line %d",
			token, OpenFile.lineNum);
	if (ipres == 999)
		*pres = MISSING_VAL;
	else
		*pres = ipres;
	if (part == WMO_PART_A || part == WMO_PART_B)
	{
		if (ipres < 100)
			*pres += 1000;
	}
	else
		*pres *= 0.1;	// parts C and D are above 100 hPa
}


/*
 * Unpack TTTDD token (temperature and dewpoint depression
 */
static
void
wmo_unpack_TTTDD(const char* token, float* temp, float* dp)
{
	int itemp, idpd;
	/*
	 * Decode temperature
	 */
	if (! strncmp(token, "///", 3))
		*temp = MISSING_VAL;
	else if (sscanf(token, "%3d", &itemp) != 1)
		ui_error((char*)"Bad temp in TTTDD token '%s' at line %d",
			token, OpenFile.lineNum);
	else
	{
		*temp = 0.1 * itemp;
		/* 
		 * Last digit odd says temperature is negative
		 */
		if (itemp % 2)
			*temp *= -1;
	}
	/*
	 * Dewpoint depression
	 */
	if (! strncmp(token + 3, "//", 2))
		*dp = MISSING_VAL;
	else if (sscanf(token + 3, "%2d", &idpd) != 1)
		ui_error((char*)"Bad dp depression in TTTDD token '%s' at line %d",
			token, OpenFile.lineNum);
	else
	{
		float dpd;
		if (idpd <= 50)
			dpd = 0.1 * idpd;
		else
			dpd = idpd - 50;
		*dp = *temp - dpd;
	}
}


/*
 * Unpack dddff token (wind speed and direction)
 */
static
void
wmo_unpack_dddff(const char* token, float* wdir, float* wspd, 
	bool convertFromKnots)
{
	int iwdir, iwspd;
	int speedHundreds = 0;
	/*
	 * Decode direction reported to nearest 5 degrees,
	 * and the hundreds digit of speed.
	 */
	if (! strncmp(token, "//", 2))
	{
		*wdir = MISSING_VAL;
		/*
		 * We may still have hundreds digit of speed here
		 */
		if (token[2] != '/')
			sscanf(token, "%1d", &speedHundreds);
	}
	else if (sscanf(token, "%3d", &iwdir) != 1)
		ui_error((char*)"Bad direction in dddff token '%s' at line %d",
			token, OpenFile.lineNum);
	else
	{
		/*
		 * Hundreds digit of wind speed is encoded in the last 
		 * digit of direction
		 */
		speedHundreds = iwdir % 5;
		*wdir  = iwdir - speedHundreds;
		if (*wdir == 360.0)
			*wdir = 0.0;
	}
	/*
	 * Decode speed
	 */
	if (! strncmp(token, "//", 2))
		*wspd = MISSING_VAL;
	else if (sscanf(token + 3, "%2d", &iwspd) != 1)
		ui_error((char*)"Bad speed in dddff token '%s' at line %d",
			token, OpenFile.lineNum);
	else
	{
		*wspd = (100 * speedHundreds) + iwspd;
		if (convertFromKnots)
			*wspd *= 0.51444444;	/* knots -> m/s */
	}
}