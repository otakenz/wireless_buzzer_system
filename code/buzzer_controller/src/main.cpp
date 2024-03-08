#include "custom_print.h"
#include <WiFi.h>
#include <cstdint>
#include <esp_now.h>

// Global copy of slave
#define NUMSLAVES 10
#define CHANNEL 11

bool someone_has_pressed = false;

typedef struct struct_message_button {
  uint8_t local_mac[6];
  float battery_level;
} struct_message_button;

typedef struct struct_message_controller {
  bool pressed;
  uint8_t winner_mac[6];
} struct_message_controller;

struct_message_button buttonData;
struct_message_controller controllerData;
esp_now_peer_info_t broadcastInfo;

esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;

constexpr char buttons_ssid[] = {"Slave:DC:54:75:5D:AE:C8"
                                 "Slave:DC:54:75:62:06:18"
                                 "Slave:DC:54:75:93:35:3C"
                                 /* "Slave:DC:54:75:62:50:FC" */
                                 "Slave:EC:DA:3B:BE:8C:88"
                                 "Slave:64:E8:33:80:BE:FC"};

constexpr uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

// Scan for slaves in AP mode
void ScanForSlave() {
  int8_t scanResults =
      WiFi.scanNetworks(false, false, false, 200, CHANNEL, buttons_ssid);
  // reset slaves
  memset(slaves, 0, sizeof(slaves));
  SlaveCnt = 0;
  println("");
  if (scanResults == 0) {
    println("No WiFi devices in AP Mode found");
  } else {
    print("Found ");
    print(scanResults);
    println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      delay(10);
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("Slave") == 0) {
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
        int mac[6];

        Serial.println(SSID);

        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1],
                        &mac[2], &mac[3], &mac[4], &mac[5])) {
          for (uint8_t ii = 0; ii < 6; ++ii) {
            if (ii == 5) {
              slaves[SlaveCnt].peer_addr[ii] = (uint8_t)mac[ii] - 1;
            } else {
              slaves[SlaveCnt].peer_addr[ii] = (uint8_t)mac[ii];
            }
          }
        }
        slaves[SlaveCnt].channel = CHANNEL; // pick a channel
        slaves[SlaveCnt].encrypt = 0;       // no encryption
        SlaveCnt++;
      }
    }
  }

  if (SlaveCnt > 0) {
    print(SlaveCnt);
    println(" Slave(s) found, processing..");
  } else {
    println("No Slave Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
  if (SlaveCnt > 0) {
    for (int i = 0; i < SlaveCnt; i++) {
      print("Processing: ");
      for (int ii = 0; ii < 6; ++ii) {
        print((uint8_t)slaves[i].peer_addr[ii], HEX);
        if (ii != 5)
          print(":");
      }
      print(" Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist(slaves[i].peer_addr);
      if (exists) {
        // Slave already paired.
        println("Already Paired");
      } else {
        // Slave not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(&slaves[i]);
        if (addStatus == ESP_OK) {
          // Pair success
          println("Pair success");
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          // How did we get so far!!
          println("ESPNOW Not Init");
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          println("Add Peer - Invalid Argument");
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          println("Peer list full");
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          println("Out of memory");
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          println("Peer Exists");
        } else {
          println("Not sure what happened");
        }
        delay(100);
      }
    }
  } else {
    // No slave found to process
    println("No Slave found to process");
  }
}

void check_esp_send(esp_err_t result) {
  print("Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else if (result == ESP_ERR_ESPNOW_IF) {
    Serial.println("Interface error.");
  } else {
    Serial.println("Not sure what happened");
  }
}

uint8_t data = 0;
// send data
void sendData() {
  data++;
  for (int i = 0; i < SlaveCnt; i++) {
    const uint8_t *peer_addr = slaves[i].peer_addr;
    if (i == 0) { // print only for first slave
      print("Sending: ");
      println(data);
    }
    esp_err_t result = esp_now_send(peer_addr, &data, sizeof(data));
    check_esp_send(result);
    delay(100);
  }
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

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  print("Last Packet Recv from: ");
  println(macStr);
  memcpy(&buttonData, incomingData, sizeof(buttonData));
  print("Battery Level: ");
  println(buttonData.battery_level);
  someone_has_pressed = true;

  controllerData.pressed = true;
  memcpy(controllerData.winner_mac, mac_addr, 6);

  Serial.println("Winner Mac:" + String(macStr));

  esp_err_t result = esp_now_send(broadcast_mac, (uint8_t *)&controllerData,
                                  sizeof(controllerData));
  check_esp_send(result);
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

  auto startTime = millis();
  auto duration = 10000;

  while (millis() - startTime < duration) {
    // In the loop we scan for slave
    ScanForSlave();
    // If Slave is found, it would be populate in `slave` variable
    // We will check if `slave` is defined and then we proceed further
    if (SlaveCnt > 0) { // check if slave channel is defined
      // `slave` is defined
      // Add slave as peer if it has not been added already
      manageSlave();
    } else {
      // No slave found to process
    }
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

  /* Serial.println("slaves[0].peer_addr[0]: " +
   * String(slaves[0].peer_addr[0])); */
  /* if (Serial.available() > 0) { */
  /*   /1* String msg = Serial.readStringUntil('\n'); *1/ */
  /*   /1* String msg = Serial.readString(); *1/ */
  /*   char msg = Serial.read(); */
  /*   Serial.println(msg); */
  /* } */
  if (Serial.available() > 0) {
    char msg = Serial.read();
    if (msg == 's') {
      ScanForSlave();
    }

    char mac[18];
    Serial.readBytes(mac, 18);
    Serial.println(mac);

    /* if (msg == 'w') { */
    /*   ScanForSlave(); */
    /* } */
  }

  /* if (someone_has_pressed) { */
  /*   Serial.println("Do you want to reset the game?"); */
  /*   while (true) { */
  /*     if (Serial.available() > 0) { */
  /*       char answer = Serial.read(); */
  /*       if (answer == 'y') { */
  /*         Serial.println("Resetting the game..."); */
  /*         someone_has_pressed = false; */
  /*         controllerData.pressed = false; */
  /*         auto result = esp_now_send(broadcast_mac, (uint8_t
   * *)&controllerData, */
  /*                                    sizeof(controllerData)); */
  /*         check_esp_send(result); */

  /*         break; */
  /*       } else if (answer == 'n') { */
  /*         Serial.println("Game will not be reset."); */
  /*         break; */
  /*       } else { */
  /*         Serial.println("Invalid input. Please enter 'y' or 'n'."); */
  /*       } */
  /*     } */
  /*   } */
  /* } */

  // wait for 3seconds to run the logic again
}
