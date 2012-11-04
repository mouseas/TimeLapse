/*
 * Controls a small digital camera to take photos at regular intervals.
 * Reads an array of 8 switches to determine the interval length.
 *
 * Written by Martin Carney (mouseasw@gmail.com)
 *
 *		            MSP430G2452
 *		            -----_-----
 *		    +3.55v |VCC+ U GND-| -GND
 *	   (LED1)  SW1 |P1.0    XIN|
 *		       SW2 |P1.1   XOUT|
 *		       SW3 |P1.2   TEST|
 *		       SW4 |P1.3    RST| +3.55v
 *		       SW5 |P1.4   P1.7|-> to camera SW-A (on/off/mode)
 *		       SW6 |P1.5   P1.6|-> to camera SW-B (photo/record) (LED2)
 *		       SW7 |P2.0   P2.5| debug 3 red
 *		       SW8 |P2.1   P2.4| debug 2 yellow
 *	  (button) SW9 |P2.2   P2.3| debug 1 green
 *		            -----------
 *
 * Switches:
 * (base interval: 1)
 * SW1:	+1
 * SW2: +2
 * SW3: +4
 * SW4: +8
 * SW5: +16
 * SW6: +32
 * SW7: +64
 * SW8: minute/second mode
 *
 * These allow intervals from 1 second to 128 minutes between pictures.
 *
 * SW9 (button): Reset timing
 *
 * LEDs will require 270 mohms (green), 482 mohms (red)...?
 */

#include "msp430.h"
#include "definitions.h"
#include <stdlib.h>

/**
 * Reads switches 1-8 and determines the number of minutes and seconds left
 * until the next exposure.
 * Each switch adds twice as much time as the previous one, except SW8, which
 * sets whether to use seconds (off) or minutes (on). Min value is 1, max is 128.
 */
void readIntervalSettings() {
	P1OUT |= sw1to6;	// enable pull-ups in order to sample switches
	P2OUT |= sw7to8;
//	_disable_interrupts();
	uint16 switchset1 = 0;
	switchset1 ^= P1IN & sw1to6; // read masked P1 input
	uint16 switchset2 = 0;
	switchset2 ^= P2IN & sw7to8; // read masked P2 input

	P1OUT &= ~sw1to6;	// disable pull-ups after sampling switches.
	P2OUT &= ~sw7to8;

	int value = 0;
	if (switchset1 & sw1) { value += 1; }
	if (switchset1 & sw2) { value += 2; }
	if (switchset1 & sw3) { value += 4; }
	if (switchset1 & sw4) { value += 8; }
	if (switchset1 & sw5) { value += 16; }
	if (switchset1 & sw6) { value += 32; }
	if (switchset2 & sw7) { value += 64; }
	if (switchset2 & sw8) {
		minutesLeft = value;
		secondsLeft = SEC_PER_MIN - 1;
	} else {
		minutesLeft = 0;
		secondsLeft = value;
	}
	clockCyclesLeft = WDT_IPS;
//	_enable_interrupts();
}

void main(void) {
	BCSCTL1 = CALBC1_1MHZ;       // Set range
	DCOCTL = CALDCO_1MHZ;        // Set DCO step and modulation

	WDTCTL = WDT_CTL;			// enable WDT as a 32 Khz timer
	IE1 |= WDTIE;				// enable WDT interrupts

	debounce = 0;
	cameraOffDelay = 0;

	clockCyclesLeft = WDT_IPS;
	secondsLeft = 0;
	minutesLeft = 0;
	sysEvent = checkTimeSettings;	// start by reading the interval settings

	cameraOn = FALSE;			// assume camera is OFF at start.

// Port 1 setup
	P1SEL = 0;					// set P1 as digital I/O for all pins
	P1DIR = ~sw1to6;			// set switches 1-6 for input, P1.6 and P1.7 for output.
	P1OUT = 0;					// disable pull-ups on switches 1-6 and P1.6 & P1.7
		// (These will be enabled right before reading the switch settings.
		// Same for switches 7-8, below)
	P1REN |= sw1to6;			// enable pull-up resistors on switches 1-6

// Port 2 setup
	P2SEL = ~(sw7to8 | sw9 | debug1 | debug2 | debug3);		// set P2 as digital I/O

	P2DIR &= ~(sw7to8 | sw9);
	P2DIR |= debug1 | debug2 | debug3;
	// set sw 7-9 for input, and debug1-3 for output

	P2OUT &= ~(sw7to8 | debug1 | debug2 | debug3);			// disable pull-ups on switches 7-8 (and debug lights)
	P2OUT |= sw9;				// enable pull-up on switch 9
	P2IES |= sw9;				// hi to low interrupt on switch 9
	P2REN |= sw7to8 | sw9;		// enable pull-up resistors on switches 7-9
	
	while(1) {
		_disable_interrupts();
		if (sysEvent) {
			_enable_interrupts();	// enable interrupts before proceeding
		} else {
			__bis_SR_register(LPM0_bits + GIE); // enable interrupts and go to sleep (LPM0).
		}

		//-----------------------
		// wake up and do stuff

		if (sysEvent & checkTimeSettings) {
			sysEvent &= ~checkTimeSettings;			// clear event bit.
			readIntervalSettings();					// set the delay until the next picture
		} else if (sysEvent & takePictureStart) {
			sysEvent &= ~takePictureStart;
			P1OUT |= takePicButton;					// turn on the pic button
		} else if (sysEvent & takePictureEnd) {
			sysEvent &= ~takePictureEnd;			// turn off the pic button
			P1OUT &= ~takePicButton;
		} else if (sysEvent & turnCameraOnStart) {
			sysEvent &= ~turnCameraOnStart;
			P1OUT |= onOffButton;					// turn on the on/off button
		} else if (sysEvent & turnCameraOnEnd) {
			sysEvent &= ~turnCameraOnEnd;
			P1OUT &= ~onOffButton;					// turn off the on/off button
		} else if (sysEvent & turnCameraOffStart) {
			sysEvent &= ~turnCameraOffStart;
			P1OUT |= onOffButton;					// turn on the on/off button
		} else if (sysEvent & turnCameraOffEnd) {
			sysEvent &= ~turnCameraOffEnd;
			P1OUT &= ~onOffButton;					// turn off the on/off button
		} else if (sysEvent & doButtons) {
			sysEvent &= ~doButtons;
			if (P1OUT & sw9) {						// if sw9 (controller on/off button) is pressed
				sysEvent |= shutDown;
			}
		} else {
			sysEvent = 0;		// clear any invalid system events.
		}
	}
}

//------------------------------------------------------------------------------
//	Watchdog Timer ISR
#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void) {
	if (debounce && --debounce == 0) {
		switchesP1 = ~P1IN;
		switchesP2 = ~P2IN;
		sysEvent |= doButtons;
	}

	// count down
	if (--clockCyclesLeft == 0) { // end of a second
		if (minutesLeft && secondsLeft == 0) {
			minutesLeft--;
			secondsLeft = 60;
		}
		if (secondsLeft) {
			clockCyclesLeft = WDT_IPS; // add more cycles
			secondsLeft--; // decrement the number of seconds.
		}
	}

	// events at specific times
	if (minutesLeft == 0 && clockCyclesLeft == 0) { // during the last minute...
		if (secondsLeft == ON_OFF_THRESHOLD) { // 0:5:0
			sysEvent |= turnCameraOnStart;
		} else if (secondsLeft == ON_OFF_THRESHOLD - 1) { // 0:4:0
			sysEvent |= turnCameraOnEnd;
		} else if (secondsLeft == 0) { // 0:0:0
			sysEvent |= takePictureEnd;
			sysEvent |= checkTimeSettings;
		}
	}
	if (minutesLeft == 0 && secondsLeft == 0 && clockCyclesLeft == TAKE_PIC_THRESHOLD) { // 0:0:3
		sysEvent |= takePictureStart;
	}

	if (sysEvent) __bic_SR_register_on_exit(LPM0_bits);
	return;
}

//------------------------------------------------------------------------------
//	Port 2 (switch) interrupt service routine
//  Resets the debounce counter.
#pragma vector = PORT2_VECTOR
__interrupt void Port_2_ISR(void) {
	P2IFG &= ~sw9;						// P2.2 IFG cleared
	debounce = DEBOUNCE_CNT;			// start debounce
} // end Port_1_ISR
