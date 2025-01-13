/*
 * Name: Rohan K H
 * Date: 05/01/2025
 * Description: The "Car Black Box" project uses Embedded C programming to monitor and record the time,
 *              gear position, and speed of a car during the last 10 events. An external EEPROM is
 *              utilized to store this data, ensuring retention even when the system powers off. 
 *              The project integrates a CLCD (Character LCD) for real-time display of information 
 *              and employs an ADC (Analog-to-Digital Converter) to measure and process the car's speed. 
 *              This system is implemented on a development kit for prototyping and testing.
 */

#include <xc.h>
#include "adc.h"

void init_adc(void)
{
	/* Selecting right justified ADRES Registers order */
	ADFM = 1;

	/* 
	 * Acqusition time selection bits 
	 * Set for 4 Tad
	 */
	ACQT2 = 0;
	ACQT1 = 1;
	ACQT0 = 0;

	/*
	 * Selecting the conversion clock of Fosc / 32 -> 1.6usecs -> 1Tad
	 * Our device frequency is 20 MHz
	 */
	ADCS0 = 0;
	ADCS1 = 1;
	ADCS2 = 0;

	/* Stop the conversion to start with */
	GODONE = 0;

	

	/* Voltage reference bit as VSS */
	VCFG1 = 0;
	/* Voltage reference bit as VDD */
	VCFG0 = 0;

	/* Just clearing the ADRESH & ADRESL registers, for time pass */
	ADRESH = 0;
	ADRESL = 0;

	/* Turn ON the ADC module */
	ADON = 1;
}

unsigned short read_adc(unsigned char channel)
{
	unsigned short reg_val;

	/*select the channel*/
	ADCON0 = (ADCON0 & 0xC3) | (channel << 2);

	/* Start the conversion */
	GO = 1;
	while (GO);
	reg_val = (ADRESH << 8) | ADRESL; 

	return reg_val;
}
