// ==========================================
// 函式庫引入與藍牙設定
// ==========================================
#include "BluetoothSerial.h"

BluetoothSerial SerialBT; 

// ==========================================
// 硬體腳位定義
// ==========================================
// --- ASL 狀態指示燈 (RGB LED) ---
const int ASL_R = 15; // 紅色腳位
const int ASL_G = 16; // 綠色腳位
const int ASL_B = 17; // 藍色腳位


// --- 感測器腳位 (左到右: OUT1 ~ OUT5) ---
const int sensorPins[5] = {32, 33, 25, 26, 27};
int s[5] = {0, 0, 0, 0, 0}; 

// --- 左馬達 (Motor A) ---
const int ENA = 13; 
const int IN1 = 12; 
const int IN2 = 14; 

// --- 右馬達 (Motor B) ---
const int IN3 = 21; 
const int IN4 = 22; 
const int ENB = 23; 

// ==========================================
// 系統狀態變數
// ==========================================
bool isAutonomous = false; 

// ==========================================
// 基礎設定 (Setup)
// ==========================================
void setup() {
  Serial.begin(115200);

  SerialBT.begin("Group_13_car"); 
  Serial.println("藍牙系統廣播中！請用手機連線...");

  // 1. 初始化 ASL
  pinMode(ASL_R, OUTPUT);
  pinMode(ASL_G, OUTPUT);
  pinMode(ASL_B, OUTPUT);

  // 2. 初始化感測器腳位
  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // 3. 初始化馬達腳位
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // 系統剛通電，馬達靜止，進入「安全狀態」(綠燈恆亮)
  stopMotors();
  setASL(0, 1, 0); 
  Serial.println("系統就緒 (安全狀態)。等待藍牙 VLS (發送 G) 觸發...");
}

// ==========================================
// 主迴圈 (Loop) 
// ==========================================
void loop() {
  // =========================================================
  // 最高優先權：遠程藍牙指令監聽 (EBS 與 VLS)
  // =========================================================
  if (SerialBT.available()) {
    char cmd = SerialBT.read(); 
    
    // 指令 1：EBS 緊急煞車 (任何時候發送 'S' 都會觸發死鎖)
    if (cmd == 'S' || cmd == 's') {
      Serial.println("\n!!! 遠程藍牙 EBS 觸發 !!! 系統已死鎖 !!!");
      brakeMotors(); 
      setASL(0, 0, 1); // 藍燈恆亮
      while(true) {
        delay(100);
      }
    }
    
    // 指令 2：VLS 起跑觸發 (只有在未起跑的安全狀態下，發送 'G' 才有效)
    if ((cmd == 'G' || cmd == 'g') && !isAutonomous) {
      Serial.println("\n>>> 藍牙 VLS 觸發！2秒後起跑... <<<");
      delay(2000); 
      
      isAutonomous = true; 
      setASL(1, 0, 0); // 切換為紅燈，進入自主導航狀態
    }
  }

  // ---------------------------------------------------------
  // 核心功能：讀取感測器
  // ---------------------------------------------------------
  Serial.print("感測器: ");
  for (int i = 0; i < 5; i++) {
    s[i] = digitalRead(sensorPins[i]); 
    Serial.print(s[i]);                
    Serial.print(" ");                 
  }
  Serial.print(" | 狀態: ");

  // ---------------------------------------------------------
  // 狀態 1：等待起跑 (安全狀態)
  // ---------------------------------------------------------
  if (!isAutonomous) {
    Serial.println("等待藍牙 VLS (發送 G)... (綠燈)"); 
    setASL(0, 1, 0); // 維持綠燈恆亮
    delay(50); 
    return; // 卡在這裡，不會執行下方的馬達控制
  }

  // ---------------------------------------------------------
  // 狀態 2：自主導航循跡 (自主導航狀態)
  // ---------------------------------------------------------
  Serial.println("循跡中 (紅燈)"); 

  if (s[2] == 1) {
    moveForward(255, 255); 
  }
  else if (s[1] == 1) {
    moveForward(150, 255);  
  }
  else if (s[0] == 1) {
    turnLeft(255);         
  }
  else if (s[3] == 1) {
    moveForward(255, 150);  
  }
  else if (s[4] == 1) {
    turnRight(255);        
  }
  else if (s[0]==0 && s[1]==0 && s[2]==0 && s[3]==0 && s[4]==0) {
    stopMotors(); 
  }

  delay(20);
}

// ==========================================
// ASL 狀態燈控制函式庫
// ==========================================
void setASL(int r, int g, int b) {
  digitalWrite(ASL_R, r);
  digitalWrite(ASL_G, g);
  digitalWrite(ASL_B, b);
}

// ==========================================
// 馬達控制函式庫
// ==========================================
void moveForward(int speedLeft, int speedRight) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, speedLeft);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, speedRight);
}

void turnLeft(int speed) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, speed);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, speed);
}

void turnRight(int speed) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, speed);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, speed);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);
}

void brakeMotors() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 255);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, 255);
}