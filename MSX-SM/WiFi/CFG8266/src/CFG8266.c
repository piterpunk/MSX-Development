/*
--
-- CFG8266.c
--   Set-up the Wi-Fi module of your MSX-SM / SM-X.
--   Revision 1.30
--
--
-- Requires SDCC and Fusion-C library to compile
-- Copyright (c) 2019-2020 Oduvaldo Pavan Junior ( ducasp@ gmail.com )
-- All rights reserved.
--
-- Redistribution and use of this source code or any derivative works, are
-- permitted provided that the following conditions are met:
--
-- 1. Redistributions of source code must retain the above copyright notice,
--    this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
-- 3. Redistributions may not be sold, nor may they be used in a commercial
--    product or activity without specific prior written permission.
-- 4. Source code of derivative works MUST be published to the public.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
-- TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
-- PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
-- CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
-- EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
-- PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
-- OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
-- WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
-- OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
-- ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../fusion-c/header/msx_fusion.h"
#include "../../fusion-c/header/io.h"
#include "../../fusion-c/header/asm.h"
#include "CFG8266.h"


// This function read from the disk and return how many bytes have been read
unsigned int MyRead (int Handle, unsigned char* Buffer, unsigned int Size)
{
    unsigned int iRet;

    Z80_registers regs;

    regs.Words.DE = (unsigned int) Buffer;
    regs.Words.HL = Size;
    regs.Bytes.B = (unsigned char)(Handle&0xff);
    DosCall(0x48, &regs, REGS_MAIN, REGS_MAIN);
    if (regs.Bytes.A == 0)
    {
        //Return how many bytes were read
        iRet = regs.Words.HL;
    }
    else
        iRet = 0;

    return iRet;
}

// Checks Input Data received from command Line and copy to the variables
unsigned int IsValidInput (char**argv, int argc, unsigned char *cServer, unsigned char *cFile, unsigned char *cPort)
{
	unsigned int ret = 1;
	unsigned char * Input = (unsigned char*)argv[0];

	ucScan = 0;

	if (argc)
	{
		if ((argc==1)||(argc==2)||(argc==4))
		{
		    if ((argc==1)||(argc==2))
            {
                if ((Input[0]=='/')&&((Input[1]=='s')||(Input[1]=='S')))
                    ucScan = 1;
                else if ((Input[0]=='/')&&((Input[1]=='n')||(Input[1]=='N')))
                    ucNagleOff = 1;
                else if ((Input[0]=='/')&&((Input[1]=='m')||(Input[1]=='M')))
                    ucNagleOn = 1;
                else if ((Input[0]=='/')&&((Input[1]=='o')||(Input[1]=='O')))
                    ucRadioOff = 1;
                else if ((Input[0]=='/')&&((Input[1]=='t')||(Input[1]=='T')))
                {
                    ucSetTimeout = 1;
                    Input = (unsigned char*)argv[1];
                    uiTimeout = atoi (Input);
                    if (uiTimeout > 600)
                        uiTimeout = 600;
                }
                else
                {
                    strcpy (cFile,Input);
                    ucLocalUpdate = 1;
                    if (argc==2)
                    {
                        Input = (unsigned char*)argv[1];
                        if ((Input[0]=='/')&&((Input[1]=='c')||(Input[1]=='C')))
                            ucIsFw=0;
                        else
                            ret=0;

                    }
                    else
                        ucIsFw=1;
                }
            }
            else
            {
                if ((Input[0]=='/')&&((Input[1]=='u')||(Input[1]=='U')))
                {
                    ucIsFw = 1;
                    Input = (unsigned char*)argv[2];
                    if (strlen (Input)<7)
                    {
                        strcpy(cPort,Input);
                        Input = (unsigned char*)argv[1];
                        strcpy(cServer,Input);
                        Input = (unsigned char*)argv[3];
                        strcpy(cFile,Input);
                        lPort = atol(cPort);
                        uiPort = (lPort&0xffff);
                    }
                    else
                        ret = 0;
                }
                else if ((Input[0]=='/')&&((Input[1]=='c')||(Input[1]=='C')))
                {
                    ucIsFw = 0;
                    Input = (unsigned char*)argv[2];
                    if (strlen (Input)<7)
                    {
                        strcpy(cPort,Input);
                        Input = (unsigned char*)argv[1];
                        strcpy(cServer,Input);
                        Input = (unsigned char*)argv[3];
                        strcpy(cFile,Input);
                        lPort = atol(cPort);
                        uiPort = (lPort&0xffff);
                    }
                    else
                        ret = 0;
                }
                else
                    ret = 0;
            }
		}
		else
            ret = 0;
	}
	else
        ret=0;

	return ret;
}

void TxByte(char chTxByte)
{
    while (myPort7&2);
#ifdef DEBUG_RS232
    printf("[%x]",chTxByte);
#endif
    myPort7 = chTxByte;
}

char *ultostr(unsigned long value, char *ptr, int base)
{
    unsigned long t = 0, res = 0;
    unsigned long tmp = value;
    unsigned char count = 0;

    if (NULL == ptr) //if null pointer
        return NULL; //nothing to do

    if (tmp == 0) //if value is zero
        ++count; //one digit
    else
    {
        while(tmp > 0)
        {
            tmp = tmp/base;
            ++count;
        }
    }

    ptr += count; // so, after the LSB
    *ptr = '\0'; // null terminator

    do
    {
        t = value / base; // useful now (find remainder) as well later (next value of value)
        res = value - base * t; // get what remains of dividing base
        // We can work up to base 16, so need to make HEX if base allows values larger than 9
        if (res < 10)
            * -- ptr = '0' + res; // convert the remainder to ASCII and put in the current position of pointer, move pointer after operation
        else if ((res >= 10) && (res < 16)) // Otherwise is a HEX value and a digit above 9
            * --ptr = 'A' - 10 + res; // convert the remainder to ASCII and put in the current position of pointer, move pointer after operation
    } while ((value = t) != 0); //value is now t, and if t is other than zero, still work to do

    return(ptr); // and return own pointer as successful conversion has been made
}

bool WaitForRXData(unsigned char *uchData, unsigned int uiDataSize, unsigned int Timeout, bool bVerbose, bool bShowReceivedData, unsigned char *uchData2, unsigned int uiDataSize2)
{
    bool bReturn = false;
    unsigned char rx_data;
	unsigned int Timeout1,Timeout2;
	unsigned int ResponseSt = 0;
	unsigned int ResponseSt2 = 0;
	unsigned int i = 0;

	if (bShowReceivedData)
    {
        printf ("Waiting for: ");
        for (i=0;i<uiDataSize;++i)
            printf("%c",uchData[i]);
        printf (" / ");
        for (i=0;i<uiDataSize;++i)
            printf("{%x}",uchData[i]);
        printf ("\r\n");
        i = 0;
    }
    //Command sent, done, just wait response
    Timeout1 = TickCount + 9; //Drives the animation every 9 ticks or so
    Timeout2 = TickCount + Timeout; //Wait up to 5 minutes

    ResponseSt = 0;
    ResponseSt2 = 0;

    do
    {
        if (Timeout>900)
        {
            if (TickCount>Timeout1)
            {
                Timeout1 = TickCount + 9;
                printf("%s",advance[i%5]); // next char
                ++i;
            }
        }
        if(UartRXData())
        {
            rx_data = GetUARTData();

            if (rx_data == uchData[ResponseSt])
            {
                if (bShowReceivedData)
                    printf ("{%x}",rx_data);
                ++ResponseSt;
                if (ResponseSt == uiDataSize)
                {
                    bReturn = 1;
                    break;
                }
            }
            else
            {
                if ((ResponseSt)&&(bShowReceivedData))
                    printf ("{%x} != [%x]",rx_data,uchData[ResponseSt]);
                else if (bShowReceivedData)
                    printf ("}%x{",rx_data);
                if ((uiDataSize==2)&&(ResponseSt==1))
                {
                    if ((bVerbose)&&(!uchData2))
                        printf ("Error %u on command %c...\r\n",rx_data,uchData[0]);
                    return false;
                }
                ResponseSt = 0;
            }

            if ((uchData2)&&(rx_data == uchData2[ResponseSt2]))
            {
                ++ResponseSt2;
                if (ResponseSt2 == uiDataSize2)
                {
                    bReturn = 2;
                    break;
                }
            }
            else
                ResponseSt2 = 0;
        }

        if (TickCount>Timeout2)
            break;
    }
    while (1);

    if (Timeout>900)
        printf("%s",aDone); // clear line

    return bReturn;
}

void FinishUpdate (bool bSendReset)
{
	unsigned int iRetries = 3;
	unsigned char uchHalt = 60;
	bool bRet=true;
	unsigned char ucRetries = 2;
	bool bReset = bSendReset;

	printf("\rFinishing flash, this will take some time, WAIT!\r\n");

	do
    {
        bRet = true;
        --ucRetries;
        if (bReset)
            TxByte('R'); //Request Reset
        else
        {
            do
            {
                for (uchHalt=60;uchHalt>0;--uchHalt)
                    Halt();
                TxByte('E'); //End Update
                bRet = WaitForRXData(endUpdate,2,1800,true,false,NULL,0);
                iRetries--;
            }
            while ((!bRet)&&(iRetries));

            if (bRet)
                bReset=true;
        }

        if (!bRet)
            printf("\rTimeout waiting to end update...\r\n");
        else
        {
            if (ucRetries)
            {
                if (ucIsFw)
                    printf("\rFirmware Update done, ESP is restarting, WAIT...\r\n");
                else
                    printf("\rCertificates Update done, ESP is restarting, WAIT...\r\n");
            }

            if (WaitForRXData(responseReady2,7,2700,false,false,NULL,0)) //Wait up to 45 seconds
            {
                if (!ucIsFw)
                {
                    printf("\rESP Reset Ok, now let's request creation of index file...\r\n");
                    iRetries = 10;
                    do
                    {
                        for (uchHalt=60;uchHalt>0;--uchHalt)
                            Halt();
                        TxByte('I'); //End Update
                        bRet = WaitForRXData(certificateDone,2,3600,false,false,NULL,0); //Wait up to 1 minute, certificate index creation takes time
                        iRetries--;
                    }
                    while ((!bRet)&&(iRetries));
                    if (bRet)
                        printf("\rDone!                                \r\n");
                    else
                        printf("\rDone, but time-out on creating certificates index file!\r\n");
                }
                else
                    printf("\rDone!                              \r\n");
                break;
            }
            else
                if (!ucRetries)
                    printf("\rTimeout error\r\n");
        }
    }
    while (ucRetries);

    return;
}

int main(char** argv, int argc)
{
	unsigned char tx_data = 0;
	unsigned char rx_data;
	unsigned char speed = 0;
	unsigned char ucAPs,ucIndex;
	unsigned char ucPWD[65];
	unsigned int uiCMDLen;
	AP stAP[100];
	unsigned int i = 0;
	unsigned int ii = 0;
	int iFile;
    Z80_registers regs;
    unsigned long SentFileSize;
    unsigned char chFileSize[30];
    unsigned char chAPStsInfo[40];
    unsigned int FileRead;
    unsigned char ucFirstBlock = 1;
    bool bResponse = false;
    unsigned char ucRetries;
    unsigned char ucHalt;
    unsigned char ucTimeOutMSB;
    unsigned char ucTimeOutLSB;
    unsigned char ucScanPage;
    unsigned char ucPageLimit;
    unsigned char ucPageCheck;
    unsigned char ucVerMajor;
    unsigned char ucVerMinor;
    unsigned char ucAPstsRspSize;
    unsigned int uiAnimationTimeOut;
    unsigned char ucServer[300];
    unsigned char ucFile[300];
    unsigned char ucPort[6];

    //Global Variables Initialization is not working with current DOS CRT
    ucLocalUpdate = 0;
    ucNagleOff = 0;
    ucNagleOn = 0;
    ucRadioOff = 0;
    ucSetTimeout = 0;
    ucScanPage = 0;
    ucPageLimit = 0;
    ucVerMajor = 0;
    ucVerMinor = 0;
    TickCount = 0; //this guarantees no leap for 18 minutes, more than enough so we do not need to check for jiffy leaping

	printf("> SM-X ESP8266 Wi-Fi Module Configuration v1.30 <\r\n(c) 2020 Oduvaldo Pavan Junior - ducasp@gmail.com\r\n\n");

    if (IsValidInput(argv, argc, ucServer, ucFile, ucPort))
    {
        do
        {
            //Set Speed
            myPort6 = speed;
            ClearUartData();
            Halt();
            TxByte('?');

            bResponse = WaitForRXData(responseOK,2,60,false,false,NULL,0);

            if (bResponse)
                break; //found speed which ESP replied
            ++speed;
        }
        while (speed<10);

        if (speed<10)
        {
            printf ("Baud Rate: %s\r\n",speedStr[speed]);
            TxByte('V'); //Request version
            bResponse = WaitForRXData(versionResponse,1,20,true,false,NULL,0);
            if (bResponse)
            {
                while(!UartRXData());
                ucVerMajor = GetUARTData();
                while(!UartRXData());
                ucVerMinor = GetUARTData();
            }
            printf ("FW Version: %c.%c\r\n",ucVerMajor+'0',ucVerMinor+'0');

            if ((ucScan)||(ucNagleOff)||(ucNagleOn)||(ucRadioOff)||(ucSetTimeout))
            {
                //Scan and choose network to connect
                if (ucScan)
                {
                    if ((ucVerMajor>=1)&&(ucVerMinor>=2)) // new firmware allow get current ap and connection status
                    {
                        TxByte('g'); //Request current AP status
                        bResponse = WaitForRXData(apstsResponse,3,30,true,false,NULL,0);
                        if (bResponse)
                        {
                            while(!UartRXData());
                            ucAPstsRspSize=GetUARTData();
                            i = 0;
                            do
                            {
                                while(!UartRXData());
                                chAPStsInfo[i]=GetUARTData();
                                ++i;
                            }
                            while(i<ucAPstsRspSize);
                        }
                    }
                    TxByte('S'); //Request SCAN
                }
                else if (ucNagleOff)
                    TxByte('N'); //Request nagle off for future connections
                else if (ucNagleOn)
                    TxByte('D'); //Request nagle on for future connections
                else if (ucRadioOff)
                    TxByte('O'); //Request to turn off Wi-Fi radio immediately
                else if (ucSetTimeout)
                {
                    ucTimeOutMSB = ((unsigned char)((uiTimeout&0xff00)>>8));
                    ucTimeOutLSB = ((unsigned char)(uiTimeout&0xff));
                    if (uiTimeout)
                        printf("\r\nSetting Wi-Fi idle timeout to %u...\r\n",uiTimeout);
                    else
                        printf("\r\nSetting Wi-Fi to always on!\r\n");
                    TxByte('T'); //Request to set time-out
                    TxByte(0);
                    TxByte(2);
                    TxByte(ucTimeOutMSB);
                    TxByte(ucTimeOutLSB);
                }

                if (ucScan)
                    bResponse = WaitForRXData(scanResponse,2,60,true,false,NULL,0);
                else if (ucNagleOff)
                    bResponse = WaitForRXData(nagleoffResponse,2,60,true,false,NULL,0);
                else if (ucNagleOn)
                    bResponse = WaitForRXData(nagleonResponse,2,60,true,false,NULL,0);
                else if (ucRadioOff)
                    bResponse = WaitForRXData(radioOffResponse,2,60,true,false,NULL,0);
                else if (ucSetTimeout)
                    bResponse = WaitForRXData(responseRadioOnTimeout,2,60,true,false,NULL,0);


                if ((bResponse)&&(ucScan))
                {
                    ucRetries = 20;
                    do
                    {
                        --ucRetries;
                        for (ucHalt = 30;ucHalt>0;--ucHalt)
                            Halt();
                        TxByte('s'); //Request SCAN result
                        bResponse = WaitForRXData(scanresResponse,2,60,false,false,scanresNoNetwork,2); //Wait up to 60 ticks
                    }
                    while ((ucRetries)&&(!bResponse));

                    if (bResponse==1)
                    {
                        //Ok, now we have the number of APs to expect
                        while(!UartRXData());
                        ucAPs = GetUARTData();
                        if (ucAPs>100)
                            ucAPs=100;
                        tx_data = 0;
                        printf ("\r\n");
                        do
                        {
                            ucIndex = 0;
                            do
                            {
                                while(!UartRXData());
                                rx_data=GetUARTData();
                                stAP[tx_data].APName[ucIndex++]=rx_data;
                            }
                            while(rx_data!=0);
                            while(!UartRXData());
                            rx_data=GetUARTData();
                            stAP[tx_data].isEncrypted = (rx_data == 'E') ? 1 : 0;
                            ++tx_data;
                        }
                        while (tx_data!=ucAPs);
                        ClearUartData();

                        do
                        {
                            Cls();
                            printf("%s%s\r\n\n",strAPSts[chAPStsInfo[0]],&chAPStsInfo[1]);
                            printf("Choose AP:\r\n\n");

                            ucIndex = scanPageLimit*ucScanPage;

                            if ((ucAPs-ucIndex)<=scanPageLimit)
                                ucPageCheck = ucAPs;
                            else
                                ucPageCheck = ucIndex + scanPageLimit;

                            for (;ucIndex<ucPageCheck;ucIndex++)
                            {
                                printf("%u - %s",(ucIndex%scanPageLimit),stAP[ucIndex].APName);
                                if (stAP[ucIndex].isEncrypted)
                                    printf(" (PWD)\r\n");
                                else
                                    printf(" (OPEN)\r\n");
                            }

                            if (ucAPs-ucIndex) // still APs left to list?
                                printf("\r\nWhich one to connect? (ESC exit/SPACE BAR next page)");
                            else
                                printf("\r\nWhich one to connect? (ESC exit)");

                            do
                            {
                                tx_data = Inkey ();

                                if (tx_data==0x1b)
                                    break;

                                if ((tx_data==' ')&&(ucAPs-ucIndex))
                                    break;

                                if ((tx_data>='0')&&(tx_data<='9'))
                                {
                                    if (((tx_data-'0')<scanPageLimit)&&(((scanPageLimit*ucScanPage)+(tx_data-'0'))<ucAPs))
                                        break;
                                }
                                if (tx_data)
                                    Beep();
                            }
                            while (1);

                            if ((tx_data!=0x1b)&&(tx_data!=' ')) // AP Choosen?
                            {
                                // Yes
                                printf(" %c\r\n\n",tx_data); // Print accepted char
                                ucIndex = (scanPageLimit*ucScanPage) + (tx_data-'0');
                                if (stAP[ucIndex].isEncrypted)
                                {
                                    //GET AP password
                                    printf("Password? ");
                                    InputString(ucPWD,64);
                                    printf("\r\n");
                                }

                                printf("Connecting to: %s \r\n",stAP[ucIndex].APName);

                                uiCMDLen = strlen(stAP[ucIndex].APName) + 1;
                                if (stAP[ucIndex].isEncrypted)
                                    uiCMDLen += strlen(ucPWD);
                                TxByte('A'); //Request connect AP
                                TxByte((unsigned char)((uiCMDLen&0xff00)>>8));
                                TxByte((unsigned char)(uiCMDLen&0xff));
                                rx_data = 0;
                                do
                                {
                                    tx_data = stAP[ucIndex].APName[rx_data];
                                    TxByte(tx_data);
                                    --uiCMDLen;
                                    ++rx_data;
                                }
                                while((uiCMDLen)&&(tx_data!=0));
                                if(uiCMDLen)
                                {
                                    rx_data = 0;
                                    do
                                    {
                                        tx_data = ucPWD[rx_data];
                                        TxByte(tx_data);
                                        --uiCMDLen;
                                        ++rx_data;
                                    }
                                    while(uiCMDLen);
                                }

                                //Command sent, done, just wait response
                                bResponse = WaitForRXData(apconfigurationResponse,2,600,true,false,NULL,0); //Wait up to 10s
                                if (bResponse)
                                    printf("Success, AP configured to be used.\r\n");
                                else
                                {
                                    if ((ucVerMajor>=1)&&(ucVerMinor>=2)) // new firmware allow get current ap and connection status
                                    {
                                        for (i=90;i>0;--i)
                                            Halt();
                                        TxByte('g'); //Request current AP status
                                        bResponse = WaitForRXData(apstsResponse,3,120,true,false,NULL,0);
                                        if (bResponse)
                                        {
                                            while(!UartRXData());
                                            ucAPstsRspSize=GetUARTData();
                                            i = 0;
                                            do
                                            {
                                                while(!UartRXData());
                                                chAPStsInfo[i]=GetUARTData();
                                                ++i;
                                            }
                                            while(i<ucAPstsRspSize);

                                            if (chAPStsInfo[0]==2)
                                                printf("Error, wrong password!\r\n");
                                            else
                                                printf("Error, if protected network, check password.\r\n");
                                        }
                                        else
                                            printf("Error, if protected network, check password.\r\n");
                                    }
                                    else
                                        printf("Error, if protected network, check password.\r\n");
                                }

                                break;
                            }
                            else if (tx_data==0x1b)
                            {
                                printf("\r\nUser canceled by ESC key...\r\n");
                                break;
                            }
                            else // space bar when next page is available, so increase page
                            {
                                if ((ucVerMajor>=1)&&(ucVerMinor>=2)) // new firmware allow get current ap and connection status
                                {
                                    TxByte('g'); //Request current AP status
                                    bResponse = WaitForRXData(apstsResponse,3,30,true,false,NULL,0);
                                    if (bResponse)
                                    {
                                        while(!UartRXData());
                                        ucAPstsRspSize=GetUARTData();
                                        i = 0;
                                        do
                                        {
                                            while(!UartRXData());
                                            chAPStsInfo[i]=GetUARTData();
                                            ++i;
                                        }
                                        while(i<ucAPstsRspSize);
                                    }
                                }
                                ++ucScanPage;
                            }
                        }
                        while(1);
                    }
                    else
                        printf("\r\nScan results: no answer...\r\n");
                }
                else
                {
                    if (ucScan)
                        printf ("\rScan request: no answer...\r\n");
                    else if (((ucNagleOff)||(ucNagleOn))&&(bResponse))
                    {
                        printf("\rNagle set as requested...\r\n");
                        return 0;
                    }
                    else if ((ucNagleOff)||(ucNagleOn))
                    {
                        printf("\rNagle not set as requested, error!\r\n");
                        return 0;
                    }
                    else if (ucRadioOff)
                    {
                        if (bResponse)
                            printf("\rRequested to turn off Wi-Fi Radio...\r\n");
                        else
                            printf("\rRequest to turnoff Wi-Fi Radio error!\r\n");
                        return 0;
                    }
                    else if (ucSetTimeout)
                    {
                        if (bResponse)
                            printf("\rWi-Fi radio on Time-out set successfully...\r\n");
                        else
                            printf("\rError setting Wi-Fi radio on Time-out!\r\n");
                        return 0;
                    }
                }
            }
            else if (ucLocalUpdate)
            {
                //ok, we are going to try to update fw from local file
                iFile = Open (ucFile,O_RDONLY);
                //Could open the file?
                if (iFile!=-1)
                {
                    // Why not use _size from fusion-c?
                    // Because it is not DOS2 compatible, and we use DOS2
                    // Why not use Lseek from fusion-c?
                    // Although it calls 0x4A in DOS 2, it won't update the pointer
                    // with the current position
                    regs.Words.HL = 0; //set pointer as 0
                    regs.Words.DE = 0; //so it will return the position
                    regs.Bytes.A = 2; //relative to the end of file, i.e.:file size
                    regs.Bytes.B = (unsigned char)(iFile&0xff);
                    DosCall(0x4A, &regs, REGS_ALL, REGS_ALL); // MOVE FILE HANDLER
                    if (regs.Bytes.A == 0) //moved, now get the file handler position, i.e.: size
                        SentFileSize = (unsigned long)(regs.Words.HL)&0xffff | ((unsigned long)(regs.Words.DE)<<16)&0xffff0000;
                    else
                        SentFileSize = 0;
                    // Convert to string
                    ultostr(SentFileSize,chFileSize,10);
                    Close(iFile);
                    printf ("File: %s Size: %s \r\n",ucFile,chFileSize);
                    if (SentFileSize)
                    {
                        iFile = Open (ucFile,O_RDONLY);
                        if (iFile!=-1)
                        {
                            FileRead = MyRead(iFile, ucServer,256); //try to read 256 bytes of data
                            if (FileRead == 256)
                            {
                                //Now request to start update over serial
                                if (ucIsFw)
                                    TxByte('Z'); //Request start of RS232 update
                                else
                                    TxByte('Y'); //Request start of RS232 cert update
                                TxByte(0);
                                TxByte(12);
                                TxByte((unsigned char)(SentFileSize&0xff));
                                TxByte((unsigned char)((SentFileSize&0xff00)>>8));
                                TxByte((unsigned char)((SentFileSize&0xff0000)>>16));
                                TxByte((unsigned char)((SentFileSize&0xff000000)>>24));
                                TxByte((unsigned char)((SentFileSize&0xff00000000)>>32));
                                TxByte((unsigned char)((SentFileSize&0xff0000000000)>>40));
                                TxByte((unsigned char)((SentFileSize&0xff000000000000)>>48));
                                TxByte((unsigned char)((SentFileSize&0xff00000000000000)>>56));
                                TxByte(ucServer[0]);
                                TxByte(ucServer[1]);
                                TxByte(ucServer[2]);
                                TxByte(ucServer[3]);

                                if (ucIsFw)
                                    bResponse = WaitForRXData(responseRSFWUpdate,2,60,true,false,NULL,0);
                                else
                                    bResponse = WaitForRXData(responseRSCERTUpdate,2,60,true,false,NULL,0);

                                if (!bResponse)
                                    printf("Error requesting to start firmware update.\r\n");
                                else
                                {
                                    uiAnimationTimeOut = TickCount + 9;
                                    do
                                    {
                                        --uiAnimationTimeOut;
                                        if (TickCount>=uiAnimationTimeOut)
                                        {
                                            uiAnimationTimeOut = 9;
                                            //Our nice animation to show we are not stuck
                                            printf("%s",advance[i%5]); // next animation step
                                            ++i;
                                        }
                                        if (!ucFirstBlock)
                                        {
                                            FileRead = MyRead(iFile, ucServer,256); //try to read 256 bytes of data
                                            if (FileRead ==0)
                                            {
                                                printf("\rError reading file...\r\n");
                                                break;
                                            }
                                        }
                                        else
                                            ucFirstBlock = 0;
                                        //Send the block
                                        TxByte('z'); //Write block
                                        TxByte((unsigned char)((FileRead&0xff00)>>8));
                                        TxByte((unsigned char)(FileRead&0xff));
                                        for (ii=0;ii<256;ii++)
                                            TxByte(ucServer[ii]);

                                        bResponse = WaitForRXData(responseWRBlock,2,600,true,false,NULL,0);

                                        if (!bResponse)
                                        {
                                            printf("\rError requesting to write firmware block.\r\n");
                                            break;
                                        }
                                        SentFileSize = SentFileSize - FileRead;
                                    }
                                    while(SentFileSize);
                                    printf("%s",aDone);

                                    //if here and last command was not error, time to finish flashing
                                    if (bResponse)
                                        FinishUpdate(false);
                                }
                            }
                            else
                                Print("\rError reading firmware file!\r\n");
                            Close(iFile);
                        }
                        else
                        {
                            printf("Error, couldn't open %s ...\r\n",ucFile);
                            return 0;
                        }
                    }
                    else
                    {
                        printf("Error, %s is 0 bytes long...\r\n",ucFile);
                        return 0;
                    }
                }
                else
                {
                    printf("Error, couldn't open %s ...\r\n",ucFile);
                    return 0;
                }
            }
            else //ok, we are going to try to update fw
            {
                if (ucIsFw)
                    printf ("Ok, updating FW using server: %s port: %u\r\nFile path: %s\nPlease Wait, it can take up to a few minutes!\r\n",ucServer,uiPort,ucFile);
                else
                    printf ("Ok, updating certificates using server: %s port: %u\r\nFile path: %s\nPlease Wait, it can take up to a few minutes!\r\n",ucServer,uiPort,ucFile);
                uiCMDLen = strlen(ucServer) + 3; //3 = 0 terminator + 2 bytes port
                uiCMDLen += strlen(ucFile);
                if (ucIsFw)
                    TxByte('U'); //Request Update Main Firmware remotely
                else
                    TxByte('u'); //Request Update spiffs remotely
                TxByte((unsigned char)((uiCMDLen&0xff00)>>8));
                TxByte((unsigned char)(uiCMDLen&0xff));
                TxByte((unsigned char)(uiPort&0xff));
                TxByte((unsigned char)((uiPort&0xff00)>>8));
                rx_data = 0;
                do
                {
                    tx_data = ucServer[rx_data];
                    TxByte(tx_data);
                    --uiCMDLen;
                    ++rx_data;
                }
                while((uiCMDLen)&&(tx_data!=0));
                rx_data = 0;
                do
                {
                    tx_data = ucFile[rx_data];
                    if (tx_data==0)
                        break;
                    TxByte(tx_data);
                    --uiCMDLen;
                    ++rx_data;
                }
                while(uiCMDLen);

                if (ucIsFw)
                    bResponse = WaitForRXData(responseOTAFW,2,18000,true,false,NULL,0);
                else
                    bResponse = WaitForRXData(responseOTASPIFF,2,18000,true,false,NULL,0);

                if (bResponse)
                {
                    if ((!ucIsFw))
                        printf("\rSuccess updating certificates!\r\n");
                    else
                        printf("\rSuccess, firmware updated, wait a minute so it is fully flashed.\r\n");
                    FinishUpdate(true);
                    return 0;
                }
                else
                    printf("\rFailed to update from remote server...\r\n");
            }
        }
        else
            printf("ESP device not found...\r\n");
    }
    else
        printf(strUsage);

	return 0;
}
