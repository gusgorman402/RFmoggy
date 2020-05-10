/*
  ELECHOUSE_CC1101.cpp - CC1101 module library
  Copyright (c) 2010 Michael.
    Author: Michael, <www.elechouse.com>
    Version: November 12, 2010

  This library is designed to use CC1101/CC1100 module on Arduino platform.
  CC1101/CC1100 module is an useful wireless module.Using the functions of the 
  library, you can easily send and receive data by the CC1101/CC1100 module. 
  Just have fun!
  For the details, please refer to the datasheet of CC1100/CC1101.
----------------------------------------------------------------------------------------------------------------
cc1101 Driver for RC Switch. Mod by Little Satan. With permission to modify and publish Wilson Shen (ELECHOUSE).
----------------------------------------------------------------------------------------------------------------
*/
#include <SPI.h>
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <Arduino.h>

/****************************************************************/
#define   WRITE_BURST       0x40            //write burst
#define   READ_SINGLE       0x80            //read single
#define   READ_BURST        0xC0            //read burst
#define   BYTES_IN_RXFIFO   0x7F            //byte number in RXfifo

byte modulation = 2;
byte frend0;
byte chan = 0;
int pa = 12;
byte last_pa;
byte SCK_PIN = 13;
byte MISO_PIN = 12;
byte MOSI_PIN = 11;
byte SS_PIN = 10;
byte GDO0;
byte GDO2;
bool spi = 0;
byte mdcf2;
byte rxbw = 0;
bool ccmode = 0;
float MHz = 433.92;
byte m4RxBw; 
byte m4DaRa;
byte packetLen = 0x3D; //max packet size of 61. Related to TX FIFO buffer and FIFOTHR

/****************************************************************/
uint8_t PA_TABLE[8]     {0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00};
//                       -30  -20  -15  -10   0    5    7    10
uint8_t PA_TABLE_315[8] {0x12,0x0D,0x1C,0x34,0x51,0x85,0xCB,0xC2,};             //300 - 348
uint8_t PA_TABLE_433[8] {0x12,0x0E,0x1D,0x34,0x60,0x84,0xC8,0xC0,};             //387 - 464
//                        -30  -20  -15  -10  -6    0    5    7    10   12
uint8_t PA_TABLE_868[10] {0x03,0x17,0x1D,0x26,0x37,0x50,0x86,0xCD,0xC5,0xC0,};  //779 - 899.99
//                        -30  -20  -15  -10  -6    0    5    7    10   11
uint8_t PA_TABLE_915[10] {0x03,0x0E,0x1E,0x27,0x38,0x8E,0x84,0xCC,0xC3,0xC0,};  //900 - 928
/****************************************************************
*FUNCTION NAME:SpiStart
*FUNCTION     :spi communication start
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiStart(void)
{
  // initialize the SPI pins
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(SS_PIN, OUTPUT);

  // enable SPI
  #ifdef ESP32
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  #else
  SPI.begin();
  #endif
}
/****************************************************************
*FUNCTION NAME:SpiEnd
*FUNCTION     :spi communication disable
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiEnd(void)
{
  // disable SPI
  SPI.endTransaction();
  SPI.end();
  digitalWrite(SCK_PIN, LOW);
}
/****************************************************************
*FUNCTION NAME: GDO_Set()
*FUNCTION     : set GDO0,GDO2 pin
*INPUT        : none
*OUTPUT       : none
****************************************************************/
void ELECHOUSE_CC1101::GDO_Set (void)
{
	pinMode(GDO0, INPUT);
	pinMode(GDO2, OUTPUT);
}
/****************************************************************
*FUNCTION NAME:Reset
*FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::Reset (void)
{
	digitalWrite(SS_PIN, LOW);
	delay(1);
	digitalWrite(SS_PIN, HIGH);
	delay(1);
	digitalWrite(SS_PIN, LOW);
	while(digitalRead(MISO_PIN));
  SPI.transfer(CC1101_SRES);
  while(digitalRead(MISO_PIN));
	digitalWrite(SS_PIN, HIGH);
}
/****************************************************************
*FUNCTION NAME:Init
*FUNCTION     :CC1101 initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::Init(void)
{
  setSpi();
  SpiStart();                   //spi initialization
  GDO_Set();                    //GDO set
  digitalWrite(SS_PIN, HIGH);
  digitalWrite(SCK_PIN, HIGH);
  digitalWrite(MOSI_PIN, LOW);
  Reset();                    //CC1101 reset
  RegConfigSettings();            //CC1101 register config
  SpiEnd();
}
/****************************************************************
*FUNCTION NAME:SpiWriteReg
*FUNCTION     :CC1101 write data to register
*INPUT        :addr: register address; value: register value
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiWriteReg(byte addr, byte value)
{
  SpiStart();
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(addr);
  SPI.transfer(value); 
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
}
/****************************************************************
*FUNCTION NAME:SpiWriteBurstReg
*FUNCTION     :CC1101 write burst data to register
*INPUT        :addr: register address; buffer:register value array; num:number to write
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{
  byte i, temp;
  SpiStart();
  temp = addr | WRITE_BURST;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(temp);
  for (i = 0; i < num; i++)
  {
  SPI.transfer(buffer[i]);
  }
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
}
/****************************************************************
*FUNCTION NAME:SpiStrobe
*FUNCTION     :CC1101 Strobe
*INPUT        :strobe: command; //refer define in CC1101.h//
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiStrobe(byte strobe)
{
  SpiStart();
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(strobe);
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
}
/****************************************************************
*FUNCTION NAME:SpiReadReg
*FUNCTION     :CC1101 read data from register
*INPUT        :addr: register address
*OUTPUT       :register value
****************************************************************/
byte ELECHOUSE_CC1101::SpiReadReg(byte addr) 
{
  byte temp, value;
  SpiStart();
  temp = addr| READ_SINGLE;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(temp);
  value=SPI.transfer(0);
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
  return value;
}

/****************************************************************
*FUNCTION NAME:SpiReadBurstReg
*FUNCTION     :CC1101 read burst data from register
*INPUT        :addr: register address; buffer:array to store register value; num: number to read
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SpiReadBurstReg(byte addr, byte *buffer, byte num)
{
  byte i,temp;
  SpiStart();
  temp = addr | READ_BURST;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(temp);
  for(i=0;i<num;i++)
  {
  buffer[i]=SPI.transfer(0);
  }
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
}

/****************************************************************
*FUNCTION NAME:SpiReadStatus
*FUNCTION     :CC1101 read status register
*INPUT        :addr: register address
*OUTPUT       :status value
****************************************************************/
byte ELECHOUSE_CC1101::SpiReadStatus(byte addr) 
{
  byte value,temp;
  SpiStart();
  temp = addr | READ_BURST;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN));
  SPI.transfer(temp);
  value=SPI.transfer(0);
  digitalWrite(SS_PIN, HIGH);
  SpiEnd();
  return value;
}
/****************************************************************
*FUNCTION NAME:SPI pin Settings
*FUNCTION     :Set Spi pins
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSpi(void){
  if (spi == 0){
  #if defined __AVR_ATmega168__ || defined __AVR_ATmega328P__
  SCK_PIN = 13; MISO_PIN = 12; MOSI_PIN = 11; SS_PIN = 10;
  #elif defined __AVR_ATmega1280__ || defined __AVR_ATmega2560__
  SCK_PIN = 52; MISO_PIN = 50; MOSI_PIN = 51; SS_PIN = 53;
  #elif ESP8266
  SCK_PIN = 14; MISO_PIN = 12; MOSI_PIN = 13; SS_PIN = 15;
  #elif ESP32
  SCK_PIN = 18; MISO_PIN = 19; MOSI_PIN = 23; SS_PIN = 5;
  #else
  SCK_PIN = 13; MISO_PIN = 12; MOSI_PIN = 11; SS_PIN = 10;
  #endif
}
}
/****************************************************************
*FUNCTION NAME:COSTUM SPI
*FUNCTION     :set costum spi pins.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSpiPin(byte sck, byte miso, byte mosi, byte ss){
  spi = 1;
  SCK_PIN = sck;
  MISO_PIN = miso;
  MOSI_PIN = mosi;
  SS_PIN = ss;
}
/****************************************************************
*FUNCTION NAME:GDO Pin settings
*FUNCTION     :set GDO Pins
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setGDO(byte gdo0, byte gdo2){
GDO0 = gdo0;
GDO2 = gdo2;  
}
/****************************************************************
*FUNCTION NAME:CCMode
*FUNCTION     :Format of RX and TX data
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setCCMode(bool s){
ccmode = s;
if (ccmode == 1){
SpiWriteReg(CC1101_IOCFG2,      0x0B);
SpiWriteReg(CC1101_IOCFG0,      0x06);
SpiWriteReg(CC1101_PKTCTRL0,    0x05);
SpiWriteReg(CC1101_MDMCFG3,     0xF8);
SpiWriteReg(CC1101_MDMCFG4,  11+rxbw);
}else{
SpiWriteReg(CC1101_IOCFG2,      0x01); //changed these settings. I didn't understand serial mode, so i switched to fixed size TX FIFO
SpiWriteReg(CC1101_IOCFG0,      0x02);
SpiWriteReg(CC1101_PKTCTRL0,    0x00); //packets should have no address or length bit. Code was adding length bit anyway
// SpiWriteReg(CC1101_MDMCFG3,     0x93);
// SpiWriteReg(CC1101_MDMCFG4,   7+rxbw);
}
setModulation(modulation);
}

//sets fixed packet length PKTLEN register. Function isn't need, sendData function
//rfcat examples set this every packet send, but I haven't found that necessary 
void ELECHOUSE_CC1101::setPktLen(byte g){
    packetLen = g;
    SpiWriteReg(CC1101_PKTLEN, packetLen);
}


/****************************************************************
*FUNCTION NAME:Modulation
*FUNCTION     :set CC1101 Modulation 
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setModulation(byte m){
if (m>4){m=4;}
modulation = m;
switch (m)
{
case 0: mdcf2=0x00; frend0=0x10; break; // 2-FSK
case 1: mdcf2=0x10; frend0=0x10; break; // GFSK
case 2: mdcf2=0x30; frend0=0x11; break; // ASK
case 3: mdcf2=0x40; frend0=0x10; break; // 4-FSK
case 4: mdcf2=0x70; frend0=0x10; break; // MSK
}
if (ccmode == 1){mdcf2 += 3;}
SpiWriteReg(CC1101_MDMCFG2,   mdcf2);
SpiWriteReg(CC1101_FREND0,   frend0);
setPA(pa);
}
/****************************************************************
*FUNCTION NAME:PA Power
*FUNCTION     :set CC1101 PA Power 
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPA(int p)
{
int a;
pa = p;

if (MHz >= 300 && MHz <= 348){
if (pa <= -30){a = PA_TABLE_315[0];}
else if (pa > -30 && pa <= -20){a = PA_TABLE_315[1];}
else if (pa > -20 && pa <= -15){a = PA_TABLE_315[2];}
else if (pa > -15 && pa <= -10){a = PA_TABLE_315[3];}
else if (pa > -10 && pa <= 0){a = PA_TABLE_315[4];}
else if (pa > 0 && pa <= 5){a = PA_TABLE_315[5];}
else if (pa > 5 && pa <= 7){a = PA_TABLE_315[6];}
else if (pa > 7){a = PA_TABLE_315[7];}
SpiWriteReg(CC1101_FSCTRL0,  0x19);  //change these FSCTRL0 settings for your particular chip. Cheaper the chip crystal, more calibration needed. RTLSDR helps!
last_pa = 1;
}
else if (MHz >= 387 && MHz <= 464){
if (pa <= -30){a = PA_TABLE_433[0];}
else if (pa > -30 && pa <= -20){a = PA_TABLE_433[1];}
else if (pa > -20 && pa <= -15){a = PA_TABLE_433[2];}
else if (pa > -15 && pa <= -10){a = PA_TABLE_433[3];}
else if (pa > -10 && pa <= 0){a = PA_TABLE_433[4];}
else if (pa > 0 && pa <= 5){a = PA_TABLE_433[5];}
else if (pa > 5 && pa <= 7){a = PA_TABLE_433[6];}
else if (pa > 7){a = PA_TABLE_433[7];}
SpiWriteReg(CC1101_FSCTRL0,  0x24);
last_pa = 2;
}
else if (MHz >= 779 && MHz <= 899.99){
if (pa <= -30){a = PA_TABLE_868[0];}
else if (pa > -30 && pa <= -20){a = PA_TABLE_868[1];}
else if (pa > -20 && pa <= -15){a = PA_TABLE_868[2];}
else if (pa > -15 && pa <= -10){a = PA_TABLE_868[3];}
else if (pa > -10 && pa <= -6){a = PA_TABLE_868[4];}
else if (pa > -6 && pa <= 0){a = PA_TABLE_868[5];}
else if (pa > 0 && pa <= 5){a = PA_TABLE_868[6];}
else if (pa > 5 && pa <= 7){a = PA_TABLE_868[7];}
else if (pa > 7 && pa <= 10){a = PA_TABLE_868[8];}
else if (pa > 10){a = PA_TABLE_868[9];}
SpiWriteReg(CC1101_FSCTRL0,  0x4B);
last_pa = 3;
}
else if (MHz >= 900 && MHz <= 928){
if (pa <= -30){a = PA_TABLE_915[0];}
else if (pa > -30 && pa <= -20){a = PA_TABLE_915[1];}
else if (pa > -20 && pa <= -15){a = PA_TABLE_915[2];}
else if (pa > -15 && pa <= -10){a = PA_TABLE_915[3];}
else if (pa > -10 && pa <= -6){a = PA_TABLE_915[4];}
else if (pa > -6 && pa <= 0){a = PA_TABLE_915[5];}
else if (pa > 0 && pa <= 5){a = PA_TABLE_915[6];}
else if (pa > 5 && pa <= 7){a = PA_TABLE_915[7];}
else if (pa > 7 && pa <= 10){a = PA_TABLE_915[8];}
else if (pa > 10){a = PA_TABLE_915[9];}
SpiWriteReg(CC1101_FSCTRL0,  0x4F);
last_pa = 4;
}
if (modulation == 2){
PA_TABLE[0] = 0;  
PA_TABLE[1] = a;
}else{
PA_TABLE[0] = a;  
PA_TABLE[1] = 0; 
}
SpiWriteBurstReg(CC1101_PATABLE,PA_TABLE,8);
}


//repeat function of setMHZ, I was just comparing algorithms. Copied from Space Teddy library
//function will give inaccurate Freq setting. Use setMHz instead
void ELECHOUSE_CC1101::set_freq(uint32_t freq)
{
    // this is split into 3 bytes that are written to 3 different registers on the CC1101
    uint32_t reg_freq = freq / (26000000>>16);
    
    uint8_t freq2 = (reg_freq>>16) & 0xFF;   // high byte, bits 7..6 are always 0 for this register
    uint8_t freq1 = (reg_freq>>8) & 0xFF;    // middle byte
    uint8_t freq0 = reg_freq & 0xFF;         // low byte
    
    SpiWriteReg(CC1101_FREQ2, freq2);
    SpiWriteReg(CC1101_FREQ1, freq1);
    SpiWriteReg(CC1101_FREQ0, freq0);

}

/****************************************************************
*FUNCTION NAME:Frequency Calculator
*FUNCTION     :Calculate the basic frequency.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setMHZ(float mhz){

MHz = mhz;

const float freq2 = 26;
const float freq1 = 0.1015625;
const float freq0 = 0.00039675;

float s1 = mhz/freq2; 
int s2 = s1;                             // freq2
float s3 = s1-s2;
float s4 = s3*100000000;
float s5 = 255.0/100000000*s4;
int s6 = s5;
float s7 = (s5-s6);
float s8 = s7*10;
int s9;                                  // freq1
if (s8>=5){s9=s6+1;}
else{s9=s6;}
float s10 = mhz-(freq2*s2+freq1*s9);
float s11 = s10/freq0;
int s12 = s11;
float s13 = (s11-s12)*(10);
int s14;                                 // freq0
if (s13>=5){s14=s12+1;}
else{s14=s12;}
if (s14 > 255){s9+=1;s14-=256;}

SpiWriteReg(CC1101_FREQ2,    s2);
SpiWriteReg(CC1101_FREQ1,    s9);
SpiWriteReg(CC1101_FREQ0,    s14);

if ((MHz >=322.88 && MHz <= 348) || (MHz >=430.5 && MHz <= 464) || (MHz >= 861 && MHz <= 928)){ SpiWriteReg(CC1101_TEST0, 0x09); }
else { SpiWriteReg(CC1101_TEST0, 0x0B); }

if (MHz >= 300 && MHz <= 348 && last_pa == 1){return;}
else if (MHz >= 387 && MHz <= 464 && last_pa == 2){return;}
else if (MHz >= 779 && MHz <= 899.99 && last_pa == 3){return;}
else if (MHz >= 900 && MHz <= 928 && last_pa == 4){return;}
else{setPA(pa);}
}

//copied from lsatan cc1101 debug program
void ELECHOUSE_CC1101::Split_MDMCFG4(void){
   int calc = SpiReadStatus(CC1101_MDMCFG4);
   int s1 = 0;
   int s2 = 0;
   for (bool i = 0; i==0;){
      if (calc >= 64){calc -= 64; s1++;}
      else if (calc >= 16){calc -= 16; s2++;}
      else{i=1;}
   }
   s1 *= 64;
   s2 *= 16;
   s1 += s2;
   m4RxBw = s1;
   s1 = SpiReadStatus(CC1101_MDMCFG4)-s1;
   m4DaRa = s1;
}

//copied from lsatan cc1101 debug program.Repeat function of setRxBw. Input as float is easier to understand than setRxBw function
void ELECHOUSE_CC1101::set_rxbw(float f){
   int s1 = 3;
   int s2 = 3;
   for (int i = 0; i<3; i++){
      if (f > 101.5625){f/=2; s1--;}
      else{i=3;}
   }
   for (int i = 0; i<3; i++){
      if (f > 58.1){f/=1.25; s2--;}
      else{i=3;}
   }
   s1 *= 64;
   s2 *= 16;
   s1 += s2;
   Split_MDMCFG4();
   SpiWriteReg(CC1101_MDMCFG4, s1+m4DaRa);
}

//copied from lsatan cc1101 debug program
void ELECHOUSE_CC1101::set_drate(float d){
   float c = d;
   int mdc3 = 0;
   if (c > 1621.83){c = 1621.83;}
   if (c < 0.0247955){c = 0.0247955;}
   int mdc4 = 0;
   for (int i = 0; i<20; i++){
      if (c <= 0.0494942){
         c = c - 0.0247955;
         c = c / 0.00009685;
         mdc3 = c;
         float s1 = (c - mdc3) *10;
         if (s1 >= 5){mdc3++;}
         i = 20;
     }else{
         mdc4++;
         c = c/2;
     }
   }
   Split_MDMCFG4();
   SpiWriteReg(CC1101_MDMCFG4,  m4RxBw+mdc4);
   SpiWriteReg(CC1101_MDMCFG3,  mdc3);
}

/****************************************************************
*FUNCTION NAME:Set RX bandwidth
*FUNCTION     :Recive bandwidth
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setRxBW(byte r){

if (r > 15){r = 15;}
r= map(r, 0, 15, 15, 0);
rxbw = r *16;

setCCMode(ccmode);
}
/****************************************************************
*FUNCTION NAME:Set Channel
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setChannel(byte ch){
chan = ch;
SpiWriteReg(CC1101_CHANNR,   chan);
}
/****************************************************************
*FUNCTION NAME:RegConfigSettings
*FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::RegConfigSettings(void) 
{   
    SpiWriteReg(CC1101_FSCTRL1,  0x06);
    
    setCCMode(ccmode);
    setMHZ(MHz);

    SpiWriteReg(CC1101_MDMCFG1,  0x02);
    SpiWriteReg(CC1101_MDMCFG0,  0xF8);
    SpiWriteReg(CC1101_CHANNR,   chan);
    SpiWriteReg(CC1101_DEVIATN,  0x00);
    SpiWriteReg(CC1101_FREND1,   0x56);
    SpiWriteReg(CC1101_MCSM0 ,   0x18);
    SpiWriteReg(CC1101_FOCCFG,   0x14);
    SpiWriteReg(CC1101_BSCFG,    0x6C);
    SpiWriteReg(CC1101_AGCCTRL2, 0x07);
    SpiWriteReg(CC1101_AGCCTRL1, 0x00);
    SpiWriteReg(CC1101_AGCCTRL0, 0x92);
    SpiWriteReg(CC1101_FSCAL3,   0xE9);
    SpiWriteReg(CC1101_FSCAL2,   0x2A);
    SpiWriteReg(CC1101_FSCAL1,   0x00);
    SpiWriteReg(CC1101_FSCAL0,   0x1F);
    SpiWriteReg(CC1101_FSTEST,   0x59);
    SpiWriteReg(CC1101_TEST2,    0x81);
    SpiWriteReg(CC1101_TEST1,    0x35);
    SpiWriteReg(CC1101_TEST0,    0x09);
    SpiWriteReg(CC1101_PKTCTRL1, 0x00);
    SpiWriteReg(CC1101_ADDR,     0x00);
    SpiWriteReg(CC1101_PKTLEN,   packetLen);

    SpiWriteReg(CC1101_IOCFG1, 0x2E);
    SpiWriteReg(CC1101_FIFOTHR, 0x0F);
    SpiWriteReg(CC1101_SYNC1, 0x00);
    SpiWriteReg(CC1101_SYNC0, 0x00);
    //SpiWriteReg(CC1101_FSCTRL0, 0x00); //if you have a cheaper chip, you will need to adjust this, frequency offset
    SpiWriteReg(CC1101_MCSM2, 0x07);
    SpiWriteReg(CC1101_MCSM1, 0x00);
    SpiWriteReg(CC1101_WOREVT1, 0x87);
    SpiWriteReg(CC1101_WOREVT0, 0x6B);
    SpiWriteReg(CC1101_WORCTRL, 0xFB);
    SpiWriteReg(CC1101_RCCTRL1, 0x41);
    SpiWriteReg(CC1101_RCCTRL0, 0x00);
    SpiWriteReg(CC1101_PTEST, 0x7F);
    SpiWriteReg(CC1101_AGCTEST, 0x3F);

}

void ELECHOUSE_CC1101::SetIdle(void)
{
    SpiStrobe(CC1101_SIDLE);
}
/****************************************************************
*FUNCTION NAME:SetTx
*FUNCTION     :set CC1101 send data
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SetTx(void)
{
  SpiStrobe(CC1101_SIDLE);
  SpiStrobe(CC1101_STX);        //start send
}
/****************************************************************
*FUNCTION NAME:SetRx
*FUNCTION     :set CC1101 to receive state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SetRx(void)
{
  SpiStrobe(CC1101_SRX);        //start receive
}
/****************************************************************
*FUNCTION NAME:SetTx
*FUNCTION     :set CC1101 send data and change frequency
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SetTx(float mhz)
{
  setMHZ(mhz);
  SpiStrobe(CC1101_SIDLE);
  SpiStrobe(CC1101_STX);        //start send
}
/****************************************************************
*FUNCTION NAME:SetRx
*FUNCTION     :set CC1101 to receive state and change frequency
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SetRx(float mhz)
{
  setMHZ(mhz);
  SpiStrobe(CC1101_SRX);        //start receive
}
/****************************************************************
*FUNCTION NAME:RSSI Level
*FUNCTION     :Calculating the RSSI Level
*INPUT        :none
*OUTPUT       :none
****************************************************************/
byte ELECHOUSE_CC1101::getRssi(void)
{
byte rssi;
rssi=SpiReadStatus(CC1101_RSSI);
if (rssi >= 128){rssi = (255 - rssi)/2+74;}
else{rssi = rssi/2+74;}
return rssi;
}
/****************************************************************
*FUNCTION NAME:LQI Level
*FUNCTION     :get Lqi state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
byte ELECHOUSE_CC1101::getLqi(void)
{
byte lqi;
lqi=SpiReadStatus(CC1101_LQI);
return lqi;
}
/****************************************************************
*FUNCTION NAME:SetSres
*FUNCTION     :Reset CC1101
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSres(void)
{
  SpiStrobe(CC1101_SRES);                  //reset cc1101  
}
/****************************************************************
*FUNCTION NAME:SendData
*FUNCTION     :use CC1101 send data
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::SendData(byte *txBuffer,byte size)
{
  //SpiWriteReg(CC1101_TXFIFO,size); //this was adding a "length byte" to beginning of packet
  SpiWriteReg(CC1101_TXFIFO, 0);  //even though PKTCTRL0 = 0x00, the chip was still adding length bit to signals. Have to Set length byte = zero to remove
  SpiWriteBurstReg(CC1101_TXFIFO,txBuffer,size);      //write data to send
  //SpiStrobe(CC1101_SCAL);
  SpiStrobe(CC1101_STX);                  //start send  
    while (!digitalRead(GDO0));               // Wait for GDO0 to be set -> sync transmitted  
    while (digitalRead(GDO0));                // Wait for GDO0 to be cleared -> end of packet
  SpiStrobe(CC1101_SFTX);                 //flush TXfifo
}
/****************************************************************
*FUNCTION NAME:CheckReceiveFlag
*FUNCTION     :check receive data or not
*INPUT        :none
*OUTPUT       :flag: 0 no data; 1 receive data 
****************************************************************/
byte ELECHOUSE_CC1101::CheckReceiveFlag(void)
{
	if(digitalRead(GDO0))			//receive data
	{
		while (digitalRead(GDO0));
		return 1;
	}
	else							// no data
	{
		return 0;
	}
}
/****************************************************************
*FUNCTION NAME:ReceiveData
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
byte ELECHOUSE_CC1101::ReceiveData(byte *rxBuffer)
{
	byte size;
	byte status[2];

	if(SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO)
	{
		size=SpiReadReg(CC1101_RXFIFO);
		SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,size);
		SpiReadBurstReg(CC1101_RXFIFO,status,2);
		SpiStrobe(CC1101_SFRX);
   
		return size;
	}
	else
	{
		SpiStrobe(CC1101_SFRX);
 		return 0;
	}
}

ELECHOUSE_CC1101 ELECHOUSE_cc1101;
