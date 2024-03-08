#include "esp32-hal-gpio.h"
#include "esp_mac.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()

#define CHANNEL 1

/* #define LED_PIN 0 */
#define LED_PIN 10
#define ADC_PIN 4
#define ADC_ENABLE_PIN 6
#define BUTTON_PIN 7

/* Adafruit_NeoPixel LED = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
 */

constexpr uint8_t controller[] = {0xDC, 0x54, 0x75, 0x62, 0x50, 0xFC};

typedef struct struct_message_button {
  uint8_t local_mac[6];
  float battery_level;
} struct_message_button;

typedef struct struct_message_controller {
  bool answer;
} struct_message_controller;

struct_message_button buttonData;
struct_message_controller controllerData;
esp_now_peer_info_t controllerInfo;

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

/* void colorWipe(uint32_t c) { */
/*   for (uint8_t i = 0; i < LED.numPixels(); i++) { */
/*     LED.setPixelColor(i, c); */
/*     LED.show(); */
/*   } */
/* } */

float get_battery_voltage() {
  digitalWrite(ADC_ENABLE_PIN, LOW);
  delayMicroseconds(10);
  int sum = 0;
  for (int i = 0; i < 100; i++) {
    sum = sum + analogRead(ADC_PIN);
  }
  float result = sum / 100.0;
  digitalWrite(ADC_ENABLE_PIN, HIGH);
  return float(result) * (1.42) - 50;
}

void sendButtonData() {
  /* colorWipe(LED.Color(0, 255, 0)); */
  esp_read_mac(buttonData.local_mac, ESP_MAC_WIFI_STA);
  /* buttonData.battery_level = get_battery_voltage(); */
  buttonData.battery_level = random(0, 100);
  // Send message via ESP-NOW
  esp_err_t result =
      esp_now_send(controller, (uint8_t *)&buttonData, sizeof(buttonData));
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  /* colorWipe(LED.Color(0, 0, 0)); */
  if (!controllerData.answer) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                  : "Delivery Fail");
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&controllerData, incomingData, sizeof(controllerData));

  if (controllerData.answer) {
    Serial.println("Controller answered");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("Controller didn't answer");
    digitalWrite(LED_PIN, LOW);
  }

  Serial.print("Bytes received: ");
  Serial.println(len);
}

void IRAM_ATTR onTimer() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    sendButtonData();
  }
}

hw_timer_t *read_button_timer = NULL;

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(ADC_ENABLE_PIN, HIGH);
  analogReadResolution(12);

  btStop();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Init ESP-NOW
  InitESPNow();

  /* LED.begin(); */
  /* LED.show();             // Disable all the pixel by default when
   * initialized */
  /* LED.setBrightness(255); // Set brightness */

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(controllerInfo.peer_addr, controller, 6);
  controllerInfo.channel = CHANNEL;
  controllerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&controllerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  /* attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), sendButtonData,
   * FALLING); */
  read_button_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(read_button_timer, &onTimer, true);
  timerAlarmWrite(read_button_timer, 50000, true);
  timerAlarmEnable(read_button_timer);
}

void loop() {}

/* #include "esp32-hal-gpio.h" */
/* #include "esp32-hal-timer.h" */
/* #include "esp_mac.h" */
/* #include <Adafruit_NeoPixel.h> */
/* #include <WiFi.h> */
/* #include <esp_now.h> */
/* #include <esp_wifi.h> // only for esp_wifi_set_channel() */

/* #define CHANNEL 11 */

/* /1* #define LED_PIN 0 *1/ */
/* #define LED_PIN 10 */
/* #define ADC_PIN 4 */
/* #define ADC_ENABLE_PIN 6 */
/* #define BUTTON_PIN 7 */

/* constexpr uint8_t controller[] = {0xDC, 0x54, 0x75, 0x62, 0x50, 0xFC}; */

/* typedef struct struct_message_button { */
/*   uint8_t local_mac[6]; */
/*   float battery_level; */
/* } struct_message_button; */

/* typedef struct struct_message_controller { */
/*   bool answer; */
/* } struct_message_controller; */

/* struct_message_button buttonData; */
/* struct_message_controller controllerData; */
/* esp_now_peer_info_t controllerInfo; */

/* // Init ESP Now with fallback */
/* void InitESPNow() { */
/*   WiFi.disconnect(); */
/*   if (esp_now_init() == ESP_OK) { */
/*     Serial.println("ESPNow Init Success"); */
/*   } else { */
/*     Serial.println("ESPNow Init Failed"); */
/*     // Retry InitESPNow, add a counte and then restart? */
/*     // InitESPNow(); */
/*     // or Simply Restart */
/*     ESP.restart(); */
/*   } */
/* } */

/* /1* void colorWipe(uint32_t c) { *1/ */
/* /1*   for (uint8_t i = 0; i < LED.numPixels(); i++) { *1/ */
/* /1*     LED.setPixelColor(i, c); *1/ */
/* /1*     LED.show(); *1/ */
/* /1*   } *1/ */
/* /1* } *1/ */
/* void check_esp_send(esp_err_t result) { */
/*   Serial.print("Send Status: "); */
/*   if (result == ESP_OK) { */
/*     Serial.println("Success"); */
/*   } else if (result == ESP_ERR_ESPNOW_NOT_INIT) { */
/*     // How did we get so far!! */
/*     Serial.println("ESPNOW not Init."); */
/*   } else if (result == ESP_ERR_ESPNOW_ARG) { */
/*     Serial.println("Invalid Argument"); */
/*   } else if (result == ESP_ERR_ESPNOW_INTERNAL) { */
/*     Serial.println("Internal Error"); */
/*   } else if (result == ESP_ERR_ESPNOW_NO_MEM) { */
/*     Serial.println("ESP_ERR_ESPNOW_NO_MEM"); */
/*   } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) { */
/*     Serial.println("Peer not found."); */
/*   } else if (result == ESP_ERR_ESPNOW_IF) { */
/*     Serial.println("Interface error."); */
/*   } else { */
/*     Serial.println("Not sure what happened"); */
/*   } */
/* } */

/* float get_battery_voltage() { */
/*   digitalWrite(ADC_ENABLE_PIN, LOW); */
/*   delayMicroseconds(10); */
/*   int sum = 0; */
/*   for (int i = 0; i < 100; i++) { */
/*     sum = sum + analogRead(ADC_PIN); */
/*   } */
/*   float result = sum / 100.0; */
/*   digitalWrite(ADC_ENABLE_PIN, HIGH); */
/*   return float(result) * (1.42) - 50; */
/* } */

/* void sendButtonData() { */
/*   /1* colorWipe(LED.Color(0, 255, 0)); *1/ */
/*   /1* esp_read_mac(buttonData.local_mac, ESP_MAC_WIFI_STA); *1/ */
/*   /1* buttonData.battery_level = get_battery_voltage(); *1/ */
/*   buttonData.battery_level = random(0, 100); */
/*   // Send message via ESP-NOW */
/*   Serial.println("Sending: " + String(buttonData.battery_level)); */
/*   esp_err_t result = */
/*       esp_now_send(controller, (uint8_t *)&buttonData, sizeof(buttonData));
 */
/*   check_esp_send(result); */
/* } */

/* // config AP SSID */
/* void configDeviceAP() { */
/*   String Prefix = "Slave:"; */
/*   String Mac = WiFi.macAddress(); */
/*   String SSID = Prefix + Mac; */
/*   String Password = "t=_Z}ex4v+W'TI1ZnE$'"; */
/*   bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 1); */
/*   if (!result) { */
/*     Serial.println("AP Config failed."); */
/*   } else { */
/*     Serial.println("AP Config Success. Broadcasting with AP: " +
 * String(SSID)); */
/*   } */
/* } */

/* // Callback when data is sent */
/* void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { */
/*   /1* colorWipe(LED.Color(0, 0, 0)); *1/ */
/*   if (!controllerData.answer) { */
/*     Serial.print("\r\nLast Packet Send Status:\t"); */
/*     Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" */
/*                                                   : "Delivery Fail"); */
/*   } */
/* } */

/* // callback when data is recv from Master */
/* void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
 */
/*   /1* char macStr[18]; *1/ */
/*   /1* snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", */
/*    * mac_addr[0], *1/ */
/*   /1*          mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
 * mac_addr[5]); */
/*    *1/ */
/*   /1* Serial.print("Last Packet Recv from: "); *1/ */
/*   /1* Serial.println(macStr); *1/ */
/*   /1* Serial.print("Last Packet Recv Data: "); *1/ */
/*   /1* Serial.println(*data); *1/ */
/*   /1* Serial.println(""); *1/ */

/*   memcpy(&controllerData, data, sizeof(controllerData)); */

/*   if (controllerData.answer) { */
/*     Serial.println("Controller answered"); */
/*     digitalWrite(LED_PIN, HIGH); */
/*   } else { */
/*     Serial.println("Controller didn't answer"); */
/*     digitalWrite(LED_PIN, LOW); */
/*   } */

/*   Serial.print("Bytes received: "); */
/*   Serial.println(data_len); */
/* } */

/* void IRAM_ATTR onButtonTimer() { */
/*   if (digitalRead(BUTTON_PIN) == LOW) { */
/*     sendButtonData(); */
/*   } */
/* } */

/* hw_timer_t *read_button_timer = NULL; */

/* void setup() { */
/*   Serial.begin(115200); */

/*   pinMode(BUTTON_PIN, INPUT_PULLUP); */
/*   pinMode(ADC_PIN, INPUT); */
/*   pinMode(ADC_ENABLE_PIN, OUTPUT); */
/*   pinMode(LED_PIN, OUTPUT); */
/*   digitalWrite(ADC_ENABLE_PIN, HIGH); */
/*   analogReadResolution(12); */

/*   btStop(); */

/*   // Set device in AP mode to begin with */
/*   WiFi.mode(WIFI_AP); */
/*   // configure device AP mode */
/*   configDeviceAP(); */
/*   /1* esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE); *1/ */

/*   // This is the mac address of the Slave in AP Mode */
/*   Serial.print("AP MAC: "); */
/*   Serial.println(WiFi.softAPmacAddress()); */

/*   // Init ESP-NOW */
/*   InitESPNow(); */

/*   /1* LED.begin(); *1/ */
/*   /1* LED.show();             // Disable all the pixel by default when */
/*    * initialized *1/ */
/*   /1* LED.setBrightness(255); // Set brightness *1/ */

/*   // Once ESPNow is successfully Init, we will register for Send CB to */
/*   // get the status of Trasnmitted packet */
/*   esp_now_register_send_cb(OnDataSent); */

/*   // Register peer */
/*   memcpy(controllerInfo.peer_addr, controller, 6); */
/*   controllerInfo.channel = CHANNEL; */
/*   controllerInfo.encrypt = false; */

/*   // Add peer */
/*   if (esp_now_add_peer(&controllerInfo) != ESP_OK) { */
/*     Serial.println("Failed to add peer"); */
/*     return; */
/*   } */
/*   // Register for a callback function that will be called when data is
 * received */
/*   esp_now_register_recv_cb(OnDataRecv); */

/*   /1* attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), sendButtonData, */
/*    * FALLING); *1/ */
/*   read_button_timer = timerBegin(0, 80, true); */
/*   timerAttachInterrupt(read_button_timer, &onButtonTimer, true); */
/*   timerAlarmWrite(read_button_timer, 50000, true); */
/*   timerAlarmEnable(read_button_timer); */
/* } */

/* void loop() { */
/*   // Chill */
/* } */
