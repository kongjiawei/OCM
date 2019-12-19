//
// Copyright (C) 2016 Finisar Pty. Ltd.
//
// Demo command line tool to communicate with the High-Resolution Optical Channel Monitor
//
// Uses Diolan SPI adapter and eval kit to interface to a PC.
//
// Type "HROCMQueryV3 list" to verify the Diolan adapter is working. The output should be like
//
// Available adapters:
// 00000000
//
// Type "HROCMQueryV3 dev" to query device information.
//
// Type "HROCMQueryV3 itu 191.4 0.05 80" to set a channel plan of 80 channels on a 50GHz grid starting at 191.4THz.
//
// Type "HROCMQueryV3 scan" to run a single scan.
//

/*! \page HROCMQueryV3 HROCMQueryV3.exe - Command Line Tool (Protocol Revision 3)
\tableofcontents
HROCMQueryV3.exe is a command line tool to communicate with the High-Resolution Optical Channel Monitor. It uses
a USB-SPI interface supplied with the eval kit. This tool uses the SPI protocol V3.

To use it:
- Open a Command Prompt
- Make sure the SPI interface is plugged into the USB port and recognized by the
operating system (you should see a device named "DLN-4 SPI Master Adapter" in the device manager). 
- Also, make sure that the HROCM is powered up.

Now type:
@code
HROCMQueryV3 dev
SPI_Header:
SPIMAGIC,f0e1c387
LENGTH,170
SEQNO,1869
OPCODE,a
COMRES,0
CRC1,20a3c1b8
OSS,1
HSS,0
LSS,0
CSS,454
ISS,468
PPEND,0
SEQARR,18854,0,0,19132,0
RDATA_Content:
HWR,1040000
FWR,1040100
SNO,HM999999
MFD,21SEP2016
LBL,FOCM01FXC1AN-CN
MID,Finisar HR OCM
Pmax,1
Smax,15440
SLW,3125
Nmax,17000
FSF,1913125000
BWXB,T
CAP,31
@endcode

This command is dumping the header of the SPI register file.

Run a scan and save result into a CSV file:
@code
HROCMQueryV3 hires
[OK] MPPW command executed (15599 channels)
HROCMQueryV3 scan>test.csv
@endcode

The first command programs a channel plan with the highest possible resolution. The second command writes the result on stdout and redirects it into a file.

<hr>

\section hqsec Command Reference

\subsection hqsec1 scan
This command executes a scan and dumps the result on stdout in CSV format. The columns are:
- Port: Always 1
- fCenter_THz: Center frequency of the channel in THz
- Power_dBm: Channel power in dBm

Example:
@code
HROCMQueryV3 scan
Port,fCenter_THz,Power_dBm
1,191.2501563,-68.7
1,191.2504688,-68.7
1,191.2507812,-68.8
1,191.2510937,-68.8
1,191.2514063,-68.7
1,191.2517188,-68.7
1,191.2520313,-68.7
1,191.2523437,-68.7
1,191.2526563,-68.5
1,191.2529688,-68.4
1,191.2532812,-68.4
1,191.2535937,-68.5
1,191.2539063,-68.8
1,191.2542188,-68.9
1,191.2545313,-69.2
.
.
.
@endcode

You can use i/O redirection to write the result to a file:
@code
HROCMQueryV3 scan>test.csv
@endcode

\subsection hqsec1a scanosnr
This command executes an OSNR scan and dumps the result on stdout in CSV format. The columns are:
- Port: Always 1
- fCenter_THz: Center frequency of the channel in THz
- Power_dBm: Channel power in dBm
- OSNR_dBm: OSNR in dBm

Example:
@code
HROCMQueryV3 scanosnr
Port,fCenter_THz,Power_dBm
Port,fCenter_THz,Power_dBm,OS
1,191.3996875,-40.4,-17.1
1,191.4490625,-40.5,-5.4
1,191.5006250,-41.0,-2.5
1,191.5509375,-41.2,-1.4
1,191.6015625,-41.0,-4.8
1,191.6496875,-40.3,-6.8
1,191.7000000,-39.3,-8.9
1,191.7481250,-39.7,-2.0
1,191.7990625,-41.4,-2.7
1,191.8500000,-42.6,-3276.8
1,191.9018750,-42.1,-3276.8
.
.
.
@endcode

You can use i/O redirection to write the result to a file:
@code
HROCMQueryV3 scanosnr>test.csv
@endcode

Use the ITU command to set up a channel plan.

\subsection hqsec2 scanraw
This command executes a scan and dumps the result on stdout in CSV format. As opposed to the scan command, the result is given in terms of slice numbers.
Note that slice numbers are 1-based. The columns are:
- Port: Always 1
- SliceStart: First slice number of the channel
- SliceEnd: Last slice number of the channel
- Power_dBm: Channel power in dBm

Example:
@code
HROCMQueryV3 scanraw
Port,SliceStart,SliceEnd,Power_dBm
1,1,1,-69.1
1,2,2,-69.1
1,3,3,-69.0
1,4,4,-69.0
1,5,5,-69.0
1,6,6,-69.0
1,7,7,-69.1
1,8,8,-69.2
1,9,9,-69.3
1,10,10,-69.4
.
.
.
@endcode

\subsection hqsec3 hires
This command writes a high-resolution channel plan into the module. Each channel is exactly one slice wide. Depending on the frequency range of the module,
this will correspond to approximately 15600 channels. 

Use the scan command to query the measured channel power values. The returned power values are absolute power levels corresponding to the power within a slice.
since the slice width is 312.5 MHz, this can be interpreted as a power density. The unit would then be dBm/312.5MHz. 

If desired, the host can integrate over those values by simply adding the channel powers (note that you need to convert them to mW first). 
E.g. adding 40 neighboring values would yield the total power within a 12.5GHz range.

Example:
@code
HROCMQueryV3 hires
[OK] MPPW command executed (15599 channels)
@endcode

\subsection hqsec4 itu {fStart} {fStep} {nChannels}
This command writes a channel plan with equidistant channels.
- {fStart} denotes the first channel center frequency in THz
- {fStep} denotes the step size in THz
- {nChannels} denotes the total number of channels

Example:
@code
HROCMQueryV3 itu 191.4 0.05 80
[OK] MPPW command executed (80 channels)
@endcode

\subsection hqsec5 mppw
This command reads a channel plan given in CSV format from stdin and sends it to the module. You can use I/O redirection to read data from a CSV file.

The file needs to provide 3 columns without header.

- Column 1, Port: Should be always 1
- Column 2, First Slice: First slice number of the channel
- Column 3, Last Slice: Last slice number of the channel

Example:
@code
HROCMQueryV3 mppw<test.csv
[OK] MPPW command executed (80 channels)
@endcode

\subsection hqsec6 dump
This command dumps the full SPI register file including the scan result on stdout. When dumped into a CSV file, it can be opened in Excel.

Example:
@code
HROCMQueryV3 dump
SPI_Header:
SPIMAGIC,f0e1c387
LENGTH,170
SEQNO,1869
OPCODE,a
COMRES,0
CRC1,20a3c1b8
OSS,1
HSS,0
LSS,0
CSS,450
ISS,468
PPEND,0
SEQARR,18854,0,0,19132,0
RDATA_Content:
HWR,1040000
FWR,1040100
SNO,HM999999
MFD,21SEP2016
LBL,FOCM01FXC1AN-CN
MID,Finisar HR OCM
Pmax,1
Smax,15440
SLW,3125
Nmax,17000
FSF,1913125000
BWXB,T
CAP,31
@endcode

\subsection hqsec7 dumpshort
This command dumps the SPI register file without the scan data.

Example:
@code
HROCMQueryV3 dumpshort
SPI_Header:
SPIMAGIC,f0e1d2c3
LENGTH,301
SEQNO1,20770
OPCODE1,6
COMRES,0
CRC1,1a8b0e9d
HWR,1030000
FWR,1030000
SNO,HM000277
MFD,14DEC2015
LBL,0000000_000
MID,Finisar HR OCM
OSS,1
HSS,0
LSS,0
CSS,422
ISS,471
Pmax,1
Smax,15600
SLW,3125
Nmax,17000
FSF,1912500000
PPEND,0
@endcode

\subsection hqsec8 res
Resets the module.

Example:
@code
HROCMQueryV3 res
[OK] RES command executed
@endcode

\subsection hqsec9 cle
Clears pending error bits.

Example:
@code
HROCMQueryV3 cle
[OK] CLE command executed
@endcode

\subsection hqsec10 update {filename}
Updates the firmware on the module using FWT and FWS commands. {filename} references a valid
.wf file. Note that the command does not reset the module. In some cases, power cycling may be
necessary.

Example:
@code
HROCMQueryV3 update 1247187_A00-01_03_00.wf
[OK] File loaded (1247187_A00-01_03_00.wf, 5222300 bytes)
........................................
[OK] FWT executed
[OK] FWS command executed
[INFO] Run RES command to restart the module
@endcode

\subsection hqsec11 fwt {filename}
Transfers a firmware file using the FWT command. The firmware is not written to NV memory. {filename} references a valid
.wf file.

Example:
@code
HROCMQueryV3 fwt 1247187_A00-01_03_00.wf
[OK] File loaded (1247187_A00-01_03_00.wf, 5222300 bytes)
........................................
[OK] FWT executed
@endcode

\subsection hqsec12 fws
Saves the previously transferred firmware to NV memory using the FWS command.

Example:
@code
HROCMQueryV3 fws
[OK] FWS command executed
@endcode

\subsection hqsec13 mid {label}
Sets the user-definable label and saves it to NV memory. {label} can be any any string of up to 32 characters length.

Example:
@code
HROCMQueryV3 mid myocm
[OK] MID command executed (myocm)
@endcode

\subsection hqsec14 list
Lists all connected SPI adapters. The returned values are unique identifiers used to address a specific SPI adapter. Note that all
adapters have the same unique identifier 0 when shipped. To address more than one adapter simultaneously, use the setid command
to change the uniqe identifiers of your SPI adapters.

Example:
@code
HROCMQueryV3 list
Available adapters:
dln0000002A
dln000856F2
@endcode

\subsection hqsec15 setid {id}
Writes a new unique identifier into the SPI adapter. All adapters are shipped with the unique identifier set to 0. 
To address more than one adapter, it is required to change those identifiers manually to make sure all connected adapters
have different unique identifiers. To change the identifier of an SPI adapter, connect only a single adapter to the PC. Then
use the setid command to modify the identifier of the connected SPI adapter.

Example:
@code
HROCMQueryV3 setid dln00001234
Setting adapter ID from dln00000000 to dln00001234
@endcode

\subsection hqsec16 hammer {nRuns}
Hammer runs many scans in a sequence. If {nRuns} is omitted, it will run infinitely until the user presses a key.

Example:
@code
HROCMQueryV3 hammer 10
[INFO] Press any key to stop
[INFO] Scan=0 t=0.00h tScan=0ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=1 t=0.00h tScan=1201ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=2 t=0.00h tScan=905ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=3 t=0.00h tScan=811ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=4 t=0.00h tScan=761ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=5 t=0.00h tScan=733ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=6 t=0.00h tScan=712ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=7 t=0.00h tScan=698ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=8 t=0.00h tScan=688ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=9 t=0.00h tScan=679ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
@endcode

\subsection hqsec16d avg {nAverages}
Sets the number of averages per scan. Note that this
command just writes the configuration into non-volatile memory. In order for the change to take effect, a RES command needs to be issued.

Example:
@code
HROCMQueryV3 avg 8
[INFO] Run RES command to register changes
@endcode

\subsection hqsec16e bws
Configure the OSNR measurement to use a constant number of slices relative to the center frequency for determining the signal power. Note that this
command just writes the configuration into non-volatile memory. In order for the change to take effect, a RES command needs to be issued.

Example:
@code
HROCMQueryV3 bwt
[INFO] Run RES command to register changes
@endcode

\subsection hqsec16f bwt
Configure the OSNR measurement to use a threshold relative to the peak power for determining the signal power. Note that this
command just writes the configuration into non-volatile memory. In order for the change to take effect, a RES command needs to be issued.

Example:
@code
HROCMQueryV3 bwt
[INFO] Run RES command to register changes
@endcode

\subsection hqsec16g factory
Resets attributes such as the number of averages or the OSNR bandwidth mode to the factory settings. Note that this command issues a reset to registe the changes.

Example:
@code
HROCMQueryV3 factory
@endcode

\section hqsecb Command Line Flags

\subsection hqsec16a -log
Creates a log file in CSV format (HROCMQuery.csv). The log file contains a line
for each command sent to the module.

Example:
@code
HROCMQueryV3 -log hammer 2
[INFO] Press any key to stop
[INFO] Scan=0 t=0.00h tScan=0ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=1 t=0.00h tScan=1201ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
@endcode

\subsection hqsec16b -logbin
Creates a binary log file HROCMQuery.bin. The log file contains all low-level
SPI communication. The software comes with a simple protocol analyzer written in LabView to analyze the binary file.

Example:
@code
HROCMQueryV3 -logbin hammer 2
[INFO] Press any key to stop
[INFO] Scan=0 t=0.00h tScan=0ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=1 t=0.00h tScan=1201ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
@endcode

The file is organized in a list of records. Each record has the following format (little endian):
@code
4 Bytes: Magic Number 0xBEEFBEEF
4 Bytes: System Tick Counter in Milliseconds
4 Bytes: SPI Result (0=OK, 1=Error)
4 Bytes: Block Size n
n Bytes: Tx Data (MOSI)
n Bytes: Rx Data (MISO)
@endcode

\subsection hqsec17 -2, -4, -12
Sets the SPI clock rate. The default rate is 12 MHz. The command line switches -2 and -4 allow
to reduce the SPI clock rate to 2 MHz and 4 MHz respectively.

Example:
@code
HROCMQueryV3 -2 -log hammer 2
[INFO] Press any key to stop
[INFO] Scan=0 t=0.00h tScan=0ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
[INFO] Scan=1 t=0.00h tScan=1201ms nCRC1=0 nCRC2=0 nCmdRetransmit=0
@endcode

\subsection hqsec18 -id {id}
specifies unique identifier of the SPI adapter. Use the command "HROCMQueryV3 list" to dump the unique identifiers of all connected SPI adapters.

Example:
@code
HROCMQueryV3 list
Available adapters:
0000002A
000856F2
HROCMQueryV3 -id 856F2 dumpshort
.
.
.
@endcode

\subsection hqsec19 -osnr {SearchMin},{SearchMax},{Threshold},{bw},{TagRange},{rbw}
specifies settings for the OSNR evaluation. These parameters are used by the itu command for creating the channel plans.

- {SearchMin} denotes the minimum noise tag search range in THz
- {SearchMax} denotes the maximum noise tag search range in THz
- {Threshold} denotes the threshold for the signal power calculation in dB. Only evaluated if BWXB is set to 'T'. Otherwise ignored.
- {bw} denotes the fixed bandwidth used for the signal power calculation in THz. Only evaluated if BWXB is set to 'S'. Otherwise ignored.
- {TagRange} denotes the width of each noise tag in THz.
- {rbw} denotes the resolution bandwidth for OSNR calculation in THz.

Example:
@code
HROCMQueryV3 -osnr 0.01 0.025 3 0.01 0.01 0.0125 itu 191.4 0.05 80
[OK] SETMPPW command executed (80 channels)
[OK] SETMPOSNR command executed (80 channels)
@endcode

*/
#include<winsock2.h>
#include "stdafx.h"
#include<stdio.h>

#include "FinisarHROCM_V3.h"
#include "SPIAdapter.h"

#pragma comment(lib,"ws2_32.lib")

#define THEVERSION "2.3.0.7"

FILE				*theLogFile = NULL;						// File handle of log file
FILE				*theLogBinFile = NULL;					// File handle of binary log file
std::string			theConfigString;						// Configuration string for class factory
std::ostringstream	theLastError;							// Accumulated error messages

double				theOsnrSearchMinTHz = 0.010;
double				theOsnrSearchMaxTHz = 0.025;
double				theOsnrThresDb = 3.0;
double				theOsnrThresTHz = 0.01;
double				theOsnrTagRangeTHz = 0.010;
double				theOsnrRbwTHz = 0.0125;

#define LOGERROR(OCM) {std::string tempError;OCM.get(OCM_KEY_LASTERROR, tempError);theLastError<<tempError;}

// Help text
int commandHelp()
{
    printf("HROCM Command Line Utility Rev.%s\n",THEVERSION);
    printf("Usage: HROCMQueryV3 cmd [Param1] [Param2] ...\n");
    printf("Examples:\n");
	printf("  HROCMQueryV3 scan                   Run scan and output data to stdout\n");
	printf("  HROCMQueryV3 scanosnr               Run osnr scan and output data to stdout\n");
	printf("                                      Use itu command to prepare channel plan\n");
	printf("  HROCMQueryV3 scan>scan.csv          Run scan and write result to CSV file\n");
    printf("  HROCMQueryV3 scanraw                Run scan and write result using slice\n");
    printf("                                      numbers\n");
    printf("  HROCMQueryV3 hires                  Write highest-resolution channel plan\n");
    printf("  HROCMQueryV3 itu 191.4 0.05 80      80 channels on 50GHz grid starting\n");
    printf("                                      at 191.4THz\n");
    printf("  HROCMQueryV3 mppw<plan.csv          Load channel plan from file\n");
	printf("  HROCMQueryV3 dev                    Query device information\n");
	printf("  HROCMQueryV3 dump                   Poll complete SPI register file\n");
    printf("  HROCMQueryV3 dumpshort              Poll SPI register header\n");
    printf("  HROCMQueryV3 res                    Reset module\n");
    printf("  HROCMQueryV3 cle                    Clear errors\n");
    printf("  HROCMQueryV3 update HWR-01_01_00.wf Update firmware (fwt+fws)\n");
    printf("  HROCMQueryV3 fwt HWR-01_01_00.wf    Transfer firmware\n");
    printf("  HROCMQueryV3 fws                    Save firmware\n");
	printf("  HROCMQueryV3 fwe                    Execute firmware\n");
	printf("  HROCMQueryV3 avg 8                  Set averaging per scan (permanent)\n");
	printf("  HROCMQueryV3 bws                    Set BWXB to 'fixed slices' (permanent)\n");
	printf("  HROCMQueryV3 bwt                    Set BWXB to 'threshold' (permanent)\n");
	printf("  HROCMQueryV3 factory                Reset attributes to factory defaults\n");
	printf("  HROCMQueryV3 mid myocm              Set module identification\n");
	printf("  HROCMQueryV3 list                   List SPI adapters IDs\n");
	printf("  HROCMQueryV3 setid dln00001234      Set SPI adapter ID to dln00001234\n");
	printf("  HROCMQueryV3 loopback               Run SPI loopback test\n");
	printf("  HROCMQueryV3 hammer                 Stress test - run scans until key pressed\n");
	printf("  HROCMQueryV3 -id 12DE dumpshort     Talk to a specific SPI adapter\n");
	printf("  HROCMQueryV3 -log hammer 30         Stress test - run 30 scans\n");
	printf("                                      Logging turned on\n");
	printf("  HROCMQueryV3 -logbin hammer 30      Stress test - run 30 scans\n");
	printf("                                      Binary logging turned on\n");
	printf("  HROCMQueryV3 -log -2 hammer 30      Stress test - run 30 scans,SPICLK = 2MHz\n");
	printf("                                      -2 -4 -20 -25 -12 are allowed clock rates\n");
	printf("  HROCMQueryV3 -osnr 0.01 0.025 3 0.01 0.01 0.0125 itu 191.4 0.05 80\n");
	printf("                                      Use non-default OSNR settings:\n");
	printf("                                      SearchMin [THz], SearchMax [THz],\n");
	printf("                                      Threshold [dB], (ignored if BWXB is 'S')\n");
	printf("                                      Threshold [THz], (ignored if BWXB is 'T')\n");
	printf("                                      TagRange [THz], RBW [THz]\n");

    return 0;
}

// List available SPI adapters
int commandListAdapters()
{
    // Query all connected adapters
	std::vector<std::string> IDs;
	listSPIAdapters(IDs);

    // Print the ids
    printf("Available adapters:\n");
	for (unsigned int i = 0; i < IDs.size(); ++i) {
		FinisarHROCM_V3 OCM(std::string("id=")+IDs[i]);
		if (OCM.open() == OCM_OK) {
			std::string rev;
			OCM.getAdapterFW(rev);
			printf("% 16s : FirmwareRev = %s\n", IDs[i].c_str(), rev.c_str());
			OCM.close();
		}
	}

    return 0;
}

// Set adapter ID
int commandSetID(std::string newID)
{
	FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	// Get current ID
	std::string oldID;
	Result = Result || OCM.getID(oldID);

	// Set new ID
	if (Result == OCM_OK && newID != oldID) {
		printf("Setting adapter ID from %s to %s\n", oldID.c_str(), newID.c_str());
		Result = Result || OCM.setID(newID);

		std::string verifyID;
		Result = Result || OCM.getID(verifyID);
		if (Result == OCM_OK && verifyID != newID) {
			theLastError << "[ERROR] Could not change ID. Please upgrade adapter firmware to FirmwareRev=6 or above" << std::endl;
			Result = OCM_FAILED;
		}
	}
	else if (Result == OCM_OK && newID == oldID) {
		theLastError << "[WARNING] Current adapter ID is already " << oldID << ". No change applied." << std::endl;
	}

	LOGERROR(OCM);
	return Result;
}

// Run a simple loopback test on the SPI bus. You need to connect MOSI to MISO for it to work.
int commandLoopback() {
	return spiLoopbackTest(theConfigString.c_str(), 65536*2, 64);
}

// Run single scan and output as frequency/power column
int commandSingleScan()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

	// Get RDataDEV
	OCM3_RDataDEV_t	*pRDataDEV = NULL;
	Result = Result || OCM.getRDataDEV(pRDataDEV);

	// Call TPC command
	Result = Result || OCM.runFullScan(OCM3_TASK_PW_MASK);

	// Output result in CSV format
	if (Result == OCM_OK && pRDataDEV!=NULL)
	{
		OCM3_GMPWResult_t *pGMPWResult = OCM.getGMPWResult();
	
		printf("Port,fCenter_THz,Power_dBm\n");
		for (unsigned int k = 0; k<pGMPWResult->GMPWVector.size(); ++k) {
			double fSliceLeft = ((pGMPWResult->GMPWVector[k].SLICESTART-1)*pRDataDEV->SLW+ pRDataDEV->FSF)/OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
			double fSliceRight= ((pGMPWResult->GMPWVector[k].SLICEEND-1+1)*pRDataDEV->SLW+ pRDataDEV->FSF)/OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
			printf("%d,%.7f,%.1f\n", pGMPWResult->GMPWVector[k].PORTNO, (fSliceLeft + fSliceRight) / 2, pGMPWResult->GMPWVector[k].POWER / 10.0);
		}
	}

	LOGERROR(OCM);
	return Result;
}

// Run single scan and output as portno/slicestart/sliceend/power column
int commandSingleScanRaw()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call TPC command
    Result = Result || OCM.runFullScan(OCM3_TASK_PW_MASK);

    // Output result in CSV format
    if (Result==OCM_OK)
    {
		OCM3_GMPWResult_t *pGMPWResult = OCM.getGMPWResult();
	    
		printf("Port,SliceStart,SliceEnd,Power_dBm\n");
        for(unsigned int k=0;k<pGMPWResult->GMPWVector.size();++k) {
            printf("%d,%d,%d,%.1f\n", pGMPWResult->GMPWVector[k].PORTNO, pGMPWResult->GMPWVector[k].SLICESTART, pGMPWResult->GMPWVector[k].SLICEEND, pGMPWResult->GMPWVector[k].POWER/10.0);
		}
    }

	LOGERROR(OCM);
	return Result;
}

// Run single scan OSNR measurement
int commandSingleScanOSNR()
{
	FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	// Get RDataDEV
	OCM3_RDataDEV_t	*pRDataDEV = NULL;
	Result = Result || OCM.getRDataDEV(pRDataDEV);

	// Call TPC command
	Result = Result || OCM.runFullScan(OCM3_TASK_OSNR_MASK);

	// Output result in CSV format
	if (Result == OCM_OK && pRDataDEV!=NULL)
	{
		OCM3_GMOSNRResult_t *pGMPWResult = OCM.getGMOSNRResult();

		printf("Port,fCenter_THz,Power_dBm,OSNR_dB\n");
		for (unsigned int k = 0; k<pGMPWResult->GMOSNRVector.size(); ++k) {
			//double fSliceLeft = ((pGMPWResult->GMOSNRVector[k].SLICESTART - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
			//double fSliceRight = ((pGMPWResult->GMOSNRVector[k].SLICEEND - 1 + 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
			double fCenter = ((pGMPWResult->GMOSNRVector[k].CENTERFREQUENCY - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
			double Power = pGMPWResult->GMOSNRVector[k].POWER / OCM3_PSCALE;
			double OSNR = pGMPWResult->GMOSNRVector[k].OSNR / OCM3_PSCALE;
			printf("%d,%.7f,%.1f,%.1f\n", pGMPWResult->GMOSNRVector[k].PORTNO, fCenter, Power, OSNR);
		}
	}

	LOGERROR(OCM);
	return Result;
}

// Run multiple scans for stability testing
int commandHammer(int nRuns)
{
	FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    printf("[INFO] Press any key to stop\n");

	// Remember last TxSeqNum
	unsigned int lastTxSeqNum0 = 0;

	DWORD t0 = ::GetTickCount();
    for(int iRun = 0;Result==OCM_OK && (nRuns==0 || iRun<nRuns);)
    {
		// Call TPC command and ignore errors. We need to start new scans as quickly as possible. In order to find out if the module is ready to start a new scan,
		// we could monitor the state of the SSS digital signal. On the eval kit, we don't have access to it. So, we keep sending TPC commands and accept that
		// most of the time COMRES=2 will be returned.
		OCM3_Response_t Head;
		unsigned int TxSeqNum = 0;
		OCM.cmdTPC(Head, TxSeqNum, OCM3_TASK_PW_MASK);
		// Purge Error Buffer
		std::string tempError;
		OCM.get(OCM_KEY_LASTERROR, tempError);

		// Wait a randomized time
		//Sleep(rand()*1000/RAND_MAX+1000);

		// If the sequence number in SEQARR[0] has changed, we can pick up new results. REMARK: At this stage, a new scan has already been started. We need to be quick
		// picking up the result as it will be overridden soon when the next result is ready.
		Result = Result || OCM.cmdPollShort(Head);
		if (Result==OCM_OK && Head.SEQARR[OCM3_PROCESS_PW]!=lastTxSeqNum0)
        {
			++iRun; // Increase cycle counter
			lastTxSeqNum0 = Head.SEQARR[OCM3_PROCESS_PW]; // Remember the current sequence number for later in order to detect changes

			// Pick up result
			OCM3_GMPWResult_t GMPWResult;
			Result = Result || OCM.cmdQueryTPC_PW(GMPWResult, lastTxSeqNum0);

            printf("[INFO] Scan=%d t=%.2fh tScan=%.0fms nCRC1=%d nCRC2=%d nCmdRetransmit=%d\n",iRun,(double)(::GetTickCount()-t0)/1000.0/3600.0,(double)(::GetTickCount()-t0)/(iRun+1),OCM.getNCRC1ErrorCount(),OCM.getNCRC2ErrorCount(),OCM.getNCmdRetransmit());
        }

		LOGERROR(OCM);

        // Check keyboard to interrupt loop
        if (_kbhit())
        {
            getch();
            break;
        }
    }

	LOGERROR(OCM);
	return Result;
}

// Clear errors
int commandCLE()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call CLE command
    Result = Result || OCM.cmdCLE();
    if (Result==OCM_OK)
        printf("[OK] CLE command executed\n");

	LOGERROR(OCM);
	return Result;
}

// Reset
int commandRES()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call RES command
    Result = Result || OCM.cmdRES();
    if (Result==OCM_OK)
        printf("[OK] RES command executed\n");

	LOGERROR(OCM);
	return Result;
}

// Dump the whole SPI register file in CSV format
int commandDump()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call Poll command
    OCM3_Response_t Head;
    std::vector<char> RDATA;
    Result = Result || OCM.cmdPollLong(Head,RDATA,0);

    // Print result in CSV format
    if (Result==OCM_OK)
    {
        OCM.printResponse(&Head);
		OCM.printRData(Head.OPCODE, RDATA.size()>0 ? &RDATA[0] : NULL, RDATA.size(), true);
    }

	LOGERROR(OCM);
	return Result;
}

// Fump only the SPI header in CSV format
int commandDumpShort()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call Poll command
    OCM3_Response_t Head;
    Result = Result || OCM.cmdPollShort(Head);

    // Print result in CSV format
	if (Result == OCM_OK) {
		OCM.printResponse(&Head);
	}

	LOGERROR(OCM);
	return Result;
}

// DEV?
int commandDEV()
{
	FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	// Call GETDEV command
	OCM3_RDataDEV_t RDataDev;
	OCM3_Response_t Head;
	Result = Result || OCM.cmdGETDEV(Head,RDataDev);
	LOGERROR(OCM);
	OCM.close();

	// Use the generic dump routine to print the result
	Result = Result || commandDump();
	LOGERROR(OCM);

	// Query current number of averages. Not all previous firmware versions have this command implemented.
	if (Result == OCM_OK) {
		unsigned short nCurrentAverage = 0;
		Result = Result || OCM.open();
		Result = Result || OCM.cmdGETAVG(nCurrentAverage);
		if (Result == OCM_OK) {
			printf("AVG,%d\n", nCurrentAverage);
		}
		Result = OCM_OK; // If there was an error, let's ignore it. It is possible that the command is not implemented in this firmware revision.
	}

	return Result;
}

// Set module identification
int commandMID(const char *MID)
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call MID command
    Result = Result || OCM.cmdMID(MID);
	if (Result == OCM_OK) {
		printf("[OK] MID command executed (%s)\n", MID);
	}

	LOGERROR(OCM);
	return Result;
}

// Write a channel plan with equidistant densely packed channels
// iStart   : Start frequency in tenth of a MHz (191.25 THz = 1912500000)
// iGrid    : Frequency grid in tenth of a MHz (50 GHZ = 500000)
// nChannels: Total number of channels
int commandITU(int iStart,int iGrid,int nChannels)
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);
    // Open OCM
    OCM_Error_t Result = OCM.open();

	// Get RDataDEV
	OCM3_RDataDEV_t	*pRDataDEV = NULL;
	Result = Result || OCM.getRDataDEV(pRDataDEV);
	if (Result != OCM_OK || pRDataDEV==NULL) {
		LOGERROR(OCM);
		return Result; // pRDataDEV could be NULL. So better return here in case of failure.
	}

	if (nChannels > pRDataDEV->Nmax) {
		Result = Result || OCM_FAILED;
		theLastError << "[ERROR] Too many channels requested: " << nChannels << " (Max=" << pRDataDEV->Nmax << ")" << std::endl;
	}

	// Setup MPPWVector
	std::vector<OCM3_MPPWRecord_t> MPPWVector;
    int sliceStart  = (iStart-iGrid/2- pRDataDEV->FSF)/ pRDataDEV->SLW;
    int sliceGrid   = iGrid/ pRDataDEV->SLW;
    for(int i=0;i<nChannels && Result==OCM_OK;++i)
    {
		if ((sliceStart + i*sliceGrid) < 0 || (sliceStart + i*sliceGrid + sliceGrid - 1) >= pRDataDEV->Smax) {
			Result = Result || OCM_FAILED;
			theLastError << "[ERROR] Slice number out of range (channel index " << i << ")" << std::endl;
		}

        OCM3_MPPWRecord_t Channel;
        Channel.PORTNO      = 1;
        Channel.SLICESTART  = 1 + sliceStart + i*sliceGrid;             // Slice numbers are 1-based, not 0-based
        Channel.SLICEEND    = Channel.SLICESTART + sliceGrid - 1;

        MPPWVector.push_back(Channel);
    }

    // Call MPPW command
    Result = Result || OCM.cmdSETMPPW(MPPWVector);
	if (Result == OCM_OK) {
		printf("[OK] SETMPPW command executed (%d channels)\n", (int)MPPWVector.size());
	}

	// Only setup MPOSNRVector if we it is not a high-resolution channel plan
	if (Result == OCM_OK && sliceGrid>1) {
		// Setup MPOSNRVector
		std::vector<OCM3_MPOSNRRecord_t> MPOSNRVector;
		int sliceStart = (iStart - iGrid / 2 - pRDataDEV->FSF) / pRDataDEV->SLW;
		int sliceGrid = iGrid / pRDataDEV->SLW;
		for (int i = 0; i < nChannels; ++i)
		{
			if ((sliceStart + i*sliceGrid) < 0 || (sliceStart + i*sliceGrid + sliceGrid - 1) >= pRDataDEV->Smax) {
				Result = Result || OCM_FAILED;
				theLastError << "[ERROR] Slice number out of range (channel index " << i << ")" << std::endl;
			}

			OCM3_MPOSNRRecord_t Channel;
			Channel.PORTNO = 1;
			Channel.CENTERSTART = 1 + sliceStart + i*sliceGrid;             // Slice numbers are 1-based, not 0-based
			Channel.CENTERSTOP = Channel.CENTERSTART + sliceGrid - 1;
			Channel.KEEPOUTLOWER = (int)round(theOsnrSearchMinTHz*OCM3_FSCALE / pRDataDEV->SLW);
			Channel.KEEPOUTUPPER = (int)round(theOsnrSearchMinTHz*OCM3_FSCALE / pRDataDEV->SLW);
			Channel.NOISELOWER = (int)round(theOsnrSearchMaxTHz*OCM3_FSCALE / pRDataDEV->SLW);
			Channel.NOISEUPPER= (int)round(theOsnrSearchMaxTHz*OCM3_FSCALE / pRDataDEV->SLW);
			if (pRDataDEV->BWXB == 'T') {
				Channel.CENTERBWTHRES = (unsigned short) round(theOsnrThresDb*OCM3_PSCALE);
			}
			else {
				Channel.CENTERBWTHRES = (unsigned short)round((theOsnrThresTHz * OCM3_FSCALE / pRDataDEV->SLW - 1) / 2);
			}
			Channel.TAGRANGE = (unsigned short)round((theOsnrTagRangeTHz * OCM3_FSCALE / pRDataDEV->SLW - 1) / 2);
			Channel.RBW = (unsigned short)round(theOsnrRbwTHz * OCM3_RBWSCALE);
			MPOSNRVector.push_back(Channel);
		}

		// Call MPOSNR command
		Result = Result || OCM.cmdSETMPOSNR(MPOSNRVector);
		if (Result == OCM_OK) {
			printf("[OK] SETMPOSNR command executed (%d channels)\n", (int)MPOSNRVector.size());
		}
	}

	LOGERROR(OCM);
	return Result;
}

// Set channel plan with the highest resolution and the maximum number of channels
int commandHIRES()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call DEV? command if needed
	OCM3_RDataDEV_t	*pRDataDEV = NULL;
	Result = Result || OCM.getRDataDEV(pRDataDEV);

    // Set channel plan
    Result = Result || commandITU(pRDataDEV->FSF+ pRDataDEV->SLW/2, pRDataDEV->SLW, pRDataDEV->Smax-1);

	LOGERROR(OCM);
	return Result;
}

// read channel plan from stdin
int commandMPPW()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

	// Call DEV? command if needed
	OCM3_RDataDEV_t	*pRDataDEV = NULL;
	Result = Result || OCM.getRDataDEV(pRDataDEV);
	
    // Setup MPPWVector and read from stdin
    std::vector<OCM3_MPPWRecord_t> MPPWVector;
    while(!feof(stdin) && Result == OCM_OK)
    {
        int iPort=0,iSliceStart=0,iSliceEnd=0;
        scanf("%d,%d,%d\n",&iPort,&iSliceStart,&iSliceEnd);
		if (iSliceStart<1 || iSliceStart>pRDataDEV->Smax) {
			Result = Result || OCM_FAILED;
			theLastError << "[ERROR] Start slice number out of range (channel index " << MPPWVector.size() << ")" << std::endl;
		}
		if (iSliceEnd<1 || iSliceEnd>pRDataDEV->Smax) {
			Result = Result || OCM_FAILED;
			theLastError << "[ERROR] End slice number out of range (channel index " << MPPWVector.size() << ")" << std::endl;
		}
		if (iSliceEnd < iSliceStart) {
			Result = Result || OCM_FAILED;
			theLastError << "[ERROR] End slice number smaller than start slice number (channel index " << MPPWVector.size() << ")" << std::endl;
		}

        OCM3_MPPWRecord_t Channel;
        Channel.PORTNO      = iPort;
        Channel.SLICESTART  = iSliceStart;
        Channel.SLICEEND    = iSliceEnd;
        MPPWVector.push_back(Channel);
    }

    // Call MPPW command
    Result = Result || OCM.cmdSETMPPW(MPPWVector);
	if (Result == OCM_OK) {
		printf("[OK] MPPW command executed (%d channels)\n", (int)MPPWVector.size());
	}

	LOGERROR(OCM);
	return Result;
}

// Firmware transfer (FWT)
int commandFWT(const char *Filename)
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Load binary file (old style...)
    std::vector<char> BinaryFile;
    FILE *f = fopen(Filename,"rb");
    if (f) {
        int c=fgetc(f);
        while(!feof(f)) {
            BinaryFile.push_back((char)c);
            c=fgetc(f);
        }
        fclose(f);
    }
    else {
        Result = Result || OCM_FAILED;
		theLastError << "[ERROR] File not found (" << Filename << ")" << std::endl;
    }
	if (Result == OCM_OK) {
		printf("[OK] File loaded (%s, %d bytes)\n", Filename, (int)BinaryFile.size());
	}

    // Call FWT command
    Result = Result || OCM.cmdFWT(BinaryFile);
	if (Result == OCM_OK) {
		printf("[OK] FWT executed\n");
	}
	else {
		printf("[ERROR] FWT failed\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Firmware execute (FWE)
int commandFWE()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call FWE command
    Result = Result || OCM.cmdFWE();
	if (Result == OCM_OK) {
		printf("[OK] FWE command executed\n");
	}
	else {
		printf("[ERROR] FWE command failed\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Firmware save (FWS)
int commandFWS()
{
    FinisarHROCM_V3 OCM(theConfigString,theLogFile,theLogBinFile);

    // Open OCM
    OCM_Error_t Result = OCM.open();

    // Call FWS command
    Result = Result || OCM.cmdFWS();
	if (Result == OCM_OK) {
		printf("[OK] FWS command executed\n");
	}
	else {
		printf("[ERROR] FWS command failed\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Update module firmware
int commandUpdate(const char *Filename)
{
    OCM_Error_t Result = OCM_OK;

    // Transfer firmware
    Result = Result || commandFWT(Filename);
    // Save firmware
    Result = Result || commandFWS();
    // Reset module
    // Result = Result || commandRES();

	if (Result == OCM_OK) {
		printf("[INFO] Run RES command to restart the module\n");
	}

    return Result;
}

// Set the averaging attribute
int commandAVG(unsigned int nAverage)
{
	FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	Result = Result || OCM.cmdSETAVG((unsigned short)nAverage);

	unsigned short nCurrentAverage = 0;
	Result = Result || OCM.cmdGETAVG(nCurrentAverage);
	if (Result == OCM_OK && nCurrentAverage != nAverage) {
		printf("[INFO] Run RES command to register changes\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Set the bandwidth mode to "Fixed Slices From Center"
int commandBWS()
{
	FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	Result = Result || OCM.cmdSETBWXB('S');
	if (Result == OCM_OK) {
		printf("[INFO] Run RES command to register changes\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Set the bandwidth mode to "Threshold from Peak"
int commandBWT()
{
	FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	Result = Result || OCM.cmdSETBWXB('T');
	if (Result == OCM_OK) {
		printf("[INFO] Run RES command to register changes\n");
	}

	LOGERROR(OCM);
	return Result;
}

// Resets averaging and bandwidth mode to the factory settings
int commandFactory()
{
	FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

	// Open OCM
	OCM_Error_t Result = OCM.open();

	printf("[INFO] Reset AVG\n");
	Result = Result || OCM.cmdCLRAVG();
	printf("[INFO] Reset BWXB\n");
	Result = Result || OCM.cmdCLRBWXB();

	LOGERROR(OCM);
	OCM.close();

	Result = Result || commandRES();

	return Result;
}

// Parse hex number
unsigned int htou(const char *s) {
	unsigned int result;

	std::stringstream ss;
	ss << std::hex << s;
	ss >> result;

	return result;
}
//创建一个互斥量
HANDLE g_hMutex = NULL;
//定义接受数组
typedef struct {
	int type;
	int number;
}TLV1;

std::vector<int>TLV2(30);
std::vector<int>TLV2_centerfre(10);
std::vector<int>TLV2_channel(10);
std::vector<int>TLV2_nchannel(10);



typedef struct {
	int frestep_50;
	int frestep_100;
	float frecenter[4];
}RecvData;

typedef struct {
	/*float frecenter;
	int frestep_50;
	int frestep_100;*/
	//double centerfre[80];
	double power_dbm[80];
	double osnr_db[80];
	double power_dbm_100[40];
	double osnr_db_100[40];
}Node;

typedef struct {
	int power_dbm[4];
	int osnr_db[4];
	//double power_dbm_100[4];
	//double osnr_db_100[4];
}Data;
//线程函数
DWORD WINAPI ThreadProcScan(LPVOID lpParameter)
{
	Node*  pThreadData = (Node*)lpParameter;
	while (1)
	{
		int i;
		WaitForSingleObject(g_hMutex, INFINITE);
		//printf("Thread1周期采集光谱信息\n");
		/*for (i = 0; i <80; i++)
		{
			pThreadData->osnr_db[i] = i;
			pThreadData->power_dbm[i] = i;
		}
		printf("threadpower:%.1f,%.1f\n", pThreadData->power_dbm[0], pThreadData->power_dbm[1]*/
		commandITU((int)(191.4 * OCM3_FSCALE + 0.5), (int)( 0.05* OCM3_FSCALE + 0.5), 80);
		{
			FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

			// Open OCM
			OCM_Error_t Result = OCM.open();

			// Get RDataDEV
			OCM3_RDataDEV_t	*pRDataDEV = NULL;
			Result = Result || OCM.getRDataDEV(pRDataDEV);

			// Call TPC command
			Result = Result || OCM.runFullScan(OCM3_TASK_OSNR_MASK);

			// Output result in CSV format
			if (Result == OCM_OK && pRDataDEV != NULL)
			{
				OCM3_GMOSNRResult_t *pGMPWResult = OCM.getGMOSNRResult();
				//printf("Port,fCenter_THz,Power_dBm,OSNR_dB\n");
				for (unsigned int k = 0; k < pGMPWResult->GMOSNRVector.size(); ++k)
				{
					//double fSliceLeft = ((pGMPWResult->GMOSNRVector[k].SLICESTART - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					//double fSliceRight = ((pGMPWResult->GMOSNRVector[k].SLICEEND - 1 + 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					//pThreadData->centerfre[k] = ((pGMPWResult->GMOSNRVector[k].CENTERFREQUENCY - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					pThreadData->power_dbm[k] = pGMPWResult->GMOSNRVector[k].POWER / OCM3_PSCALE;
					pThreadData->osnr_db[k] = pGMPWResult->GMOSNRVector[k].OSNR / OCM3_PSCALE;
					//printf("%.7f,%.1f,%.1f\n", pThreadData->power_dbm[k], pThreadData->osnr_db[k]);
				}
			}
		}
		commandITU((int)(191.4 * OCM3_FSCALE + 0.5), (int)(0.1 * OCM3_FSCALE + 0.5), 40); //0.5应该没有意义，前面有int，真蠢
		{
			
			FinisarHROCM_V3 OCM(theConfigString, theLogFile, theLogBinFile);

			// Open OCM
			OCM_Error_t Result = OCM.open();

			// Get RDataDEV
			OCM3_RDataDEV_t	*pRDataDEV = NULL;
			Result = Result || OCM.getRDataDEV(pRDataDEV);

			// Call TPC command
			Result = Result || OCM.runFullScan(OCM3_TASK_OSNR_MASK);

			// Output result in CSV format
			if (Result == OCM_OK && pRDataDEV != NULL)
			{
				OCM3_GMOSNRResult_t *pGMPWResult = OCM.getGMOSNRResult();
				//printf("Port,fCenter_THz,Power_dBm,OSNR_dB\n");
				for (unsigned int k = 0; k < pGMPWResult->GMOSNRVector.size(); ++k)
				{
					//double fSliceLeft = ((pGMPWResult->GMOSNRVector[k].SLICESTART - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					//double fSliceRight = ((pGMPWResult->GMOSNRVector[k].SLICEEND - 1 + 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					//pThreadData->centerfre[k] =' ((pGMPWResult->GMOSNRVector[k].CENTERFREQUENCY - 1)*pRDataDEV->SLW + pRDataDEV->FSF) / OCM3_FSCALE; // Slice numbers are 1-based, not 0-based
					pThreadData->power_dbm_100[k] = pGMPWResult->GMOSNRVector[k].POWER / OCM3_PSCALE;
					pThreadData->osnr_db_100[k] = pGMPWResult->GMOSNRVector[k].OSNR / OCM3_PSCALE;
					//printf("%.1f,%.1f\n", pThreadData->power_dbm_100[k], pThreadData->osnr_db_100[k]);
				}
			}
		}
		ReleaseMutex(g_hMutex);
		Sleep(2000);
	}
}


// Main command line interface
int _tmain(int argc, _TCHAR* argv[])
{
	
	int Result = 0;
	 {
	   TLV1 *myRecvData = (TLV1*)malloc(sizeof(TLV1));
	   //Data *myData = (Data*)malloc(sizeof(Data));
	   Node *myNode = (Node*)malloc(sizeof(Node));
	  //创建一个互斥量
	   g_hMutex = CreateMutex(NULL, FALSE, NULL);

	   //创建一个子线程
	   HANDLE hThread = CreateThread(NULL, 0, ThreadProcScan, myNode, 0, NULL);

	   //关闭线程句柄
	   CloseHandle(hThread);
	   Sleep(100);
 
	 //初始化WSA
	   WORD sockVersion = MAKEWORD(2, 2);
	   WSADATA wsaData;
	   if (WSAStartup(sockVersion, &wsaData) != 0)
	   {
		   return 0;
	   }

	   //创建套接字
	   SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	   if (slisten == INVALID_SOCKET) 
	   {
		   printf("socket error");
		   return 0;
	   }


	   //绑定IP和端口
	   sockaddr_in sin;
	   sin.sin_family = AF_INET;


	   sin.sin_port = htons(8888);
	   sin.sin_addr.S_un.S_addr = inet_addr("192.168.108.239");
	   if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) 
	   {
		   printf("bind error !");
	   }

	   //开始监听
	   if (listen(slisten, 5) == SOCKET_ERROR)
	   {
		   printf("listen error !");
		   return 0;
	   }

	   //确认连接
	   SOCKET sClient;
	   sockaddr_in remoteAddr;
	   int nAddrlen = sizeof(remoteAddr);
	   printf("等待连接...\n");
	   sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
	   if (sClient == INVALID_SOCKET)
	   {
		   printf("accept error !");
		   return 0;
	   }
	   printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
	   while (true)
	   {
		   //接受数据
		   int needrecv;
		   needrecv= sizeof(TLV1);
		   char *buffer = (char*)malloc(needrecv);
		   int pos = 0;
		   while (pos < needrecv)
		   {
			   int ret = recv(sClient, buffer + pos, 1024, 0);
			   if (ret < 0)
			   {
				   printf("Server Recieve Data Failed!\n");
				   return 0;
			   }
			   pos += ret;
		   }
		   memcpy(myRecvData, buffer, needrecv);
		   free(buffer);
		  // *(buffer + needrecv) = '\0';

		   //读入TLV2
		   int type = myRecvData->type;
		   int number = myRecvData->number;
		 
		   needrecv = type * number * sizeof(int);
		   char *buffer1 = (char*)malloc(needrecv);
		   printf("type:%d,number:%d\n", myRecvData->type, myRecvData->number);
		   pos = 0;
		   while (pos < needrecv)
		   {
			   int ret = recv(sClient, buffer1+ pos, 1024, 0);
			   if (ret < 0)
			   {
				   printf("Server Recieve Data Failed!\n");
				   return 0;
			   }
			   pos += ret;
		   }
		   memcpy(&TLV2[0], buffer1, needrecv);
		   free(buffer1);
		   int i;
		   int j = 0;
		   int k = 0;
		   int l = 0;

		   for (i = 0; i <type * number; i++){
			   if (i % 3 == 0) {
				   TLV2_centerfre[j] = TLV2[i];
				   j++;
			   }
			   if (i % 3 == 1) {
				   TLV2_channel[k] = TLV2[i];
				   k++;
			   }
			   if (i % 3 == 2) {
				   TLV2_nchannel[l] = TLV2[i];
				   l++;
			   }
			}
		   printf("centerfre:");
		   for (i = 0; i < number; i++) {
			   printf("%f\t", TLV2_centerfre[i]/100.0);
		   }
		   printf("\n");

		   printf("channel:");
		   for (i = 0; i < number; i++) {
			   printf("%d\t", TLV2_channel[i]);
		   }
		   printf("\n");

		   printf("nchannel:");
		   for (i = 0; i < number; i++) {
			   printf("%d\t", TLV2_nchannel[i]);
		   }
		   printf("\n");

		   int channelsum = 0;
		   int needSend;
		   for (i = 0; i < number; i++)
		   {
			   channelsum = channelsum + TLV2_nchannel[i];
		   }
		   needSend = channelsum * 2 * sizeof(double);
		   char *sendbuffer = (char*)malloc(needSend);
		  
		   
		   //读入数据
		   std::vector<double>send2Client(channelsum * 2);
		   int index;
		
		   WaitForSingleObject(g_hMutex, INFINITE);

		   for (i = 0; i < number; i++)
		   {
			   if (TLV2_channel[i] == 50)
			   {
				   index = round((TLV2_centerfre[i] - 19140) / 5);
				   send2Client[2 * i] = myNode->power_dbm[index];
				   send2Client[2 * i + 1] = myNode->osnr_db[index];
			   }
			   if (TLV2_channel[i] == 100)
			   {
				   index = round((TLV2_centerfre[i] - 19140) / 10);
				   send2Client[2 * i] = myNode->power_dbm_100[index];
				   send2Client[2 * i + 1] = myNode->osnr_db_100[index];
				   printf("Send:%.1f,%.1f\n", myNode->power_dbm_100[index], myNode->osnr_db_100[index]);
			   }
		   }
		   //if (TLV2_channel[0] == 50)
		   //{
			  // for (i = 0; i < sizeof(myRecvData->frecenter)/4; i++)
			  // {
				 //  index = round(((myRecvData->frecenter[i]) * 100 - 19140) / 5);
				 //  myData->osnr_db[i] = myNode->osnr_db[index];
				 //  myData->power_dbm[i] = myNode->power_dbm[index];
				 // // printf("Send:%.1f,%.1f\n", myData->osnr_db[i], myData->power_dbm[i]);
			  // }
			  //
		   //}
		   //else
		   //{
			  // for (i = 0; i < sizeof(myRecvData->frecenter) / 4; i++)
			  // {
				 //  //index = round(((myRecvData->frecenter[i]) * 100 - 19140) / 5);
				 //  myData->osnr_db[i] = 0;
				 //  myData->power_dbm[i] = 0;
				 // 
			  // }
		   //}
		   //if (cfrestep_100)
		   //{
			  // for (i = 0; i < sizeof(myRecvData->frecenter) / 4; i++)
			  // {
				 //  index = round(((myRecvData->frecenter[i]) * 100 - 19140) / 10);
				 //  myData->osnr_db_100[i] = myNode->osnr_db_100[index];
				 //  myData->power_dbm_100[i] = myNode->power_dbm_100[index];
				 ////  printf("50GHz channel Send:%.1f,%.1f\n", myData->osnr_db_100[i], myData->power_dbm_100[i]);
			  // }

		   //}
		   //else
		   //{

			  // for (i = 0; i < sizeof(myRecvData->frecenter) / 4; i++)
			  // {
				 //  //index = round(((myRecvData->frecenter[i]) * 100 - 19140) / 10);
				 //  myData->osnr_db_100[i] = 0;
				 //  myData->power_dbm_100[i] = 0;
				 // // printf("Send:%.1f,%.1f\n", myData->osnr_db_100[i], myData->power_dbm_100[i]);
			  // }
		   //}
		   ReleaseMutex(g_hMutex);
		  
		   memcpy(sendbuffer, &send2Client[0], needSend);
		   int position = 0;
		   while (position < needSend)
		   {
			   int length = send(sClient, sendbuffer + position, sizeof(Data), 0);
			   if (length < 0)
			   {
				   printf("Server Transmit Data Failed!\n");
				   return 0;
			   }
			   position += length;
		   }
		  // free(sendbuffer);
		   send(sClient, "end", 3, 0);
		  // free(myData);
	   }
       closesocket(sClient);
	   closesocket(slisten);
	   WSACleanup();
       return 0;
     }
    // Seed the random number generator so that the Tx sequence number
    // always starts with a different number.
    // This is important because this tool terminates after issuing a command.
    // The next call would have the same sequence number again and would be ignored by the module.
    srand( (unsigned)time( NULL ) );

    // Index to command name item
    int iArg=1;
	unsigned int    SPIClock = SPID_DEFAULT_CLOCKRATE;	// Default = 12 MHz
	std::string		SPIAdapterID = "";					// SPI adapter ID

    // See if there are options
    for(;iArg<argc;++iArg)
    {
		if (strcmp(argv[iArg], "-log") == 0)       // Option -log creates a log file
		{
			bool fileExists = false;
			char Filename[] = "HROCMQuery.csv";
			FILE *f = fopen(Filename, "r");
			if (f)
			{
				fileExists = true;
				fclose(f);
			}
			theLogFile = fopen(Filename, "at");
			if (!theLogFile)
			{
				Result = Result || OCM_FAILED;
				theLastError << "[ERROR] Log file open failed (" << Filename << ")" << std::endl;
			}
			else
			{
				if (!fileExists) {
					fprintf(theLogFile, "TickCount,Length,nCRC1Error,nCRC2Error,nRetransmit,TxOPCODE,TxSEQNO,TxCRC1,LENGTH,OPCODE1,SEQNO1,COMRES,PPEND,SEQARR_PW,SEQARR_VC,SEQARR_CS,SEQARR_OSNR,SEQARR_CP,HSS,OSS,CRC1,CRC2Value,CRC2,Diolan\n");
				}
			}
		}
		else if (strcmp(argv[iArg], "-logbin") == 0)       // Option -logbin creates a binary log file
		{
			char Filename[] = "HROCMQuery.bin";
			theLogBinFile = fopen(Filename, "ab");
			if (!theLogBinFile)
			{
				Result = Result || OCM_FAILED;
				theLastError << "[ERROR] Log file open failed (" << Filename << ")" << std::endl;
			}
		}
		else if (strcmp(argv[iArg], "-id") == 0)    // Option -id sets the SPI adapter ID
		{
			if (++iArg < argc) {
				SPIAdapterID = argv[iArg];// The id is a hex number
			}
		}
		else if (strcmp(argv[iArg], "-2") == 0)    // Option -2 sets SPI clock rate to 2 MHz
		{
			SPIClock = 2000000;
		}
		else if (strcmp(argv[iArg],"-4")==0)    // Option -4 sets SPI clock rate to 4 MHz
        {
            SPIClock = 4000000;
        }
		else if (strcmp(argv[iArg], "-20") == 0)    // Option -20 sets SPI clock rate to 20 MHz (experimental)
		{
			SPIClock = 20000000;
		}
		else if (strcmp(argv[iArg], "-25") == 0)    // Option -25 sets SPI clock rate to 25 MHz (experimental)
		{
			SPIClock = 25000000;
		}
		else if (strcmp(argv[iArg],"-12")==0)    // Option -12 sets SPI clock rate to 12 MHz
        {
            SPIClock = 12000000;
        }
		else if (strcmp(argv[iArg], "-osnr") == 0)    // Option -osnr sets the OSNR channel plan parameters
		{
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrSearchMinTHz = atof(argv[iArg]);
			}
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrSearchMaxTHz = atof(argv[iArg]);
			}
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrThresDb = atof(argv[iArg]);
			}
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrThresTHz = atof(argv[iArg]);
			}
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrTagRangeTHz = atof(argv[iArg]);
			}
			if (++iArg < argc && Result == OCM_OK) {
				theOsnrRbwTHz = atof(argv[iArg]);
			}
		}
		else {
			break;
		}
    }

	std::ostringstream configString;
	configString << "id=" << SPIAdapterID << ";spiclk=" << SPIClock;
	theConfigString = configString.str();

    // Print selected SPI clock rate
    // printf("[INFO] SPICLK = %.1f MHz\n",theSPIClock/1000000.0);

    if (argc<iArg+1)
        Result = Result || commandHelp();
	else if (strcmp(argv[iArg], "list") == 0)
		Result = Result || commandListAdapters();
	else if (strcmp(argv[iArg], "setid") == 0 && argc>(iArg + 1))
		Result = Result || commandSetID(argv[iArg + 1]);
	else if (strcmp(argv[iArg], "loopback") == 0)
		Result = Result || commandLoopback();
	else if (strcmp(argv[iArg],"cle")==0)
        Result = Result || commandCLE();
	else if (strcmp(argv[iArg], "res") == 0)
		Result = Result || commandRES();
	else if (strcmp(argv[iArg], "dev") == 0)
		Result = Result || commandDEV();
	else if (strcmp(argv[iArg],"scan")==0)
        Result = Result || commandSingleScan();
    else if (strcmp(argv[iArg],"scanraw")==0)
        Result = Result || commandSingleScanRaw();
	else if (strcmp(argv[iArg], "scanosnr") == 0)
		Result = Result || commandSingleScanOSNR();
	else if (strcmp(argv[iArg],"hammer")==0)
        Result = Result || commandHammer(argc>(iArg+1) ? atoi(argv[iArg+1]) : 0);
    else if (strcmp(argv[iArg],"dump")==0)
        Result = Result || commandDump();
    else if (strcmp(argv[iArg],"dumpshort")==0)
        Result = Result || commandDumpShort();
    else if (strcmp(argv[iArg],"mid")==0 && argc>(iArg+1))
        Result = Result || commandMID(argv[iArg+1]);
    else if (strcmp(argv[iArg],"itu")==0 && argc>(iArg+3))
        Result = Result || commandITU((int)(atof(argv[iArg+1])*OCM3_FSCALE+0.5),(int)(atof(argv[iArg+2])*OCM3_FSCALE+0.5),atoi(argv[iArg+3]));
    else if (strcmp(argv[iArg],"hires")==0)
        Result = Result || commandHIRES();
    else if (strcmp(argv[iArg],"psa")==0)
        Result = Result || commandMPPW();
    else if (strcmp(argv[iArg],"fwt")==0 && argc>(iArg+1))
        Result = Result || commandFWT(argv[iArg+1]);
    else if (strcmp(argv[iArg],"fws")==0)
        Result = Result || commandFWS();
    else if (strcmp(argv[iArg],"fwe")==0)
        Result = Result || commandFWE();
    else if (strcmp(argv[iArg],"update")==0 && argc>(iArg+1))
        Result = Result || commandUpdate(argv[iArg+1]);
	else if (strcmp(argv[iArg], "avg") == 0 && argc>(iArg + 1))
		Result = Result || commandAVG(atoi(argv[iArg + 1]));
	else if (strcmp(argv[iArg], "bws") == 0)
		Result = Result || commandBWS();
	else if (strcmp(argv[iArg], "bwt") == 0)
		Result = Result || commandBWT();
	else if (strcmp(argv[iArg], "factory") == 0)
		Result = Result || commandFactory();
	else
        Result = Result || commandHelp();

	if (Result != OCM_OK) {
		if (theLastError.str() == "") {
			fprintf(stderr, "[ERROR] Unspecified Error\n");
		}
		else {
			fprintf(stderr, "%s", theLastError.str().c_str());
		}
	}

	if (theLogFile) {
		fclose(theLogFile);
	}

	if (theLogBinFile) {
		fclose(theLogBinFile);
	}

	return Result;
}

