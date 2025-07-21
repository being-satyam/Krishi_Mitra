#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"
#include "images.h"

// OLED Display setup
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Sensor and Actuator pins
#define MOISTURE_PIN 19
#define PIR_PIN      21
#define WATER_LVL_PIN 18
#define ACT_Pin      4

// LoRa configuration
#define RF_FREQUENCY                866000000  // 866 MHz
#define TX_OUTPUT_POWER             14
#define LORA_BANDWIDTH              0
#define LORA_SPREADING_FACTOR       7
#define LORA_CODINGRATE             1
#define LORA_PREAMBLE_LENGTH        8
#define LORA_SYMBOL_TIMEOUT         0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON        false

#define BUFFER_SIZE 128
char txpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
bool lora_idle = true;

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED ON
  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "C-DOT LoRa Tx Init");
  display.display();

  pinMode(MOISTURE_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(WATER_LVL_PIN, INPUT);
  pinMode(ACT_Pin, OUTPUT);
  digitalWrite(ACT_Pin, HIGH);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  RadioEvents.TxDone = OnTxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(
    MODEM_LORA,
    TX_OUTPUT_POWER,
    0,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    true,
    0,
    0,
    LORA_IQ_INVERSION_ON,
    3000
  );

  display.drawString(0, 15, "LoRa Tx Ready");
  display.display();
  delay(1000);
}

void loop() {
  float raw_moist = analogRead(MOISTURE_PIN);
  float moist = (4095.0 - raw_moist) / 4095.0 * 100.0;

  const char* pumpStatus = (moist >= 65) ? "OFF" : "ON";
  digitalWrite(ACT_Pin, moist >= 65 ? HIGH : LOW);

  int pirStatus = digitalRead(PIR_PIN);
  int waterLevel = analogRead((WATER_LVL_PIN/4095.0)*100);
  

  // Format payload
  String payload = "Moist:" + String(moist, 1) +
                   " Pump:" + pumpStatus +
                   " PIR:" + String(pirStatus) +
                   " Water:" + String(waterLevel);
  payload.toCharArray(txpacket, BUFFER_SIZE);

  Serial.println("Sending LoRa Packet: " + payload);
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));

  // OLED Display
  display.clear();
  display.setFont(ArialMT_Plain_16);  //Medium Font Size
  display.drawString(0, 0, "C-DOT LoRa Tx");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 20, "Moist: " + String(moist, 1) + "%");
  display.drawString(0, 32, "Pump: " + String(pumpStatus));
  display.drawString(0, 44, "PIR: " + String(pirStatus));
  display.drawString(0, 56, "Water: " + String(waterLevel) + "%");
  display.display();

  delay(000);
}

void OnTxDone() {
  Serial.println("LoRa Packet Sent Successfully");
  lora_idle = true;
}
