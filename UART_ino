#include <HardwareSerial.h>

#define UART_RX_PIN 16 // Define UART RX pin
#define UART_TX_PIN 17 // Define UART TX pin
#define BAUD_RATE 9600 // Define baud rate
#define Act_PIN 2 // Define actuator pin

HardwareSerial boltSerial(1); 

void setup() {
  Serial.begin(115200); 
  boltSerial.begin(BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN); 
  pinMode(Act_PIN, OUTPUT); 
}

void loop() {
  
  boltSerial.println("Hello from ESP32!");

  //code to receive data from Bolt
  if (boltSerial.available()) {
    String receivedData = boltSerial.readString();
    Serial.println("Received data from Bolt: " + receivedData);
    if (receivedData.startsWith("PUMP_ON")) {
      digitalWrite(Act_PIN, HIGH);
      Serial.println("PUMP_ON command received, turning on Act_PIN.");
      
    } else if (receivedData.startsWith("PUMP_OFF")) {
      digitalWrite(Act_PIN, LOW);
      Serial.println("PUMP_OFF command received, turning off Act_PIN.");

    } else {
      Serial.println("Unknown command received.");
    }
  }

  delay(5000); // Delay for 5 seconds
}
