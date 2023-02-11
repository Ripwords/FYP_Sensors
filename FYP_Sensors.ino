// ============================================================================
// Library Imports
// ============================================================================
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EasyButton.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <WiFiManager.h>
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
#define LDR A0
#define resetPin 0
#define firebaseLED D6
#define wifiLED D5
#define DHT_PIN D4
String sensors[] = { "light", "temperature", "humidity" };
double data_list[sizeof(sensors) / sizeof(*sensors)];

// ============================================================================
// Configuration
// ============================================================================
String account = "teohjjteoh@gmailcom";
String account_com = "teohjjteoh@gmail.com";
String acc_password = "Bello123";

// ============================================================================
// Variables
// ============================================================================
unsigned long prev = 0;
unsigned long epochTime;
unsigned long tempLogInterval = 1;
unsigned long logging_interval = 1;

// ============================================================================
// Initialize Variables
// ============================================================================
WiFiManager wm;
FirebaseData FBD;
FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
DHT dht(DHT_PIN, DHT11);
NTPClient timeClient(ntpUDP, "pool.ntp.org");
EasyButton resetButton(resetPin);

// ============================================================================
// Utility Functions
// ============================================================================
void space(int num = 1) {
  for (int i = 0; i < num; i++) {
    Serial.println("");
  }
}

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

String pathGen(String sensor, bool current = false) {
  String dbPath = account + "/" + sensor;
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

// ============================================================================
// Core Functions
// ============================================================================
void updateData(bool current = false) {
  delay(10);
  int list_size = sizeof(sensors) / sizeof(*sensors);
  bool ready = (millis() / 1000) >= prev + logging_interval;
  if (Firebase.ready() && (ready || prev == 0)) {
    data_list[0] = analogRead(LDR);
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
    for (int i = 0; i < list_size; i++) {
      String dbPath = pathGen(sensors[i], current);
      if (Firebase.RTDB.setFloat(&FBD, dbPath, data_list[i])){
        Serial.println(("Log " + sensors[i] + ": " + String(data_list[i])));
        space(1);
      } else {
        Serial.println("FAILED UPLOAD: " + String(data_list[i]));
        Serial.println("REASON: " + FBD.errorReason());
      }
    }
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
// Program Start
// ============================================================================
void setup() {
  // Configure pin modes
  pinMode(LDR, INPUT);
  pinMode(resetPin, INPUT);
  pinMode(0, INPUT_PULLUP);
  pinMode(firebaseLED, OUTPUT);
  pinMode(wifiLED, OUTPUT);
  digitalWrite(wifiLED, HIGH);
  digitalWrite(firebaseLED, LOW);

  // Start checking WiFi reset button
  resetButton.begin();
  resetButton.onPressed(resetWiFi);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(10);
  space(2);

  wm.setConnectRetries(3);
  wm.setConnectTimeout(30);
  wm.setConfigPortalTimeout(120);
  wm.setConfigPortalBlocking(false);
  wm.autoConnect("Hydroponic Monitor", "admin123");

  while (WiFi.status() != WL_CONNECTED) {
    wm.process();
    blinkLED(500);
  }
  space(1);
  Serial.println("WiFi connected");
  digitalWrite(wifiLED, LOW);

  // Connect to Firebase
  connectFirebase();  
}

// ============================================================================
// Start Program Loop
// ============================================================================
void loop() {
  resetButton.read();
  // Update sensor value every logging_interval milliseconds 
  updateData(true);
}