/*
--
-- 16C550CZiModem.c
--
--   Quick Access to 16C550 on 0x80 - 0x87.
--   Revision 0.10
--
-- Requires SDCC and Fusion-C library to compile
-- Copyright (c) 2020 Oduvaldo Pavan Junior ( ducasp@ gmail.com )
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
#include <string.h>
#include <stdlib.h>
#include "../../fusion-c/header/msx_fusion.h"
#include "16C550CZiModem.h"

//Our big internal FIFO
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 512
#endif

// we want those variables to not stay in first page, otherwise
// our hook won't be able to access them, look at memory map and
// use realloc if needed
__at 0x8000 unsigned char uchrxbuffer[BUFFER_SIZE + 20];
__at (0x8000+ BUFFER_SIZE + 20) unsigned char ucHookBackup[5];
__at (0x8000+ BUFFER_SIZE + 25) unsigned char ucRTSAsserted;
__at (0x8000+ BUFFER_SIZE + 26) unsigned char * BufferTop;
__at (0x8000+ BUFFER_SIZE + 28) unsigned char * Top;
__at (0x8000+ BUFFER_SIZE + 30) unsigned char * Bottom;
__at (0x8000+ BUFFER_SIZE + 32) unsigned int iFree;
__at (0x8000+ BUFFER_SIZE + 34) unsigned char interruptRealloc[512];


unsigned char AFESupport;

unsigned char check16C550C(void)
{
    unsigned char Ret = NOUART;
    unsigned char ucTest;

    AFESupport = 0;

    // First check scratchpad, if it doesn't exist, done
    ucTest = mySR;
    ucTest += 2;
    mySR = ucTest;
    // If matches the new value
    if (ucTest == mySR)
    {
        //Ok, we have a scratch register, now check for FIFO
        myIIR_FCR = 0x07; // Clear FIFO, enable FIFO
        ucTest = myIIR_FCR&0xc0; //check 7th and 8th bit
        if (ucTest==0xc0)
        {
            //Hey, we have a FIFO! so this is at least a 16550
            Ret = U16C550;
            //Now test if AFE can be enabled
            myMCR = 0x22;
            ucTest = myMCR&0x20;
            if (ucTest == 0x20)
            {
                Ret = U16C550C;
                AFESupport = 1;
            }
        }
    }

    return Ret;
}

// Program interrupt handler to call our routine first
void programInt(void)
{
    if (AFESupport)
    {
__asm
    ; Disable interrupts, it will not be nice if an interrupt occurs before all
    ; was properly set
    DI
    ; Save the registers we are going to use
    PUSH BC
    PUSH DE
    PUSH HL
    ; This is the area where we are going to save the original interrupt handler
    ; Why? It is the beginning of the routine that is called before main is
    ; executed and it will not be called anymore, so we put it to good use
    ; First copy our interrupt routine to _interruptRealloc
    LD DE,#_interruptRealloc
    ; This is the address Z80 will call everytime interrupts are triggered
    LD HL,#_myAFEIntHandler
    ; Counter of how many bytes need to be transferred
    LD BC,#_enterIntMode - _myAFEIntHandler
    ; Copy data
    LDIR

    LD DE,#_ucHookBackup
    ; This is the address Z80 will call everytime interrupts are triggered
    LD HL,#0xFD9A
    ; Counter of how many bytes need to be transferred
    LD BC,#0x5
    ; Copy data
    LDIR
    ; HL changes after LDIR, so restore it to the interrupt handler address
    LD HL,#0xFD9A
    ; First we will put the JP command
    LD B,#0xC3
    ; Now, put that into the memory
    LD (HL),B
    ; Increment our memory copy address
    INC HL
    ; Load the memory address of our function in BC pair of registers
    LD BC,#_interruptRealloc
    ; Move the LSB of the address just after JP
    LD (HL),C
    ; Increment our memory copy address
    INC HL
    ; Move the MSB of the address after the LSB
    LD (HL),B
    ; We are done, restore registers we have used
    POP HL
    POP DE
    POP BC
    ; Enable interrupts again
    EI
__endasm;
    }
    else
    {
__asm
    ; Disable interrupts, it will not be nice if an interrupt occurs before all
    ; was properly set
    DI
    ; Save the registers we are going to use
    PUSH BC
    PUSH DE
    PUSH HL
    ; This is the area where we are going to save the original interrupt handler
    ; Why? It is the beginning of the routine that is called before main is
    ; executed and it will not be called anymore, so we put it to good use
    ; First copy our interrupt routine to _interruptRealloc
    LD DE,#_interruptRealloc
    ; This is the address Z80 will call everytime interrupts are triggered
    LD HL,#_myIntHandler
    ; Counter of how many bytes need to be transferred
    LD BC,#_myAFEIntHandler - _myIntHandler
    ; Copy data
    LDIR

    LD DE,#_ucHookBackup
    ; This is the address Z80 will call everytime interrupts are triggered
    LD HL,#0xFD9A
    ; Counter of how many bytes need to be transferred
    LD BC,#0x5
    ; Copy data
    LDIR
    ; HL changes after LDIR, so restore it to the interrupt handler address
    LD HL,#0xFD9A
    ; First we will put the JP command
    LD B,#0xC3
    ; Now, put that into the memory
    LD (HL),B
    ; Increment our memory copy address
    INC HL
    ; Load the memory address of our function in BC pair of registers
    LD BC,#_interruptRealloc
    ; Move the LSB of the address just after JP
    LD (HL),C
    ; Increment our memory copy address
    INC HL
    ; Move the MSB of the address after the LSB
    LD (HL),B
    ; We are done, restore registers we have used
    POP HL
    POP DE
    POP BC
    ; Enable interrupts again
    EI
__endasm;
    }
}

// Restore interrupt handler so we won't be called before processing interrupt
// Useful when exiting the program, as once our program no longer runs, it won't
// proccess anything and interrupts will go crazy if handler is not restored
void restoreInt(void)
{
__asm
    ; Disable interrupts, it will not be nice if an interrupt occurs before all
    ; was properly set
    DI
    ; Save the registers we are going to use
    PUSH BC
    PUSH DE
    PUSH HL
    ; This is the area where we are going to restore the original interrupt handler
    LD DE,#0xFD9A
    ; This is the address where the original handler was saved
    LD HL,#_ucHookBackup
    ; 5 bytes
    LD BC,#0x5
    ; copy it
    LDIR
    ; We are done, restore registers we have used
    POP HL
    POP DE
    POP BC
    ; Enable interrupts again
    EI
__endasm;
}

// This function, if called, will check if our FIFO is the reason of interrupt
// and if it was, transfer all bytes stored in it to our memory. Once done,
// call the original interrupt handler.
// You might wonder what is a naked function... SDCC will not push or pull any
// registers, it will not return, etc... As interrupt call is a jump, that is
// just what we need, and we take care of saving whatever we change and restore
// it before jumping back to the original interrupt handler
//
// You want to be off the page 0, as bios will be switched in/out of it
void myIntHandler(void) __naked
{
__asm
    ;Check if it is our interrupt
    in a,(#0x82)
    bit 0,a
    ;If 1st bit is set, UART did not interrupt, done
	jp NZ,_ucHookBackup
    ; Assert RTS so other side will not send anything
    in a,(#0x84)
    and #0x0d
    out (#0x84),a
    ld hl,(#_Top)
    ld de,(#_iFree)
    jr 00007$
00002$:
    ; Check if there are more bytes in UART FIFO
    in a,(#0x85)
    bit 0,a
    ;If 1st bit is not set, no more data in UART FIFO, so we are done
    jr	Z,00005$
00007$:
    ;Ok, move data to register B
    in a,(#0x80)
    ; Copy the data we got to memory
    ld (hl),a
    ; Now deal with FIFO variables
    ; 1st - Check if Top = top RAM position
    ; Start with LSB
    ld a,(#_BufferTop)
    cp l
    ; if LSB not equal, it is different and no need to check MSB
    jr NZ,00003$
    ; LSB equal, and MSB?
    ld a,(#_BufferTop + 1)
    cp h
    ; if MSB not equal, it is different
    jr NZ,00003$
    ; Equal, so we will leap here and Top goes back to index 0
    ld hl,#_uchrxbuffer
    jr 00004$
00003$:
    ; Different, so just add 1 to current Top value, cannot add DE, so move to HL
    inc hl
00004$:
    ; 2nd - Update iFree
    dec de ;dec DE will not update flags :-(
    ld a,e
    or d
    jr NZ,00002$ ; If not zero, we are good to continue
    ; 0 bytes free? full
    ; First save iFree (0)
    ld (#_iFree),de
    ; Now Save Top
    ld (#_Top),hl
    ; Full
    ; Now leave with RTS asserted and disable interrupts
    jr 00012$

00005$:
    ; First save iFree
    ld (#_iFree),de
    ; Now Save Top
    ld (#_Top),hl
    ; check if we have at least 10 bytes free
    ld a,e
    ld l,#10
    sub l
    ld l,a
    ; if no carry, we have enough space
    jr NC,00001$
    ; carry, continue sub
    ld h,#0
    ld a,d
    sbc a,h
    ; if carry, not enough space, so exit and do not de-assert RTS
    jr C,00012$
00001$:
    ; De-assert RTS so other side can send stuff again
    in a,(#0x84)
    or #0x02
    out (#0x84),a
00006$:
    jp _ucHookBackup
00012$:
    ; carry, so keep RTS asserted and disable UART interrupts so application can get from the buffer
    ld  a,#1
    ld (_ucRTSAsserted),a
    xor a
    out (#0x81),a
    jp _ucHookBackup
__endasm;
}

// This function, if called, will check if our FIFO is the reason of interrupt
// and if it was, transfer all bytes stored in it to our memory. Once done,
// call the original interrupt handler.
// You might wonder what is a naked function... SDCC will not push or pull any
// registers, it will not return, etc... As interrupt call is a jump, that is
// just what we need, and we take care of saving whatever we change and restore
// it before jumping back to the original interrupt handler
//
// You want to be off the page 0, as bios will be switched in/out of it
void myAFEIntHandler(void) __naked
{
__asm
    ;Check if it is our interrupt
    in a,(#0x82)
    bit 0,a
    ;If 1st bit is set, UART did not interrupt, done
	jp NZ,_ucHookBackup
    ld hl,(#_Top)
    ld de,(#_iFree)
    jr 00007$
00002$:
    ; Check if there are more bytes in UART FIFO
    in a,(#0x85)
    bit 0,a
    ;If 1st bit is not set, no more data in UART FIFO, so we are done
    jr	Z,00005$
00007$:
    ;Ok, move data to register B
    in a,(#0x80)
    ; Copy the data we got to memory
    ld (hl),a
    ; Now deal with FIFO variables
    ; 1st - Check if Top = top RAM position
    ; Start with LSB
    ld a,(#_BufferTop)
    cp l
    ; if LSB not equal, it is different and no need to check MSB
    jr NZ,00003$
    ; LSB equal, and MSB?
    ld a,(#_BufferTop + 1)
    cp h
    ; if MSB not equal, it is different
    jr NZ,00003$
    ; Equal, so we will leap here and Top goes back to index 0
    ld hl,#_uchrxbuffer
    jr 00004$
00003$:
    ; Different, so just add 1 to current Top value, cannot add DE, so move to HL
    inc hl
00004$:
    ; 2nd - Update iFree
    dec de ;dec DE will not update flags :-(
    ld a,e
    or d
    jr NZ,00002$ ; If not zero, we are good to continue
    ; 0 bytes free? full
    ; First save iFree (0)
    ld (#_iFree),de
    ; Now Save Top
    ld (#_Top),hl
    ; Full
    ; Now leave with interrupts disabled
    jr 00012$

00005$:
    ; First save iFree
    ld (#_iFree),de
    ; Now Save Top
    ld (#_Top),hl
    ; check if we have at least 10 bytes free
    ld a,e
    ld l,#10
    sub l
    ld l,a
    ; if no carry, we have enough space
    jr NC,00001$
    ; carry, continue sub
    ld h,#0
    ld a,d
    sbc a,h
    ; if carry, not enough space, so exit and deactivate INT
    jr C,00012$
00001$:
00006$:
    jp _ucHookBackup
00012$:
    ; carry, so disable UART interrupts so application can get from the buffer
    ld  a,#1
    ld (_ucRTSAsserted),a
    xor a
    out (#0x81),a
    jp _ucHookBackup
__endasm;
}


// Just set everything for our interrupt handling routine to be called
// and then enable ESP UART FIFO to interrupt us when there is data.
void enterIntMode(void)
{
    // our RAM FIFO control variables are reset
    Top = uchrxbuffer;
    Bottom = uchrxbuffer;
    ucRTSAsserted = 0;
    iFree = BUFFER_SIZE;
    BufferTop = uchrxbuffer + BUFFER_SIZE - 1;
    //Assert RTS so other side won't send anything
    myMCR = myMCR & 0x0d;
    //Enable Fifo, 8 bytes fifo level trigger and Clear Uart FIFOs
    myIIR_FCR = 0x87;
    //Set our Interrupt Handler
    programInt();
    //Enable 8N1 and also DLAB
    myLCR = 0x83;
    //For now 57600, so divide by 2
    myRBR_THR = 2;
    myIER = 0;
    //Enable 8N1 and disable DLAB
    myLCR = 0x03;
    //Enable interrupt
    myIER = 0x01;
    if(AFESupport)
        //Turn on AFE
        myMCR = myMCR | 0x22;
    else
        //De-assert RTS so other side can send
        myMCR = myMCR | 0x02;
}

// Disable UART interruptions and restore original interrupt handler
void exitIntMode(void)
{
    //Set Interrupt Mode Off
    myIER = 0x00;
    Halt();
    //Enable Fifo, 8 bytes fifo level trigger and Clear Uart
    myIIR_FCR = 0x87;
    restoreInt();
}

unsigned char UartFIFOFull(void)
{
    if (iFree == 0)
        return 1;
    else
        return 0;
}

unsigned char UartTXInprogress(void)
{
    if(myLSR&0x20)
        return 0;
    else
        return 1;
}

unsigned char UartRXData(void)
{
    if (iFree != BUFFER_SIZE)
        return 1;
    else
        return 0;
}

unsigned char GetUARTData(void)
{
    unsigned char ret = 0xff;

    if (iFree != BUFFER_SIZE )
    {
__asm
    di
__endasm;
        ++iFree;
__asm
    ei
__endasm;
        ret = *Bottom;

        if (Bottom < BufferTop)
            ++Bottom;
        else
            Bottom = uchrxbuffer;

        if ((ucRTSAsserted)&&(iFree>80))
        {
            ucRTSAsserted = 0;
__asm
    di
__endasm;
            //Re-enable interrupts
            myIER = 0x01;
            if (!AFESupport)
                //De-assert RTS so other side can send
                myMCR = myMCR | 0x02;
__asm
    ei
__endasm;
        }
    }
    return ret;
}

unsigned int GetReceivedBytes(void)
{
    return (BUFFER_SIZE - iFree);
}

unsigned char U16550CTxByte(char chTxByte)
{
	unsigned char ret = 0;
	unsigned char UartStatus;
	unsigned char Leaping;
	unsigned int Retries;

	Retries = TickCount + 3; //Wait up to 3 Interrupts
	if (Retries<TickCount) //Leaping?
		Leaping = 1;
	else
		Leaping = 0;

	do
	{
		UartStatus = myLSR&0x20 ;
		if (!UartStatus)
		{
			if (Leaping)
			{
				if (TickCount<10)
				{
					Leaping = 0;
					if (TickCount>Retries)
						break;
				}
				else
					if (TickCount>Retries)
						break;
			}
			else
				if (TickCount>Retries)
					break;
		}
		else
		{
			myRBR_THR = chTxByte;
			ret = 1;
			break;
		}
	}
	while (1);

#ifdef log_verbose
	if (UartStatus)
		Print("> UART Status- TX Stuck after 3 interrupts...\n");
#endif

	return ret;
}

unsigned char U16550CTxData(char * chData, unsigned char Size)
{
	unsigned char ret = 1;

	unsigned char i;

	if(Size==0)
	{
		for (i = 0; (chData[i]!=0 && ret == 1); ++i)
			ret	= U16550CTxByte(chData[i]);
	}
	else
	{
		for (i = 0; ( i < Size && ret == 1); ++i)
			ret	= U16550CTxByte(chData[i]);
	}


	return ret;
}
