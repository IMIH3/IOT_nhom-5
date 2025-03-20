#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

// Thông tin WiFi và Blynk
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "SmartHome"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// Định nghĩa chân
#define LED_PIN 2        // Chân điều khiển đèn LED
#define DHT_PIN 4        // Chân cảm biến nhiệt độ DHT11
#define BUZZER_PIN 5     // Chân còi báo cháy
#define RELAY_PIN 15     // Chân relay điều khiển cửa cuốn
#define BUTTON_PIN 16    // Chân công tắc vật lý
#define RST_PIN 9        // Chân RST của RFID
#define SDA_PIN 10       // Chân SDA của RFID

// Ngưỡng nhiệt độ báo cháy
#define TEMP_THRESHOLD 50

// Khởi tạo đối tượng
DHT dht(DHT_PIN, DHT11);
MFRC522 rfid(SDA_PIN, RST_PIN);

// UID thẻ từ hợp lệ (thay bằng UID của thẻ bạn dùng)
byte validUID[] = {0x12, 0x34, 0x56, 0x78};

void setup() {
  Serial.begin(115200);

  // Khởi tạo các chân
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Khởi tạo DHT
  dht.begin();

  // Khởi tạo RFID
  SPI.begin();
  rfid.PCD_Init();

  // Kết nối WiFi và Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// Điều khiển đèn LED từ Blynk
BLYNK_WRITE(V0) {
  int value = param.asInt();
  digitalWrite(LED_PIN, value);
}

// Điều khiển cửa cuốn từ Blynk
BLYNK_WRITE(V1) {
  int value = param.asInt();
  digitalWrite(RELAY_PIN, value);
}

// Hàm kiểm tra thẻ RFID
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return false;
  }
  
  bool match = true;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != validUID[i]) {
      match = false;
      break;
    }
  }
  
  rfid.PICC_HaltA();
  return match;
}

// Hàm kiểm tra và báo cháy
void checkFire() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("Lỗi đọc cảm biến nhiệt độ!");
    return;
  }

  Serial.print("Nhiệt độ: ");
  Serial.println(temp);

  if (temp > TEMP_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    Blynk.notify("CẢNH BÁO: Phát hiện cháy!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void loop() {
  Blynk.run();

  // Kiểm tra công tắc vật lý
  if (digitalRead(BUTTON_PIN) == LOW) {
    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
    delay(200); // Chống dội công tắc
  }

  // Kiểm tra thẻ RFID
  if (checkRFID()) {
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000); // Mở cửa trong 5 giây
    digitalWrite(RELAY_PIN, LOW);
  }

  // Kiểm tra báo cháy
  checkFire();

  delay(100); // Giảm tải CPU
}
