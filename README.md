# Mini lathe DC300 RPM Display

 Original Author: Jeffrey Nelson (nelsonjm@macpod.net)
 
 This is a small arduino sketch for reading the RPM of some Chinese lathes and mills. The original sketch from Jeffrey Nelson is changed so it can run on a ATTINY85 board. 

  Modifications: 
  - Trigger on receive of clockpulse, no need for strobe signal
  - Signals compiler to compile receiving package code using fastest code possible (-Os ), allowing for filtering noise
  - Build in double reading of clock levels to filter out noise
  - Made reading of package uninterruptable, no more missing clock pulses
  - Modified to run on a ATTINY85 (digistump) board
  - Modifief for a common TM1637 7 segement display driver

The display and lathe can be connected to the microcontroller using common dupont wires and 2.54 mm headers.
The microprocesser and display can use the 5V power supply from the lathe.
On my lathe, there is a JST-XL connector to the DIN plug for the original display. I removed the DIN plug and used the JST-XL connector to connect the display and the microcontroller. I mounted the display on top of the hole for the DIN plug.

The sketch needs a library for the TM1637 display that you can download here [TM1637 library](https://github.com/avishorp/TM1637.git)

A link to more information [Site 1](https://www.homemodelenginemachinist.com/threads/arduino-rpm-application-for-sieg-lathes-and-mills.30694/)
