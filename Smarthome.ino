#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
////////////id blynk
#define BLYNK_TEMPLATE_ID "TMPL6mZEdazNI"
#define BLYNK_TEMPLATE_NAME "smarthome"
#define BLYNK_AUTH_TOKEN "Your Auth Token"
char ssid[] = "wifi"; 
char pass[] = "123456"
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

// UID thẻ từ hợp lệ (ban đầu)
byte validUID[] = {0x12, 0x34, 0x56, 0x78};
bool updateMode = false; // Biến để bật/tắt chế độ thay đổi thẻ

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

// Bật/tắt chế độ thay đổi thẻ từ Blynk (Virtual Pin V3)
BLYNK_WRITE(V3) {
  updateMode = param.asInt();
  if (updateMode) {
    Serial.println("Chế độ thay đổi thẻ đã bật. Quẹt thẻ mới để cập nhật!");
    Blynk.notify("Chế độ thay đổi thẻ đã bật!");
  } else {
    Serial.println("Chế độ thay đổi thẻ đã tắt.");
  }
}

// Hàm kiểm tra và thay đổi thẻ RFID
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  if (updateMode) {
    // Thay đổi thẻ: Lưu UID mới vào validUID
    for (byte i = 0; i < 4; i++) {
      validUID[i] = rfid.uid.uidByte[i];
    }
    Serial.print("Thẻ mới đã được cập nhật! UID: ");
    for (byte i = 0; i < 4; i++) {
      Serial.print(validUID[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    Blynk.notify("Thẻ mới đã được cập nhật!");
    updateMode = false; // Tắt chế độ cập nhật sau khi thay đổi
    Blynk.virtualWrite(V3, 0); // Tắt nút trên app
  } else {
    // Kiểm tra thẻ bình thường
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != validUID[i]) {
        match = false;
        break;
      }
    }
    
    if (match) {
      // Thẻ đúng: Mở cửa
      Serial.println("Thẻ hợp lệ!");
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000); // Mở cửa trong 5 giây
      digitalWrite(RELAY_PIN, LOW);
    } else {
      // Thẻ sai: Báo về app và bật đèn
      Serial.println("Thẻ không hợp lệ!");
      Blynk.notify("CẢNH BÁO: Thẻ RFID không hợp lệ!");
      digitalWrite(LED_PIN, HIGH); // Bật đèn
      delay(5000); // Đèn sáng trong 5 giây
      digitalWrite(LED_PIN, LOW);  // Tắt đèn
    }
  }
  
  rfid.PICC_HaltA();
}

// Hàm kiểm tra và báo cháy + hiển thị nhiệt độ
void checkFireAndTemp() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("Lỗi đọc cảm biến nhiệt độ!");
    return;
  }

  Serial.print("Nhiệt độ: ");
  Serial.println(temp);

  // Gửi nhiệt độ lên Blynk (Virtual Pin V2)
  Blynk.virtualWrite(V2, temp);

  // Kiểm tra báo cháy
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
  checkRFID();

  // Kiểm tra báo cháy và hiển thị nhiệt độ
  checkFireAndTemp();

  delay(100); // Giảm tải CPU
}
