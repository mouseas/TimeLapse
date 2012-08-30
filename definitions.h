#ifndef TIMELAPSE_H_
#define TIMELAPSE_H_

#include "msp430.h"
#include <stdlib.h>

typedef char int8;
typedef int int16;
typedef long int32;

typedef unsigned char uint8;
typedef unsigned int uint16;
typedef unsigned long uint32;

//********************************************************
// constant definitions

#define FALSE	0
#define TRUE	1

#define MY_CLOCK 1000000			// clock speed
#define WDT_CLOCK 32000				// 32 Khz WD clock
#define WDT_CTL WDT_MDLY_32			// WDT control; 32 Khz, delay mode, SMCLK
#define WDT_IPS MY_CLOCK/WDT_CLOCK	// Interrupts per second

#define DEBOUNCE_CNT 20				// 20 watchdog cycles to debounce a switch.
#define ON_OFF_THRESHOLD 5			// 5 seconds before taking a picture, turn the camera on.

// P1.0-1.7
#define sw1 0x01
#define sw2 0x02
#define sw3 0x04
#define sw4 0x08
#define sw5 0x10
#define sw6 0x20
#define takePicButton 0x40
#define onOffButton 0x80

#define sw1to6 sw1 | sw2 | sw3 | sw4 | sw5 | sw6

//P2.0-2.2
#define sw7 0x01
#define sw8 0x02
#define sw9 0x04					// interrupt button

#define sw7to8 sw7 | sw8

//****************************************************************
// System Event definitions

volatile uint16 sysEvent;			// event register

#define checkTimeSettings	0x0001
#define takePictureStart	0x0002
#define takePictureEnd		0x0004
#define turnCameraOnStart	0x0008
#define turnCameraOnEnd		0x0010
#define turnCameraOffStart	0x0020
#define turnCameraOffEnd	0x0040
#define doButtons			0x0080

volatile char switchesP1;			// P1 switches state
volatile char switchesP2;			// p2 switches state

//****************************************************************
// Variable and function definitions

volatile char cameraOn;				// boolean: whether the camera is on
volatile char mode;					// boolean: FALSE for Seconds, TRUE for Minutes

volatile int debounce;				// switch debounce delay
volatile int WDTDelay;				// delay used by WDTPause()
volatile int cameraOffDelay;		// delay after taking a picture and before turning the camera off.

volatile int clockCyclesLeft;		// how many WDT cycles until the end of a second
volatile int secondsLeft;			// how many seconds until the camera should take a picture
volatile int minutesLeft;			// how many minutes until the camera should take a picture

void WDTPause(int length);			// pause using the WDT for a short time.
void readIntervalSettings();		// read switches 1-8 to determine the length of the timing interval.


#endif /*TIMELAPSE_H_*/
