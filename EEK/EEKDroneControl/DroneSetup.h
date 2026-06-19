
//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      display.println(" CONNECTED");
      display.display();
      //in case this is a new SSID
      saved_ssid = WiFi.SSID();
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      run_command("command", 10, 0);
      if (!lastCommandOK) {
        Serial.println("Restarting: first `command` attempt failed ");
        ESP.restart(); 
      } else {
        digitalWrite(LED_CONN_GREEN, HIGH);
        digitalWrite(LED_CONN_RED, LOW);
        run_command("battery?", 10, 0);
        disconnected_tick = 0;
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      disconnected_tick++;
      connected = false;
      if (disconnected_tick < 10) {
        Serial.println("WiFi not connected, expecting reconnection or Choose new SSID");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Disconnected from:");
        display.println(saved_ssid.c_str());
        display.println("Waiting...");
        display.println("Choose New SSID?");
        display.display();
        digitalWrite(LED_CONN_GREEN, LOW);
        digitalWrite(LED_BATT_GREEN, LOW);
        digitalWrite(LED_BATT_YELLOW, LOW);
        digitalWrite(LED_BATT_RED, LOW);
      }
      digitalWrite(LED_CONN_RED, HIGH);
      delay(2000);
      break;
    default:
      delay(1000);
      break;
  }
}

void manage_SSID_connection() {
  digitalWrite(LED_CONN_RED, HIGH);
  connected = false;
  lastCommandOK = false;
  WiFi.mode(WIFI_STA);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  preferences.begin("tello-control", false);
  saved_ssid = preferences.getString("tello_ssid", "");
  display.clearDisplay();
  display.setCursor(0, 0);
  if (saved_ssid == "") {
    Serial.println("No value saved for Tello SSID");
    display.println("No saved Tello SSID");
    display.println("Use connect command");
    display.println("in Serial Monitor");
    display.println("With active SSID");
    display.display();
    delay(2000);
  } else {
    Serial.print("Using saved Tello SSID: ");
    Serial.println(saved_ssid);
    display.println("Saved Tello SSID:");
    display.println(saved_ssid);
    display.display();
    delay(1000);
    connectToWiFi(saved_ssid.c_str());
    //wait for connection to settle
    delay(3000);
  }
}
