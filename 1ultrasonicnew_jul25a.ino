#define BLYNK_TEMPLATE_ID "TMPL61UiZMdg8"
#define BLYNK_TEMPLATE_NAME "Visitor counter"
#define BLYNK_AUTH_TOKEN "Xmgge98gIRf3Mr29fdh9qeczRmKZ1vBh"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Ultrasonic.h>

// WiFi Credentials
char ssid[] = "GameJam";
char pass[] = "123456789";

// SH1106 Display Setup (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Ultrasonic Sensors
#define ENTRY_SENSOR_TRIG 32
#define ENTRY_SENSOR_ECHO 33
#define EXIT_SENSOR_TRIG 4
#define EXIT_SENSOR_ECHO 2
Ultrasonic entrySensor(ENTRY_SENSOR_TRIG, ENTRY_SENSOR_ECHO);
Ultrasonic exitSensor(EXIT_SENSOR_TRIG, EXIT_SENSOR_ECHO);
#define BUZZER_PIN 19

// Detection Parameters
#define DETECTION_RANGE 30 // Increased to 60cm
#define CLEAR_RANGE 70      // Slightly higher than detection range
#define DEBOUNCE_TIME 500   // 500ms debounce time

// Counters
volatile int countIn = 0;
volatile int countOut = 0;
volatile int currentCount = 0;

// Sensor states
bool entryActive = false;
bool exitActive = false;
unsigned long lastEntryTime = 0;
unsigned long lastExitTime = 0;

// WiFi/Blynk state
bool wifiConnected = false;
bool blynkConnected = false;
unsigned long lastWifiAttempt = 0;
#define WIFI_RETRY_INTERVAL 30000  // 30 seconds between connection attempts

// Blynk Virtual Pins
BLYNK_WRITE(V0) {  // Reset counter
  if (param.asInt() == 1) {
    countIn = countOut = currentCount = 0;
    Serial.println("Counters reset via Blynk");
    updateCounters();
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(2000);  // Allow serial to stabilize
  Serial.println("\nVisitor Counter Initializing...");
  Serial.println("Detection range set to 60cm");

  // Initialize Display
  Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22
  if(!display.begin(0x3C, true)) {
    Serial.println("SH1106 allocation failed");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20, 20);
  display.print("Visitor");
  display.setCursor(20, 40);
  display.print("Counter");
  display.display();
  Serial.println("OLED initialized");

  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  beep(100);

  // Attempt WiFi connection in background
  attemptWifiConnection();
  updateDisplay();
}

void loop() {
  // Handle WiFi/Blynk connection management in background
  manageNetworkConnection();
  
  if (blynkConnected) {
    Blynk.run();
  }
  
  // Read sensors
  int entryDistance = entrySensor.read();
  int exitDistance = exitSensor.read();
  
  // Debug sensor readings
  static unsigned long lastSensorPrint = 0;
  if (millis() - lastSensorPrint > 1000) {
    Serial.print("Entry: ");
    Serial.print(entryDistance);
    Serial.print("cm | Exit: ");
    Serial.print(exitDistance);
    Serial.print("cm | IN: ");
    Serial.print(countIn);
    Serial.print(" | OUT: ");
    Serial.print(countOut);
    Serial.print(" | TOTAL: ");
    Serial.print(currentCount);
    Serial.print(" | WiFi: ");
    Serial.print(wifiConnected ? "Connected" : "Disconnected");
    Serial.print(" | Blynk: ");
    Serial.println(blynkConnected ? "Connected" : "Disconnected");
    lastSensorPrint = millis();
  }

  // Entry detection logic (60cm range)
  if (entryDistance < DETECTION_RANGE && !entryActive && (millis() - lastEntryTime > DEBOUNCE_TIME)) {
    entryActive = true;
    lastEntryTime = millis();
    Serial.println("Entry sensor triggered (60cm range)");
  }
  
  // Exit detection logic (60cm range, only if someone is inside)
  if (exitDistance < DETECTION_RANGE && currentCount > 0 && !exitActive && (millis() - lastExitTime > DEBOUNCE_TIME)) {
    exitActive = true;
    lastExitTime = millis();
    Serial.println("Exit sensor triggered (60cm range)");
  }

  // Complete entry detection (when object moves beyond clear range)
  if (entryActive && entryDistance > CLEAR_RANGE && (millis() - lastEntryTime > DEBOUNCE_TIME/2)) {
    countIn++;
    currentCount = countIn - countOut;
    Serial.println("+ Entry confirmed at 60cm range");
    entryActive = false;
    updateCounters();
  }

  // Complete exit detection (when object moves beyond clear range)
  if (exitActive && exitDistance > CLEAR_RANGE && (millis() - lastExitTime > DEBOUNCE_TIME/2)) {
    countOut++;
    currentCount = countIn - countOut;
    Serial.println("- Exit confirmed at 60cm range");
    exitActive = false;
    updateCounters();
  }
  
  delay(50);
}

void updateCounters() {
  beep(200);
  updateDisplay();
  if (blynkConnected) {
    updateBlynk();
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("** Visitor Counter **");

  // Current count
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("Now: ");
  display.print(currentCount);

  // IN/OUT counts
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("IN:");
  display.print(countIn);
  display.setCursor(64, 45);
  display.print("OUT:");
  display.print(countOut);

  // Status
  display.setCursor(0, 56);
  display.print(wifiConnected ? "WiFi:ON " : "WiFi:OFF");
  display.setCursor(70, 56);
  display.print("Blynk:");
  display.print(blynkConnected ? "ON" : "OFF");
  
  display.display();
}

void updateBlynk() {
  Blynk.virtualWrite(V1, countIn);
  Blynk.virtualWrite(V2, countOut);
  Blynk.virtualWrite(V3, currentCount);
  Serial.println("Blynk updated - Current: " + String(currentCount));
}

void beep(unsigned int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void attemptWifiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WiFi...");
    WiFi.begin(ssid, pass);
    
    // Non-blocking connection attempt (5 seconds max)
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
      delay(100);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("\nWiFi connected!");
      
      // Now try to connect to Blynk
      Serial.print("Connecting to Blynk...");
      Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
      if (Blynk.connect()) {
        blynkConnected = true;
        Serial.println("Blynk connected!");
        Blynk.syncVirtual(V0);
      } else {
        Serial.println("Blynk connection failed");
      }
    } else {
      wifiConnected = false;
      blynkConnected = false;
      Serial.println("\nWiFi connection failed - working offline");
    }
    lastWifiAttempt = millis();
  }
}

void manageNetworkConnection() {
  // If we're not connected and it's time to retry
  if (!wifiConnected && millis() - lastWifiAttempt > WIFI_RETRY_INTERVAL) {
    attemptWifiConnection();
    updateDisplay(); // Update display to show new connection status
  }
  
  // If we were connected but lost connection
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    blynkConnected = false;
    Serial.println("Lost WiFi connection");
    updateDisplay();
  }
} 