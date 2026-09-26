#include <SPI.h>
#include <LoRa.h>

#define LORA_NSS  10
#define LORA_RST  9
#define LORA_DIO0 2

// Must match transmitter
#define LORA_FREQUENCY 433E6

// Number of comma-separated fields in the packet
#define NUM_FIELDS 19

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

  Serial.println("CanSat LoRa Receiver ready. Waiting for telemetry...");
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
    // id,ax,ay,az,gx,gy,gz,mpuTemp,bmeTemp,hum,pres,gas,relAlt,lat,lon,gpsAlt,sats,fix,servoState
    // -----------------------------------------

    double values[NUM_FIELDS];

    int index = 0;
    int start = 0;

    for (int i = 0; i <= received.length(); i++) {

      if (received[i] == ',' || i == received.length()) {

        String value = received.substring(start, i);

        values[index] = value.toDouble();

        index++;

        start = i + 1;

        if (index >= NUM_FIELDS) {
          break;
        }
      }
    }

    // If the packet was malformed / short, skip it
    if (index < NUM_FIELDS) {
      Serial.println("{\"error\":\"malformed_packet\"}");
      return;
    }

    // -----------------------------------------
    // Build JSON output
    // -----------------------------------------

    String json = "{";

    json += "\"packet_id\":" + String((long)values[0]) + ",";

    json += "\"mpu\":{";
    json += "\"accel\":{\"x\":" + String(values[1], 3) + ",\"y\":" + String(values[2], 3) + ",\"z\":" + String(values[3], 3) + "},";
    json += "\"gyro\":{\"x\":" + String(values[4], 2) + ",\"y\":" + String(values[5], 2) + ",\"z\":" + String(values[6], 2) + "},";
    json += "\"temp_c\":" + String(values[7], 2);
    json += "},";

    json += "\"bme688\":{";
    json += "\"temp_c\":" + String(values[8], 2) + ",";
    json += "\"humidity\":" + String(values[9], 2) + ",";
    json += "\"pressure_hpa\":" + String(values[10], 2) + ",";
    json += "\"gas_kohm\":" + String(values[11], 2) + ",";
    json += "\"rel_altitude_m\":" + String(values[12], 2);
    json += "},";

    json += "\"gps\":{";
    json += "\"lat\":" + String(values[13], 6) + ",";
    json += "\"lon\":" + String(values[14], 6) + ",";
    json += "\"alt_m\":" + String(values[15], 2) + ",";
    json += "\"satellites\":" + String((int)values[16]) + ",";
    json += "\"fix\":" + String((int)values[17] == 1 ? "true" : "false");
    json += "},";

    json += "\"parachute\":{";
    json += "\"servo_open\":" + String((int)values[18] == 1 ? "true" : "false");
    json += "},";

    json += "\"lora\":{";
    json += "\"rssi\":" + String(LoRa.packetRssi()) + ",";
    json += "\"snr\":" + String(LoRa.packetSnr(), 2);
    json += "}";

    json += "}";

    Serial.println(json);
  }
}