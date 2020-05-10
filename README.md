ESP8266 wifi version of RfCat RfKitten

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

Modded setMHz to change TEST0 registerModded setCCMode. Changed serial mode to TX FIFO, packet has no address, length, or crc bytes

Modded SendData. First byte in TX FIFO is packet length? Changed to 0. Packet length byte was being added to packets

rfmoggy_minimal_wifiClient.ino  **********************************

Connects to your wifi network. Simple web interface

Touchtunes Jukebox Menu - Signals for controlling jukeboxes commonly found in US bars

Garage Door Menu - Signals for brute forcing DIP Switch remote controls. Protocols extracted from KLIK3U Universal Garage Door Remote

RfPwnOn Menu - Creates an OOK binary signal for brute forcing and/or transmitting


