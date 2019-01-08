# esp8266oddclock
What happens when you rip the ESP8266's stable clock source out from under its feet?

The answer is it keep trying, as best as it can.

The center frequency of the channel remains the same, but the width of the channel squishes/stretches appropriately. 

And here's the kicker.  If you clock two ESPs the same, they can talk to each other over whatever crazy wacky channel stuff you do!

The code that controls the PLL, you can do that on any ESP:

```

	if( lcode == 0 || lcode == 1  || lcode == 2)
	{
		mode = 0;
		pico_i2c_writereg(103,4,1,0x88);	
		pico_i2c_writereg(103,4,2,0x91);	

	}
	if( lcode == 2048 || lcode == 2049 || lcode == 2050 )
	{
		pico_i2c_writereg(103,4,1,0x88);
		pico_i2c_writereg(103,4,2,0xf1);
		mode = 1;
	}

	if( lcode == 4096 || lcode == 4097 || lcode == 4098 )
	{
		pico_i2c_writereg(103,4,1,0x48);
		pico_i2c_writereg(103,4,2,0xf1);	
		mode = 2;
}
```

The first register, only two useful values I've found are 0x48 and 0x88, which seems to control some primary devisor. The second register can be 0xX1, etc. and controls the primary divisor from the internal 1080MHz clock.  See this for more information: https://github.com/cnlohr/nosdk8266/blob/master/src/nosdk8266.c#L44
