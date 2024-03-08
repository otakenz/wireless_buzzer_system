
#include "HWCDC.h"
#include <WiFi.h>
#include <cstdint>
#include <esp_now.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()

#define CHANNEL 1

bool someone_has_pressed = false;

constexpr uint8_t button1[] = {0xDC, 0x54, 0x75, 0x62, 0x06, 0x18};
constexpr uint8_t button2[] = {0xDC, 0x54, 0x75, 0x93, 0x35, 0x3C};
constexpr uint8_t button3[] = {0xEC, 0xDA, 0x3B, 0xBE, 0x8C, 0x88};
constexpr uint8_t button4[] = {0x64, 0xE8, 0x33, 0x80, 0xBE, 0xFC};
constexpr uint8_t button5[] = {0xDC, 0x54, 0x75, 0x5D, 0xAE, 0xC8};

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
      delay(1000);
      Serial.println(WiFi.RSSI());

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

/* #include "custom_print.h" */
/* #include <WiFi.h> */
/* #include <cstdint> */
/* #include <esp_now.h> */

/* // Global copy of slave */
/* #define NUMSLAVES 10 */
/* #define CHANNEL 11 */

/* bool someone_has_pressed = false; */

/* typedef struct struct_message_button { */
/*   uint8_t local_mac[6]; */
/*   float battery_level; */
/* } struct_message_button; */

/* typedef struct struct_message_controller { */
/*   bool answer; */
/* } struct_message_controller; */

/* struct_message_button buttonData; */
/* struct_message_controller controllerData; */

/* esp_now_peer_info_t slaves[NUMSLAVES] = {}; */
/* int SlaveCnt = 0; */

/* constexpr char buttons_ssid[] = {"Slave:DC:54:75:5D:AE:C8" */
/*                                  "Slave:DC:54:75:62:06:18" */
/*                                  "Slave:DC:54:75:93:35:3C" */
/*                                  /1* "Slave:DC:54:75:62:50:FC" *1/ */
/*                                  "Slave:EC:DA:3B:BE:8C:88" */
/*                                  "Slave:64:E8:33:80:BE:FC"}; */

/* // Init ESP Now with fallback */
/* void InitESPNow() { */
/*   WiFi.disconnect(); */
/*   if (esp_now_init() == ESP_OK) { */
/*     println("ESPNow Init Success"); */
/*   } else { */
/*     println("ESPNow Init Failed"); */
/*     // Retry InitESPNow, add a counte and then restart? */
/*     // InitESPNow(); */
/*     // or Simply Restart */
/*     ESP.restart(); */
/*   } */
/* } */

/* // Scan for slaves in AP mode */
/* void ScanForSlave() { */
/*   int8_t scanResults = */
/*       WiFi.scanNetworks(false, false, false, 200, CHANNEL, buttons_ssid); */
/*   // reset slaves */
/*   memset(slaves, 0, sizeof(slaves)); */
/*   SlaveCnt = 0; */
/*   println(""); */
/*   if (scanResults == 0) { */
/*     println("No WiFi devices in AP Mode found"); */
/*   } else { */
/*     print("Found "); */
/*     print(scanResults); */
/*     println(" devices "); */
/*     for (int i = 0; i < scanResults; ++i) { */
/*       // Print SSID and RSSI for each device found */
/*       String SSID = WiFi.SSID(i); */
/*       int32_t RSSI = WiFi.RSSI(i); */
/*       String BSSIDstr = WiFi.BSSIDstr(i); */

/*       delay(10); */
/*       // Check if the current device starts with `Slave` */
/*       if (SSID.indexOf("Slave") == 0) { */
/*         // SSID of interest */
/*         print(i + 1); */
/*         print(": "); */
/*         print(SSID); */
/*         print(" ["); */
/*         print(BSSIDstr); */
/*         print("]"); */
/*         print(" ("); */
/*         print(RSSI); */
/*         print(")"); */
/*         println(""); */
/*         // Get BSSID => Mac Address of the Slave */
/*         int mac[6]; */

/*         /1* Serial.println("Slave BSSID: " + BSSIDstr); *1/ */

/*         if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0],
 * &mac[1], */
/*                         &mac[2], &mac[3], &mac[4], &mac[5])) { */
/*           for (uint8_t ii = 0; ii < 6; ++ii) { */
/*             slaves[SlaveCnt].peer_addr[ii] = (uint8_t)mac[ii]; */
/*           } */
/*         } */
/*         slaves[SlaveCnt].channel = CHANNEL; // pick a channel */
/*         slaves[SlaveCnt].encrypt = 0;       // no encryption */
/*         SlaveCnt++; */
/*       } */
/*     } */
/*   } */

/*   if (SlaveCnt > 0) { */
/*     print(SlaveCnt); */
/*     println(" Slave(s) found, processing.."); */
/*   } else { */
/*     println("No Slave Found, trying again."); */
/*   } */

/*   // clean up ram */
/*   WiFi.scanDelete(); */
/* } */

/* // Check if the slave is already paired with the master. */
/* // If not, pair the slave with master */
/* void manageSlave() { */
/*   if (SlaveCnt > 0) { */
/*     for (int i = 0; i < SlaveCnt; i++) { */
/*       print("Processing: "); */
/*       for (int ii = 0; ii < 6; ++ii) { */
/*         print((uint8_t)slaves[i].peer_addr[ii], HEX); */
/*         if (ii != 5) */
/*           print(":"); */
/*       } */
/*       print(" Status: "); */
/*       // check if the peer exists */
/*       bool exists = esp_now_is_peer_exist(slaves[i].peer_addr); */
/*       if (exists) { */
/*         // Slave already paired. */
/*         println("Already Paired"); */
/*       } else { */
/*         // Slave not paired, attempt pair */
/*         esp_err_t addStatus = esp_now_add_peer(&slaves[i]); */
/*         if (addStatus == ESP_OK) { */
/*           // Pair success */
/*           println("Pair success"); */
/*         } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) { */
/*           // How did we get so far!! */
/*           println("ESPNOW Not Init"); */
/*         } else if (addStatus == ESP_ERR_ESPNOW_ARG) { */
/*           println("Add Peer - Invalid Argument"); */
/*         } else if (addStatus == ESP_ERR_ESPNOW_FULL) { */
/*           println("Peer list full"); */
/*         } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) { */
/*           println("Out of memory"); */
/*         } else if (addStatus == ESP_ERR_ESPNOW_EXIST) { */
/*           println("Peer Exists"); */
/*         } else { */
/*           println("Not sure what happened"); */
/*         } */
/*         delay(100); */
/*       } */
/*     } */
/*   } else { */
/*     // No slave found to process */
/*     println("No Slave found to process"); */
/*   } */
/* } */

/* void check_esp_send(esp_err_t result) { */
/*   print("Send Status: "); */
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

/* uint8_t data = 0; */
/* // send data */
/* void sendData() { */
/*   data++; */
/*   for (int i = 0; i < SlaveCnt; i++) { */
/*     const uint8_t *peer_addr = slaves[i].peer_addr; */
/*     if (i == 0) { // print only for first slave */
/*       print("Sending: "); */
/*       println(data); */
/*     } */
/*     esp_err_t result = esp_now_send(peer_addr, &data, sizeof(data)); */
/*     check_esp_send(result); */
/*     delay(100); */
/*   } */
/* } */

/* // callback when data is sent from Master to Slave */
/* void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { */
/*   char macStr[18]; */
/*   snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
 * mac_addr[0], */
/*            mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
 */
/*   print("Last Packet Sent to: "); */
/*   println(macStr); */
/*   print("Last Packet Send Status: "); */
/*   println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" */
/*                                          : "Delivery Fail"); */
/* } */

/* // Callback when data is received */
/* void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int
 * len) { */
/*   if (someone_has_pressed) { */
/*     Serial.println("someone has already pressed the button"); */
/*     return; */
/*   } */

/*   char macStr[18]; */
/*   snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
 * mac_addr[0], */
/*            mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
 */
/*   print("Last Packet Recv from: "); */
/*   println(macStr); */
/*   memcpy(&buttonData, incomingData, sizeof(buttonData)); */
/*   print("Battery Level: "); */
/*   println(buttonData.battery_level); */
/*   someone_has_pressed = true; */

/*   controllerData.answer = true; */
/*   esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&controllerData, */
/*                                   sizeof(controllerData)); */
/*   check_esp_send(result); */
/* } */

/* void setup() { */
/*   Serial.begin(115200); */
/*   // Set device in STA mode to begin with */
/*   WiFi.mode(WIFI_STA); */
/*   // This is the mac address of the Master in Station Mode */
/*   print("STA MAC: "); */
/*   println(WiFi.macAddress()); */
/*   // Init ESPNow with a fallback logic */
/*   InitESPNow(); */
/*   // Once ESPNow is successfully Init, we will register for Send CB to */
/*   // get the status of Trasnmitted packet */
/*   esp_now_register_send_cb(OnDataSent); */

/*   unsigned long startTime = 0; */

/*   startTime = millis(); */
/*   while (millis() - startTime < 10000) { */
/*     // In the loop we scan for slave */
/*     ScanForSlave(); */
/*     // If Slave is found, it would be populate in `slave` variable */
/*     // We will check if `slave` is defined and then we proceed further */
/*     if (SlaveCnt > 0) { // check if slave channel is defined */
/*       // `slave` is defined */
/*       // Add slave as peer if it has not been added already */
/*       manageSlave(); */
/*       // pair success or already paired */
/*       // Send data to device */
/*       /1* sendData(); *1/ */
/*     } else { */
/*       // No slave found to process */
/*     } */
/*   } */

/*   esp_now_register_recv_cb(OnDataRecv); */
/* } */

/* void loop() { */

/*   /1* Serial.println("slaves[0].peer_addr[0]: " + */
/*    * String(slaves[0].peer_addr[0])); *1/ */
/*   /1* if (Serial.available() > 0) { *1/ */
/*   /1*   /2* String msg = Serial.readStringUntil('\n'); *2/ *1/ */
/*   /1*   /2* String msg = Serial.readString(); *2/ *1/ */
/*   /1*   char msg = Serial.read(); *1/ */
/*   /1*   Serial.println(msg); *1/ */
/*   /1* } *1/ */
/*   if (someone_has_pressed) { */
/*     Serial.println("Do you want to reset the game?"); */
/*     while (true) { */
/*       if (Serial.available() > 0) { */
/*         char answer = Serial.read(); */
/*         if (answer == 'y') { */
/*           Serial.println("Resetting the game..."); */
/*           someone_has_pressed = false; */
/*           controllerData.answer = false; */
/*           auto result = */
/*               esp_now_send(buttonData.local_mac, (uint8_t *)&controllerData,
 */
/*                            sizeof(controllerData)); */
/*           check_esp_send(result); */

/*           break; */
/*         } else if (answer == 'n') { */
/*           Serial.println("Game will not be reset."); */
/*           break; */
/*         } else { */
/*           Serial.println("Invalid input. Please enter 'y' or 'n'."); */
/*         } */
/*       } */
/*     } */
/*   } */

/*   // wait for 3seconds to run the logic again */
/* } */
