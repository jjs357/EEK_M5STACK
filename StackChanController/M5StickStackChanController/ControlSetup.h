
//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      display.println(" CONNECTED");
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
        virt_digitalWrite(conn, 1);
        run_command("battery?", 10, 0);
        disconnected_tick = 0;
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      disconnected_tick++;
      connected = false;
      if (disconnected_tick < 10) {
        Serial.println("WiFi not connected, expecting reconnection or Choose new SSID");
        clearText();
        display.println("Disconnected from:");
        display.println(saved_ssid.c_str());
        display.println("Waiting...");
        display.println("Choose New SSID?");
        virt_digitalWrite(conn, 0);
      }
      delay(2000);
      break;
    default:
      delay(1000);
      break;
  }
}

void manage_SSID_connection() {
  connected = false;
  lastCommandOK = false;
  WiFi.mode(WIFI_STA);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  preferences.begin("chan-control", false);
  saved_ssid = preferences.getString("chan_ssid", "");
  clearText();
  if (saved_ssid == "") {
    Serial.println("No value saved for StackChan SSID");
    display.println("No saved Chan SSID");
    display.println("Use connect command");
    display.println("in Serial Monitor");
    display.println("With active SSID");
    delay(2000);
  } else {
    Serial.print("Using saved StackChan SSID: ");
    Serial.println(saved_ssid);
    display.println("Saved Chan SSID:");
    display.println(saved_ssid);
    delay(1000);
    connectToWiFi(saved_ssid.c_str());
    //wait for connection to settle
    delay(3000);
  }
}
