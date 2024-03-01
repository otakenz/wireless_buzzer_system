#include <WiFi.h>
#include <cstdint>
#include <esp_now.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()

#define CHANNEL 1

bool someone_has_pressed = false;

constexpr uint8_t button1[] = {0xDC, 0x54, 0x75, 0x62, 0x06, 0x18};
constexpr uint8_t button2[] = {0xDC, 0x54, 0x75, 0x93, 0x35, 0x3C};

typedef struct struct_message_button {
  uint8_t local_mac[6];
  float battery_level;
} struct_message_button;

typedef struct struct_message_controller {
  bool answer;
} struct_message_controller;

struct_message_button buttonData;
struct_message_controller controllerData;

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

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  /* colorWipe(LED.Color(0, 0, 0)); */
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (someone_has_pressed) {
    return;
  }

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: ");
  Serial.println(macStr);
  memcpy(&buttonData, incomingData, sizeof(buttonData));
  Serial.print("Battery Level: ");
  Serial.println(buttonData.battery_level);
  someone_has_pressed = true;

  controllerData.answer = true;
  esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&controllerData,
                                  sizeof(controllerData));
}

hw_timer_t *read_button_timer = NULL;

void add_button_peer(const uint8_t *button_addr) {
  esp_now_peer_info_t button_info;
  memcpy(button_info.peer_addr, button_addr, 6);
  button_info.channel = CHANNEL;
  button_info.encrypt = false;
  if (esp_now_add_peer(&button_info) != ESP_OK) {
    Serial.println("Failed to add button");
  }
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  btStop();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Init ESP-NOW
  InitESPNow();

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  add_button_peer(button1);
  add_button_peer(button2);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (someone_has_pressed) {
    Serial.println("Do you want to reset the game?");
    while (true) {
      if (Serial.available() > 0) {
        char answer = Serial.read();
        if (answer == 'y') {
          Serial.println("Resetting the game...");
          someone_has_pressed = false;
          controllerData.answer = false;
          esp_now_send(buttonData.local_mac, (uint8_t *)&controllerData,
                       sizeof(controllerData));
          break;
        } else if (answer == 'n') {
          Serial.println("Game will not be reset.");
          break;
        } else {
          Serial.println("Invalid input. Please enter 'y' or 'n'.");
        }
      }
    }
  }
}
