// ============================================================================
// Library Imports
// ============================================================================
// WiFi Libraries
#include <WiFi.h>
#include <WiFiManager.h>
// Timestamp Libraries
#include <WiFiUdp.h>
#include <NTPClient.h>
// Firebase
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
// OTA Update
#include <AsyncElegantOTA.h>
// Sensors
// #include <EasyButton.h>
#include <DHT.h>

// ============================================================================
// Firebase Details 
// ============================================================================
#define FIREBASE_HOST "https://hydroponic-monitor-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "fShvShVEvXf8xqNHR4PesegFpGDn44Kn1uBQ9UMW"
#define API_KEY "AIzaSyAOczKFgRE_QjZBmdMDJsyN-vnQFicAvNY"

// ============================================================================
// Sensors
// ============================================================================
#define LIGHT_SENSOR_PIN 34
#define firebaseLED 21
#define wifiLED 5
#define DHT_PIN 33
String sensors[] = { "light", "temperature", "humidity" };
double data_list[sizeof(sensors) / sizeof(*sensors)];

// ============================================================================
// Configuration
// ============================================================================
int sensor_index = 0;
String account = "teohjjteoh@gmailcom";
String account_index = account + "/" + String(sensor_index);
String account_com = "teohjjteoh@gmail.com";
String acc_password = "Bello123";

// ============================================================================
// Variables
// ============================================================================
unsigned long prev = 0;
unsigned long epochTime;
unsigned long tempLogInterval = 3;
unsigned long logging_interval = 3;

// ============================================================================
// Initialize Variables
// ============================================================================
WiFiManager wm;
FirebaseData FBD;
FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
DHT dht(DHT_PIN, DHT11);
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 1000);
AsyncWebServer ota_server(80);
// EasyButton resetButton(GPIO0);

// ============================================================================
// Utility Functions
// ============================================================================
void space(int num = 1) {
  for (int i = 0; i < num; i++) {
    Serial.println("");
  }
}

unsigned long getTime() {
  return timeClient.getEpochTime();
}

String pathGen(String sensor, bool current = false) {
  String dbPath = account_index + "/" + sensor;
  if (current == true) {
    dbPath = dbPath + "/current";
  } else {
    unsigned long time = getTime();
    dbPath = dbPath + "/" + String(time);
  }
  return dbPath;
}

void blinkLED(int interval) {
  digitalWrite(wifiLED, LOW);
  delay(interval);
  digitalWrite(wifiLED, HIGH);
  delay(interval);
}

void logLocalIP() {
  if (Firebase.ready()) {
    if (Firebase.RTDB.setString(&FBD, account_index + "/localip", WiFi.localIP().toString().c_str())) {
      Serial.println("Logged Local IP");
    } else {
      Serial.println("FAILED UPLOAD: " + FBD.errorReason());
    }
  }
}

// ============================================================================
// Core Functions
// ============================================================================
void updateData(bool current = false) {
  delay(10);
  int list_size = sizeof(sensors) / sizeof(*sensors);
  bool ready = (millis() / 1000) >= prev + logging_interval;
  if (Firebase.ready() && (ready || prev == 0)) {
    data_list[0] = analogRead(LIGHT_SENSOR_PIN);
    data_list[1] = dht.readTemperature();
    data_list[2] = dht.readHumidity();
    if (Firebase.RTDB.getInt(&FBD, account + "/logging_interval")) {
      if (FBD.dataType() == "int") {
        tempLogInterval = FBD.intData();
      }
    } else {
      Serial.println("FAILED READ");
      Serial.println("REASON: " + FBD.errorReason());
    }
    if (Firebase.RTDB.setFloatAsync(&FBD, account_index + "/uptime", millis())) {
      Serial.println("Logged UpTime");
    } else {
      Serial.println("FAILED UPLOAD: " + FBD.errorReason());
    }
    for (int i = 0; i < list_size; i++) {
      String dbPath = pathGen(sensors[i], current);
      if (Firebase.RTDB.setFloatAsync(&FBD, dbPath, data_list[i])){
        Serial.println(("Log " + sensors[i] + ": " + String(data_list[i])));
      } else {
        Serial.println("FAILED UPLOAD: " + String(data_list[i]));
        Serial.println("REASON: " + FBD.errorReason());
      }
    }
    space(1);
    digitalWrite(wifiLED, LOW);
    prev = millis() / 1000;
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(firebaseLED, LOW);
      blinkLED(500);
    } else {
      digitalWrite(firebaseLED, HIGH);
    }
  }
  if (logging_interval != tempLogInterval) {
    Serial.println("Logging Interval Updated");
    logging_interval = tempLogInterval;
  }
}

void connectFirebase() {
  config.api_key = API_KEY;
  config.database_url = FIREBASE_HOST;

  auth.user.email = account_com;
  auth.user.password = acc_password;
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Connected");
  digitalWrite(firebaseLED, HIGH);  

  // Set initial state for timing the logging intervals
  prev = millis() / 1000;
}

// ============================================================================
// Button Handlers
// ============================================================================
void resetWiFi() {
  digitalWrite(firebaseLED, LOW);
  Serial.println("WiFi Config Erased");
  wm.erase();
  ESP.restart();
}

// ============================================================================
// OTA Server
// ============================================================================
void initOTA_Server() {
  AsyncElegantOTA.begin(&ota_server);
  ota_server.begin();
}

// ============================================================================
// Program Start
// ============================================================================
void setup() {
  // Configure pin modes
  pinMode(0, INPUT_PULLUP);
  pinMode(firebaseLED, OUTPUT);
  pinMode(wifiLED, OUTPUT);
  digitalWrite(wifiLED, HIGH);
  digitalWrite(firebaseLED, LOW);

  // Start checking WiFi reset button
  // resetButton.begin();
  // resetButton.onPressed(resetWiFi);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(10);
  space(2);

  wm.setConnectRetries(2);
  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(300);
  wm.setConfigPortalBlocking(false);
  wm.autoConnect("Hydroponic Monitor", "admin123");

  while (WiFi.status() != WL_CONNECTED) {
    wm.process();
    blinkLED(500);
  }
  space(1);
  Serial.println("WiFi connected");
  digitalWrite(wifiLED, LOW);

  // Initialize OTA Server
  initOTA_Server();

  // Connect to Firebase
  connectFirebase();  
  logLocalIP();
}

// ============================================================================
// Start Program Loop
// ============================================================================
void loop() {
  // timeClient.update();
  // resetButton.read();
  updateData(true);
}