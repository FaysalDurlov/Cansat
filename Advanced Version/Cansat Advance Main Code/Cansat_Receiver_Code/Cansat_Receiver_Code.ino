#include <SPI.h>
#include <LoRa.h>

#define LORA_NSS  10
#define LORA_RST  9
#define LORA_DIO0 2

// Must match transmitter
#define LORA_FREQUENCY 433E6


void setup() {

  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {

    Serial.println("LoRa init failed.");
    Serial.println("Check wiring and frequency.");

    while (1) {
      delay(10);
    }
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("       CanSat LoRa Receiver");
  Serial.println("========================================");
  Serial.println("LoRa initialized successfully.");
  Serial.println("Waiting for telemetry...");
  Serial.println();
}


void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    // -----------------------------------------
    // Receive packet
    // -----------------------------------------

    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // -----------------------------------------
    // Convert received string into values
    // -----------------------------------------

    float values[12];

    int index = 0;
    int start = 0;

    for (int i = 0; i <= received.length(); i++) {

      if (received[i] == ',' || i == received.length()) {

        String value = received.substring(start, i);

        values[index] = value.toFloat();

        index++;

        start = i + 1;

        if (index >= 12) {
          break;
        }
      }
    }

    // -----------------------------------------
    // Display data
    // -----------------------------------------

    Serial.println();
    Serial.println("========================================");
    Serial.println("           CANSAT DATA");
    Serial.println("========================================");

    // Packet ID
    Serial.print("Packet ID          : ");
    Serial.println((int)values[0]);

    Serial.println();
    Serial.println("---------- MPU-9250 DATA ----------");

    // Accelerometer
    Serial.print("Acceleration X     : ");
    Serial.print(values[1], 3);
    Serial.println(" g");

    Serial.print("Acceleration Y     : ");
    Serial.print(values[2], 3);
    Serial.println(" g");

    Serial.print("Acceleration Z     : ");
    Serial.print(values[3], 3);
    Serial.println(" g");

    Serial.println();

    // Gyroscope
    Serial.print("Gyroscope X        : ");
    Serial.print(values[4], 3);
    Serial.println(" deg/s");

    Serial.print("Gyroscope Y        : ");
    Serial.print(values[5], 3);
    Serial.println(" deg/s");

    Serial.print("Gyroscope Z        : ");
    Serial.print(values[6], 3);
    Serial.println(" deg/s");

    Serial.println();

    // MPU temperature
    Serial.print("MPU Temperature    : ");
    Serial.print(values[7], 2);
    Serial.println(" °C");


    Serial.println();
    Serial.println("---------- BME688 DATA ----------");

    // BME temperature
    Serial.print("Temperature        : ");
    Serial.print(values[8], 2);
    Serial.println(" °C");

    // Humidity
    Serial.print("Humidity           : ");
    Serial.print(values[9], 2);
    Serial.println(" %");

    // Pressure
    Serial.print("Pressure           : ");
    Serial.print(values[10], 2);
    Serial.println(" hPa");

    // Gas
    Serial.print("Gas / Air Quality  : ");
    Serial.print(values[11], 2);
    Serial.println("");


    Serial.println();
    Serial.println("---------- LoRa SIGNAL ----------");

    Serial.print("RSSI               : ");
    Serial.print(LoRa.packetRssi());
    Serial.println(" dBm");

    Serial.print("SNR                : ");
    Serial.print(LoRa.packetSnr());
    Serial.println(" dB");

    Serial.println("========================================");
    Serial.println();
  }
}