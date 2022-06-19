/*
 * blinker.c
 *
 *  Created on: 2022. 6. 14.
 *      Author: hsk
 */

#include "msp.h"
#include "blinker.h"
// Blinker LEDS
// Front right P8.5 Yellow LED
// Front left  P8.0 Yellow LED
// Back right  P8.7 Red LED
// Back left   P8.6 Red LED
// ------------Blinker_Init------------
// Initialize four LED blinkers on TIRSLK 1.1
// Input: none
// Output: none
void Blinker_Init(void){
  P8->SEL0 &= ~0xE1;
  P8->SEL1 &= ~0xE1;    // configure P8.0,P8.5-P8.7 as GPIO
  P8->DIR |= 0xE1;      // make P8.0,P8.5-P8.7 out
  P8->OUT &= ~0xE1;     // all LEDs off
}

//------------Blinker_Output------------
// Output to four LED blinkers on TIRSLK
// Input: data to write to LEDs (uses bits 7,6,5,0)
//   bit 7 Red back right LED
//   bit 6 Red back left LED
//   bit 5 Yellow front right LED
//   bit 0 Yellow front left LED
// Output: none
void Blinker_Output(uint8_t data){  // write four outputs bits of P8
  P8->OUT = (P8->OUT&0x1E)|data;
}
