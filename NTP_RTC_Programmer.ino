/*
  RTC Programmer NTP
  
  Configure a DS1307 RTC using the time collected from an NTP server.  Netowkring configured using DHCP
  

  Requires 
    Arduino Ethernet Shield  
    DS1307 RTC 
      Pinout:   +5V    Vcc
                GND    GND
                A4     SCL
                A5     SQW
*/

#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <RTClib.h>

//RTC Encapsulation
RTC_DS1307 RTC;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };//MAC Address for Ethernet Shield

unsigned int localPort = 8888; //Local port to listen for UDP Packets

IPAddress timeServer(192, 43, 244, 18);//NTP Server IP (time.nist.gov)

const int NTP_PACKET_SIZE= 48;  //NTP Time stamp is in the firth 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE];  //Buffer to hold incomming and outgoing packets

EthernetUDP Udp;  //UDP Instance to let us send and recieve packets

unsigned long epoch; //Unix Epoch time (NTP or RTC depending on state)

void setup() {
  Serial.begin(9600); //Start the serial interface

  Wire.begin();  //Start Wire
  
  RTC.begin();  //Start the RTC

  delay(1000);
  //check to see if the RTC is already configured
  if( !RTC.isrunning() ) {
    Serial.println("RTC Not Configured");
    Serial.println("Starting Ethernet");  //configure Ethernet
    if(Ethernet.begin(mac) == 0) {
      Serial.println("Fatal Error: Unable to obtain DHCP address");
      for(;;)  //do nothing forever!
        ;      //TODO Change this to retry a few times
    }
    
    Serial.println("Ethernet Configured");
    Udp.begin(localPort);

    //get the NTP timestamp
    epoch = getNTP();
    
    //set the RTC
    RTC.adjust(epoch);
    
    //display what we did!
    Serial.println("RTC Configured:");
    
    //show Unix Epoch
    Serial.print("Unix Epoch: ");
    Serial.println(epoch);
    
    //show UTC
    Serial.print("UTC: ");
    showUTC(epoch);
  } else {
    Serial.println("RTC Already Configured");
  }
}

void loop() {
  //repeatedly show the current date and time, taken from the RTC
  DateTime now = RTC.now();

  Serial.print(now.unixtime());
  Serial.print(" - ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" - ");

  showUTC(now.unixtime());
  
  delay(5000);
}

void showUTC(unsigned long epoch) {
  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');  
  if ( ((epoch % 3600) / 60) < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':'); 
  if ( (epoch % 60) < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.println(epoch %60); // print the second
}

unsigned long getNTP() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);  
  if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    
    return epoch;
  }
}

// send an NTP request to the time server at the given address 
void sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:         
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}




