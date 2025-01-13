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

#include "external_eeprom.h"
#include "main.h"
#include "i2c.h"

void write_external_eeprom(unsigned char address, unsigned char data)
{
	i2c_start();
	i2c_write(SLAVE_WRITE);
	i2c_write(address);
	i2c_write(data);
	i2c_stop();
    for(unsigned long wait = 3000; wait--;);
}

unsigned char read_external_eeprom(unsigned char address)
{
	unsigned char data;

	i2c_start();
	i2c_write(SLAVE_WRITE);
	i2c_write(address);
	i2c_rep_start();
	i2c_write(SLAVE_READ);
	data = i2c_read();
	i2c_stop();

	return data;
}


















