/*
The sensor reading client.
Reads data from dallas DS1820 sensors,
packs it into json and
send its to server via WiFI,
using ESP8266 chip.
Description of AT commands used
for communication with the esp8266
can be found here:
https://github.com/espressif/ESP8266_AT/wiki/basic_at_0019000902
*/

#include <OneWire.h>
#include <ArduinoJson.h>  
#include <SoftwareSerial.h>

SoftwareSerial ESPserial(3, 4); // RX | TX

OneWire  ds(2);  // on pin 10 (a 4.7K resistor is necessary)

void setup(void) {
    Serial.begin(9600);
    ESPserial.begin(9600);  
  
    // Reset esp chip and connect to AP (Access point0
    Serial.println(sendData("AT+RST\r\n", 3000));
    Serial.println(sendData("AT+CWJAP=\"OLIMP WiFi\",\"itiktitikt\"\r\n", 10000)); 
    Serial.println(sendData("AT+CWMODE=1\r\n", 5000));
    Serial.println(sendData("AT+CIPMUX=0\r\n", 3000));  
}

void loop(void) {
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;
    String address;
    int size1;
  
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
  
    if ( !ds.search(addr)) {
      ds.reset_search();
      delay(250);
      return;
    }
    switch (addr[0]) {
    case 0x10:  // DS1820
      type_s = 1;
      break;
    case 0x28:  // DS18B20
      type_s = 0;
      break;
    case 0x22:  // DS1822
      type_s = 0;
      break;
    default:
      return;
    }
  
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
  
    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);         // Read Scratchpad
    
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }
  
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } 
    else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      // default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
  
    // Send chip address and temperature
    for ( i = 0; i < 8; i++) {
    address += String(addr[i], HEX);
    }
    root["sensor"] = "temperature";
    root["address"] = address;
    root["data"] = celsius;
  
    sendJSON(root, root.measureLength());
}

String sendData(String command, const int timeout)
{
    String response = "";
    
    ESPserial.print(command); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(ESPserial.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = ESPserial.read(); // read the next character.
        response+=c;
      }
      // break;  
    }

    return response;
}

int sendJSON(JsonObject& packet, const int size)
{
 
      sendData("AT+CIPSTART=\"UDP\",\"192.168.10.40\",1234\r\n", 3000);
      String command_size = "AT+CIPSEND=" + String(size) + "\r\n";
      sendData(command_size, 3000);
      packet.printTo(ESPserial);
      delay(5000);
      sendData("AT+CIPCLOSE\r\n", 3000);
      return 0;
}
