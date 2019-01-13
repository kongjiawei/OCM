#include "StdAfx.h"
#include <iomanip>
#include <string>
#include <sstream>
#include <iostream>
#include "FinisarHROCM.h"
#include "FinisarHROCM_V3.h"
#include "CCRC32.h"

#define OPCODE_NOP			0x01
#define OPCODE_RES			0x02
#define OPCODE_MID			0x03
#define OPCODE_CLE			0x04
#define OPCODE_TPC			0x06
#define OPCODE_FWT			0x07
#define OPCODE_FWS			0x08
#define OPCODE_FWE			0x09
#define OPCODE_GETDEV		0x0A
#define OPCODE_SETMPPW		0x0B
#define OPCODE_GETMPPW		0x0C
#define OPCODE_GETMPW		0x0D
#define OPCODE_SETMPVC		0x0E
#define OPCODE_GETMPVC		0x0F
#define OPCODE_GETMVC		0x10
#define OPCODE_SETMPCS		0x11
#define OPCODE_GETMPCS		0x12
#define OPCODE_GETMCS		0x13
#define OPCODE_SETMPOSNR	0x14
#define OPCODE_GETMPOSNR	0x15
#define OPCODE_GETMOSNR		0x16
#define OPCODE_SETMPCP		0x17
#define OPCODE_GETMPCP		0x18
#define OPCODE_GETMCP		0x19
#define OPCODE_ATG			0x1C
#define OPCODE_ATS			0x1D
#define OPCODE_ATC			0x1E

#define OCM_SPIMAGIC_V1		0xF0E1D2C3
#define OCM_SPIMAGIC_V3		0xF0E1C387

// Long timeout for firmware update or reset (in ms -> 3 minutes)
#define OCM_LONGTIMEOUT 3*60*1000

//#define LOGSTART1(s,p1) {if (_log) fprintf(_log,"%u,"##s,::GetTickCount(),p1);}
#define LOGRESULT(Result) {if (_log) fprintf(_log,"%s\n",(Result)==0 ? "SPI=OK":"SPI=ERROR");}
#define LOGFAILED() LOGRESULT(1)

#define SLICE2FREQ(slice) ((((int)(slice) - 1)*_lastRDataDEV.SLW + _lastRDataDEV.FSF) / OCM3_FSCALE);
#define FREQ2SLICE(freq) (((int)round((freq) * OCM3_FSCALE) - _lastRDataDEV.FSF) / _lastRDataDEV.SLW + 1)

#define LOGERROR(msg) {_lastError << "[ERROR] " << msg << " (" << removePath(__FILE__) << ", Line " << __LINE__ << ")" << std::endl;}
#define LOGWARNING(msg) {_lastError << "[WARNING] " << msg << " (" << removePath(__FILE__) << ", Line " << __LINE__ << ")" << std::endl;}

std::string FinisarHROCM_V3::OCM3_ParseOPCODE(int OPCODE) {
	std::string LUT[] = { "???", "NOP", "RES", "MID", "CLE", "???", "TPC", "FWT", "FWS", "FWE", "GETDEV", "SETMPPW", "GETMPPW", "GETMPW", "SETMPVC", "GETMPVC", "GETMVC", 
		"SETMPCS", "GETMPCS", "GETMCS", "SETMPOSNR", "GETMPOSNR", "GETMOSNR", "SETMPCP", "GETMPCP", "GETMCP" };

	if (OPCODE < sizeof(LUT) / sizeof(std::string)) {
		return LUT[OPCODE];
	}

	return std::string("???");
}

// Constructor (does not communicate with OCM)
FinisarHROCM_V3::FinisarHROCM_V3(std::string createString,FILE *log, FILE *logbin) : _lastHead(), _lastRDataDEV()
{
    _seqnum						= rand();           // We start the sequence numbers at a random count
	_lastTxSeqNum				= 0;				// Last transmit sequence number of regular scan
	_lastTxSeqNumOSNR			= 0;				// Last transmit sequence number of OSNR scan
	_lastTxSeqNumValid			= false;			// Indicates that there no start trigger is pending
    _recover_ms					= 5;                // We have to wait at least 5ms after each SPI transfer
    _nretry						= 2000/_recover_ms; // Approximately 2 seconds
	_log						= log;              // Handle to log file (can be NULL)
	_logbin						= logbin;           // Handle to log file to write binary data (can be NULL)
	_logbinFilename[0]			= 0;				// Filename of the binary log file
	_nSPIMAGICErrorCount		= 0;				// Count SPIMAGIC errors
	_nCRC1ErrorCount			= 0;                // Counts CRC1 errors
    _nCRC2ErrorCount			= 0;                // Counts CRC2 errors
    _nCmdRetransmit				= 0;                // Counts how often a command has been retransmitted
	_isInit						= false;			// true : We already queried DEV? and _lastRDataDEV is valid
	_lastTPCTask				= 0;				// Last initiated TPC tasks

	_spi = createSPIAdapter(createString.c_str());
}

// Constructor (does not communicate with OCM)
FinisarHROCM_V3::FinisarHROCM_V3(std::string createString, const char *logbinFilename) : _lastHead(), _lastRDataDEV()
{
	_seqnum						= rand();           // We start the sequence numbers at a random count
	_lastTxSeqNum				= 0;				// Last transmit sequence number of regular scan
	_lastTxSeqNumOSNR			= 0;				// Last transmit sequence number of OSNR scan
	_lastTxSeqNumValid			= false;			// Indicates that there no start trigger is pending
	_recover_ms					= 5;                // We have to wait at least 5ms after each SPI transfer
	_nretry						= 2000/_recover_ms; // Approximately 2 seconds
	_log						= NULL;             // Handle to log file (can be NULL)
	_logbin						= NULL;				// Handle to log file to write binary data (can be NULL)
	_nSPIMAGICErrorCount		= 0;				// Count SPIMAGIC errors
	_nCRC1ErrorCount			= 0;                // Counts CRC1 errors
	_nCRC2ErrorCount			= 0;                // Counts CRC2 errors
	_nCmdRetransmit				= 0;                // Counts how often a command has been retransmitted
	_isInit						= false;			// true : We already queried DEV? and _lastRDataDEV is valid
	_lastTPCTask				= 0;				// Last initiated TPC tasks
	strcpy(_logbinFilename, logbinFilename);

	_spi = createSPIAdapter(createString.c_str());
}

// Destructor (also closes connection)
FinisarHROCM_V3::~FinisarHROCM_V3()
{
    close();
}

// Open OCM
OCM_Error_t FinisarHROCM_V3::open()
{
	SPID_Error_t spiResult = _spi != NULL ? SPID_OK : SPID_FAILED;

	spiResult = spiResult || _spi->Open();

	OCM_Error_t Result = spiResult == SPID_OK ? OCM_OK : OCM_FAILED;
	Result = Result || logbinOpen();

	if (Result != OCM_OK) {
		LOGERROR("Could not open SPI adapter");
	}

	return Result;
}

// Close OCM
void FinisarHROCM_V3::close()
{
	if (_spi != NULL) {
		_spi->Close();
	}

	logbinClose();
}

// Open binary log file
OCM_Error_t FinisarHROCM_V3::logbinOpen()
{
	OCM_Error_t Result = OCM_OK;

	if (_logbinFilename[0] != 0 && _logbin == NULL) {
		_logbin = fopen(_logbinFilename, "wb");
		if (_logbin == NULL) {
			Result = OCM_FAILED;
			LOGERROR((std::string("Could not write file: ") + _logbinFilename).c_str());
		}
	}

	return Result;
}

// Close binary log file
OCM_Error_t FinisarHROCM_V3::logbinClose()
{
	OCM_Error_t Result = OCM_OK;

	if (_logbinFilename[0] != 0 && _logbin!=NULL) {
		fclose(_logbin);
		_logbin = NULL;
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::getID(std::string &ID)
{
	SPID_Error_t spiResult = _spi!=NULL ? SPID_OK : SPID_FAILED;

	spiResult = spiResult || _spi->GetID(ID);

	OCM_Error_t Result = spiResult == SPID_OK ? OCM_OK : OCM_FAILED;
	if (Result != OCM_OK) {
		LOGERROR("Could not get SPI adapter ID");
	}
	return Result;
}

OCM_Error_t FinisarHROCM_V3::setID(std::string ID)
{
	SPID_Error_t spiResult = _spi != NULL ? SPID_OK : SPID_FAILED;

	spiResult = spiResult || _spi->SetID(ID);

	OCM_Error_t Result = spiResult == SPID_OK ? OCM_OK : OCM_FAILED;
	if (Result != OCM_OK) {
		LOGERROR("Could not set SPI adapter ID");
	}
	return Result;
}

OCM_Error_t FinisarHROCM_V3::getAdapterFW(std::string &rev)
{
	SPID_Error_t spiResult = _spi != NULL ? SPID_OK : SPID_FAILED;

	spiResult = spiResult || _spi->GetFW(rev);

	OCM_Error_t Result = spiResult == SPID_OK ? OCM_OK : OCM_FAILED;
	if (Result != OCM_OK) {
		LOGERROR("Could not get SPI adapter firmware revision");
	}
	return Result;
}

OCM_Error_t FinisarHROCM_V3::checkInit()
{
	OCM_Error_t Result = OCM_OK;
	if (!_isInit) {
		Result = Result || cmdGETDEV(_lastHead,_lastRDataDEV);
		if (Result == OCM_OK) {
			_isInit = true;
		}
	}
	return Result;
}

OCM_Error_t FinisarHROCM_V3::get(int key, double &value)
{
	OCM_Error_t Result = OCM_OK;
	
	Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary

	switch (key)
	{
	case OCM_KEY_PARAM_MAXCH:					value = _lastRDataDEV.Nmax; break;
	case OCM_KEY_PARAM_FIRSTSLICEFREQ:			value = _lastRDataDEV.FSF/ OCM3_FSCALE; break;
	case OCM_KEY_PARAM_SLICEWIDTH:				value = _lastRDataDEV.SLW/ OCM3_FSCALE; break;
	case OCM_KEY_PARAM_MAXSLICES:				value = _lastRDataDEV.Smax; break;
	case OCM_KEY_PARAM_MODULETEMP:				value = _lastHead.CSS / OCM3_PSCALE; break;
	case OCM_KEY_PARAM_OPTICSTEMP:				value = _lastHead.ISS / OCM3_PSCALE; break;
	case OCM_KEY_PARAM_HARDWARESTATUS:			value = _lastHead.HSS; break;
	case OCM_KEY_PARAM_HARDWARESTATUSLATCHED:	value = _lastHead.LSS; break;
	case OCM_KEY_PARAM_OPERATIONALSTATUS:		value = _lastHead.OSS; break;
	case OCM_KEY_PARAM_SCANNO:					value = _lastGMPWResult.Head.SCAN; break;
	case OCM_KEY_PARAM_CAP_OSNRTHRES:			value = (_lastRDataDEV.BWXB == 'T') ; break;
	case OCM_KEY_PARAM_CAP_OSNRFIXED:			value = (_lastRDataDEV.BWXB != 'T'); break;
	default: Result = OCM_FAILED; break;
	}

	if (Result != OCM_OK) {
		LOGERROR(std::showbase << std::hex << "Failed to get key " << key << std::noshowbase << std::dec);
	}

	return Result;
}

std::string FinisarHROCM_V3::trim(std::string str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	if (first != (size_t)(-1) && last != (size_t)(-1)) {
		return str.substr(first, (last - first + 1));
	}

	return str;
}

const char *FinisarHROCM_V3::removePath(const char *filename)
{
	const char *p = strchr(filename, '\\');
	while (p) {
		filename = p + 1;
		p = strchr(filename, '\\');
	}
	return filename;
}

OCM_Error_t FinisarHROCM_V3::set(int key, double value)
{
	OCM_Error_t Result = OCM_FAILED;

	//switch (key)
	//{
	//}

	if (Result != OCM_OK) {
		std::ostringstream out;
		out << std::showbase << std::hex << "Failed to set key " << key << std::noshowbase << std::dec;
		LOGERROR(out.str().c_str());
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::get(int key, std::string &value)
{
	OCM_Error_t Result = OCM_OK;

	std::ostringstream out;
	char buf[33];
	memset(buf, 0, sizeof(buf));

	switch (key)
	{
	case OCM_KEY_LASTERROR:
		value = _lastError.str();
		_lastError.str("");
		_lastError.clear();
		break;
	case OCM_KEY_PARAM_SERIALNO:
		Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
		memcpy(buf, _lastRDataDEV.SNO, sizeof(_lastRDataDEV.SNO));
		value = trim(buf);
		break;
	case OCM_KEY_PARAM_MANUFACTURINGDATE:
		Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
		memcpy(buf, _lastRDataDEV.MFD, sizeof(_lastRDataDEV.MFD));
		value = trim(buf);
		break;
	case OCM_KEY_PARAM_LABEL:
		Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
		memcpy(buf, _lastRDataDEV.LBL, sizeof(_lastRDataDEV.LBL));
		value = trim(buf);
		break;
	case OCM_KEY_PARAM_MODULEID:
		Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
		memcpy(buf, _lastRDataDEV.MID, sizeof(_lastRDataDEV.MID));
		value = trim(buf);
		break;
	case OCM_KEY_PARAM_FIRMWAREVERSION:
		{
			Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
			unsigned int major = (_lastRDataDEV.FWR & 0xFF000000) >> 24;
			unsigned int minor = (_lastRDataDEV.FWR & 0x00FF0000) >> 16;
			unsigned int impl = (_lastRDataDEV.FWR & 0x0000FF00) >> 8;
			unsigned int rc = (_lastRDataDEV.FWR & 0x000000FF) >> 0;
			out << major << "." << minor << "." << impl;
			if (rc)
				out << "rc" << rc;
			value = out.str();
			break;
		}
	case OCM_KEY_PARAM_HARDWAREVERSION:
		{
			Result = Result || checkInit(); // Populate _lastRDataDEV and _lastHead if necessary
			unsigned int major = (_lastRDataDEV.HWR & 0xFF000000) >> 24;
			unsigned int minor = (_lastRDataDEV.HWR & 0x00FF0000) >> 16;
			unsigned int impl = (_lastRDataDEV.HWR & 0x0000FF00) >> 8;
			unsigned int rc = (_lastRDataDEV.HWR & 0x000000FF) >> 0;
			out << major << "." << minor << "." << impl;
			if (rc)
				out << "rc" << rc;
			value = out.str();
			break;
		}
	default: Result = OCM_FAILED; 
		break;
	}

	if (Result != OCM_OK) {
		LOGERROR(std::showbase << std::hex << "Failed to get key " << key << std::noshowbase << std::dec);
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::set(int key, std::string &value)
{
	OCM_Error_t Result = OCM_OK;

	switch(key)
	{
	case OCM_KEY_PARAM_MODULEID:
		Result = Result || cmdMID(value.c_str());
		break;
	default:
		Result = OCM_FAILED;
		break;
	}

	if (Result != OCM_OK) {
		LOGERROR(std::showbase << std::hex << "Failed to set key " << key << std::noshowbase << std::dec);
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::get(int key, std::vector<double> &value)
{
	OCM_Error_t Result = OCM_OK;

	Result = Result || checkInit();

	switch (key)
	{
	case OCM_KEY_CHANNELPLAN_FCENTER:
	case OCM_KEY_CHANNELPLAN_FSTART:
	case OCM_KEY_CHANNELPLAN_FSTOP:
		value.resize(_lastMPPWVector.size());
		for (unsigned int i = 0; i<_lastMPPWVector.size() && Result == OCM_OK; ++i)
		{
			double fStart = SLICE2FREQ(_lastMPPWVector[i].SLICESTART);
			double fStop = SLICE2FREQ(_lastMPPWVector[i].SLICEEND);

			if (key == OCM_KEY_CHANNELPLAN_FSTART)
				value[i] = fStart;

			if (key == OCM_KEY_CHANNELPLAN_FSTOP)
				value[i] = fStop;

			if (key == OCM_KEY_CHANNELPLAN_FCENTER)
				value[i] = (fStart+fStop)/2;
		}
		break;
	case OCM_KEY_SCAN_OSNR:
		value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
			value[k] = _lastGMOSNRResult.GMOSNRVector[k].OSNR / OCM3_PSCALE;
		}
		break;
	case OCM_KEY_SCAN_OSNRBANDWIDTHLOWER:
		value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[k].BANDWIDTHLOWER);
		}
		break;
	case OCM_KEY_SCAN_OSNRBANDWIDTHUPPER:
		value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[k].BANDWIDTHUPPER);
		}
		break;
	case OCM_KEY_SCAN_OSNRNOISETAGLOWER:
		value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[k].NOISETAGLOWER);
		}
		break;
	case OCM_KEY_SCAN_OSNRNOISETAGUPPER:
		value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[k].NOISETAGUPPER);
		}
		break;
	case OCM_KEY_SCAN_PEAKPOWER:
		//value.resize(_lastGMOSNRResult.GMOSNRVector.size());
		//for (unsigned int k = 0; k < _lastGMOSNRResult.GMOSNRVector.size(); ++k) {
		//	value[k] = _lastGMOSNRResult.GMOSNRVector[k].POWER / OCM3_PSCALE;
		//}
		value = _lastPeakPower;
		break;
	case OCM_KEY_SCAN_FCENTER:
		if (_lastGMOSNRResult.GMOSNRVector.size() > 0) {
			// If the channel plan contains a HiRes section, we are prepending it to the frequency vector
			unsigned int hiResSection = 0;
			for (unsigned int i = 0; i < _lastGMPWResult.GMPWVector.size(); ++i) {
				if (_lastGMPWResult.GMPWVector[i].SLICESTART == _lastGMPWResult.GMPWVector[i].SLICEEND) {
					hiResSection = i + 1;
				}
				else {
					break;
				}
			}
			if (hiResSection > 0) {
				value.resize(hiResSection + _lastGMOSNRResult.GMOSNRVector.size());
				for (unsigned int i = 0; i<hiResSection; ++i) {
					value[i] = SLICE2FREQ(_lastGMPWResult.GMPWVector[i].SLICESTART);
				}
				for (unsigned int i = 0; i<_lastGMOSNRResult.GMOSNRVector.size(); ++i) {
					value[hiResSection + i] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[i].CENTERFREQUENCY);
				}
			}
			else {
				value.resize(_lastGMOSNRResult.GMOSNRVector.size());
				for (unsigned int i = 0; i<_lastGMOSNRResult.GMOSNRVector.size(); ++i) {
					value[i] = SLICE2FREQ(_lastGMOSNRResult.GMOSNRVector[i].CENTERFREQUENCY);
				}
			}
		}
		else {
			value.resize(_lastGMPWResult.GMPWVector.size());
			for (unsigned int k = 0; k < _lastGMPWResult.GMPWVector.size(); ++k) {
				double fSliceLeft = SLICE2FREQ(_lastGMPWResult.GMPWVector[k].SLICESTART);
				double fSliceRight = SLICE2FREQ(_lastGMPWResult.GMPWVector[k].SLICEEND);
				double power = _lastGMPWResult.GMPWVector[k].POWER / OCM3_PSCALE;
				value[k] = (fSliceLeft + fSliceRight) / 2;
			}
		}
		break;
	case OCM_KEY_SCAN_POWER:
		value.resize(_lastGMPWResult.GMPWVector.size());
		for (unsigned int k = 0; k < _lastGMPWResult.GMPWVector.size(); ++k) {
			double fSliceLeft = SLICE2FREQ(_lastGMPWResult.GMPWVector[k].SLICESTART);
			double fSliceRight = SLICE2FREQ(_lastGMPWResult.GMPWVector[k].SLICEEND);
			double power = _lastGMPWResult.GMPWVector[k].POWER / OCM3_PSCALE;
			value[k] = power;
		}
		// If there is OSNR-data overwrite the data after the hiRes section with the results from the OSNR calculation
		if (_lastGMOSNRResult.GMOSNRVector.size() > 0) {
			// Work out if there is a hiRes section
			unsigned int hiResSection = 0;
			for (unsigned int i = 0; i < _lastGMPWResult.GMPWVector.size(); ++i) {
				if (_lastGMPWResult.GMPWVector[i].SLICESTART == _lastGMPWResult.GMPWVector[i].SLICEEND) {
					hiResSection = i + 1;
				}
				else {
					break;
				}
			}
			if (hiResSection > 0) {
				for (unsigned int i = 0; i<_lastGMOSNRResult.GMOSNRVector.size() && hiResSection+i<value.size(); ++i) {
					value[hiResSection + i] = _lastGMOSNRResult.GMOSNRVector[i].POWER / OCM3_PSCALE;
				}
			}
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRCENTERSTART:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastMPOSNRVector[k].CENTERSTART);
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRCENTERSTOP:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = SLICE2FREQ(_lastMPOSNRVector[k].CENTERSTOP);
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRCENTERBWTHRES:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			if (_lastRDataDEV.BWXB == 'T') {
				value[k] = - _lastMPOSNRVector[k].CENTERBWTHRES/ OCM3_PSCALE;
			}
			else {
				value[k] = (int)(1 + 2*_lastMPOSNRVector[k].CENTERBWTHRES)*_lastRDataDEV.SLW/OCM3_FSCALE;
			}
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRTAGRANGE:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = (int)(1 + 2 * _lastMPOSNRVector[k].TAGRANGE)*_lastRDataDEV.SLW / OCM3_FSCALE;
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRNOISELOWER:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = (int)_lastMPOSNRVector[k].NOISELOWER*_lastRDataDEV.SLW / OCM3_FSCALE;
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRNOISEUPPER:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = (int)_lastMPOSNRVector[k].NOISEUPPER*_lastRDataDEV.SLW / OCM3_FSCALE;
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRKEEPOUTLOWER:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = (int)_lastMPOSNRVector[k].KEEPOUTLOWER *_lastRDataDEV.SLW / OCM3_FSCALE;
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRKEEPOUTUPPER:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = (int)_lastMPOSNRVector[k].KEEPOUTUPPER *_lastRDataDEV.SLW / OCM3_FSCALE;
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRRBW:
		value.resize(_lastMPOSNRVector.size());
		for (unsigned int k = 0; k < _lastMPOSNRVector.size(); ++k) {
			value[k] = _lastMPOSNRVector[k].RBW/ OCM3_RBWSCALE;
		}
		break;
	default:
		Result = OCM_FAILED;
		break;
	}

	if (Result != OCM_OK) {
		LOGERROR(std::showbase << std::hex << "Failed to get key " << key << std::noshowbase << std::dec);
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::set(int key, std::vector<double> &value)
{
	OCM_Error_t Result = OCM_OK;

	Result = Result || checkInit();

	switch (key)
	{
	case OCM_KEY_CHANNELPLAN_FSTART:
	case OCM_KEY_CHANNELPLAN_FSTOP:
		if (_lastMPPWVector.size() != value.size()) {
			_lastMPPWVector.resize(value.size());
		}
		if (Result == OCM_OK && (_lastRDataDEV.FSF == 0 || _lastRDataDEV.SLW == 0 || _lastRDataDEV.Smax == 0)) {
			LOGERROR("Could not determine module scan range");
			Result = OCM_FAILED;
		}

		for (unsigned int i = 0; i<value.size() && Result == OCM_OK; ++i)
		{
			_lastMPPWVector[i].PORTNO = 1;

			if (key == OCM_KEY_CHANNELPLAN_FSTART)
			{
				int sliceStart = FREQ2SLICE(value[i]);
				if (sliceStart < 1 || sliceStart > _lastRDataDEV.Smax) {
					LOGERROR("Start slice number " << sliceStart << " out of range (channel index " << i << ")");
					Result = Result || OCM_FAILED;
				}
				_lastMPPWVector[i].SLICESTART = sliceStart;         // Slice numbers are 1-based
			}

			if (key == OCM_KEY_CHANNELPLAN_FSTOP)
			{
				int sliceEnd = FREQ2SLICE(value[i]) - 1;
				if (sliceEnd < 1 || sliceEnd > _lastRDataDEV.Smax) {
					LOGERROR("End slice number " << sliceEnd << " out of range (channel index " << i << ")");
					Result = Result || OCM_FAILED;
				}
				_lastMPPWVector[i].SLICEEND = sliceEnd;             // Slice numbers are 1-based
			}
		}
		break;
	case OCM_KEY_CHANNELPLAN_OSNRCENTERSTART:
	case OCM_KEY_CHANNELPLAN_OSNRCENTERSTOP:
	case OCM_KEY_CHANNELPLAN_OSNRCENTERBWTHRES:
	case OCM_KEY_CHANNELPLAN_OSNRTAGRANGE:
	case OCM_KEY_CHANNELPLAN_OSNRNOISELOWER:
	case OCM_KEY_CHANNELPLAN_OSNRNOISEUPPER:
	case OCM_KEY_CHANNELPLAN_OSNRKEEPOUTLOWER:
	case OCM_KEY_CHANNELPLAN_OSNRKEEPOUTUPPER:
	case OCM_KEY_CHANNELPLAN_OSNRRBW:
		if (_lastMPOSNRVector.size() != value.size()) {
			_lastMPOSNRVector.resize(value.size());
		}
		if (Result == OCM_OK && (_lastRDataDEV.FSF == 0 || _lastRDataDEV.SLW == 0 || _lastRDataDEV.Smax == 0)) {
			LOGERROR("Could not determine module scan range");
			Result = OCM_FAILED;
		}
		for (unsigned int k = 0; k < value.size() && Result == OCM_OK; ++k) {
			_lastMPOSNRVector[k].PORTNO = 1;

			if (key == OCM_KEY_CHANNELPLAN_OSNRCENTERSTART) {
				int slice = FREQ2SLICE(value[k]);
				if (slice < 1 || slice > _lastRDataDEV.Smax) {
					LOGERROR("Slice number " << slice << " out of range (channel index " << k << ")");
					Result = Result || OCM_FAILED;
				}
				_lastMPOSNRVector[k].CENTERSTART = slice;
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRCENTERSTOP) {
				int slice = FREQ2SLICE(value[k]) - 1;
				if (slice < 1 || slice > _lastRDataDEV.Smax) {
					LOGERROR("Slice number " << slice << " out of range (channel index " << k << ")");
					Result = Result || OCM_FAILED;
				}
				_lastMPOSNRVector[k].CENTERSTOP = slice;
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRCENTERBWTHRES) {
				if (_lastRDataDEV.BWXB == 'T') {
					if (value[k] > 0) { // Only negative values are allowed in this mode
						Result = Result || OCM_FAILED;
						LOGERROR("Mode not supported. BWXB is set to 'T'. Use command line tool to set it to 'S' (HROCMQueryV3.exe bws).")
					}
					else {
						_lastMPOSNRVector[k].CENTERBWTHRES = (unsigned short)round(-value[k] * OCM3_PSCALE);
					}
				}
				else {
					if (value[k] <= 0) { // Only positive values are allowed in this mode
						Result = Result || OCM_FAILED;
						LOGERROR("Mode not supported. BWXB is set to 'S'. Use command line tool to set it to 'T' (HROCMQueryV3.exe bwt).")
					}
					else {
						_lastMPOSNRVector[k].CENTERBWTHRES = (unsigned short)round((value[k] * OCM3_FSCALE / _lastRDataDEV.SLW - 1) / 2);
					}
				}
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRTAGRANGE) {
				_lastMPOSNRVector[k].TAGRANGE = (unsigned short)round((value[k]* OCM3_FSCALE / _lastRDataDEV.SLW - 1)/2);
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRNOISELOWER) {
				_lastMPOSNRVector[k].NOISELOWER = (unsigned short)round(value[k] * OCM3_FSCALE / _lastRDataDEV.SLW);
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRNOISEUPPER) {
				_lastMPOSNRVector[k].NOISEUPPER = (unsigned short)round(value[k] * OCM3_FSCALE / _lastRDataDEV.SLW);
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRKEEPOUTLOWER) {
				_lastMPOSNRVector[k].KEEPOUTLOWER = (unsigned short)round(value[k] * OCM3_FSCALE / _lastRDataDEV.SLW);
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRKEEPOUTUPPER) {
				_lastMPOSNRVector[k].KEEPOUTUPPER = (unsigned short)round(value[k] * OCM3_FSCALE / _lastRDataDEV.SLW);
			}
			if (key == OCM_KEY_CHANNELPLAN_OSNRRBW) {
				_lastMPOSNRVector[k].RBW = (unsigned short)round(value[k]* OCM3_RBWSCALE);
			}
		}
		break;
	default:
		Result = OCM_FAILED;
		break;
	}

	if (Result != OCM_OK) {
		LOGERROR(std::showbase << std::hex << "Failed to set key " << key << std::noshowbase << std::dec);
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::preset()
{
	_lastMPPWVector.clear();
	_lastMPOSNRVector.clear();

	return OCM_OK;
}


OCM_Error_t FinisarHROCM_V3::setChannelPlan()
{
	OCM_Error_t Result = OCM_OK;
	
	if (_lastMPPWVector.size() > 0) {
		Result = Result || cmdSETMPPW(_lastMPPWVector);
	}

	if (_lastMPOSNRVector.size() > 0) {
		Result = Result || cmdSETMPOSNR(_lastMPOSNRVector);
	}

	// Indicate that we need another TPC
	_lastTxSeqNumValid = false;

	// printf("setChannelPlan(): _lastTxSeqNum=%d, _seqno=%d, _lastTxSeqNumValid=%d\n", _lastTxSeqNum, _seqnum, (int)_lastTxSeqNumValid);

	return Result;
}

OCM_Error_t FinisarHROCM_V3::queryState()
{
	OCM_Error_t Result = OCM_OK;

	Result = Result || cmdGETDEV(_lastHead, _lastRDataDEV);
	//Result = Result || checkInit();

	return Result;
}

OCM_Error_t FinisarHROCM_V3::startScan()
{
	OCM_Error_t Result = OCM_OK;
	OCM3_Response_t Head;

	OCM3_TPCProcessMask_t TaskVector = OCM3_TASK_PW_MASK;
	if (_lastMPOSNRVector.size() > 0) {
		TaskVector |= OCM3_TASK_OSNR_MASK;
	}

	Result = Result || cmdTPC(Head, _lastTxSeqNum, TaskVector);
	if (Result == OCM_OK) {
		_lastTxSeqNumOSNR = _lastTxSeqNum;
		_lastTPCTask = TaskVector;
		_lastTxSeqNumValid = true;
	}

	//Result = Result || cmdTPCRaw(Head, _lastTxSeqNum, OCM3_TASK_PW_MASK);
	//if (Result == OCM_OK) {
	//	_lastTPCTask = TaskVector;
	//	_lastTxSeqNumValid = true;
	//}

	//if (_lastMPOSNRVector.size() > 0) {
	//	::Sleep(1000);
	//	Result = Result || cmdTPCRaw(Head, _lastTxSeqNumOSNR, OCM3_TASK_OSNR_MASK);
	//	if (Result == OCM_OK) {
	//		_lastTPCTask = TaskVector;
	//		_lastTxSeqNumValid = true;
	//	}
	//}

	// printf("startScan(): _lastTxSeqNum=%d, _seqno=%d, _lastTxSeqNumValid=%d\n", _lastTxSeqNum, _seqnum, (int)_lastTxSeqNumValid);

	return Result;
}

OCM_Error_t FinisarHROCM_V3::readScan()
{
	// Poll and wait for the last TxSeqNum we transmitted using the startScan command
	OCM_Error_t Result = OCM_OK;

	if (!_lastTxSeqNumValid) {
		Result = OCM_FAILED;
		LOGERROR("No scan started");
	}

	// Poll and wait for the last TxSeqNum we transmitted using the startScan command
	if ((_lastTPCTask & OCM3_TASK_PW_MASK) != 0) {
		Result = Result || cmdQueryTPC_PW(_lastGMPWResult, _lastTxSeqNum);
	}

	// Poll and wait for the last TxSeqNum we transmitted using the startScan command
	if ((_lastTPCTask & OCM3_TASK_OSNR_MASK) != 0) {
		Result = Result || cmdQueryTPC_OSNR(_lastGMOSNRResult, _lastTxSeqNumOSNR);
	}

	Result = Result || runPostProcessing();

	// printf("readScan(): _lastTxSeqNum=%d, _seqno=%d, _lastTxSeqNumValid=%d, Result=%d\n", _lastTxSeqNum, _seqnum, (int)_lastTxSeqNumValid,(int)Result);

	return Result;
}

bool FinisarHROCM_V3::isScanFinished()
{
	OCM_Error_t Result = OCM_OK;

	Result = Result || cmdPollShort(_lastHead);
	if (Result != OCM_OK) {
		return true;
	}

	if (_lastHead.SEQNO != _lastTxSeqNum && _lastHead.SEQNO != _lastTxSeqNumOSNR) {
		return false;
	}

	if (_lastHead.COMRES < 0) {
		return false;
	}

	if (_lastTxSeqNum == 0) {
		return true;
	}

	if ((_lastTPCTask & OCM3_TASK_PW_MASK) != 0 && _lastHead.SEQARR[OCM3_PROCESS_PW] != _lastTxSeqNum) {
		return false;
	}

	if ((_lastTPCTask & OCM3_TASK_OSNR_MASK) != 0 && _lastHead.SEQARR[OCM3_PROCESS_OSNR] != _lastTxSeqNumOSNR) {
		return false;
	}

	return true;
}

// Run an SPI transfer and wait afterwards to make sure OCM recovers
OCM_Error_t FinisarHROCM_V3::spiTransfer(char *writeBuffer, char *readBuffer, size_t length)
{
    logTx(writeBuffer,length);
    // Check maximum size in a single SPI transfer
    if (length>OCM3_LENMAX)
    {
        LOGFAILED();
		LOGERROR("Requested SPI block size is too large " << length << ">" << OCM3_LENMAX);
		return OCM_FAILED;
    }
	SPID_Error_t spiResult = _spi != NULL ? SPID_OK : SPID_FAILED;

    spiResult = spiResult || _spi->Transfer(writeBuffer, readBuffer, length);
    Sleep(_recover_ms); // The OCM needs 5ms to recover.
    logRx(readBuffer,length);
	logBin(spiResult, writeBuffer, readBuffer, length);
    
	OCM_Error_t Result = spiResult==SPID_OK ? OCM_OK : OCM_FAILED;
	LOGRESULT(Result);

	if (Result != OCM_OK) {
		LOGERROR("SPI transfer failed");
	}

	return Result;
}

// Run an SPI transfer and wait afterwards to make sure OCM recovers
OCM_Error_t FinisarHROCM_V3::spiTransfer(std::vector<char> &Tx,std::vector<char> &Rx)
{
    logTx(&Tx[0],Tx.size());
    // Check maximum size in a single SPI transfer
    if (Tx.size()>OCM3_LENMAX)
    {
        LOGFAILED();
		LOGERROR("Requested SPI block size is too large " << Tx.size() << ">" << OCM3_LENMAX);
		return OCM_FAILED;
    }
	SPID_Error_t spiResult = _spi != NULL ? SPID_OK : SPID_FAILED;

	spiResult = spiResult || _spi->Transfer(Tx,Rx);
    Sleep(_recover_ms); // The OCM needs 5ms to recover.
    logRx(&Rx[0],Rx.size());
	logBin(spiResult, &Tx[0], &Rx[0], Tx.size());

	OCM_Error_t Result = spiResult == SPID_OK ? OCM_OK : OCM_FAILED;
	LOGRESULT(Result);

	if (Result != OCM_OK) {
		LOGERROR("SPI transfer failed");
	}

	return Result;
}

// Write binary data to log file
OCM_Error_t FinisarHROCM_V3::logBin(SPID_Error_t Result, char *writeBuffer, char *readBuffer, size_t length)
{
	if (_logbin != NULL) {
		DWORD tickCountMs = ::GetTickCount();
		unsigned int magic = 0xBEEFBEEF;
		fwrite(&magic, sizeof(magic), 1, _logbin);				// Write magic number 0xBEEFBEEF
		fwrite(&tickCountMs, sizeof(tickCountMs), 1, _logbin);	// Write system tick count in ms
		fwrite(&Result, sizeof(Result), 1, _logbin);			// Write SPI transfer result
		fwrite(&length, sizeof(length), 1, _logbin);			// Write package length
		fwrite(writeBuffer, 1, length, _logbin);				// Write writeBuffer
		fwrite(readBuffer, 1, length, _logbin);					// Write readBuffer
	}

	return OCM_OK;
}

// Construct the header of a command package
int FinisarHROCM_V3::fillInHROCMCommand(char* buffer, unsigned int datasize, int opcode, unsigned int seqnum)
{
    OCM3_cmd_t cmd;
    cmd.SPIMAGIC    = OCM_SPIMAGIC_V3;
    cmd.LENGTH      = (datasize==0)?sizeof(cmd):sizeof(cmd)+datasize+4;
    cmd.SEQNO       = seqnum;
    cmd.OPCODE      = opcode;
    cmd.CRC1        = 0;

    CCRC32 crc;
    cmd.CRC1 = crc.FullCRC((const unsigned char *)&cmd, 16);
    memcpy(buffer, &cmd, sizeof(cmd));

    if(datasize>0)
    {
      CCRC32 crc;
      unsigned int crc2 = crc.FullCRC((const unsigned char *)buffer, cmd.LENGTH-4);
      memcpy(buffer+cmd.LENGTH-4, &crc2, sizeof(crc2));
    }
    return cmd.LENGTH;
}

OCM_Error_t FinisarHROCM_V3::checkSPIMAGICEMPTY(OCM3_Response_t* pResponse)
{
	if (pResponse->SPIMAGIC == 0xFFFFFFFF) {
		return OCM_FAILED;
	}

	return OCM_OK;
}

OCM_Error_t FinisarHROCM_V3::checkSPIMAGIC(OCM3_Response_t* pResponse)
{
	if (pResponse->SPIMAGIC != OCM_SPIMAGIC_V3) {
		return OCM_FAILED;
	}

	return OCM_OK;
}

// Check CRC1 of a received package
OCM_Error_t FinisarHROCM_V3::checkCRC1(OCM3_Response_t* pResponse,size_t size)
{
	if (size < 20) {
		return OCM_FAILED;
	}

    CCRC32 crc;
	if (crc.FullCRC((const unsigned char *)pResponse, 20) != pResponse->CRC1) {
		return OCM_FAILED;
	}

    return OCM_OK;
}

// Check CRC2 of a received package
OCM_Error_t FinisarHROCM_V3::checkCRC2(OCM3_Response_t* pResponse,size_t size)
{
	if (checkSPIMAGIC(pResponse) != OCM_OK) {
		return OCM_FAILED;
	}

	if (checkCRC1(pResponse, size) != OCM_OK) {
		return OCM_FAILED;
	}

	if (size < pResponse->LENGTH) {
		return OCM_FAILED;
	}

    CCRC32 crc;
    unsigned long *pCRC = (unsigned long *) (((char*)pResponse) + pResponse->LENGTH - 4);
	if (crc.FullCRC((const unsigned char *)pResponse, pResponse->LENGTH - 4) != *pCRC) {
		return OCM_FAILED;
	}

    return OCM_OK;
}

// For debugging: Print contents of a response package
// Set PrintFullPackage==true if we want to look into the RData section
void FinisarHROCM_V3::printResponse(OCM3_Response_t* pResponse,bool PrintFullPackage)
{
    OCM3_Response_t& rsp = *pResponse;

    printf("SPI_Header:\n");
    printf("SPIMAGIC,%x\n", rsp.SPIMAGIC);
    printf("LENGTH,%u\n", rsp.LENGTH);
    printf("SEQNO,%u\n", rsp.SEQNO);
    printf("OPCODE,%x\n", rsp.OPCODE);
    printf("COMRES,%d\n", rsp.COMRES);
    printf("CRC1,%x\n", rsp.CRC1);

    printf("OSS,%x\n", rsp.OSS);
    printf("HSS,%x\n", rsp.HSS);
    printf("LSS,%x\n", rsp.LSS);
    printf("CSS,%d\n", rsp.CSS);
    printf("ISS,%d\n", rsp.ISS);
    printf("PPEND,%u\n", rsp.PPEND);

	printf("SEQARR,");
	for (int k = 0; k < rsp.NSEQARR; ++k) {
		printf("%s%u", k>0 ? "," : "", rsp.SEQARR[k]);
	}
	printf("\n");

	if (PrintFullPackage) {
		printRData(rsp.OPCODE, (char*) (rsp.SEQARR + rsp.NSEQARR), (char*)pResponse + rsp.LENGTH - (char*)(rsp.SEQARR + rsp.NSEQARR) - sizeof(unsigned int), true);
	}

    CCRC32 crc;
    unsigned int ThisCRC = crc.FullCRC((const unsigned char *)&rsp, 20);
	if (ThisCRC != rsp.CRC1) {
		printf("Warning: CRC1 is wrong: %x\n", ThisCRC);
	}
    //printf("CRC2Verify:%x\n", crc.FullCRC((const unsigned char *)&rsp, sizeof(OCM3_Response_t)-4));
}

// For debugging: Print contents of an RDATA section
void FinisarHROCM_V3::printRData(unsigned int OPCODE,char *pRData,size_t size, bool PrintFullPackage)
{
	if (!pRData) {
		return;
	}

	printf("RDATA_Content:\n");
	switch (OPCODE) {
	case  OPCODE_GETDEV: {
		char s[33];
		OCM3_RDataDEV_t *pDEV = (OCM3_RDataDEV_t *)pRData;

		printf("HWR,%x\n", pDEV->HWR);
		printf("FWR,%x\n", pDEV->FWR);
		memset(s, 0, sizeof(s));
		memcpy(s, pDEV->SNO, sizeof(pDEV->SNO));
		printf("SNO,%s\n", s);
		memset(s, 0, sizeof(s));
		memcpy(s, pDEV->MFD, sizeof(pDEV->MFD));
		printf("MFD,%s\n", s);
		memset(s, 0, sizeof(s));
		memcpy(s, pDEV->LBL, sizeof(pDEV->LBL));
		printf("LBL,%s\n", s);
		memset(s, 0, sizeof(s));
		memcpy(s, pDEV->MID, sizeof(pDEV->MID));
		printf("MID,%s\n", s);
		printf("Pmax,%d\n", pDEV->Pmax);
		printf("Smax,%d\n", pDEV->Smax);
		printf("SLW,%d\n", pDEV->SLW);
		printf("Nmax,%d\n", pDEV->Nmax);
		printf("FSF,%u\n", pDEV->FSF);
		printf("LSF,%u\n", pDEV->FSF + pDEV->SLW*(pDEV->Smax - 1));
		printf("BWXB,%c\n", pDEV->BWXB);
		printf("CAP,%u\n", pDEV->CAP);
		break;
	}
	case OPCODE_GETMPW: {
		OCM3_GMPWHead_t *pHead = (OCM3_GMPWHead_t*)pRData;
		OCM3_GMPWRecord_t *pRecord = (OCM3_GMPWRecord_t*)(pRData + sizeof(OCM3_GMPWHead_t));
		unsigned int nRecords = (unsigned int) ((size - sizeof(OCM3_GMPWHead_t)) / sizeof(OCM3_GMPWRecord_t));

		printf("MPSEQNO,%u\n", pHead->MPSEQNO);
		printf("SCAN,%u\n", pHead->SCAN);
		printf("RECORDS,%u\n", nRecords);
		for (unsigned int k = 0; k < nRecords; ++k) {
			printf("%d,%d,%d,%d\n", pRecord[k].PORTNO, pRecord[k].SLICESTART, pRecord[k].SLICEEND, pRecord[k].POWER);
		}
		break;
	}
	case OPCODE_GETMOSNR: {
		OCM3_GMOSNRHead_t *pHead = (OCM3_GMOSNRHead_t*)pRData;
		OCM3_GMOSNRRecord_t *pRecord = (OCM3_GMOSNRRecord_t*)(pRData + sizeof(OCM3_GMOSNRHead_t));
		unsigned int nRecords = (unsigned int) ((size - sizeof(OCM3_GMOSNRHead_t)) / sizeof(OCM3_GMOSNRRecord_t));

		printf("MPSEQNO,%u\n", pHead->MPSEQNO);
		printf("SCAN,%u\n", pHead->SCAN);
		printf("RECORDS,%u\n", nRecords);
		for (unsigned int k = 0; k < nRecords; ++k) {
			printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", pRecord[k].PORTNO, pRecord[k].SLICESTART, pRecord[k].SLICEEND, pRecord[k].OSNR, pRecord[k].POWER, pRecord[k].NOISETAGLOWER, pRecord[k].NOISETAGUPPER, pRecord[k].BANDWIDTHLOWER, pRecord[k].BANDWIDTHUPPER, pRecord[k].CENTERFREQUENCY);
		}
		break;
	}
	default:
		printf("Warning: Unknown RDATA OPCODE 0x%X\n",OPCODE);
		break;
	}
}

void FinisarHROCM_V3::logTx(char *pData,size_t size)
{
    if (!_log)
        return;

    fprintf(_log,"%u,%d,",::GetTickCount(),(int)size);
    fprintf(_log,"%d,%d,%d,",_nCRC1ErrorCount,_nCRC2ErrorCount,_nCmdRetransmit);

    if (size>=sizeof(OCM3_cmd_t))
    {
        OCM3_cmd_t *p = (OCM3_cmd_t*) pData;

        fprintf(_log,"%s,%u,", OCM3_ParseOPCODE(p->OPCODE).c_str(),p->SEQNO);

        CCRC32 crc;
        unsigned int CRC1 = crc.FullCRC((const unsigned char*)p, 16);
        fprintf(_log,"CRC1=%s,",p->CRC1 == CRC1 || p->OPCODE==0 ? "OK":"FAIL");
    }
    else
        fprintf(_log,"???,???,???,");
}

void FinisarHROCM_V3::logRx(char *pData,size_t size)
{
    if (!_log)
        return;

    if (size>=sizeof(OCM3_Response_t))
    {
        OCM3_Response_t *p = (OCM3_Response_t*) pData;

        fprintf(_log,"%u,%s,%u,%d,%u,%u,%u,%u,%u,%u,%X,%X,", p->LENGTH, OCM3_ParseOPCODE(p->OPCODE).c_str(),p->SEQNO,p->COMRES,p->PPEND, p->SEQARR[0], p->SEQARR[1], p->SEQARR[2], p->SEQARR[3], p->SEQARR[4], p->HSS,p->OSS);

        CCRC32 crc;
        unsigned int CRC1 = crc.FullCRC((const unsigned char*)p, 20);
        bool CRC1OK = p->CRC1 == CRC1;
        fprintf(_log,"CRC1=%s,",CRC1OK ? "OK":"FAIL");

        bool CRC2OK = true;
        if (CRC1OK && size>=p->LENGTH)
        {
            CRC2OK = checkCRC2(p,size)==OCM_OK;
            unsigned int *pCRC = (unsigned int *) (((char*)p) + p->LENGTH - 4);
            fprintf(_log,"%08X,CRC2=%s,",*pCRC, CRC2OK ? "OK":"FAIL");
        }
        else
            fprintf(_log,"???,???,");

        if (!CRC1OK || !CRC2OK)
        {
            char Filename[32];
            sprintf(Filename,"%u.bin",::GetTickCount());
            FILE *f = fopen(Filename,"wb");
            if (f)
            {
                fwrite(pData,1,size,f);
                fclose(f);
            }
        }
    }
    else
        fprintf(_log,"???,???,???,???,???,???,???,???,???,???,???,???,???,???,???,");
}

// Polls the header information (does not pick up RDATA)
OCM_Error_t FinisarHROCM_V3::cmdPollShort(OCM3_Response_t &Head)
{
    OCM_Error_t Result = OCM_OK;
	int lastError = 0;

    for(int k=0;Result==OCM_OK;++k) {
		if (k > _nretry) {
			Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			break;
		}

        // Construct command package (all zeroes for poll command)
		OCM3_Response_t Command;
		memset(&Command, 0, sizeof(OCM3_Response_t));
        // Prefill response package
        memset(&Head, 0, sizeof(OCM3_Response_t));

        // Run SPI transfer (and wait 5 ms)
        Result = Result || spiTransfer((char*)&Command,(char*)&Head, sizeof(OCM3_Response_t));
		if (Result != OCM_OK) {
			break;
		}

		// If SPIMAGIC is 0xFFFFFFFF, retry
		if (checkSPIMAGICEMPTY(&Head) != OCM_OK)
		{
			lastError = 3;
			_nSPIMAGICErrorCount++;
			continue;
		}

		// If SPIMAGIC is wrong, retry
		if (checkSPIMAGIC(&Head) != OCM_OK) {
			lastError = 1;
			_nSPIMAGICErrorCount++;
			continue;
		}
		
        // If CRC1 checksum is false, retry
        if(checkCRC1(&Head,sizeof(OCM3_Response_t))!=OCM_OK) {
			lastError = 2;
            _nCRC1ErrorCount++;
            continue;
        }

        break;
    }

	if (Result != OCM_OK) {
		switch (lastError) {
		case 1:
			if (Head.SPIMAGIC == OCM_SPIMAGIC_V1) {
				LOGERROR("The module uses SPI Protocol Version 1. Please use HROCMQuery.exe with this module.");
			}
			else {
				LOGERROR(std::showbase << std::hex << "SPIMAGIC mismatch (" << Head.SPIMAGIC << " should be " << OCM_SPIMAGIC_V3 << ")" << std::noshowbase << std::dec);
			}
			break;
		case 2:
			LOGERROR(std::showbase << std::hex << "CRC1 failed (" << Head.CRC1 << ")" << std::noshowbase << std::dec);
			break;
		case 3:
			LOGERROR("SPIMAGIC is 0xFFFFFFFF - Module turned on?");
			break;
		case 0:
			LOGERROR("Timeout");
			break;
		default:
			break;
		}
	}

    return Result;
};

// Polls the whole header including RDATA
OCM_Error_t FinisarHROCM_V3::cmdPollLong(OCM3_Response_t &Head,std::vector<char> &RDATA,unsigned int seqnum)
{
    OCM_Error_t Result = OCM_OK;

	if (seqnum == 0) {
		Result = Result || cmdPollShort(Head);    // If we are not waiting for a specific sequence number, just run regular poll
	}
	else {
		Result = Result || waitForReply(Head, seqnum); // Wait for a header which contains seqnum
	}
	if (Result != OCM_OK) {
		return Result;
	}

    // Now that we know how long the whole block is, reserve memory for the whole result and poll again.
    std::vector<char> Command;
    std::vector<char> Response;
    Command.assign(Head.LENGTH,0);
    Response.resize(Head.LENGTH);

    for(int k=0;Result==OCM_OK;++k) {
		if (k > _nretry) {
			Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
		}

        Result = Result || spiTransfer(Command,Response);
		if (Result != OCM_OK) {
			break;
		}

        OCM3_Response_t *pResponse = (OCM3_Response_t*)&Response[0];

		// If SPIMAGIC is wrong, retry
		if (checkSPIMAGIC(pResponse) != OCM_OK) {
			_nSPIMAGICErrorCount++;
			LOGWARNING(std::showbase << std::hex << "SPIMAGIC mismatch - retrying (" << pResponse->SPIMAGIC << " should be " << OCM_SPIMAGIC_V3 << ")" << std::noshowbase << std::dec);
			continue;
		}

		// Check CRC1, if it's false, try again
        if(checkCRC1(pResponse,Head.LENGTH)!=OCM_OK) {
            _nCRC1ErrorCount++;
			LOGWARNING(std::showbase << std::hex << "CRC1 failed - retrying (" << pResponse->CRC1 << ")" << std::noshowbase << std::dec);
			continue;
        }

        // Check CRC2, if it's false, try again
        if(checkCRC2(pResponse,Head.LENGTH)!=OCM_OK) {
            _nCRC2ErrorCount++;
			LOGWARNING("CRC2 failed - retrying");
			continue;
        }

        break;
    }

    if (Result==OCM_OK) {
        // Copy payload to result vector
        OCM3_Response_t *pResponse = (OCM3_Response_t*)&Response[0];
        RDATA.resize(pResponse->LENGTH - ((char*)&pResponse->SEQARR[pResponse->NSEQARR] - &Response[0]) - 4);
		if (RDATA.size() > 0) {
			memcpy(&RDATA[0], &pResponse->SEQARR[pResponse->NSEQARR], RDATA.size());
		}
    }

    return Result;
};

// Polls the channel plan and power values.
OCM_Error_t FinisarHROCM_V3::cmdQueryTPC_PW(OCM3_GMPWResult_t &GMPWResult,unsigned int TxSeqNum)
{
    OCM_Error_t Result = OCM_OK;

	// Make sure we have supporting information in _lastRDataDEV
	Result = Result || checkInit();

	// Wait until sequence number shows up in SEQARR[0]
	OCM3_Response_t	Head;
	Result = Result || waitTaskComplete(Head, OCM3_PROCESS_PW, TxSeqNum);

	// Send GMPW command
	Result = Result || cmdSimple(OPCODE_GETMPW);

	// Read RDATA
    std::vector<char> RDATA;
    Result = Result || cmdPollLong(Head,RDATA,0);
	if (Result != OCM_OK) {
		return Result;
	}

	// Copy header information to target object
	memcpy(&GMPWResult.Head, &RDATA[0], sizeof(GMPWResult.Head));

	// Work out the number of records
	const size_t headerSize = 8;
	size_t nRecords = (RDATA.size() - headerSize) / sizeof(OCM3_GMPWRecord_t);
	//printf("nRecords=%d\n", nRecords);

    OCM3_GMPWRecord_t *pRecord = (OCM3_GMPWRecord_t *) (&RDATA[headerSize]);
	GMPWResult.GMPWVector.resize(nRecords);

    memcpy((char*)&GMPWResult.GMPWVector[0],(char*)pRecord,nRecords*sizeof(OCM3_GMPWRecord_t));

    return Result;
};

// Polls the OSNR Result.
OCM_Error_t FinisarHROCM_V3::cmdQueryTPC_OSNR(OCM3_GMOSNRResult_t &GMOSNRResult, unsigned int TxSeqNum)
{
	OCM_Error_t Result = OCM_OK;

	// Make sure we have supporting information in _lastRDataDEV
	Result = Result || checkInit();

	// Wait until sequence number shows up in SEQARR[1]
	OCM3_Response_t	Head;
	Result = Result || waitTaskComplete(Head, OCM3_PROCESS_OSNR, TxSeqNum);

	// Send GMOSNR command
	Result = Result || cmdSimple(OPCODE_GETMOSNR);

	// Read RDATA
	std::vector<char> RDATA;
	Result = Result || cmdPollLong(Head, RDATA, 0);
	if (Result != OCM_OK) {
		return Result;
	}

	// Copy header information to target object
	memcpy(&GMOSNRResult.Head, &RDATA[0], sizeof(GMOSNRResult.Head));

	// Work out the number of records
	const size_t headerSize = 8;
	size_t nRecords = (RDATA.size() - headerSize) / sizeof(OCM3_GMOSNRRecord_t);

	OCM3_GMOSNRRecord_t *pRecord = (OCM3_GMOSNRRecord_t *)(&RDATA[headerSize]);
	GMOSNRResult.GMOSNRVector.resize(nRecords);

	memcpy((char*)&GMOSNRResult.GMOSNRVector[0], (char*)pRecord, nRecords*sizeof(OCM3_GMOSNRRecord_t));

	return Result;
};

// Most basic command "NOP" - No Operation.
// Demonstrates how the handshake using SEQNUM1 should work.
OCM_Error_t FinisarHROCM_V3::cmdNOP(OCM3_Response_t &Head)
{
    return cmdSimple(OPCODE_NOP);
}

// Send Clear-Error command (CLE)
OCM_Error_t FinisarHROCM_V3::cmdCLE()
{
    return cmdSimple(OPCODE_CLE);
}

// Send Reset command (RES)
OCM_Error_t FinisarHROCM_V3::cmdRES()
{
    int RetrySave = setTimeout(OCM_LONGTIMEOUT); // Set long timeout
    OCM_Error_t Result = cmdSimple(OPCODE_RES);
    setTimeout(RetrySave);
    return Result;
}

// Send Module Identification command (MID)
OCM_Error_t FinisarHROCM_V3::cmdMID(const char *MID)
{
    // Construct the command package
    std::vector<char> Command;
    std::vector<char> Response;
    Command.resize(sizeof(OCM3_cmd_t)+strlen(MID)+4);
    Response.resize(Command.size());
    char* pData = (char*) &Command[sizeof(OCM3_cmd_t)];
    strcpy(pData,MID);
    fillInHROCMCommand(&Command[0], (unsigned int)strlen(MID), OPCODE_MID , _seqnum++);

    // Send out the command
    OCM_Error_t Result = spiTransfer(Command,Response);

    // Wait until it's accepted using SEQNUM1
    Result = Result || waitForSuccess(_seqnum-1);

    return Result;
}

// Get Device Information (DEV?)
OCM_Error_t FinisarHROCM_V3::cmdGETDEV(OCM3_Response_t &Head,OCM3_RDataDEV_t &RDataDev)
{
	OCM_Error_t Result = cmdSimple(OPCODE_GETDEV);

	std::vector<char> RDATA;
	Result = Result || cmdPollLong(Head, RDATA, _seqnum - 1);

	if (Result == OCM_OK && Head.OPCODE == OPCODE_GETDEV && RDATA.size()==sizeof(RDataDev)) {
		memcpy(&RDataDev, &RDATA[0], sizeof(RDataDev));
	}
	else if (Result == OCM_OK) {
		LOGERROR("Response error: OPCODE=" << Head.OPCODE << " RDATA.size=" << RDATA.size());
		Result = Result || OCM_FAILED;
	}

	return Result;
}


// Send Trigger-And-Process command (TPC)
//OCM_Error_t FinisarHROCM_V3::cmdTPC(std::vector<OCM3_GMPWRecord_t> &TPCVector,bool DCPW)
//{
//    OCM3_Response_t Head;
//    OCM3_RData_t RData;
//    unsigned int TxSeqNum;
//    return cmdTPC(Head,RData,TPCVector,TxSeqNum,DCPW);
//}

// Send Trigger-And-Process command (TPC) and wait for the results
OCM_Error_t FinisarHROCM_V3::runFullScan(OCM3_TPCProcessMask_t TaskVector)
{
	OCM3_Response_t Head;
	unsigned int TxSeqNum = 0;

    // Send TPC command
    OCM_Error_t Result = cmdTPC(Head,TxSeqNum, TaskVector);

    // Wait until it's accepted using SEQNUM and pick up the whole response
	if ((TaskVector& OCM3_TASK_PW_MASK) != 0) {
		Result = Result || cmdQueryTPC_PW(_lastGMPWResult, TxSeqNum);
	}

	// Wait until it's accepted using SEQNUM and pick up the whole response
	if ((TaskVector& OCM3_TASK_OSNR_MASK) != 0) {
		Result = Result || cmdQueryTPC_OSNR(_lastGMOSNRResult, TxSeqNum);
	}

    return Result;
}

// Send Trigger-And-Process command (TPC)
OCM_Error_t FinisarHROCM_V3::cmdTPC(OCM3_Response_t &Head,unsigned int &TxSeqNum, OCM3_TPCProcessMask_t TaskVector)
{
    // Construct the command package
    std::vector<char> Command;
    std::vector<char> Response;
    Command.resize(sizeof(OCM3_cmd_t)+8);
    Response.resize(Command.size());
	OCM3_TPCProcessMask_t* pTaskVector = (OCM3_TPCProcessMask_t*) &Command[sizeof(OCM3_cmd_t)];
    *pTaskVector = TaskVector;
    TxSeqNum = _seqnum++;
    fillInHROCMCommand(&Command[0], 4, OPCODE_TPC , TxSeqNum);

    OCM_Error_t Result = OCM_OK;
    for(int k=0;Result==OCM_OK;++k)
    {
        if (k>_nretry)
        {
            Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
        }

        // Send out the command
        Result = Result || spiTransfer(Command,Response);

        // Check the response
        bool Retransmit = false;
        Result = Result || waitCommandAccepted(Head,TxSeqNum,Retransmit);

        // We need to retransmit the command
        if (Retransmit)
        {
            ++_nCmdRetransmit;
			LOGWARNING("Retransmitting TPC command");
            continue;
        }

        break;
    }

    return Result;
}

// Set Measurement Plan Power (MPPW)
OCM_Error_t FinisarHROCM_V3::cmdSETMPPW(std::vector<OCM3_MPPWRecord_t> &MPPWVector)
{
	if (MPPWVector.size() == 0) {
		LOGERROR("MPPWVector empty");
		return OCM_FAILED;
	}

    // Construct the command package
    std::vector<char> Command;
    std::vector<char> Response;
    Command.resize(sizeof(OCM3_cmd_t)+MPPWVector.size()*sizeof(OCM3_MPPWRecord_t)+4);
    Response.resize(Command.size());
    OCM3_MPPWRecord_t* pData = (OCM3_MPPWRecord_t*) &Command[sizeof(OCM3_cmd_t)];
    memcpy(pData,&MPPWVector[0],MPPWVector.size()*sizeof(OCM3_MPPWRecord_t));

	unsigned int TxSeqNum = _seqnum++;
    fillInHROCMCommand(&Command[0], (unsigned int)(MPPWVector.size()*sizeof(OCM3_MPPWRecord_t)), OPCODE_SETMPPW, TxSeqNum);

	OCM_Error_t Result = OCM_OK;
	for (int k = 0; Result == OCM_OK; ++k)
	{
		if (k>_nretry)
		{
			Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
		}

		// Send out the command
		Result = Result || spiTransfer(Command, Response);

		// Check the response
		bool Retransmit = false;
		OCM3_Response_t	Head;
		Result = Result || waitCommandAccepted(Head, TxSeqNum, Retransmit);

		// We need to retransmit the command
		if (Retransmit)
		{
			++_nCmdRetransmit;
			LOGWARNING("Retransmitting MPPW command");
			continue;
		}

		break;
	}

    return Result;
}

// Set Measurement Plan OSNR (MPOSNR)
OCM_Error_t FinisarHROCM_V3::cmdSETMPOSNR(std::vector<OCM3_MPOSNRRecord_t> &MPOSNRVector)
{
	if (MPOSNRVector.size() == 0) {
		LOGERROR("MPOSNRVector empty");
		return OCM_FAILED;
	}

	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + MPOSNRVector.size()*sizeof(OCM3_MPOSNRRecord_t) + 4);
	Response.resize(Command.size());
	OCM3_MPOSNRRecord_t* pData = (OCM3_MPOSNRRecord_t*)&Command[sizeof(OCM3_cmd_t)];
	memcpy(pData, &MPOSNRVector[0], MPOSNRVector.size()*sizeof(OCM3_MPOSNRRecord_t));

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], (unsigned int)(MPOSNRVector.size()*sizeof(OCM3_MPOSNRRecord_t)), OPCODE_SETMPOSNR, TxSeqNum);

	OCM_Error_t Result = OCM_OK;
	for (int k = 0; Result == OCM_OK; ++k)
	{
		if (k>_nretry)
		{
			Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
		}

		// Send out the command
		Result = Result || spiTransfer(Command, Response);

		// Check the response
		bool Retransmit = false;
		OCM3_Response_t	Head;
		Result = Result || waitCommandAccepted(Head, TxSeqNum, Retransmit);

		// We need to retransmit the command
		if (Retransmit)
		{
			++_nCmdRetransmit;
			LOGWARNING("Retransmitting MPOSNR command");
			continue;
		}

		break;
	}

	return Result;
}

// Firmware transfer (FWT), large files are allowed
OCM_Error_t FinisarHROCM_V3::cmdFWT(std::vector<char> &BinaryFile)
{
    OCM_Error_t Result      = OCM_OK;
    char *p                 = &BinaryFile[0];
    size_t RemainingBytes   = BinaryFile.size();
    unsigned int Offset     = 0;
    size_t LengthMax        = OCM3_LENMAX - sizeof(OCM3_cmd_t) - 8;

    while(RemainingBytes>0 && Result==OCM_OK)
    {
        size_t sizeChunk = RemainingBytes>LengthMax ? LengthMax : RemainingBytes;
        Result = Result || cmdFWT(Offset,p,sizeChunk);
        p += sizeChunk;
        Offset += (unsigned int)sizeChunk;
        RemainingBytes-=sizeChunk;
        printf(".");
    }
    printf("\n");

    return Result;
}

// Firmware transfer (FWT), transfer a chunk of maximum length of OCM3_LENMAX
OCM_Error_t FinisarHROCM_V3::cmdFWT(unsigned int Offset,char *Buffer,size_t BufSiz)
{
    // Construct the command package
    std::vector<char> Command;
    std::vector<char> Response;
    Command.resize(sizeof(OCM3_cmd_t)+BufSiz+8);
    Response.resize(Command.size());

    unsigned int* pOffset = (unsigned int*) &Command[sizeof(OCM3_cmd_t)];
    *pOffset = Offset;

    char* pData = (char*) &Command[sizeof(OCM3_cmd_t)+4];
    memcpy(pData,Buffer,BufSiz);

	unsigned int TxSeqNum = _seqnum++;
    fillInHROCMCommand(&Command[0], (unsigned int)(BufSiz+4), OPCODE_FWT , TxSeqNum);

    // Send out the command
    OCM_Error_t Result = spiTransfer(Command,Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

    return Result;
}

// Firmware save (FWS)
OCM_Error_t FinisarHROCM_V3::cmdFWS()
{
    int RetrySave = setTimeout(OCM_LONGTIMEOUT); // Set long timeout
    OCM_Error_t Result = cmdSimple(OPCODE_FWS);
    setTimeout(RetrySave);
    return Result;
}

// Firmware save (FWE)
OCM_Error_t FinisarHROCM_V3::cmdFWE()
{
    int RetrySave = setTimeout(OCM_LONGTIMEOUT); // Set long timeout
    OCM_Error_t Result = cmdSimple(OPCODE_FWE);
    setTimeout(RetrySave);
    return Result;
}

// Set averaging (ATS AVG)
OCM_Error_t FinisarHROCM_V3::cmdSETAVG(unsigned short nAverage)
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 8 ); // ATTR + VAL + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];
	unsigned short* pVAL = pATTR+1;

	*pATTR = 0;  // <ATTR>
	*pVAL = (unsigned short) nAverage; // <VAL>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 4 , OPCODE_ATS, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	return Result;
}

// Get averaging (Get Active AVG)
OCM_Error_t FinisarHROCM_V3::cmdGETAVG(unsigned short &nAverage)
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 6); // ATTR + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];

	*pATTR = 1;  // <ATTR>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 2, OPCODE_ATG, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	std::vector<char> RDATA;
	OCM3_Response_t	Head;
	Result = Result || cmdPollLong(Head, RDATA, _seqnum - 1);

	if (Result == OCM_OK && Head.OPCODE == OPCODE_ATG && RDATA.size() == sizeof(unsigned short)) {
		nAverage = *(unsigned short*)&RDATA[0];
	}
	else if (Result == OCM_OK) {
		LOGERROR("Response error: OPCODE=" << Head.OPCODE << " RDATA.size=" << RDATA.size());
		Result = Result || OCM_FAILED;
	}

	return Result;
}

// Reset number of averages to factory settings (ATC AVG)
OCM_Error_t FinisarHROCM_V3::cmdCLRAVG()
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 6); // ATTR + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];

	*pATTR = 0;  // <ATTR>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 2, OPCODE_ATC, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	return Result;
}

// Set bandwidth mode (ATS BWXB)
OCM_Error_t FinisarHROCM_V3::cmdSETBWXB(int mode)
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 8); // ATTR + VAL + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];
	unsigned short* pVAL = (unsigned short*) (pATTR + 1);

	*pATTR = 2;  // <ATTR>
	*pVAL = (unsigned short)mode; // <VAL>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 4, OPCODE_ATS, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	return Result;
}

// Get bandwidth mode (Get Active BWXB)
OCM_Error_t FinisarHROCM_V3::cmdGETBWXB(int &mode)
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 6); // ATTR + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];

	*pATTR = 3; // <ATTR>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 2, OPCODE_ATG, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	std::vector<char> RDATA;
	OCM3_Response_t	Head;
	Result = Result || cmdPollLong(Head, RDATA, _seqnum - 1);

	if (Result == OCM_OK && Head.OPCODE == OPCODE_ATG && RDATA.size() == sizeof(unsigned short)) {
		mode = RDATA[0];
	}
	else if (Result == OCM_OK) {
		LOGERROR("Response error: OPCODE=" << Head.OPCODE << " RDATA.size=" << RDATA.size());
		Result = Result || OCM_FAILED;
	}

	return Result;
}

// Reset bandwidth mode to factory settings (ATC BWXB)
OCM_Error_t FinisarHROCM_V3::cmdCLRBWXB()
{
	// Construct the command package
	std::vector<char> Command;
	std::vector<char> Response;
	Command.resize(sizeof(OCM3_cmd_t) + 6); // ATTR + CRC
	Response.resize(Command.size());

	unsigned short* pATTR = (unsigned short*)&Command[sizeof(OCM3_cmd_t)];

	*pATTR = 2;  // <ATTR>

	unsigned int TxSeqNum = _seqnum++;
	fillInHROCMCommand(&Command[0], 2, OPCODE_ATC, TxSeqNum);

	// Send out the command
	OCM_Error_t Result = spiTransfer(Command, Response);

	// Check the response (we don't do retransmits)
	Result = Result || waitForSuccess(TxSeqNum);

	return Result;
}

// Execute simple command without payload
OCM_Error_t FinisarHROCM_V3::cmdSimple(int OpCode)
{
    OCM3_cmd_t Command;
    OCM3_cmd_t Response;

    fillInHROCMCommand((char*)&Command, 0, OpCode , _seqnum++);

    // Send out the command
    OCM_Error_t Result = spiTransfer((char*)&Command,(char*)&Response,sizeof(OCM3_cmd_t));

    // Wait until it's accepted using SEQNUM1, retry if COMRES<0
    Result = Result || waitForSuccess(_seqnum-1);

    return Result;
}

// Polls response package until the desired sequence number comes along
// Polls until SEQNO are found and COMRES>=0
OCM_Error_t FinisarHROCM_V3::waitForReply(OCM3_Response_t &Head,unsigned int seqnum)
{
    OCM_Error_t Result = OCM_OK;

    for(int k=0;Result==OCM_OK;++k)
    {
        if (k>_nretry)
        {
            Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
        }

        Result = Result || cmdPollShort(Head);

        if (Head.SEQNO==seqnum && Head.COMRES>=0) // Sequence number found and COMRES not pending
            break;
    }

    return Result;
}

// Wait until command finished (COMRES==0)
OCM_Error_t FinisarHROCM_V3::waitForSuccess(unsigned int seqnum)
{
    OCM_Error_t Result=OCM_OK;

	OCM3_Response_t Head;
	Result = Result || waitForReply(Head, seqnum);

    // Make sure COMRES is 0
    if (Result==OCM_OK && Head.COMRES!=0)
    {
		Result = Result || OCM_FAILED;
		LOGERROR("COMRES=" << Head.COMRES);
    }

    return Result;
}

OCM_Error_t FinisarHROCM_V3::waitCommandAccepted(OCM3_Response_t &Head,unsigned int seqnum,bool &Retransmit)
{
    OCM_Error_t Result = OCM_OK;
    Retransmit = false;

    for(int k=0;Result==OCM_OK;++k)
    {
        if (k>_nretry) {
            Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
        }

        Result = Result || cmdPollShort(Head);

		if (Head.COMRES >= 0) { // COMRES not pending
			break;
		}
    }

    if (Head.COMRES>0) // An error occurred
    {
		LOGERROR("COMRES=" << Head.COMRES);
		Result = Result || OCM_FAILED;
    }

    if (Result==OCM_OK && Head.SEQNO!=seqnum) // Sequence number SEQNO1 is wrong
    {
        Retransmit=true;
    }

    return Result;
}

OCM_Error_t FinisarHROCM_V3::isTaskComplete(OCM3_Response_t &Head, int iSEQARR, unsigned int TxSeqNum, bool &taskCompleted)
{
	OCM_Error_t Result = OCM_OK;
	taskCompleted = false;

	Result = Result || cmdPollShort(Head);

	if (Head.SEQARR[iSEQARR] == TxSeqNum) {
			taskCompleted = true;
	}

	return Result;
}

OCM_Error_t FinisarHROCM_V3::waitTaskComplete(OCM3_Response_t &Head, int iSEQARR, unsigned int TxSeqNum)
{
	OCM_Error_t Result = OCM_OK;
	bool taskCompleted = false;

	for (int k = 0; Result == OCM_OK; ++k)
	{
		if (k>_nretry)
		{
			Result = Result || OCM_FAILED; // Timeout (time is approximately _nretry*_recover_ms
			LOGERROR("Timeout");
		}

		Result = Result || isTaskComplete(Head, iSEQARR, TxSeqNum, taskCompleted);

		if (taskCompleted) {
			break;
		}
	}

	return Result;
}


int FinisarHROCM_V3::setTimeout(int ms)
{
    int RetrySave = _nretry;
    _nretry = ms/_recover_ms;
    return RetrySave;
}

OCM_Error_t FinisarHROCM_V3::runPostProcessing()
{
	OCM_Error_t Result = OCM_OK;

	// Work out how many high-resolution channels there are. A high-resolution channel is exactly
	// one slice wide. I.e. start/stop slice are the same.
	int hiResSection = -1;
	for (unsigned int i = 0; i < _lastGMPWResult.GMPWVector.size(); ++i) {
		if (_lastGMPWResult.GMPWVector[i].SLICESTART == _lastGMPWResult.GMPWVector[i].SLICEEND) {
			hiResSection = (int)(i + 1);
		}
		else {
			break;
		}
	}

	// If there is a hiRes section, perform the post-processing
	// To emulate the post-processing behaviour, the first channel plan has to be a high-resolution section with channels being exactly one slice wide.
	// The wider channels are following after. To run the post processing on each wide channel, the data of the high-resolution section is used.
	if (hiResSection > 1 && _lastGMOSNRResult.GMOSNRVector.size()>0) {
		int hiResFirstSlice = _lastGMPWResult.GMPWVector[0].SLICESTART;
		//int hiResLastSlice = _lastGMPWResult.GMPWVector[hiResSection - 1].SLICEEND;

		// Result vectors do not include the hiRes section
		_lastPeakPower.assign(_lastGMOSNRResult.GMOSNRVector.size(), OCM3_PMIN_CLIP);

		// Now loop through the wide channels
		for (int iChannel = 0; iChannel<(int)_lastGMOSNRResult.GMOSNRVector.size() && Result == OCM_OK; ++iChannel) {
			// Calculate peak power from center frequency. 
			int peakSlice = _lastGMOSNRResult.GMOSNRVector[iChannel].CENTERFREQUENCY;
			_lastPeakPower[iChannel] = (peakSlice - hiResFirstSlice) >= 0 && peakSlice<hiResSection ? _lastGMPWResult.GMPWVector[peakSlice- hiResFirstSlice].POWER / OCM3_PSCALE : OCM3_PMIN_CLIP;
		}
	}
	else
	{
		_lastPeakPower.clear();
	}

	return Result;
}
