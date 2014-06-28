/***************************************************
  This is an MQTT publish example for the Wicked Device WildFire
  
  This script uses some of the methods from the buildtest example for CC3000 that Limor Fried, Kevin Townsend and Phil Burgess 
  wrote for Adafruit Industries to make debugging easier.
 ****************************************************/

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <cc3000_PubSubClient.h>
//#include "ClickButton.h"

//ClickButton button(2, LOW, CLICKBTN_PULLUP);
boolean on;

#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

#define DEVICE_NAME "CC3000"

// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
                                         ADAFRUIT_CC3000_IRQ, 
                                         ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "XXXXXXXX"        // cannot be longer than 32 characters!
#define WLAN_PASS       "XXXXXXXX"


// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

Adafruit_CC3000_Client client;
     
// We're going to set our broker IP and union it to something we can use
union ArrayToIp {
  byte array[4];
  uint32_t ip;
};

void callback (char* topic, byte* payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(" - Payload:");
  Serial.write(payload, length);
  Serial.println("");
  if (length == 2 && strncmp((char*)payload,"on",2)==0) {
    digitalWrite(8, HIGH);
    digitalWrite(7, LOW);
    on = true;
  } else if (length == 3 && strncmp((char*)payload,"off",3)==0) {
    digitalWrite(8, LOW);
    digitalWrite(7, HIGH);
    on = false;
  }else{
    //Serial.println((int)payload);
  //analogWrite(8, payload);
  }
}

ArrayToIp server = { 226,117,163,46 };
cc3000_PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000);

void button(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 300) 
  {
    if(on){
       digitalWrite(7, HIGH);
       digitalWrite(8, LOW);
       mqttclient.publish("devices/asd/action","off");
       on = false;
     }else{ 
       digitalWrite(7, LOW);
       digitalWrite(8, HIGH);
       mqttclient.publish("devices/asd/action","on");
       on = true;
     }
  }
  last_interrupt_time = interrupt_time;
}

void(* resetFunc) (void) = 0;

void setup(void)
{
  pinMode(4,OUTPUT);
  pinMode(2,INPUT);
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(8,OUTPUT);
  
  //button.debounceTime   = 20;   // Debounce timer in ms
  
  digitalWrite(4, LOW);
  digitalWrite(6, HIGH);
  digitalWrite(2, HIGH);
  
    attachInterrupt(0, button, FALLING);

    
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));
  
  if(digitalRead(2)==LOW){
      if(!cc3000.begin()) Serial.println(F("Failed. Check your wiring?"));
    Serial.println(F("\nDeleting old connection profiles"));
    if (!cc3000.deleteProfiles()) {
      Serial.println(F("Failed!"));
    }
    resetFunc();
  }

  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);

  displayDriverMode();
  
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin(false, true, DEVICE_NAME))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
  
  digitalWrite(8, HIGH);
  delay(100);
  digitalWrite(8, LOW);  
    delay(50);
  digitalWrite(8, HIGH);
  delay(100);
  digitalWrite(8, LOW); 
  
    Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
      while(!cc3000.startSmartConfig(DEVICE_NAME))
      {
        Serial.println(F("SmartConfig failed, again waiting for a SmartConfig connection ...")); 
      }
  }
  
  digitalWrite(8, HIGH);
  delay(100);
  digitalWrite(8, LOW); 
  
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */  
  while (!displayConnectionDetails()) {
    delay(1000);
  }
   
   // connect to the broker
   if (!client.connected()) {
     client = cc3000.connectTCP(server.ip, 1883);
   }
   
   // did that last thing work? sweet, let's do something
   if(client.connected()) {
    if (mqttclient.connect("arduinoClient", "asd", "asd")) {
      mqttclient.subscribe("devices/asd");
    }
   } 
}

void loop(void) {
  
   mqttclient.loop();
   
  /*button.Update();
   
  if(button.clicks == 1){
     if(on){
       digitalWrite(7, HIGH);
       digitalWrite(8, LOW);
       mqttclient.publish("devices/asd/action","off");
       on = false;
     }else{ 
       digitalWrite(7, LOW);
       digitalWrite(8, HIGH);
       mqttclient.publish("devices/asd/action","on");
       on = true;
     }
   }*/
}




/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
