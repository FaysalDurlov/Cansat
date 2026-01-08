#include <SPI.h>
#include <LoRa.h>

// Pin configuration for LoRa
#define SS   10
#define RST  9
#define DIO0 2

void setup() {
  Serial.begin(9600);
  while (!Serial);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {  // Bangladesh frequency
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Receiver ready!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received packet
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.println("Received: " + incoming);
  }
}

