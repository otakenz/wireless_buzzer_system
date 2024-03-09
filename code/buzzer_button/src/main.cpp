#include "custom_print.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-timer.h"
#include "esp_mac.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <cstdint>
#include <esp_now.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()

#define CHANNEL 11

/* #define LED_PIN 0 */
#define LED_PIN 10
#define ADC_PIN 4
#define ADC_ENABLE_PIN 6
#define BUTTON_PIN 7

constexpr uint8_t controller[] = {0xDC, 0x54, 0x75, 0x62, 0x50, 0xFC};
constexpr uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message_button {
  uint8_t local_mac[6];
  uint8_t battery_level;
} struct_message_button;

typedef struct struct_message_controller {
  bool pressed;
  bool ping;
  uint8_t winner_mac[6];
} struct_message_controller;

struct_message_button buttonData;
struct_message_controller controllerData;
esp_now_peer_info_t controllerInfo;
esp_now_peer_info_t broadcastInfo;

void check_esp_err(esp_err_t result) {
  print("Send Status: ");
  switch (result) {
  case ESP_OK:
    println("Pair success");
    break;
  case ESP_ERR_ESPNOW_FULL:
    println("Peer list full");
    break;
  case ESP_ERR_ESPNOW_EXIST:
    println("Peer Exists");
    break;
  case ESP_ERR_ESPNOW_NOT_INIT:
    println("ESPNOW not Init.");
    break;
  case ESP_ERR_ESPNOW_ARG:
    println("Invalid Argument");
    break;
  case ESP_ERR_ESPNOW_INTERNAL:
    println("Internal Error");
    break;
  case ESP_ERR_ESPNOW_NO_MEM:
    println("ESP_ERR_ESPNOW_NO_MEM");
    break;
  case ESP_ERR_ESPNOW_NOT_FOUND:
    println("Peer not found.");
    break;
  case ESP_ERR_ESPNOW_IF:
    println("Interface error.");
    break;
  default:
    println("Not sure what happened");
    break;
  }
}

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    println("ESPNow Init Success");
  } else {
    println("ESPNow Init Failed");
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

uint8_t get_battery_voltage() {
  digitalWrite(ADC_ENABLE_PIN, LOW);
  delayMicroseconds(10);
  int sum = 0;
  for (int i = 0; i < 100; i++) {
    sum = sum + analogRead(ADC_PIN);
  }
  float result = sum / 100.0;
  digitalWrite(ADC_ENABLE_PIN, HIGH);
  return static_cast<uint8_t>(float(result) * (1.42) - 50);
}

void sendButtonData() {
  if (controllerData.pressed) {
    return;
  }
  /* colorWipe(LED.Color(0, 255, 0)); */
  esp_read_mac(buttonData.local_mac, ESP_MAC_WIFI_STA);
  /* buttonData.battery_level = get_battery_voltage(); */
  buttonData.battery_level = random(0, 100);
  // Send message via ESP-NOW
  println("Sending: " + String(buttonData.battery_level));

  esp_err_t result =
      esp_now_send(controller, (uint8_t *)&buttonData, sizeof(buttonData));
  check_esp_err(result);
}

// config AP SSID
void configDeviceAP() {
  String Prefix = "Slave:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "t=_Z}ex4v+W'TI1ZnE$'";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    println("AP Config failed.");
  } else {
    println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

bool compareMacAddress(const uint8_t *mac1, const uint8_t *mac2) {
  for (int i = 0; i < 6; ++i) {
    if (mac1[i] != mac2[i]) {
      return false;
    }
  }
  return true;
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  /* colorWipe(LED.Color(0, 0, 0)); */
  print("\r\nLast Packet Send Status:\t");
  println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                         : "Delivery Fail");
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {

  memcpy(&controllerData, data, sizeof(controllerData));

  if (controllerData.pressed) {
    if (compareMacAddress(controllerData.winner_mac, buttonData.local_mac)) {
      println("Controller: You won!");
      digitalWrite(LED_PIN, HIGH);
    } else {
      println("Controller: Someone pressed the button first!");
    }
  } else {
    println("Controller: reset LED!");
    digitalWrite(LED_PIN, LOW);
  }

  if (controllerData.ping) {
    println("Ping from controller received!");
    controllerData.ping = false;
    for (uint8_t i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
    // TODO: flicker LED in blue for a short time
  }

  print("Bytes received: ");
  println(data_len);
}

void IRAM_ATTR onButtonTimer() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    sendButtonData();
  }
}

hw_timer_t *read_button_timer = NULL;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(ADC_ENABLE_PIN, HIGH);
  analogReadResolution(12);

  btStop();

  // Set device in AP mode to begin with
  WiFi.mode(WIFI_MODE_APSTA);
  // configure device AP mode
  configDeviceAP();
  /* esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE); */

  // This is the mac address of the Slave in AP Mode
  print("AP MAC: ");
  println(WiFi.softAPmacAddress());

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
    println("Failed to add peer");
    return;
  }

  memcpy(broadcastInfo.peer_addr, broadcast_mac, 6);
  broadcastInfo.channel = CHANNEL;
  broadcastInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&broadcastInfo) != ESP_OK) {
    println("Failed to add peer");
    return;
  }

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  /* attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), sendButtonData,
   * FALLING); */
  read_button_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(read_button_timer, &onButtonTimer, true);
  timerAlarmWrite(read_button_timer, 50000, true);
  timerAlarmEnable(read_button_timer);
}

void loop() {
  // Chill
}
