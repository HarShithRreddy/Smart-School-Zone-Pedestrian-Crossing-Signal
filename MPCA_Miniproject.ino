#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// Pin definitions
#define RED_LED     11
#define YELLOW_LED  10
#define GREEN_LED   9
#define BUZZER      8
#define BUTTON      2
#define LDR_PIN     A0
#define IR_PIN      4

// Timing
#define CROSSING_TIME   10000  // 10 seconds for crossing
#define YELLOW_TIME     2000   // 2 seconds yellow warning
#define COOLDOWN_TIME   30000  // 30 seconds cooldown

// School hours
#define MORNING_START  8
#define MORNING_END    9
#define EVENING_START  15
#define EVENING_END    16

bool cooldownActive = false;
unsigned long cooldownStart = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT);

  Serial.begin(9600);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  // Set time - comment this out after first upload
  rtc.adjust(DateTime(2026, 4, 8, 8, 30, 0));
}

bool isSchoolHours(int hour) {
  return (hour >= MORNING_START && hour < MORNING_END) ||
         (hour >= EVENING_START && hour < EVENING_END);
}

void adjustBrightness() {
  int ldrValue = analogRead(LDR_PIN);
  if (ldrValue > 850) {
    Serial.println("Low light detected!");
  }
}

void pedestrianCrossing() {
  Serial.println("Crossing cycle started!");

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  delay(YELLOW_TIME);

  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1000); // 1000Hz beep  Serial.println("Pedestrians crossing...");
  delay(CROSSING_TIME);

  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("Crossing done.");

  cooldownActive = true;
  cooldownStart = millis();
}

void activeMode() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  if (cooldownActive) {
    if (millis() - cooldownStart >= COOLDOWN_TIME) {
      cooldownActive = false;
      Serial.println("Cooldown over.");
    }
    return;
  }

  // Check IR sensor first — person must be present
  bool personDetected = (digitalRead(IR_PIN) == LOW);

  // Check button press with debounce
  if (digitalRead(BUTTON) == LOW) {
    delay(50);
    if (digitalRead(BUTTON) == LOW) {
      if (personDetected) {
        Serial.println("Button pressed + person detected!");
        pedestrianCrossing();
      } else {
        Serial.println("Button pressed but no person detected!");
      }
    }
  }
}

void standbyMode() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  delay(500);
  digitalWrite(YELLOW_LED, LOW);
  delay(500);

  // Allow override in standby too
  if (digitalRead(BUTTON) == LOW) {
    delay(50);
    if (digitalRead(BUTTON) == LOW) {
      Serial.println("Override from standby!");
      digitalWrite(YELLOW_LED, LOW);
      pedestrianCrossing();
    }
  }
}

void loop() {
  DateTime now = rtc.now();
  int currentHour = now.hour();

  adjustBrightness();

  Serial.print("Time: ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.println(now.minute());

  if (isSchoolHours(currentHour)) {
    Serial.println("Mode: ACTIVE");
    activeMode();
  } else {
    Serial.println("Mode: STANDBY");
    standbyMode();
  }

  delay(2000);
}