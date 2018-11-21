# esp8266oddclock
What happens when you rip the ESP8266's stable clock source out from under it's feet?

The answer is it keep trying, as best as it can.

The center frequency of the channel remains the same, but the width of the channel squishes/stretches appropriately. 

And here's the kicker.  If you clock two ESPs the same, they can talk to each other over whatever crazy wacky channel stuff you do!
