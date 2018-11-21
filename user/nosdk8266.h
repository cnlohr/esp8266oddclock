#ifndef _NOSDK8266_H
#define _NOSDK8266_H


#ifdef PICO66
//#define pico_i2c_writereg( reg, hostid, par, val ) { asm volatile( "_movi a2, " #reg "\n_movi.n a3, " #hostid "\n_movi.n a4, " #par "\nmovi a5, " #val "\n_call0 pico_i2c_writereg_asm" : : : "a2", "a3", "a4", "a5", "a0" ); }  //Doesn't work.
void pico_i2c_writereg_asm( uint32_t a, uint32_t b);
#define pico_i2c_writereg( reg, hostid, par, val ) pico_i2c_writereg_asm( (hostid<<2) + 0x60000a00 + 0x300, (reg | (par<<8) | (val<<16) | 0x01000000 ) )

#else
#define pico_i2c_writereg rom_i2c_writeReg
void rom_i2c_writeReg( int reg, int hosid, int par, int val ); 
#endif


#endif

