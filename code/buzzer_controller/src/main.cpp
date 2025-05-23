#include "HWCDC.h"
#include "custom_print.h"
#include <WiFi.h>
#include <cstdint>
#include <esp_now.h>

// Global copy of slave
#define NUMSLAVES 5
#define CHANNEL 11

bool someone_has_pressed = false;

typedef struct struct_message_button {
  uint8_t local_mac[6];
  uint8_t battery_level;
} struct_message_button;

typedef struct struct_message_controller {
  bool pressed;
  bool lock;
  bool ping;
  uint8_t winner_mac[6];
} struct_message_controller;

typedef struct struct_message_pc {
  uint8_t buttons_mac[NUMSLAVES][6];
  uint8_t battery_level[NUMSLAVES];
  int32_t RSSI[NUMSLAVES];
  uint8_t status[NUMSLAVES];
} struct_message_pc;

struct_message_button buttonData;
struct_message_controller controllerData;
struct_message_pc pcData;
esp_now_peer_info_t broadcastInfo;

esp_now_peer_info_t slaves[NUMSLAVES] = {};

int regSlavesCnt = 0;
String SSID = "";
int32_t RSSI = 0;
String BSSIDstr = "";
bool scanList[NUMSLAVES] = {false, false, false, false, false};

const String buttons_ssid[] = {"54:32:04:87:B5:A4", "54:32:04:87:B2:B0",
                               "54:32:04:89:06:8C", "54:32:04:87:4C:EC",
                               "64:E8:33:80:BE:FC"};
/* const String buttons_ssid[] = {"54:32:04:89:27:E4", "54:32:04:87:27:C4", */
/*                                "54:32:04:87:27:CC", "54:32:04:87:27:DC", */
/*                                "54:32:04:87:27:EC"}; */

constexpr uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

// config AP SSID
void configDeviceAP() {
  String Prefix = "Master:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "t=_Z'TI1ZnE$'}ex4v+W";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 1);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
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

String macToString(const uint8_t *mac) {
  char macStr[18]; // Buffer to hold the string representation of the MAC

  // Format the MAC address into the buffer
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Return the formatted MAC address as a String
  return String(macStr);
}

void slavesSetUp() {
  for (int i = 0; i < NUMSLAVES; i++) {
    uint8_t temp_address[6];

    sscanf(buttons_ssid[i].c_str(), "%x:%x:%x:%x:%x:%x", &temp_address[0],
           &temp_address[1], &temp_address[2], &temp_address[3],
           &temp_address[4], &temp_address[5]);

    memcpy(slaves[i].peer_addr, temp_address, 6);

    slaves[i].channel = CHANNEL; // pick a channel
    slaves[i].encrypt = 0;       // no encryption
  }
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
  for (int i = 0; i < NUMSLAVES; i++) {
    print("Attempt to pair with Slave: ");

    println(macToString(slaves[i].peer_addr));

    // check if the peer exists
    bool exists = esp_now_is_peer_exist(slaves[i].peer_addr);

    if (!exists && regSlavesCnt < NUMSLAVES) {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(&slaves[i]);
      check_esp_err(addStatus);
      if (addStatus == ESP_OK) {
        memcpy(&pcData.buttons_mac[i], slaves[i].peer_addr, 6);
        regSlavesCnt++;
      }
      delay(100);

    } else {
      // Slave already paired.
      println("Already Paired");
    }
  }
}

// Scan for slaves in AP mode
void ScanForSlave() {
  int8_t scanResults =
      /* WiFi.scanNetworks(false, false, false, 300, CHANNEL, buttons_ssid); */
      WiFi.scanNetworks(false, false, false, 300, CHANNEL);

  // reset slaves
  /* memset(slaves, 0, sizeof(slaves)); */
  /* println(""); */

  if (scanResults == 0) {
    println("No WiFi devices in AP Mode found");
    return;
  }

  print("Found ");
  print(scanResults);
  println(" devices ");

  uint8_t count = 0;

  // processing this current scan results
  for (int i = 0; i < scanResults; ++i) {
    // Print SSID and RSSI for each device found
    SSID = WiFi.SSID(i);
    RSSI = WiFi.RSSI(i);
    BSSIDstr = WiFi.BSSIDstr(i);

    delay(10);

    // Check if the current device starts with `Slave`
    if (SSID.indexOf("Slave") != 0)
      continue;

    count++;

    // SSID of interest
    print(i + 1);
    print(": ");
    print(SSID);
    print(" [");
    print(BSSIDstr);
    print("]");
    print(" (");
    print(RSSI);
    print(")");
    println("");
    // Get BSSID => Mac Address of the Slave
    uint8_t mac[6];

    if (6 != sscanf(SSID.c_str(), "Slave:%x:%x:%x:%x:%x:%x", &mac[0], &mac[1],
                    &mac[2], &mac[3], &mac[4], &mac[5])) {
      println("Failed to parse SSID");
      continue;
    }

    // Check if the current device is already paired with the master.
    for (uint8_t i = 0; i < NUMSLAVES; i++) {
      auto slaveFound = compareMacAddress(mac, pcData.buttons_mac[i]);
      if (slaveFound) {
        println("Slave no " + String(i) + " found");
        pcData.RSSI[i] = RSSI;
        pcData.status[i] = 1;
        scanList[i] = true;
        break;
      }
    }
  }

  for (uint8_t i = 0; i < NUMSLAVES; i++) {
    if (!scanList[i]) {
      println("Slave no " + String(i) + " not found");
      pcData.RSSI[i] = 0;
      pcData.status[i] = 0;
    }
    scanList[i] = false;
  }

  if (count == 0) {
    // If none of slaves found, reset the RSSI and status
    for (uint8_t i = 0; i < NUMSLAVES; i++) {
      pcData.RSSI[i] = 0;
      pcData.status[i] = 0;
    }
  }
  /* manageSlave(); */

  // clean up ram
  WiFi.scanDelete();
}

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  print("Last Packet Sent to: ");
  println(macStr);
  print("Last Packet Send Status: ");
  println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                         : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (someone_has_pressed) {
    return;
  }

  if (controllerData.lock) {
    return;
  }

  auto macStr = macToString(mac_addr);

  print("Last Packet Recv from: ");
  println(macStr);
  memcpy(&buttonData, incomingData, sizeof(buttonData));
  print("Battery Level: ");
  println(buttonData.battery_level);

  for (uint8_t i = 0; i < regSlavesCnt; i++) {
    auto isRegisteredSlave = compareMacAddress(mac_addr, pcData.buttons_mac[i]);
    if (isRegisteredSlave) {
      pcData.battery_level[i] = buttonData.battery_level;
      Serial.println("BAT;" + String(buttonData.battery_level));

      someone_has_pressed = true;
      controllerData.pressed = true;
      controllerData.ping = false;
      memcpy(controllerData.winner_mac, mac_addr, 6);

      Serial.println("WSSID;" + String(macStr));

      esp_err_t result = esp_now_send(broadcast_mac, (uint8_t *)&controllerData,
                                      sizeof(controllerData));
      check_esp_err(result);
    }
  }
}

void ping_button(const uint8_t *mac_addr) {
  if (controllerData.lock) {
    return;
  }

  controllerData.ping = true;
  auto result = esp_now_send(mac_addr, (uint8_t *)&controllerData,
                             sizeof(controllerData));
  check_esp_err(result);
}

void setup() {
  Serial.begin(115200);
  // Set device in STA mode to begin with
  WiFi.mode(WIFI_MODE_APSTA);
  configDeviceAP();
  // This is the mac address of the Master in Station Mode
  print("STA MAC: ");
  println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  memcpy(broadcastInfo.peer_addr, broadcast_mac, 6);
  broadcastInfo.channel = CHANNEL;
  broadcastInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&broadcastInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  slavesSetUp();
  manageSlave();

  controllerData.lock = true;
}

void loop() {
  if (Serial.available() > 0) {
    char msg = Serial.read();

    // Scan for slaves
    if (msg == 's') {
      /* pcData.RSSI[0] = 0; */
      /* memset(&pcData.buttons_mac, 0, sizeof(pcData.buttons_mac)); */
      println("Scanning for slaves");
      ScanForSlave();
      for (int i = 0; i < regSlavesCnt; i++) {
        Serial.println("ID;" + String(i) + "|" + "SSID;" +
                       macToString(pcData.buttons_mac[i]) + "|" + "RSSI;" +
                       String(pcData.RSSI[i]) +
                       "|"
                       "BAT;" +
                       String(pcData.battery_level[i]) + "|" + "STA;" +
                       String(pcData.status[i]));
      }
    }

    // Send reset command to all slaves
    if (msg == 'r') {
      someone_has_pressed = false;
      controllerData.pressed = false;
      controllerData.ping = false;
      println("Reset all buttons");
      auto result = esp_now_send(broadcast_mac, (uint8_t *)&controllerData,
                                 sizeof(controllerData));
      check_esp_err(result);
    }

    // Lock all buttons
    if (msg == 'l') {
      controllerData.lock = true;
      controllerData.pressed = false;
      println("Lock all buttons");
      auto result = esp_now_send(broadcast_mac, (uint8_t *)&controllerData,
                                 sizeof(controllerData));
      check_esp_err(result);
    }

    // Unlock all button
    if (msg == 'u') {
      controllerData.lock = false;
      controllerData.pressed = false;
      println("Unlock all buttons");
      auto result = esp_now_send(broadcast_mac, (uint8_t *)&controllerData,
                                 sizeof(controllerData));
      check_esp_err(result);
    }

    // Ping the button 1
    if (msg == '1') {
      println("Ping button 1");
      ping_button(pcData.buttons_mac[0]);
    }

    // Ping the button 2
    if (msg == '2') {
      println("Ping button 2");
      ping_button(pcData.buttons_mac[1]);
    }

    // Ping the button 3
    if (msg == '3') {
      println("Ping button 3");
      ping_button(pcData.buttons_mac[2]);
    }

    // Ping the button 4
    if (msg == '4') {
      println("Ping button 4");
      ping_button(pcData.buttons_mac[3]);
    }

    // Ping the button 5
    if (msg == '5') {
      println("Ping button 5");
      ping_button(pcData.buttons_mac[4]);
    }
  }
}
