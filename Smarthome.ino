#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <ESP32Servo.h>

// Định nghĩa chân kết nối
#define SS_PIN         21      // SDA pin của RFID RC522
#define RST_PIN        22      // RST pin của RFID RC522
#define DHTPIN         4       // Pin kết nối với DHT11
#define DHTTYPE        DHT11   // Loại cảm biến DHT
#define BUZZER_PIN     5       // Pin còi báo động
#define LED_PIN        2       // Pin đèn LED
#define PIR_PIN        15      // Pin cảm biến chuyển động
#define SERVO_PIN      12      // Pin điều khiển servo cửa
#define BUTTON_PIN     14      // Pin nút nhấn điều khiển cửa

// Ngưỡng nhiệt độ cảnh báo cháy
const float FIRE_TEMPERATURE_THRESHOLD = 50.0;

// Góc servo cho vị trí cửa
const int DOOR_CLOSED_ANGLE = 0;     // Góc khi cửa đóng
const int DOOR_OPEN_ANGLE = 90;      // Góc khi cửa mở
const unsigned long DOOR_AUTO_CLOSE_TIME = 5000;  // Thời gian tự đóng cửa (5 giây)

// Khởi tạo các đối tượng
MFRC522 rfid(SS_PIN, RST_PIN);
DHT dht(DHTPIN, DHTTYPE);
Servo doorServo;

// Biến trạng thái
bool doorState = false;          // Trạng thái cửa (false: đóng, true: mở)
bool lightState = false;         // Trạng thái đèn (false: tắt, true: bật)
bool fireAlarm = false;          // Trạng thái báo cháy
bool motionDetected = false;     // Phát hiện chuyển động
unsigned long lastReadTime = 0;  // Thời gian đọc cảm biến cuối cùng
unsigned long lastMotionTime = 0;// Thời gian phát hiện chuyển động cuối cùng
unsigned long doorOpenTime = 0;  // Thời gian mở cửa

// Danh sách thẻ RFID được phép
String authorizedCards[] = {
  "A1B2C3D4",  // Thay thế bằng ID thẻ thực
  "E5F6G7H8"   // Thay thế bằng ID thẻ thực
};
int numCards = 2;  // Số lượng thẻ trong danh sách

// Kiểm tra thẻ RFID có hợp lệ không
bool isAuthorizedCard(String cardID) {
  for (int i = 0; i < numCards; i++) {
    if (cardID == authorizedCards[i]) {
      return true;
    }
  }
  return false;
}

// Đọc thẻ RFID
String readRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return "";
  }
  
  String cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    cardID.concat(String(rfid.uid.uidByte[i], HEX));
  }
  cardID.toUpperCase();
  
  // Dừng PICC
  rfid.PICC_HaltA();
  // Dừng mã hóa PCD
  rfid.PCD_StopCrypto1();
  
  return cardID;
}

// Mở cửa
void openDoor() {
  if (!doorState) {
    doorServo.write(DOOR_OPEN_ANGLE);
    doorState = true;
    doorOpenTime = millis();
    Serial.println("Đã mở cửa");
  }
}

// Đóng cửa
void closeDoor() {
  if (doorState) {
    doorServo.write(DOOR_CLOSED_ANGLE);
    doorState = false;
    Serial.println("Đã đóng cửa");
  }
}

// Bật đèn
void turnOnLight() {
  digitalWrite(LED_PIN, HIGH);
  lightState = true;
  Serial.println("Đã bật đèn");
}

// Tắt đèn
void turnOffLight() {
  digitalWrite(LED_PIN, LOW);
  lightState = false;
  Serial.println("Đã tắt đèn");
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo các chân
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  
  // Trạng thái ban đầu
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Khởi tạo Servo
  doorServo.attach(SERVO_PIN);
  doorServo.write(DOOR_CLOSED_ANGLE); // Đảm bảo cửa đóng khi khởi động
  
  // Khởi tạo RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Đã khởi tạo RFID");
  
  // Khởi tạo DHT
  dht.begin();
  Serial.println("Đã khởi tạo DHT11");
  
  Serial.println("Hệ thống đã sẵn sàng!");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Đọc nhiệt độ và độ ẩm mỗi 10 giây
  if (currentTime - lastReadTime > 10000) {
    lastReadTime = currentTime;
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    // Kiểm tra đọc dữ liệu có lỗi không
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Lỗi đọc cảm biến DHT11!");
    } else {
      Serial.print("Nhiệt độ: ");
      Serial.print(temperature);
      Serial.print(" °C, Độ ẩm: ");
      Serial.print(humidity);
      Serial.println(" %");
      
      // Kiểm tra cảnh báo cháy
      if (temperature > FIRE_TEMPERATURE_THRESHOLD && !fireAlarm) {
        fireAlarm = true;
        digitalWrite(BUZZER_PIN, HIGH);
        Serial.println("CẢNH BÁO: Phát hiện nhiệt độ cao!");
      } 
      else if (temperature <= FIRE_TEMPERATURE_THRESHOLD && fireAlarm) {
        fireAlarm = false;
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Nhiệt độ trở lại bình thường");
      }
    }
  }
  
  // Đọc thẻ RFID
  String cardID = readRFID();
  if (cardID != "") {
    Serial.print("Đọc thẻ RFID: ");
    Serial.println(cardID);
    
    // Nếu thẻ hợp lệ
    if (isAuthorizedCard(cardID)) {
      Serial.println("Thẻ hợp lệ - mở cửa");
      openDoor();
    } else {
      Serial.println("Thẻ không hợp lệ!");
      // Phát âm báo động ngắn để thông báo thẻ không hợp lệ
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  
  // Kiểm tra nút nhấn
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Nút nhấn được kích hoạt - mở cửa");
    openDoor();
    delay(500); // Chống dội nút
  }
  
  // Kiểm tra cảm biến chuyển động
  bool motion = digitalRead(PIR_PIN);
  if (motion && !motionDetected) {
    motionDetected = true;
    lastMotionTime = currentTime;
    
    // Bật đèn khi phát hiện chuyển động
    turnOnLight();
    
    Serial.println("Phát hiện chuyển động!");
  } 
  else if (!motion && motionDetected && currentTime - lastMotionTime > 30000) {
    // Tắt đèn sau 30 giây nếu không còn phát hiện chuyển động
    motionDetected = false;
    turnOffLight();
    
    Serial.println("Không còn chuyển động");
  }
  
  // Tự động đóng cửa sau khoảng thời gian định sẵn
  if (doorState && currentTime - doorOpenTime > DOOR_AUTO_CLOSE_TIME) {
    closeDoor();
  }
}
