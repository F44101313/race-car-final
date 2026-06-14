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

// --- 左馬達 (Motor A - 負責左側前後兩輪) ---
const int ENA = 13; 
const int IN1 = 12; 
const int IN2 = 14; 

// --- 右馬達 (Motor B - 負責右側前後兩輪) ---
const int IN3 = 21; 
const int IN4 = 22; 
const int ENB = 23; 

// ==========================================
// 系統狀態與記憶變數
// ==========================================
bool isAutonomous = false; 

// 短期記憶變數：紀錄最後一次看到線的位置 ('C'=中間, 'L'=左邊, 'R'=右邊)
char lastKnown = 'C'; 

// ==========================================
// 基礎設定 (Setup)
// ==========================================
void setup() {
  Serial.begin(115200);

  // 藍牙廣播名稱
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

  // 系統剛通電，馬達靜止，進入「安全狀態」
  stopMotors();
  setASL(0, 1, 0); // 綠燈恆亮
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
    
    // 指令 1：EBS 緊急煞車
    if (cmd == 'S' || cmd == 's') {
      Serial.println("\n!!! 遠程藍牙 EBS 觸發 !!! 系統已死鎖 !!!");
      brakeMotors(); 
      setASL(0, 0, 1); // 藍燈恆亮
      while(true) {
        delay(100); 
      }
    }
    
    // 指令 2：VLS 起跑觸發
    if ((cmd == 'G' || cmd == 'g') && !isAutonomous) {
      Serial.println("\n>>> 藍牙 VLS 觸發！2秒後起跑... <<<");
      delay(2000); 
      
      isAutonomous = true; 
      setASL(1, 0, 0); // 切換為紅燈
    }
  }

  // ---------------------------------------------------------
  // 核心功能：讀取並印出感測器數值 (含反相邏輯修正)
  // ---------------------------------------------------------
  Serial.print("感測器: ");
  for (int i = 0; i < 5; i++) {
    s[i] = !digitalRead(sensorPins[i]); // 加 '!' 將白底變0，黑線變1
    Serial.print(s[i]);                
    Serial.print(" ");                 
  }
  Serial.print("| ");

  // ---------------------------------------------------------
  // 狀態 1：等待起跑 (安全狀態)
  // ---------------------------------------------------------
  if (!isAutonomous) {
    Serial.println("狀態: 等待藍牙 VLS (發送 G)... (綠燈)"); 
    setASL(0, 1, 0); 
    delay(50); 
    return; 
  }

  // ---------------------------------------------------------
  // 狀態 2：自主導航循跡 (高速競技版參數)
  // ---------------------------------------------------------
  Serial.print("狀態: 循跡中 | "); 

  int currentL = 0;
  int currentR = 0;
  String action = "";

  // --- 記憶更新區 ---
  if (s[0] == 1 || s[1] == 1) {
    lastKnown = 'L'; // 記住：線在左邊
  } else if (s[4] == 1 || s[3] == 1) {
    lastKnown = 'R'; // 記住：線在右邊
  } else if (s[2] == 1) {
    lastKnown = 'C'; // 記住：線在中間
  }

  // --- 馬達控制區 (等比例放大高速版) ---
  
  // --- 馬達控制區 (幾何最佳化版) ---
  if (s[2] == 1) {
    currentL = 150; currentR = 150; action = "直線置中";
    moveForward(currentL, currentR); 
  }
  else if (s[1] == 1) {
    currentL = 100; currentR = 200; action = "微左(1:2)";
    moveForward(currentL, currentR);  
  }
  else if (s[0] == 1) {
    currentL = -150; currentR = 150; action = "急左彎(坦克)"; 
    turnLeft(150);         
  }
  else if (s[3] == 1) {
    currentL = 200; currentR = 100; action = "微右(1:2)";
    moveForward(currentL, currentR);  
  }
  else if (s[4] == 1) {
    currentL = 150; currentR = -150; action = "急右彎(坦克)"; 
    turnRight(150);        
  }
  // 遇到全白 (大直角或斷線)：啟動短期記憶搜尋
  else if (s[0]==0 && s[1]==0 && s[2]==0 && s[3]==0 && s[4]==0) {
    if (lastKnown == 'L') {
      currentL = -150; currentR = 150; action = "迷失:記憶尋左";
      turnLeft(150);
    } 
    else if (lastKnown == 'R') {
      currentL = 150; currentR = -150; action = "迷失:記憶尋右";
      turnRight(150);
    } 
    else {
      currentL = 0; currentR = 0; action = "徹底迷失:煞車";
      stopMotors();
    }
  }

  // 遙測系統：印出當下動作與左右輪轉速
  Serial.print("動作: ");
  Serial.print(action);
  Serial.print(" | 左輪: ");
  Serial.print(currentL);
  Serial.print(" | 右輪: ");
  Serial.println(currentR);

  delay(10);
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