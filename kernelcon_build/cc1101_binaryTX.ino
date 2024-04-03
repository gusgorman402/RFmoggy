#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
//#include <cc1101_debug_service.h>

#define max_samples 5000

/* Put your SSID & Password */
const char* ssid = "RFmoggy";  // Enter SSID here
const char* password = "diymoggy";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0); 

/* Wifi Client mode */
//const char* ssid = "yourWifiNetwork";  // Enter SSID here
//const char* password = "yourWifiPassword";  //Enter Password here



ESP8266WebServer server(80);   //instantiate server at port 80 (http port)

int gdo0;
uint8_t chipstate;

int data_pulses[max_samples];
String signalData;

//I was too lazy to write a de bruijn generator. Generated de bruijn sequences from https://damip.net/article-de-bruijn-sequence

String menu_htmlHeader;
String main_menu = "";
String binarytx_menu = "";


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handleMainMenu() {
 Serial.println("User called the main menu");
 //String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", main_menu); //Send web page
}


void handleTxBinaryMenu(){
  Serial.println("User called the Binary TX menu");
  server.send(200, "text/html", binarytx_menu);
}



void handleTxBinarySignal(){
  //Serial.println(ESP.getFreeHeap(),DEC);
  //int data_pulses[max_samples];
  //String signalData;

  int counter=0;
  int pos = 0;
  bool invert_pulses = false;

  float frequency_MHz = server.arg("txbinaryMHz").toFloat();
  int RF_modulation = server.arg("txbinaryModulation").toInt();
  float deviation_kHz = server.arg("txbinaryDeviation").toFloat();
  //float datarate_kbps = server.arg("txbinaryBaud").toFloat();
  int samplepulse = server.arg("txbinaryPulseSize").toInt();
  unsigned int gap_minimum = server.arg("txbinaryGapMinimum").toInt();
  if( server.arg("invert").equals("true") ){ invert_pulses = true; }
  signalData = server.arg("txbinarySignal");

  ELECHOUSE_cc1101.setModulation(RF_modulation);
  ELECHOUSE_cc1101.setDeviation(deviation_kHz);
  ELECHOUSE_cc1101.setDRate(250.0);
  ELECHOUSE_cc1101.setMHZ(frequency_MHz);

  for (int i=0; i < max_samples; i++){
    data_pulses[i]=0;
  }

  signalData.replace(" ","");
  signalData.replace("\n","");
  signalData.replace("Pause:","");
  int count_binconvert = 0;
  String lastbit_convert="1";
  //Serial.println("");Serial.print("Received data: ");
  //Serial.println(signalData);
  //Serial.println(ESP.getFreeHeap(),DEC);

  for (int i = 0; i < signalData.length()+1; i++){
    if (lastbit_convert != signalData.substring(i, i+1)){
      if (lastbit_convert == "1"){
        lastbit_convert="0";
      }else if (lastbit_convert == "0"){
        lastbit_convert="1";
      }
      count_binconvert++;
    }
    
    if (signalData.substring(i, i+1)=="["){
      data_pulses[count_binconvert]= signalData.substring(i+1, signalData.indexOf("]", i)).toInt();
      lastbit_convert="0";
      i += signalData.substring(i, signalData.indexOf("]", i)).length();
    }else{
      data_pulses[count_binconvert] += samplepulse;
    }
  }

  //Serial.print("Number of pulses: "); Serial.println(count_binconvert);

  for (int i = 0; i < count_binconvert; i++){
    //if( i % 2 != 0){ data_pulses[i] *= -1; }  //give odd numbered array positions a negative pulse value
    Serial.print(data_pulses[i]);
    Serial.print(",");
  }

  setTX_async();
  ELECHOUSE_cc1101.SetTx();
  chipstate = 0xFF;
  while(chipstate != 0x13){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }
  
  //works!!!
  //for (int i = 0; i<count_binconvert; i+=2){
   // digitalWrite(5,HIGH);
   // delayMicroseconds(data_pulses[i]);
   // digitalWrite(5,LOW);
//delayMicroseconds(data_pulses[i+1]);
 // }
  //end of working code
  // delay(2000);
 

  //if( gap_minimum < 2500 ){ gap_minimum = 2500; }
  //if( gap_minimum >= 2500){ gap_minimum = gap_minimum - 2500; }

  int mod_value = 0;
  if( invert_pulses == true ){ mod_value = 1; }
  for(int x = 0; x <= count_binconvert; x++){
    //Serial.println("Sending pulses from binary string: ");
    //Serial.print(data_pulses[x]); Serial.print(",");
    //if(data_pulses[x] >= gap_minimum){
      //ELECHOUSE_cc1101.setSidle();
      //delayMicroseconds(data_pulses[x] - 2500); //going from idle to Tx takes extra ~2.5ms
     // ELECHOUSE_cc1101.SetTx();
      //continue;
    //}
    if(x % 2 == mod_value){
      digitalWrite(gdo0, HIGH);
      delayMicroseconds(data_pulses[x]);
    }
    else{
      digitalWrite(gdo0, LOW);
      delayMicroseconds(data_pulses[x]);
    }
  }

  Serial.println();
  Serial.println("Transmission complete");

  ELECHOUSE_cc1101.setSidle();
  //setTX_FIFO();
  signalData = "";
  
  server.send(200, "text/html", "<h1>Signal is sent</h1><p><a href=\"/\">Return to Main Menu</a></p><p><a href=\"txbinary\">Return to TX Binary Signal</a></p>");
}


void setTX_async(){
  chipstate = 0xFF;
  ELECHOUSE_cc1101.setSidle();
  while(chipstate != 0x01){ chipstate = (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F); }

  pinMode(gdo0, OUTPUT);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x0D);
  ELECHOUSE_cc1101.setPktFormat(3);
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
  
  
  //ELECHOUSE_cc1101.setCCMode(0);  //any value besides 1
  //ELECHOUSE_cc1101.setModulation(2); //modulation is ASK by default
  ELECHOUSE_cc1101.setMHZ(433.92);
  //ELECHOUSE_cc1101.set_rxbw(100);
  ELECHOUSE_cc1101.setDRate(1.777);
  //ELECHOUSE_cc1101.setIdle();
  //ELECHOUSE_cc1101.setPA(12); 


//wifi client mode
/*************************************************************************/

  //WiFi.mode(WIFI_STA); //ESP8266 will transmit most recent AP SSID without this line
  //WiFi.begin(ssid, password);     //Connect to your WiFi router
  //Serial.println("");

  // Wait for connection
  //while (WiFi.status() != WL_CONNECTED) {
    //delay(500);
    //Serial.print(".");
  //}
 
  //If connection successful show IP address in serial monitor
  //Serial.println("");
  //Serial.print("Connected to ");
  //Serial.println(ssid);
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());  //IP address assigned to your ESP */

/*************************************************************************/



//wifi host mode
/*************************************************************************/
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 11);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
/*************************************************************************/
  
  server.on("/", handleMainMenu);      //Which routine to handle at root location. This is display page
  
  server.on("/txbinary", handleTxBinaryMenu);
  
  server.on("/txbinarysignal", HTTP_POST, handleTxBinarySignal);
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
  main_menu += "<h3>ESP8266 CC1101 Sub-GHz Transmitter</h3>";
  
  main_menu += "<p><a href=\"txbinary\">Transmit Binary Signal</a></p>";
  main_menu += "<br>Frequency Range: 300-348 MHz, 387-464 MHz, 779-928MHz";
  main_menu += "<br><br>Warning: No error checks for inputs. Your responsibility to verify inputs are valid (e.g. MHz ranges)";
  main_menu += "</body></html>";

  
  binarytx_menu += menu_htmlHeader;
  binarytx_menu += "</style></head>";
  binarytx_menu += "<title>TX Binary Signal</title>";
  binarytx_menu += "<body><h1>TX Binary Signal</h1>";
  //binarytx_menu += "<h2>Transmit Signal Binary</h3>";
  binarytx_menu += "<h3>Copy string from URH Interpretation Window<h3>";
  binarytx_menu += "<form action=\"txbinarysignal\" method=\"POST\">";
  binarytx_menu += "<p><label>Transmit Freq MHz </label><input type=\"text\" name=\"txbinaryMHz\" size=\"10\" value=\"315.0\"></p>";
  binarytx_menu += "<p><label>Modulation </label><input type=\"text\" name=\"txbinaryModulation\" size=\"10\" value=\"0\"><br>";
  binarytx_menu += "<p>0 = 2FSK ** 1 = GFSK ** 2 = ASK</p>";
  binarytx_menu += "<p><label>Deviation (kHz) </label><input type=\"text\" name=\"txbinaryDeviation\" size=\"10\" value=\"40.0\"></p>";
  //binarytx_menu += "<p><label>Baud (kbps) (any value higher than signal datarate)</label><input type=\"text\" name=\"txbinaryBaud\" size=\"10\" value=\"250.0\"></p>";
  binarytx_menu += "<p><label>Pulse size (microseconds)</label><input type=\"text\" name=\"txbinaryPulseSize\" size=\"10\" value=\"60\"></p>";
  //binarytx_menu += "<p><label>Minimum gap length (pulses bigger than this are considered gaps)</label><input type=\"text\" name=\"txbinaryGapMinimum\" size=\"10\" value=\"100000\"></p>";
  binarytx_menu += "<input type=\"checkbox\" name=\"invert\" value=\"true\">Invert pulse polarity<br>";
  binarytx_menu += "<p><label>Binary signal:</label></p>";
  binarytx_menu += "<textarea id=\"txbinarySignal\" name=\"txbinarySignal\" rows=\"5\" cols=\"50\"></textarea><br>";
  binarytx_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  binarytx_menu += "<br>ASK signals work best<br>";
  binarytx_menu += "Large sequences (> ~2000 pulses) may crash device<br>";
  binarytx_menu += "</form><p><a href=\"/\">Return to Main Menu</a></p></body></html>";

}


void loop(void){
  server.handleClient();          //Handle client requests
  //cc1101_debug.debug();
}
