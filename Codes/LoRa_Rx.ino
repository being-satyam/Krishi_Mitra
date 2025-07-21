#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "HT_SSD1306Wire.h"
#include "images.h"

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

const char* ssid = "";
const char* password = "";
const char* serverURL = "http://<PC-IP>:PORT/post-data";

#define RF_FREQUENCY 866000000
#define TX_OUTPUT_POWER 14
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8

#define BUFFER_SIZE 128
char rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
int16_t rssi, rxSize;
bool lora_idle = true;

void postData(float moist, String pump, int pir, float water) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"moisture\":" + String(moist, 2) + ",";
    payload += "\"pump\":\"" + pump + "\",";
    payload += "\"pir\":" + String(pir) + ",";
    payload += "\"water_level\":" + String(water, 2);
    payload += "}";

    int httpResponseCode = http.POST(payload);
    Serial.println("HTTP POST Response: " + String(httpResponseCode));
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send data.");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "Connecting WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display.drawString(0, 15, "WiFi Connected");
  display.display();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(
    MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH, 0, false,
    0, true, 0, 0, false, true
  );

  display.drawString(0, 30, "LoRa Receiver Ready!");
  display.display();
}

void loop() {
  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi_val, int8_t snr) {
  rxSize = size;
  rssi = rssi_val;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Radio.Sleep();

  String data = String(rxpacket);
  Serial.println("Received: " + data);

  float moist = 0, water = 0;
  int pir = 0;
  String pump = "";

  int moistIndex = data.indexOf("Moist:");
  int pumpIndex = data.indexOf("Pump:");
  int pirIndex = data.indexOf("PIR:");
  int waterIndex = data.indexOf("Water:");

  if (moistIndex != -1 && pumpIndex != -1 && pirIndex != -1 && waterIndex != -1) {
    String moistStr = data.substring(moistIndex + 6, pumpIndex); moistStr.trim();
    String pumpStr = data.substring(pumpIndex + 5, pirIndex); pumpStr.trim();
    String pirStr = data.substring(pirIndex + 4, waterIndex); pirStr.trim();
    String waterStr = data.substring(waterIndex + 6); waterStr.trim();

    moist = moistStr.toFloat();
    pump = pumpStr;
    pir = pirStr.toInt();
    water = waterStr.toFloat();

    // Display on OLED
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Moist: " + String(moist, 1) + "%");
    display.drawString(0, 12, "Pump: " + pump);
    display.drawString(0, 24, "Motion: " + String(pir ? "YES" : "No"));
    display.drawString(0, 36, "Water: " + String(water, 1) + "%");
    display.display();

    postData(moist, pump, pir, water);
  } else {
    Serial.println("Invalid packet format.");
  }

  lora_idle = true;
}
