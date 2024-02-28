#include <Arduino.h>

void setup() {
  // Initialize serial communication at a baud rate of 115200
  Serial.begin(115200);
  // Wait for the serial connection to be established
  while (!Serial) {
    delay(10);
  }
}

void loop() {
  if (Serial.available()) {
    /* auto user_input = static_cast<char>(Serial.read()); */
    auto user_input = Serial.readString();

    if (user_input == "on") {
      Serial.println("LED ON");
    } else if (user_input == "off") {
      Serial.println("LED OFF");
    } else {
      Serial.println("Invalid input");
    }
  }
}
