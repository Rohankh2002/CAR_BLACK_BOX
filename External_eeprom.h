//#ifndef EEPROM_H
//#define EEPROM_H
//
//void write_internal_eeprom(unsigned char address, unsigned char data); 
//unsigned char read_internal_eeprom(unsigned char address);
//
//#endif

#ifndef EXTERNAL_EEPROM_H
#define	EXTERNAL_EEPROM_H

#define SLAVE_READ		0xA1
#define SLAVE_WRITE		0xA0

//void init_ds1307(void);
void write_external_eeprom(unsigned char address1,  unsigned char data);
unsigned char read_external_eeprom(unsigned char address1);

#endif	/* EXTERNAL_EEPROM_H */
