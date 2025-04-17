# LC29H-GPS-
LC29H GPS Module Interface for Zephyr RTOS

This repository contains a robust Zephyr RTOS driver for the LC29H GPS module, featuring:

    Complete NMEA sentence parsing (GPGGA, GPRMC, ..)
    Configuration command management
    Real-time position/velocity data extraction
    Error handling and checksum verification

Features:
    Multi-sentence support: Handles both GPGGA and GPRMC NMEA sentences
    Real-time processing: Event-driven UART communication
    Data validation: Automatic checksum verification
    Configuration commands: Built-in support for LC29H-specific commands
    Structured data: Clean GPS data structure for easy integration

GPS NMEA 0183 Sentences Structure:
All NMEA 0183 sentences start with the $ sign and end with a carriage return and a line feed; each data field in the sentence is separated with a comma:
$aaaaa,df1,df2,df3*hh<CR><LF>
A NMEA 0183 sentence can have a maximum of 80 characters plus a carriage return and a line feed. Let us examine now a GPS NMEA 0183 example sentence:
$GPGGA,181908.00,3404.7041778,N,07044.3966270,W,4,13,1.00,495.144,M,29.200,M,0.10,0000,*40

Reference:
https://docs.arduino.cc/learn/communication/gps-nmea-data-101/

Example:
RMC Recommended Minimum Navigation Information
          1      2    3    4   5      6  7   8   9   10 11 12
          |      |    |    |   |      |  |   |   |    |  |  |
$--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a*hh
1) Time (UTC)
2) Status, V = Navigation receiver warning
3) Latitude
4) N or S
5) Longitude
6) E or W
7) Speed over ground, knots
8) Track made good, degrees true
9) Date, ddmmyy
10) Magnetic Variation, degrees
11) E or W
12) Checksum
