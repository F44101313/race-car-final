// 定義感測器連接的 ESP32 腳位 (對應 OUT1 到 OUT5)
const int sensorPins[5] = {32, 33, 25, 26, 27};
// 用來儲存感測器數值的陣列
int sensorValues[5] = {0, 0, 0, 0, 0};

void setup() {
  // 初始化序列埠通訊，設定鮑率為 115200 (ESP32 常見預設值)
  Serial.begin(115200);

  // 利用 for 迴圈，將 5 個感測器腳位都設定為輸入模式 (INPUT)
  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
  }
  
  Serial.println("感測器初始化完成！開始讀取數值...");
}

void loop() {
  // 1. 韌體訊號讀取：讀取 5 個感測器的狀態並存入陣列
  for (int i = 0; i < 5; i++) {
    sensorValues[i] = digitalRead(sensorPins[i]);
  }

  // 2. 建立感測器狀態表：將陣列數值格式化印出
  Serial.print("狀態表 [OUT1-OUT5]: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(sensorValues[i]);
    Serial.print(" "); 
  }
  Serial.println(); // 印完 5 個數值後換行

  // 稍微延遲 100 毫秒，避免序列埠被資料塞爆，也方便肉眼觀察
  delay(100);
}