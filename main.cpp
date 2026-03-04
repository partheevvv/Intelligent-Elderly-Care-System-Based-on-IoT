#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>
#include "MAX30105.h"
#include "heartRate.h"

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensors
MPU6050 mpu;
MAX30105 particleSensor;

// LED pin
#define LED_PIN 15

// Variables
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg; // Added to match your variable list

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);

  // I2C start (SDA=21, SCL=22)
  Wire.begin(21, 22);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // MPU6050 init
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 not connected");
  } else {
    Serial.println("MPU6050 connected successfully");
  }

  // MAX30102 init
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("System Ready");
  Serial.println("========================");
}

void loop() {
  // --- HEART RATE ---
  long irValue = particleSensor.getIR();
  
  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    // Limit BPM to reasonable range
    if (beatsPerMinute < 40 || beatsPerMinute > 200) {
      beatsPerMinute = 0; // Invalid reading
    }
  }

  // --- FALL DETECTION ---
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Calculate Vector Magnitude
  float accel = sqrt((float)ax * ax + (float)ay * ay + (float)az * az) / 16384.0;

  bool fallDetected = false;
  // Adjusted threshold (Note: 1.05g is very sensitive, basically any movement)
  if (accel > 1.05) {
    fallDetected = true;
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // --- DISPLAY ---
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Heart BPM: ");
  if (beatsPerMinute > 0) {
    display.println(beatsPerMinute, 1);
  } else {
    display.println("--");
  }

  display.setCursor(0, 20);
  display.print("Accel: ");
  display.println(accel, 2);

  display.setCursor(0, 40);
  if (fallDetected) {
    display.print("FALL DETECTED!");
  } else {
    display.print("Status: Normal");
  }
  display.display();

  // --- SERIAL MONITOR OUTPUT ---
  Serial.print("BPM: ");
  if (beatsPerMinute > 0) {
    Serial.print(beatsPerMinute, 1);
  } else {
    Serial.print("--");
  }

  Serial.print(" | Accel: ");
  Serial.print(accel, 2);
  Serial.print(" | Status: ");
  if (fallDetected) {
    Serial.println("FALL DETECTED!");
  } else {
    Serial.println("Normal");
  }

  delay(200);
}