
.global pico_i2c_writereg_asm
.align 4
pico_i2c_writereg_asm:
	_s32i.n	a3, a2, 0
	_memw
	_l32i.n	a3, a2, 0
	bbci	a3, 25, .term_pico_writereg
.reloop_pico_writereg:
	_memw
	_l32i.n	a3, a2, 0
	bbsi	a3, 25, .reloop_pico_writereg
.term_pico_writereg:
	_ret.n

