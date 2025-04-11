#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// WiFi credentials
const char* ssid = "minh"; 
const char* password = "123"; 

// MQTT Broker
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* topic_control = "home/control";
const char* topic_alert = "home/alert";

// Pin definitions
#define SERVO_PIN 13  // Servo điều khiển cửa
#define LED_PIN 12    // Đèn (LED hoặc relay)
#define FAN_PIN 14    // Quạt (relay)
#define RST_PIN 15    // Pin RST cho RFID
#define SDA_PIN 5     // Pin SDA cho RFID
#define GAS_PIN 34    // Pin analog cho cảm biến khí ga

// RFID
MFRC522 rfid(SDA_PIN, RST_PIN);
String authorizedUID = "12 34 56 78"; // UID của thẻ được phép
int failedAttempts = 0;

// Servo
Servo doorServo;

// Gas sensor
int gasThreshold = 500; // Ngưỡng phát hiện khí ga

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Setup pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(GAS_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);

  // Setup servo
  doorServo.attach(SERVO_PIN);
  doorServo.write(0); // Đóng cửa ban đầu

  // Setup RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Quét thẻ RFID...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }
  Serial.println("Đã kết nối WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Kiểm tra RFID
  checkRFID();

  // Kiểm tra cảm biến khí ga
  checkGasSensor();
}

// Hàm kết nối lại với MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.println("Đang kết nối tới MQTT Broker...");
    if (client.connect("ESP32Client")) { 
      Serial.println("Đã kết nối");
      client.subscribe(topic_control);
    } else {
      Serial.print("Kết nối thất bại, rc=");
      Serial.print(client.state());
      Serial.println(" Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}

// Hàm xử lý tin nhắn từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Tin nhắn nhận được [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Xử lý lệnh từ ứng dụng Android
  if (message == "DOOR_OPEN") {
    doorServo.write(90); // Mở cửa
    Serial.println("Cửa đã mở");
  } else if (message == "DOOR_CLOSE") {
    doorServo.write(0); // Đóng cửa
    Serial.println("Cửa đã đóng");
  } else if (message == "LIGHT_ON") {
    digitalWrite(LED_PIN, HIGH); // Bật đèn
    Serial.println("Đèn đã bật");
  } else if (message == "LIGHT_OFF") {
    digitalWrite(LED_PIN, LOW); // Tắt đèn
    Serial.println("Đèn đã tắt");
  } else if (message == "FAN_ON") {
    digitalWrite(FAN_PIN, HIGH); // Bật quạt
    Serial.println("Quạt đã bật");
  } else if (message == "FAN_OFF") {
    digitalWrite(FAN_PIN, LOW); // Tắt quạt
    Serial.println("Quạt đã tắt");
  }
}

// Hàm kiểm tra thẻ RFID
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  Serial.print("UID thẻ: ");
  Serial.println(uid);

  if (uid == authorizedUID) {
    Serial.println("Thẻ hợp lệ");
    failedAttempts = 0;
    doorServo.write(90); // Mở cửa
    delay(3000);
    doorServo.write(0); // Đóng cửa
  } else {
    Serial.println("Thẻ không hợp lệ");
    failedAttempts++;
    if (failedAttempts >= 3) {
      client.publish(topic_alert, "CẢNH BÁO ĐỘT NHẬP");
      Serial.println("Gửi cảnh báo: CẢNH BÁO ĐỘT NHẬP");
      failedAttempts = 0;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Hàm kiểm tra cảm biến khí ga
void checkGasSensor() {
  int gasValue = analogRead(GAS_PIN);
  Serial.print("Giá trị khí ga: ");
  Serial.println(gasValue);

  if (gasValue > gasThreshold) {
    client.publish(topic_alert, "CẢNH BÁO CHÁY");
    Serial.println("Gửi cảnh báo: CẢNH BÁO CHÁY");
    delay(5000); // Tránh gửi liên tục
  }
}
