#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pin Definitions ---
const int fsrPins[4] = {A0, A1, A2, A3}; 
const int buzzerPin = 8; 

// --- Sensor Variables ---
int fsrReadings[4];      
float weights[4];
float totalWeight = 0.0;

// --- Static Grip Alarm Variables ---
unsigned long lastChangeTimes[4] = {0, 0, 0, 0};
int lastReadings[4] = {0, 0, 0, 0};

// Alarm Tuning
const int wiggleRoom = 40;            // Allowable noise before timer resets
const unsigned long maxStaticTime = 5000; // 5 seconds static limit
const int gripThreshold = 80;         // Minimum reading to trigger grip tracking

void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  
  // Initialize OLED (use 0x3C or 0x3D)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    Serial.println(F("OLED Allocation Failed"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE); 
  
  // Boot Screen
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("HAVS Grip System");
  display.setCursor(0, 30);
  display.println("4 Sensors Active...");
  display.display();
  
  // Startup tone
  tone(buzzerPin, 1000, 150);
  delay(1500);
}

void loop() {
  totalWeight = 0.0;
  bool triggerAlarm = false;

  // 1. Read sensors and process alarm logic
  for (int i = 0; i < 4; i++) {
    fsrReadings[i] = analogRead(fsrPins[i]);
    
    // Approximate force conversion (Adjust 20.0 multiplier post-calibration)
    weights[i] = (fsrReadings[i] / 1023.0) * 20.0; 
    totalWeight += weights[i];

    // Reset stopwatch if grip force fluctuates
    if (abs(fsrReadings[i] - lastReadings[i]) > wiggleRoom) {
      lastChangeTimes[i] = millis();
      lastReadings[i] = fsrReadings[i];
    }

    // Check if any single sensor is held statically above the grip threshold
    bool isGripped = (fsrReadings[i] > gripThreshold);
    bool staticTooLong = (millis() - lastChangeTimes[i] > maxStaticTime);

    if (isGripped && staticTooLong) {
      triggerAlarm = true;
    }
  }

  // 2. Control Buzzer Alarm
  if (triggerAlarm) {
    tone(buzzerPin, 880); // 880 Hz alarm tone
  } else {
    noTone(buzzerPin);
  }

  // 3. Render Dashboard on OLED Screen
  display.clearDisplay();
  display.setTextSize(1);

  // Row 1: Top 2 Sensors
  display.setCursor(0, 0);
  display.print("F1: "); display.print(weights[0], 1); display.print("k");
  display.setCursor(64, 0);
  display.print("F2: "); display.print(weights[1], 1); display.print("k");

  // Row 2: Bottom 2 Sensors
  display.setCursor(0, 16);
  display.print("F3: "); display.print(weights[2], 1); display.print("k");
  display.setCursor(64, 16);
  display.print("F4: "); display.print(weights[3], 1); display.print("k");

  // Row 3: Total Load
  display.setCursor(0, 36);
  display.setTextSize(2);
  display.print("TOT:"); display.print(totalWeight, 1); display.print("kg");

  // Row 4: Status Indicator
  display.setTextSize(1);
  display.setCursor(0, 56);
  if (triggerAlarm) {
    display.print("!! STATIC GRIP WARN !!");
  } else {
    display.print("Status: Monitoring");
  }

  display.display();
  delay(100); 
}