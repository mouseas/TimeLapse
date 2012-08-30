/*
 * Controls a small digital camera to take photos at regular intervals.
 * Reads an array of 8 switches to determine the interval length.
 *
 * Written by Martin Carney (mouseasw@gmail.com)
 *
 *		            MSP430G2452
 *		            -----------
 *		    +3.55v |VCC+ U GND-| -GND
 *	   (LED1)  SW1 |P1.0    XIN|
 *		       SW2 |P1.1   XOUT|
 *		       SW3 |P1.2   TEST|
 *		       SW4 |P1.3    RST| +3.55v
 *		       SW5 |P1.4   P1.7|-> to camera SW-A (on/off/mode)
 *		       SW6 |P1.5   P1.6|-> to camera SW-B (photo/record) (LED2)
 *		       SW7 |P2.0   P2.5|
 *		       SW8 |P2.1   P2.4|
 *		       SW9 |P2.2   P2.3|
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

void readIntervalSettings() {

}

void main(void) {
	WDTCTL = WDT_CTL;			// enable WDT as a 32 Khz timer
	IE1 |= WDTIE;				// enable WDT interrupts

	debounce = 0;
	cameraOffDelay = 0;

	clockCyclesLeft = 0;
	secondsLeft = 0;
	minutesLeft = 0;
	sysEvent = checkTimeSettings;	// start by reading the interval settings

	cameraOn = FALSE;			// assume camera is OFF at start.

	P1DIR = sw1to6;
	P2DIR = sw7to8 | sw9;
	
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
			readIntervalSettings();
		}
	}
}

//------------------------------------------------------------------------------
//	Watchdog Timer ISR
#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void) {
	if (debounce && --debounce == 0) {
		switchesP1 - P1IN;
		switchesP2 = P2IN;
		sysEvent |= doSwitches;
	}
	if (--clockCyclesLeft == 0) { // end of a second
		if (secondsLeft && --secondsLeft == 0) { // count down seconds and minutes
			if (minutesLeft) {
				secondsLeft = 60;
				minutesLeft--;
			}
		}
		if (minutesLeft == 0) {
			if (secondsLeft == ON_OFF_THRESHOLD) {
				sysEvent |= turnCameraOnStart;
			} else if (secondsLeft == ON_OFF_THRESHOLD - 1) {
				sysEvent |= turnCameraOnEnd;
			}
		}
	}

	if (sysEvent) __bic_SR_register_on_exit(LPM0_bits);
	return;
}

//------------------------------------------------------------------------------
//	Port 2 (switch) interrupt service routine
//  Resets the debounce counter.
#pragma vector = PORT2_VECTOR
__interrupt void Port_2_ISR(void) {
	P2IFG &= ~0x04;						// P2.2 IFG cleared
	debounce = DEBOUNCE_CNT;			// start debounce
} // end Port_1_ISR
