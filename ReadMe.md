#
    FILE: HX711_R4_R3_v01
    AUTHOR: RAM
    PURPOSE: Handle load cell data collection using the HX711 amplifier
    STATUS: runs on both R4 and R3 versions. Tested 12/13/2023

    this version has working components hi-speed looping while writing to open file
    - to change: clean so doesn't use as much space
    - make saving data faster by only accessing RTC once per loop, put that time stamp on every one in that loop - needed?
    - set the rtc to the computer time - uses RTCLib (not RTC)

    on R4 compile: 
    Sketch uses 74148 bytes (28%) of program storage space. Maximum is 262144 bytes.
    Global variables use 6672 bytes (20%) of dynamic memory, leaving 26096 bytes for local variables. Maximum is 32768 bytes. 
    
    on R3 compile:
    Sketch uses 20266 bytes (62%) of program storage space. Maximum is 32256 bytes.
    Global variables use 1645 bytes (80%) of dynamic memory, leaving 403 bytes for local variables. Maximum is 2048 bytes.

#