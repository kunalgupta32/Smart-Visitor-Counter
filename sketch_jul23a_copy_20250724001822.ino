#include <dummy.h>

#define BLYNK_TEMPLATE_ID "TMPL6ZF-pL4cz"
#define BLYNK_TEMPLATE_NAME "Visitor counter"
#define BLYNK_AUTH_TOKEN "7huNh-4hOanMQLd1z19IG8bigV0Gjf8l"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <NewPing.h>

// WiFi credentials
char ssid[] = "motive1_2.4";
char pass[] = "20690303son";

// Sensor pins
#define TRIG1 14
#define ECHO1 27
#define TRIG2 26
#define ECHO2 25

#define MAX_DISTANCE 200
#define TRIGGER_DISTANCE 10

// Buzzer and LED pins
#define BUZZER_PIN 4
#define LED_PIN 2  // LED pin added here

// OLED (SH1106)
Adafruit_SH1106G display(128, 64, &Wire, -1);

// Sensor objects
NewPing sensor1(TRIG1, ECHO1, MAX_DISTANCE);
NewPing sensor2(TRIG2, ECHO2, MAX_DISTANCE);

// Visitor data
int totalIn = 0;
int totalOut = 0;
int visitors = 0;

unsigned long lastTriggerTime = 0;
const unsigned long debounceDelay = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // LED pin setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // OLED display
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  // WiFi connection
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.display();

  // Blynk config (non-blocking)
  Blynk.config(BLYNK_AUTH_TOKEN);
  if (Blynk.connect(5000)) {
    Serial.println("Blynk connected");
    display.setCursor(0, 10);
    display.println("Blynk Connected");
  } else {
    Serial.println("Blynk failed");
    display.setCursor(0, 10);
    display.println("Blynk Failed");
  }
  display.display();

  delay(1000);
}

void loop() {
  Blynk.run();

  int d1 = sensor1.ping_cm();
  int d2 = sensor2.ping_cm();
  unsigned long currentTime = millis();

  if (currentTime - lastTriggerTime > debounceDelay) {
    if (d1 > 0 && d1 < TRIGGER_DISTANCE) {
      delay(200);
      int d2_confirm = sensor2.ping_cm();
      if (d2_confirm > 0 && d2_confirm < TRIGGER_DISTANCE) {
        totalIn++;
        visitors++;
        lastTriggerTime = currentTime;
        Serial.println("IN Detected");
        buzz();
        blinkLED();         // LED blink added here
      }
    } else if (d2 > 0 && d2 < TRIGGER_DISTANCE) {
      delay(200);
      int d1_confirm = sensor1.ping_cm();
      if (d1_confirm > 0 && d1_confirm < TRIGGER_DISTANCE) {
        totalOut++;
        if (visitors > 0) visitors--;
        lastTriggerTime = currentTime;
        Serial.println("OUT Detected");
        buzz();
        blinkLED();         // LED blink added here
      }
    }
  }

  updateDisplay();
  updateBlynk();
  delay(100);
}

void buzz() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

void blinkLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void updateDisplay() {
  display.clearDisplay();

  // Top center: Visitors
  String visitorsText = "VISITORS: " + String(visitors);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(visitorsText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 0);
  display.println(visitorsText);

  // Bottom left: IN
  display.setCursor(0, 56);
  display.print("IN: ");
  display.print(totalIn);

  // Bottom right: OUT
  String outText = "OUT: " + String(totalOut);
  display.getTextBounds(outText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(128 - w, 56);
  display.print(outText);

  display.display();
}

void updateBlynk() {
  Blynk.virtualWrite(V0, visitors);
  Blynk.virtualWrite(V1, totalIn);
  Blynk.virtualWrite(V2, totalOut);
}
