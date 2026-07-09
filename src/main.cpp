#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VideoFrame.h>

// ============ PIN CONFIG ============
#define SDA_PIN 6
#define SCL_PIN 7
#define TOUCH_PIN 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============ ROBO EYES ============
#include <FluxGarage_RoboEyes.h>
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ============ VARIABLES ============
enum DisplayMode { MODE_ROBOEYES, MODE_VIDEO };
DisplayMode currentMode = MODE_ROBOEYES;

unsigned long lastTapTime = 0;
bool waitingForSecondTap = false;
const unsigned long DOUBLE_TAP_TIMEOUT = 300;

unsigned long eventTimer;
bool event1wasPlayed = 0, event2wasPlayed = 0, event3wasPlayed = 0;

unsigned long previousVideoMillis = 0;
int currentFrame = 0;
uint8_t frameBuffer[1024];

// LED indikator
#define LED_PIN 2

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n========================================");
  Serial.println("   ROBO EYES + VIDEO (FIXED)");
  Serial.println("========================================");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("1. Init I2C...");
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(800000);
  
  Serial.println("2. Init OLED...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("   ❌ OLED FAILED!");
    while(1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  Serial.println("   ✅ OLED SUCCESS!");
  
  // FORCE DISPLAY - TEST TAMPILAN
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("HI PIO!!");
  display.display();
  delay(1500);
  
  Serial.println("3. Init Touch Sensor...");
  pinMode(TOUCH_PIN, INPUT);
  Serial.println("   ✅ Touch OK");
  
  Serial.println("4. Init Robo Eyes...");
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setPosition(DEFAULT);
  roboEyes.close();
  roboEyes.update(); // Force update pertama
  eventTimer = millis();
  Serial.println("   ✅ Robo Eyes OK");
  
  // Tampilkan status awal
  display.clearDisplay();
  display.display();
  
  Serial.println("5. SYSTEM READY!");
  Serial.println("   Mode: Robo Eyes");
  Serial.println("   Double tap sensor to switch");
  Serial.println("========================================\n");
  
  digitalWrite(LED_PIN, LOW);
}

// ============ LOOP ============
void loop() {
  readTouchSensor();
  
  if (currentMode == MODE_ROBOEYES) {
    runRoboEyesMode();
  } else {
    runVideoMode();
  }
  
  delay(1);
}

// ============ TOUCH SENSOR ============
void readTouchSensor() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(TOUCH_PIN);
  
  if (lastState == LOW && currentState == HIGH) {
    unsigned long now = millis();
    Serial.println("\n[TAP DETECTED]");
    
    if (!waitingForSecondTap) {
      lastTapTime = now;
      waitingForSecondTap = true;
      Serial.println("  Tap 1");
    } else {
      if (now - lastTapTime <= DOUBLE_TAP_TIMEOUT) {
        Serial.println("  DOUBLE TAP!");
        toggleMode();
        waitingForSecondTap = false;
      } else {
        lastTapTime = now;
        Serial.println("  Timeout, reset");
      }
    }
  }
  lastState = currentState;
}

void toggleMode() {
  Serial.print("  Switching mode: ");
  if (currentMode == MODE_ROBOEYES) {
    currentMode = MODE_VIDEO;
    currentFrame = 0;
    previousVideoMillis = millis();
    Serial.println("VIDEO");
    
    display.clearDisplay();
    display.display();
    delay(500);
    
  } else {
    currentMode = MODE_ROBOEYES;
    roboEyes.close();
    roboEyes.setMood(DEFAULT);
    roboEyes.update();
    eventTimer = millis();
    event1wasPlayed = 0;
    event2wasPlayed = 0;
    event3wasPlayed = 0;
    Serial.println("ROBOEYES");
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("HI PIO!!");
    display.display();
    delay(500);
  }
}

void runRoboEyesMode() {
  roboEyes.update(); // Update eyes drawings (WAJIB! setiap loop)
  
  // Sequence animasi menggunakan timer (tanpa delay)
  if(millis() >= eventTimer + 2000 && !event1wasPlayed) {
    event1wasPlayed = 1;
    roboEyes.setMood(HAPPY);
    roboEyes.anim_laugh(); // Animasi tertawa (searah)
    Serial.println("  [Robo] Happy");
  }
  
  if(millis() >= eventTimer + 4000 && !event2wasPlayed) {
    event2wasPlayed = 1;
    roboEyes.setMood(TIRED);
    Serial.println("  [Robo] Tired");
  }
  
  if(millis() >= eventTimer + 6000 && !event3wasPlayed) {
    event3wasPlayed = 1;
    roboEyes.setMood(ANGRY);
    Serial.println("  [Robo] Angry");
  }
  
  if(millis() >= eventTimer + 8000) {
    roboEyes.setMood(DEFAULT);
    roboEyes.setPosition(DEFAULT);
    eventTimer = millis();
    event1wasPlayed = 0;
    event2wasPlayed = 0;
    event3wasPlayed = 0;
    Serial.println("  [Robo] Reset to Default");
  }
}

// ============ VIDEO MODE ============
void runVideoMode() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousVideoMillis >= FRAME_DELAY) {
    previousVideoMillis = currentMillis;
    
    if(currentFrame < TOTAL_FRAMES) {
      // Baca frame dari PROGMEM
      memcpy_P(frameBuffer, video_frames[currentFrame], 1024);
      
      display.clearDisplay();
      display.drawBitmap(0, 0, frameBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.display(); // Pastikan display di-refresh
      
      currentFrame++;
      if (currentFrame >= TOTAL_FRAMES) {
        currentFrame = 0;
        Serial.println("  [Video] Loop restart");
      }
    }
  }
}