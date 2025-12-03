#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define OLED_SDA 8
#define OLED_SCL 9
#define LED_PIN 7
#define BUZZER_PIN 6
#define BTN1_PIN 5
#define ENCODER_CLK 10
#define ENCODER_DT 11
#define ENCODER_SW 4  // Encoder button untuk reset
#define PIR_PIN 3

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// System States
struct SystemData {
  bool systemActive;
  bool motionDetected;
  int encoderValue;
  int buzzerVolume;
};

// Global buzzer control
volatile bool g_buzzerActive = false;
volatile int g_buzzerVolume = 5;  // Volume control (1-10)

// Queue and Mutex handles
QueueHandle_t dataQueue;
SemaphoreHandle_t ledMutex;
SemaphoreHandle_t buzzerMutex;
SemaphoreHandle_t displayMutex;

// Global variables
volatile bool btn1Pressed = false;
volatile bool encoderBtnPressed = false;  // Encoder button untuk reset
volatile int encoderPos = 5;  // Start dari nilai tengah
volatile int lastCLK = HIGH;

// Task handles
TaskHandle_t Core0TaskHandle;
TaskHandle_t Core1TaskHandle;

// Interrupt Service Routines
void IRAM_ATTR btn1ISR() {
  btn1Pressed = true;
}

void IRAM_ATTR encoderBtnISR() {
  encoderBtnPressed = true;
}

void IRAM_ATTR encoderISR() {
  int clkState = digitalRead(ENCODER_CLK);
  int dtState = digitalRead(ENCODER_DT);
  
  if (clkState != lastCLK && clkState == LOW) {
    if (dtState != clkState) {
      encoderPos++;
    } else {
      encoderPos--;
    }
    encoderPos = constrain(encoderPos, 0, 10);
  }
  lastCLK = clkState;
}

// Core 0 Task: Handle Input and Display
void core0Task(void * parameter) {
  SystemData data;
  data.systemActive = false;
  data.motionDetected = false;
  data.encoderValue = 5;
  data.buzzerVolume = 5;
  
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 200;
  
  while(true) {
    // Handle Button 1 (Activate/Deactivate System)
    if (btn1Pressed && (millis() - lastDebounceTime > debounceDelay)) {
      data.systemActive = !data.systemActive;
      btn1Pressed = false;
      lastDebounceTime = millis();
      
      Serial.print("Sistem ");
      Serial.println(data.systemActive ? "AKTIF" : "NONAKTIF");
      
      if (xSemaphoreTake(displayMutex, portMAX_DELAY)) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 10);
        if (data.systemActive) {
          display.println("Sistem");
          display.println("Aktif");
        } else {
          display.println("Sistem");
          display.println("Nonaktif");
        }
        display.display();
        xSemaphoreGive(displayMutex);
      }
    }
    
    // Handle Button 2 (Reset/Manual Control)
    if (encoderBtnPressed && (millis() - lastDebounceTime > debounceDelay)) {
      data.motionDetected = false;
      encoderBtnPressed = false;
      lastDebounceTime = millis();
      
      Serial.println("SYSTEM RESET - Encoder Button Pressed");
      
      if (xSemaphoreTake(displayMutex, portMAX_DELAY)) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 20);
        display.println("Reset");
        display.display();
        xSemaphoreGive(displayMutex);
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    // Read Rotary Encoder (Adjust Buzzer Volume)
    data.encoderValue = encoderPos;
    data.buzzerVolume = map(data.encoderValue, 0, 10, 1, 10);
    
    // Debug encoder
    static int lastEncoderValue = -1;
    if (data.encoderValue != lastEncoderValue) {
      Serial.print("Encoder Value: ");
      Serial.print(data.encoderValue);
      Serial.print(" | Buzzer Volume: ");
      Serial.println(data.buzzerVolume);
      lastEncoderValue = data.encoderValue;
    }
    
    // Read PIR Sensor (Only if system is active)
    if (data.systemActive) {
      int pirState = digitalRead(PIR_PIN);
      
      // Debug PIR
      static int lastPirState = -1;
      if (pirState != lastPirState) {
        Serial.print("PIR State: ");
        Serial.println(pirState == HIGH ? "MOTION DETECTED!" : "No motion");
        lastPirState = pirState;
      }
      
      if (pirState == HIGH && !data.motionDetected) {
        data.motionDetected = true;
        Serial.println(">>> GERAKAN TERDETEKSI - ALARM AKTIF <<<");
        
        if (xSemaphoreTake(displayMutex, portMAX_DELAY)) {
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
          display.println("PERINGATAN!");
          display.println("");
          display.setTextSize(2);
          display.println("Gerakan");
          display.println("Terdeteksi");
          display.display();
          xSemaphoreGive(displayMutex);
        }
      } else if (pirState == LOW && data.motionDetected) {
        data.motionDetected = false;
        Serial.println("Gerakan berhenti");
        
        if (xSemaphoreTake(displayMutex, portMAX_DELAY)) {
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 10);
          display.println("Sistem");
          display.println("Aktif");
          display.display();
          xSemaphoreGive(displayMutex);
        }
      }
      
      // Display Encoder Value and Volume on OLED when system active and no motion
      if (!data.motionDetected) {
        if (xSemaphoreTake(displayMutex, portMAX_DELAY)) {
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
          display.println("Sistem Aktif");
          display.println("");
          display.print("Encoder: ");
          display.println(data.encoderValue);
          display.print("Volume: ");
          display.print(data.buzzerVolume);
          display.println("/10");
          display.println("");
          display.println("Menunggu gerakan...");
          display.display();
          xSemaphoreGive(displayMutex);
        }
      }
    }
    
    // Send data to Core 1 via Queue
    xQueueSend(dataQueue, &data, portMAX_DELAY);
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Core 1 Task: Control LED and Buzzer
void core1Task(void * parameter) {
  SystemData receivedData;
  
  while(true) {
    // Receive data from Core 0
    if (xQueueReceive(dataQueue, &receivedData, portMAX_DELAY)) {
      
      // Debug received data
      static bool lastSystemActive = false;
      static bool lastMotionDetected = false;
      
      if (receivedData.systemActive != lastSystemActive || receivedData.motionDetected != lastMotionDetected) {
        Serial.println("=== CORE 1 DATA ===");
        Serial.print("System Active: ");
        Serial.println(receivedData.systemActive ? "YES" : "NO");
        Serial.print("Motion Detected: ");
        Serial.println(receivedData.motionDetected ? "YES" : "NO");
        Serial.print("Buzzer Volume: ");
        Serial.println(receivedData.buzzerVolume);
        Serial.println("==================");
        
        lastSystemActive = receivedData.systemActive;
        lastMotionDetected = receivedData.motionDetected;
      }
      
      // Control LED based on system status
      if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
        if (receivedData.systemActive) {
          digitalWrite(LED_PIN, HIGH);
        } else {
          digitalWrite(LED_PIN, LOW);
        }
        xSemaphoreGive(ledMutex);
      }
      
      // Set buzzer flag based on motion detection
      if (xSemaphoreTake(buzzerMutex, portMAX_DELAY)) {
        if (receivedData.systemActive && receivedData.motionDetected) {
          g_buzzerActive = true;
          g_buzzerVolume = receivedData.buzzerVolume;
          Serial.print(">>> BUZZER ENABLED - Volume: ");
          Serial.println(g_buzzerVolume);
        } else {
          g_buzzerActive = false;
          digitalWrite(BUZZER_PIN, LOW);
        }
        xSemaphoreGive(buzzerMutex);
      }
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Buzzer Task: Generate tone dengan volume control untuk Wokwi
void buzzerTask(void * parameter) {
  while(true) {
    if (g_buzzerActive) {
      // Volume control melalui pola beep yang berbeda
      // Volume 1-3: Beep lambat (lebih banyak jeda)
      // Volume 4-7: Beep sedang
      // Volume 8-10: Beep cepat (lebih sedikit jeda)
      
      int beepCount = g_buzzerVolume;  // Jumlah beep per cycle
      int beepDelay = map(g_buzzerVolume, 1, 10, 200, 50);  // Delay antar beep
      
      // Generate beep pattern
      for (int i = 0; i < beepCount; i++) {
        // Generate ~500 Hz tone
        for (int j = 0; j < 50; j++) {
          digitalWrite(BUZZER_PIN, HIGH);
          vTaskDelay(pdMS_TO_TICKS(1));
          digitalWrite(BUZZER_PIN, LOW);
          vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Jeda antar beep
        vTaskDelay(pdMS_TO_TICKS(beepDelay));
      }
      
      // Jeda antar cycle
      vTaskDelay(pdMS_TO_TICKS(300));
      
    } else {
      // Buzzer off
      digitalWrite(BUZZER_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C for OLED with custom pins
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Sistem Keamanan");
  display.println("IoT ESP32-S3");
  display.println("");
  display.println("Tekan BTN1 untuk");
  display.println("mengaktifkan");
  display.display();
  
  // Initialize GPIO Pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);  // Encoder button
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  
  // Set initial states
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("Buzzer initialized - Toggle mode for Wokwi compatibility");
  
  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(BTN1_PIN), btn1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), encoderBtnISR, FALLING);  // Encoder button
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  
  // Create Queue
  dataQueue = xQueueCreate(10, sizeof(SystemData));
  
  // Create Mutexes
  ledMutex = xSemaphoreCreateMutex();
  buzzerMutex = xSemaphoreCreateMutex();
  displayMutex = xSemaphoreCreateMutex();
  
  // Create Tasks
  xTaskCreatePinnedToCore(core0Task, "Core0Task", 10000, NULL, 1, &Core0TaskHandle, 0);
  
  xTaskCreatePinnedToCore(core1Task, "Core1Task", 10000, NULL, 1, &Core1TaskHandle, 1);
  
  // Create Buzzer Task on Core 1 with higher priority
  TaskHandle_t BuzzerTaskHandle;
  xTaskCreatePinnedToCore(buzzerTask, "BuzzerTask", 4096, NULL, 2, &BuzzerTaskHandle, 1);
  
  Serial.println("Sistem Keamanan IoT Siap!");
}

void loop() {
  // Loop kosong karena menggunakan FreeRTOS tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}