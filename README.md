NodeMCU ESP8266 wifi version of RfCat RfKitten using inexpensive CC1101

The code is not memory efficient, but it runs fine (didn't pass any variables e.g. String by reference, etc.)

I tried to keep the INO code simple so users & beginners can easily understand, mod, create your own personalized version of RFmoggy 


CC1101 Registers set to transmit OOK signals only

ELECHOUSE_CC1101_SRC_DRV.cpp  ******************************

Modified version of LSatan CC1101 library

Added set_rxbw function (copied from LSatan debug tool)

Added set_drate

Added split_mdmcgf4

Added set_mhz (repetitive, for testing)

Added setPktLen (not needed even though rfcat examples always use it in fixed length mode)

Added setIdle (for testing. MCSM1 register sets to idle automatically)

Added all registers to Init function. Got register values for ASK/OOK from docs&smartrf studio, space teddy, rfkitten

Modded setMHz to change TEST0 register

Modded setCCMode. Changed serial mode to TX FIFO, packet has no address, length, or crc bytes

Modded SendData. Removed SFTX (tx fifo flush). Not needed if no Underflow, make sure packet sizes equal PKTLEN register!

Modded SendData. Don't send length byte as first byte in TX FIFO.

rfmoggy_minimal_wifiClient.ino  **********************************

Connects to your wifi network. Simple web interface

Touchtunes Jukebox Menu - Signals for controlling jukeboxes commonly found in US bars

Garage Door Menu - Signals for brute forcing DIP Switch remote controls. Protocols extracted from KLIK3U Universal Garage Door Remote

De Bruijn Menu - Generates De Bruijn sequences for garage door models. From Samy Kamkar Open Sesame 

RfPwnOn Menu - Creates an OOK binary signal for brute forcing and/or transmitting


