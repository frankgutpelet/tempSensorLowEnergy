#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h> // Der Hardware-Timer

// ---------- WLAN ----------
const char* ssid     = "WMOSKITO";
const char* password = ".ubX54bVSt#vxW11m.";
const char* myhostname = "tempSEnsorPool";

// ---- Statische IP (an dein Netz anpassen) ----
IPAddress local_IP(192, 168, 178, 133);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 178, 1);
  uint8_t bssid[] = {0x2C,0x3A,0xFD,0x21,0xFD,0xC4};  // MAC deines Routers
  int32_t channel = 11;  // WLAN-Kanal

// ---------- DS18B20 ----------
#define ONE_WIRE_BUS 4  // GPIO4 entspricht D2 am D1 Mini
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Ticker sleepTimer;
bool called = false;

// Webserver auf Port 80
ESP8266WebServer server(80);

// Diese Funktion wird vom Timer aufgerufen
void startDeepSleep() {
  //Serial.println("Timer abgelaufen. Deep Sleep startet...");
  ESP.deepSleep(600 * 1000000); // 10 Minuten
}

void handleTemperature() {
  //Serial.println("Send temp");
  //Serial.println(server.uri());
  //Serial.println(server.client().remoteIP());

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  if (temp == DEVICE_DISCONNECTED_C) {
    server.send(500, "application/json", "{\"error\": \"Sensor nicht gefunden\"}");
  } else {
    String json = "{\"room\": \"Bad\", \"temperature\": " + String(temp, 2) + "}";
    server.send(200, "application/json", json);
  }
  if (called)
  {
    startDeepSleep();
  }
  called = true;
}

void setup() {
  sensors.begin();
  Serial.begin(9600);

  WiFi.hostname("bad-sensor");
  WiFi.mode(WIFI_STA);

  // ---- Statische IP setzen ----
  /*WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
*/
  WiFi.hostname(myhostname);

  WiFi.begin(ssid, password, channel, bssid, true);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());    //Serial.print(".");
  }

  Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());

  Serial.println("Connected");
  Serial.print("Kanal: ");
  Serial.println(WiFi.channel());

  Serial.print("BSSID: ");
  Serial.println(WiFi.BSSIDstr());
  
  // 3. mDNS starten (damit bad-sensor.local funktioniert)
  if (MDNS.begin("bad-sensor")) {
  }

  // 4. Routen
  server.on("/temp", HTTP_GET, handleTemperature);
  server.on("/", []() {
    server.send(200, "text/plain", "Bad-Sensor aktiv. Abruf unter /temp");
  });

  server.begin();
  // Timer initialisieren: Einmalig (once) nach 15 Sekunden die Funktion aufrufen
  sleepTimer.once(5, startDeepSleep);
}

void loop() {
  // mDNS und Server am Laufen halten
  MDNS.update();
  server.handleClient();
}
