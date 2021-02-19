/*
  Original Author: Jeffrey Nelson <nelsonjm@macpod.net>
  Description: Very quickly written crappy code to read tachometer port data of the SX2 mini mill and likely the SC2 mini lathe.
   for more information check http://macpod.net
  This code was written on an 5v 16Mhz Arduino. I'm planning on writing

  Tachometer Port Interface:
  2 GND
  2 5V+
  1 LCDCS - Frame indicator,  pulled low during the transmission of a frame.
  1 DC300CL - Clock line, pulled low when data should be read (we read on the falling edges)
  1 DC300DI - Data line.

  Data information:
  Reports if the spindel is stopped or running
  Reports speed of the spindel in 10rpm increments

  Data format:
  Every .75 seconds a packet is sent over the port.
  Each packet consists of:
        (ONLY If the mill uses the new mill protocol): a 36 bit header
        Following this potential header are 4 frames. Each Frame consists of 17 bytes.
        The first 8 bits represent an address, and the other bits represent data.

  Frame 0: Represents 7-segment data used for 1000's place of rpm readout.
    Address: 0xA0
    Data: First bit is always 0
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is always 0
  Frame 1: Represents 7-segment data used for 100's place of rpm readout.
    Address: 0xA1
    Data: First bit is always 0
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is always 0
  Frame 2: Represents 7-segment data used for 10's place of rpm readout.
    Address: 0xA2
    Data: First bit is always 0
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is 1 if the spindel is not rotating, 0 otherwise.
  Frame 3: Represents 7-segment data used for 1's place of rpm readout. This isn't used.
    Address: 0xA3
    Data: This is always 0x20

  Original Tachometer port 7-segment display layout:
    d
  c   g
    f
  b   e
    a

  abcdefg
  00 = 1111101
  01 = 0000101
  02 = 1101011
  03 = 1001111
  04 = 0010111
  05 = 1011110
  06 = 1111110
  07 = 0001101
  08 = 1111111
  09 = 1011111

  Modifications made by Huub Buis http://www.github.com/metalworkertools/DC300_RPM_Display
  - Trigger on receive of clockpulse, no need for strobe signal
  - Signals compiler to compile receiving package code using fastest code possible (-Os ), allowing for filtering noise
  - Build in double reading of clock levels to filter out noise
  - Made reading of package uninterruptable, no more missing clock pulses ()

*/

//#pragma GCC optimize ("-Os")
#pragma GCC optimize ("-O3")
//#pragma GCC push_options
//#pragma GCC pop_options

#define ATtiny85    //uncomment for ATtiny board

#ifdef ATtiny85
#define DC300CL 0       // DC300 Clock
#define DC300DI 2       // DC300 Data
#define LCDPINS PINB
#define DisplayCLK 3      //SCL pin, USB, disconnect for programming
#define DisplayDIO 4      //SDA pin, USB, disconnect for programming
#define FailPin 1
#define FailLevel HIGH

//#define SignalPin LED_BUILTIN
#else             //Arduino  
#define DC300CL 2       // DC300 Clock
#define DC300DI 3       // DC300 Data
#define LCDPINS PIND
#define DisplayCLK 11      //SCL pin, USB, disconnect for programming
#define DisplayDIO 12      //SDA pin, USB, disconnect for programming#include <util/atomic.h>
#define FailPin 8          //Use this pin to connect overload led
#define FailLevel HIGH
#define SignalPin 9
#endif

#include <avr/io.h>
#include <util/atomic.h>
#include <TM1637Display.h>    //driver for TM1637 7 segement display driver

//#define SignalPacketEnd SignalPin   //Flash pin on PacketEnd End
//#define SignalPacketError SignalPin      //Show packet received ok
//#define SignalProcessEnd SignalPin    //Flash pin on Process End
//#define ShowPacketOk LED_BUILTIN      //Show packet received ok
//#define ShowErrorLed LED_BUILTIN    //Flash pin on error
//#define ShowErrorNumber             //Show the Error number
#define ShowErrorSign               //Show "----" when error received
#define BitClear(Register,BitNr)  ((Register & _BV(BitNr))==0)
#define BitSet(Register,BitNr)    ((Register & _BV(BitNr))!=0)

#define PACKET_BITS 68
// For newer mills there is a 36 bit header.
#define PACKET_BITS_HEADER 36

// Uncomment for newer mill protocol
//#define PACKET_BITS_COUNT PACKET_BITS+PACKET_BITS_HEADER
// Uncomment for old mill protocol
#define PACKET_BITS_COUNT PACKET_BITS

volatile uint8_t packet_bits[PACKET_BITS_COUNT];
volatile uint8_t packet_bits_pos = 0;
TM1637Display display(DisplayCLK, DisplayDIO);

#define SEG_f SEG_A +SEG_G + SEG_E + SEG_F
#define SEG_a SEG_A +SEG_G + SEG_E + SEG_F +SEG_B + SEG_C
#define SEG_i SEG_E + SEG_F
#define SEG_l SEG_E + SEG_F +SEG_D

const uint8_t ErrorSegments0[] = {SEG_G, 0, 0, SEG_G};
const uint8_t ErrorSegments1[] = {0, SEG_G, SEG_G, 0};
const uint8_t ErrorSegments2[] = {SEG_G, SEG_G, SEG_G, SEG_G,};
const uint8_t ErrorSegmentsFail[] = {SEG_f, SEG_a, SEG_i, SEG_l};

void setup() {
  pinMode(DC300CL, INPUT);
  pinMode(DC300DI, INPUT);
#ifdef FailPin
  pinMode(FailPin, INPUT_PULLUP);
#endif
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
#endif
#ifdef ShowErrorLed
  pinMode(ShowErrorLed, OUTPUT);
  digitalWrite(ShowErrorLed, LOW );
#endif
#ifdef SignalPacketEnd
  pinMode(SignalPacketEnd, OUTPUT);
#endif
#ifdef SignalProcessEnd
  pinMode(SignalProcessEnd, OUTPUT);
#endif
#ifdef ShowPacketOk
  pinMode(ShowPacketOk, OUTPUT);
#endif
#ifdef SignalPacketError
  pinMode(SignalPacketError, OUTPUT);
#endif
  display.setBrightness(7); //set the diplay to maximum brightness
// uncomment following lines for testing the display  
//  DisplayRPM(8888);
//  delay(100);
//  ShowFail();
//  display.setSegments(ErrorSegments0);
//  delay(500);
//  display.setSegments(ErrorSegments1);
//  delay(500);
  DisplayRPM(-1);
}
void ShowFail()
{
  display.setSegments(ErrorSegmentsFail);
  delay(700);
  display.setSegments(ErrorSegments2);
  delay(200);
}
void ShowError(int Error)
{
  static bool ShowOne = false;
#ifdef ShowErrorLed
  digitalWrite(ShowErrorLed, HIGH);
  delay(50);
  digitalWrite(ShowErrorLed, LOW );
#endif
#ifdef ShowErrorNumber
  display.showNumberDec(Error); //Display the RPM value;
#endif
#ifdef ShowErrorSign
  if (ShowOne)
    display.setSegments(ErrorSegments1);
  else
    display.setSegments(ErrorSegments0);
  ShowOne = !ShowOne;
#endif
}

//display the RPM value
void DisplayRPM(int RPM)
{
  if (RPM >= 0)
    display.showNumberDec(RPM); //Display the RPM value;
  else
    ShowError(RPM);
}
void ReadPacket() __attribute__((optimize("-O3"))) ;
void ReadPacket()
{
  static byte i;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {   //This code may not be interrupted, otherwise clock pulses get lost
    for (packet_bits_pos = 0; packet_bits_pos < PACKET_BITS_COUNT; packet_bits_pos++)
    {
      while (BitSet(LCDPINS, DC300CL) && BitSet(LCDPINS, DC300CL)  );
      packet_bits[packet_bits_pos] = LCDPINS;
      while (BitClear(LCDPINS, DC300CL) && BitClear(LCDPINS, DC300CL) );
    }
  }
}
void loop() {
  int rpm;
#ifdef FailPin                               //If pin is defined check the pin for the required level and show Fail if level is equal
  {
    if (digitalRead(FailPin) == FailLevel)
    {
      ShowFail();
      return;
    }
  }
#endif
  ReadPacket();
#ifdef SignalPacketEnd
  digitalWrite(SignalPacketEnd, LOW);    //Signal whe have read packet, should be at end of packet
  digitalWrite(SignalPacketEnd, HIGH);
#endif
#ifdef SignalPacketError
  if (rpm < 0)                        //Signal whe have read a bad packet
  {
    digitalWrite(SignalPacketError, LOW);
    digitalWrite(SignalPacketError, HIGH);
  }
#endif
  delay(300);       //If we started in the middle of a packet, we are in sync again, if we started at the beginning, nothing changed
  rpm = get_rpm();
#ifdef ShowPacketOk
  if (rpm >= 0)
  {
    digitalWrite(ShowPacketOk, LOW);
    delay(100);
    digitalWrite(ShowPacketOk, HIGH);    //Show whe have read packet, should be at end of packet
  }
#endif
  DisplayRPM(rpm);
#ifdef SignalProcessEnd
  digitalWrite(SignalProcessEnd, LOW);    //Show whe have read and processed packet
  delay(50);
  digitalWrite(SignalProcessEnd, HIGH);
#endif
}

//----------------------------------------------------------------------------------------------------
// Assumes 68 bits were received properly. Returns the spindle speed or -1 if the values are absurd.
//int get_rpm() __attribute__((optimize("-O3"))) ;
int get_rpm()
{
  int temp, ret = 0;
  if (build_address(0) != 0xA0) {
    return -1;
  }
  temp = get_digit_from_data(build_data(8));
  if (temp == -1) {
    return -2;
  }
  ret += temp * 1000;
  if (build_address(17) != 0xA1) {
    return -3;
  }
  temp = get_digit_from_data(build_data(25));
  if (temp == -1) {
    return -3;
  }
  ret += temp * 100;
  if (build_address(34) != 0xA2) {
    return -5;
  }
  temp = get_digit_from_data(build_data(42));
  if (temp == -1) {
    return -6;
  }
  ret += temp * 10;
  if (build_address(51) != 0xA3) {
    return -7;
  }
  temp = build_data(59);
  if (temp != 0x20) {
    return -8;
  }
  return ret;
}
// An address is 8 bits long
uint8_t GetBit(uint8_t Data, uint8_t BitNr) __attribute__((optimize("-O3"))) ;
uint8_t GetBit(uint8_t Data, uint8_t BitNr)
{
  if BitSet(Data, BitNr) return 1;
  return 0;
}
uint8_t build_address(uint8_t start_address)
{
  uint8_t ret = 0x1;
  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }
  for (uint8_t i = start_address; i < start_address + 8; i++) {
    ret = (ret << 1) ^ GetBit(packet_bits[i], DC300DI);
  }
  return ret;
}
// Data is 9 bits long
uint16_t build_data(uint8_t start_address)
{
  uint16_t ret = 0;
  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }
  for (uint8_t i = start_address; i < start_address + 9; i++) {
    ret = (ret << 1) ^ GetBit(packet_bits[i], DC300DI);
  }
  return ret;
}
int get_digit_from_data(uint16_t data)
{
  uint16_t segments = (data & 0xFE) >> 1;
  switch (segments) {
    case 0x7D: return 0;
    case 0x05: return 1;
    case 0x6B: return 2;
    case 0x4F: return 3;
    case 0x17: return 4;
    case 0x5E: return 5;
    case 0x7E: return 6;
    case 0x0D: return 7;
    case 0x7F: return 8;
    case 0x5F: return 9;
  }
  return -1;
}
