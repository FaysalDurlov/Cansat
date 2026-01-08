#include <SPI.h>
#include <LoRa.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize LoRa
  LoRa.setPins(10, 9, 2); // NSS, RESET, DIO0
  if (!LoRa.begin(433E6)) { 
    Serial.println("Starting LoRa failed!");
    while (0);
  }

  Serial.println("LoRa Receiver Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    
    Serial.println("Received: " + incoming);

    // Optional: parse the data if you want
    parseTelemetry(incoming);
  }     
}

void parseTelemetry(String data) {
  float temperature = extractValue(data, "T:", ",");
  float pressure = extractValue(data, "P:", ",");
  float altitude = extractValue(data, "A:", ",");
  float accX = extractValue(data, "X:", ",");
  float accY = extractValue(data, "Y:", ",");
  float accZ = extractValue(data, "Z:", "");

  Serial.println("--- Parsed Data ---");
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Pressure: "); Serial.println(pressure);
  Serial.print("Altitude: "); Serial.println(altitude);
  Serial.print("AccX: "); Serial.println(accX);
  Serial.print("AccY: "); Serial.println(accY);
  Serial.print("AccZ: "); Serial.println(accZ);
  Serial.println("-------------------");
}

float extractValue(String data, String key, String endChar) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) return 0;
  startIndex += key.length();
  int endIndex = endChar != "" ? data.indexOf(endChar, startIndex) : data.length();
  if (endIndex == -1) endIndex = data.length();
  String valueStr = data.substring(startIndex, endIndex);
  return valueStr.toFloat();
}
