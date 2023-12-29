Alternative ADC + display for BM545
===================================

I got my hands on broken Tesla BM545 milivolt meter and picoampere meter. After
a bit of debugging I've found that the analog part works fine and that
something has died in the double integration ADC build from discrete TTL chips.
After a bit of consideration I've decided to replace the ADC since that is
actually easier than fixing the large board full of TTL logic.

The ADC board was replaced with Arduino, MCP3421 18bit sigma delta ADC and
HD44780 LCD display that even fits the front panel hole without modifications.

This also performs much better than the original circuitry.

The arduino sits at -2.5V and +2.5V from LM317 and LM337 since the output from
the analog part of the meter is in the range of [-2V, +2V], that means that you
have to be vary of the ground loops when connecting the USB programming cable.
You should remove the signal ground and earth ground connection at the back of
the meter for the duration of updating the program.
