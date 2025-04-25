#define BLYNK_TEMPLATE_ID "TMPL6j9Sm_8GA" 
#define BLYNK_TEMPLATE_NAME "NHÀ MINH"
#define BLYNK_AUTH_TOKEN "e5-GCef4Qms10VGVGmCx2xxn1p4L2EZ5"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <DNSServer.h>
#include <WebServer.h>

// AP mode settings
#define USE_AP_MODE true  // Đặt true để sử dụng chế độ AP
const char* ap_ssid = "ESP32";  // Tên WiFi phát ra
const char* ap_pass = "12345678";  // Mật khẩu WiFi phát ra

// Thông tin WiFi client mode
char ssid[] = "abcxyz";
char pass[] = "12345678";

#define GAS_SENSOR_PIN 34  // Cảm biến khí ga
#define LED_PIN 15         // LED 
#define BUZZER_PIN 18      // Buzzer
#define SERVO_PIN 13       // Servo
#define IR_SENSOR_PIN 19   // Cảm biến hồng ngoại

// Ngưỡng cảm biến khí ga
#define GAS_THRESHOLD 100

// Timeout và thời gian chờ
#define WIFI_TIMEOUT 20000      // 20 giây cho kết nối WiFi
#define BLYNK_TIMEOUT 10000     // 10 giây cho kết nối Blynk
#define RECONNECT_INTERVAL 30000 // 30 giây giữa các lần thử kết nối lại

// DNS Server và Web Server để tạo Captive Portal
DNSServer dnsServer;
WebServer webServer(80);
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

Servo myServo;  
bool buzzerActive = false;
bool irAlertSent = false;
bool ledState = false;
unsigned long lastReconnectAttempt = 0;
unsigned long lastPrintStatus = 0;

// Trang web khi kết nối vào WiFi của ESP32
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Smart Home</title>";
  html += "<style>";
  html += "* {box-sizing: border-box;}";
  html += "body {font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif, 'Apple Color Emoji', 'Segoe UI Emoji', 'Segoe UI Symbol'; margin: 0; padding: 20px; background: #f5f5f5; color: #333; line-height: 1.6; font-size: 16px;}";
  html += "h1 {color: #0066cc; margin-bottom: 20px; font-size: 24px;}";
  html += "h2 {font-size: 20px; margin-top: 20px; margin-bottom: 15px;}";
  html += ".container {max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1);}";
  html += ".status {margin-bottom: 15px; padding: 10px; border-radius: 5px; background: #f0f0f0;}";
  html += ".sensor-value {font-weight: bold; color: #0066cc;}";
  html += ".alert {color: #ff3300; font-weight: bold;}";
  html += ".btn {display: inline-block; background: #0066cc; color: white; padding: 10px 15px; text-decoration: none; border-radius: 5px; margin-top: 10px; cursor: pointer; border: none; font-size: 16px;}";
  html += ".btn:hover {background: #004c99;}";
  html += "ul {padding-left: 20px;}";
  html += "li {margin-bottom: 8px;}";
  html += "@media (max-width: 600px) {body {padding: 10px;} .container {padding: 15px;} h1 {font-size: 22px;} .btn {width: 100%; text-align: center;}}";
  html += "</style></head>";
  html += "<body><div class='container'>";
  html += "<h1>ESP32 Smart Home Control</h1>";
  
  // Trạng thái khí ga
  int gasValue = analogRead(GAS_SENSOR_PIN);
  gasValue = map(gasValue, 0, 4095, 0, 100);
  html += "<div class='status'>Giá trị khí ga: <span class='sensor-value'>" + String(gasValue) + "</span>";
  if (gasValue > GAS_THRESHOLD) {
    html += " <span class='alert'>CẢNH BÁO: Nồng độ khí ga vượt ngưỡng!</span>";
  } else {
    html += " (Bình thường)";
  }
  html += "</div>";
  
  // Trạng thái cảm biến hồng ngoại
  int irState = digitalRead(IR_SENSOR_PIN);
  html += "<div class='status'>Cảm biến hồng ngoại: <span class='sensor-value'>";
  html += (irState == LOW) ? "Có người" : "Không có người";
  html += "</span></div>";
  
  // Trạng thái LED
  html += "<div class='status'>LED: <span class='sensor-value'>";
  html += (digitalRead(LED_PIN) == LOW) ? "Bật" : "Tắt";
  html += "</span></div>";
  
  // Trạng thái Buzzer
  html += "<div class='status'>Buzzer: <span class='sensor-value'>";
  html += (digitalRead(BUZZER_PIN) == HIGH) ? "Đang kêu" : "Tắt";
  html += "</span></div>";
  
  // Trạng thái kết nối
  html += "<div class='status'>Blynk: <span class='sensor-value'>";
  html += Blynk.connected() ? "Đã kết nối" : "Chưa kết nối";
  html += "</span></div>";
  
  // Nút điều khiển và làm mới trang
  html += "<button onclick='location.reload()' class='btn'>Làm mới</button>";
  
  // Thông tin kết nối Blynk
  html += "<div style='margin-top: 20px; padding-top: 20px; border-top: 1px solid #ddd;'>";
  html += "<h2>Thông tin kết nối Blynk</h2>";
  html += "<p>Để điều khiển thiết bị qua Blynk, vui lòng cài đặt ứng dụng Blynk và kết nối với các thông tin sau:</p>";
  html += "<ul>";
  html += "<li>BLYNK_TEMPLATE_ID: TMPL6j9Sm_8GA</li>";
  html += "<li>BLYNK_TEMPLATE_NAME: NHÀ MINH</li>";
  html += "<li>BLYNK_AUTH_TOKEN: e5-GCef4Qms10VGVGmCx2xxn1p4L2EZ5</li>";
  html += "</ul>";
  html += "</div>";
  
  html += "</div></body></html>";
  webServer.send(200, "text/html", html);
}

// Xử lý khi truy cập URL không tồn tại - chuyển hướng về trang chính
void handleNotFound() {
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void setupCaptivePortal() {
  // Thiết lập DNS Server để chuyển hướng tất cả yêu cầu tới IP của ESP32
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Cấu hình web server
  webServer.on("/", handleRoot);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  
  Serial.println("Captive Portal đã khởi tạo");
}

void connectWiFiAndBlynk() {
  if (USE_AP_MODE) {
    // Tạo điểm phát WiFi (Access Point)
    Serial.print("Đang tạo Access Point: ");
    Serial.println(ap_ssid);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ap_ssid, ap_pass);
    
    Serial.print("Địa chỉ IP của AP: ");
    Serial.println(WiFi.softAPIP());
    
    // Thiết lập Captive Portal
    setupCaptivePortal();
    
    // Kết nối Blynk qua chế độ AP
    Serial.println("Đang kết nối Blynk qua AP mode...");
    Blynk.config(BLYNK_AUTH_TOKEN);
    unsigned long startTime = millis();
    while (!Blynk.connected() && millis() - startTime < BLYNK_TIMEOUT) {
      Blynk.connect();
      delay(500);
    }
    
    if (Blynk.connected()) {
      Serial.println("Đã kết nối Blynk qua AP mode!");
    } else {
      Serial.println("Không thể kết nối Blynk qua AP mode!");
    }
  } else {
    // Kết nối WiFi
    Serial.print("Đang kết nối WiFi: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nĐã kết nối WiFi!");
      Serial.print("Địa chỉ IP: ");
      Serial.println(WiFi.localIP());
      
      // Kết nối Blynk với timeout
      Serial.println("Đang kết nối Blynk...");
      Blynk.config(BLYNK_AUTH_TOKEN);
      startTime = millis();
      while (!Blynk.connected() && millis() - startTime < BLYNK_TIMEOUT) {
        Blynk.connect();
        delay(500);
      }
      
      if (Blynk.connected()) {
        Serial.println("Đã kết nối Blynk!");
      } else {
        Serial.println("Không thể kết nối Blynk!");
      }
    } else {
      Serial.println("\nKhông thể kết nối WiFi!");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Đợi Serial ổn định
  
  Serial.println("\n=== Smart Home System ===");
  
  // Khởi tạo chân
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  digitalWrite(LED_PIN, HIGH);  // Tắt LED ban đầu (HIGH vì sinking)
  digitalWrite(BUZZER_PIN, LOW); // Tắt buzzer ban đầu
  
  // Khởi tạo servo trước khi kết nối WiFi
  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0);
  Serial.println("Servo đã khởi tạo thành công");
  
  // Kết nối WiFi và Blynk
  connectWiFiAndBlynk();
}

// Hàm xử lý sự kiện khi kết nối Blynk thành công
BLYNK_CONNECTED() {
  Serial.println("Blynk đã kết nối! Đồng bộ trạng thái...");
  Blynk.syncVirtual(V0, V1); // Đồng bộ trạng thái servo và LED
}

// Điều khiển servo từ Blynk
BLYNK_WRITE(V0) {
  int angle = param.asInt(); // Lấy giá trị từ slider (0-110)
  myServo.write(angle);
  Serial.print("Servo Angle: ");
  Serial.println(angle);
}

// Điều khiển LED từ Blynk
BLYNK_WRITE(V1) {
  ledState = param.asInt(); // Lấy giá trị từ switch (0-1)
  updateLED();
  Serial.print("LED State (Blynk): ");
  Serial.println(ledState);
}

// Cập nhật trạng thái LED dựa trên cảm biến IR và ledState
void updateLED() {
  int irState = digitalRead(IR_SENSOR_PIN);
  if (irState == LOW || ledState == 1) {
    digitalWrite(LED_PIN, LOW); // Bật LED
    if (Blynk.connected()) {
      Blynk.virtualWrite(V1, 1);
    }
  } else {
    digitalWrite(LED_PIN, HIGH); // Tắt LED
    if (Blynk.connected()) {
      Blynk.virtualWrite(V1, 0);
    }
  }
}

void loop() {
  if (USE_AP_MODE) {
    // Xử lý DNS Server và Web Server cho Captive Portal
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
  
  // Kiểm tra và thử kết nối lại nếu cần
  if (!USE_AP_MODE && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("Kết nối WiFi bị mất. Đang kết nối lại...");
      connectWiFiAndBlynk();
    }
  } else if (!Blynk.connected()) {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("Kết nối Blynk bị mất. Đang kết nối lại...");
      Blynk.connect();
    }
  }
  
  // Chạy Blynk nếu đã kết nối
  if (Blynk.connected()) {
    Blynk.run();
  }
  
  // Đọc giá trị cảm biến khí ga
  int gasValue = analogRead(GAS_SENSOR_PIN);
  gasValue = map(gasValue, 0, 4095, 0, 100); // Chuyển đổi sang thang 0-100
  
  // In trạng thái 5 giây một lần để giảm tải Serial
  if (millis() - lastPrintStatus > 5000) {
    lastPrintStatus = millis();
    Serial.print("Giá trị khí ga: ");
    Serial.print(gasValue);
    if (USE_AP_MODE) {
      Serial.print(", AP Mode, số client kết nối: ");
      Serial.print(WiFi.softAPgetStationNum());
    } else {
      Serial.print(", WiFi: ");
      Serial.print(WiFi.status() == WL_CONNECTED ? "kết nối" : "ngắt kết nối");
    }
    Serial.print(", Blynk: ");
    Serial.println(Blynk.connected() ? "kết nối" : "ngắt kết nối");
  }
  
  // Gửi giá trị cảm biến lên Blynk
  if (Blynk.connected()) {
    Blynk.virtualWrite(V2, gasValue);
  }
  
  // Kiểm tra ngưỡng khí ga
  if (gasValue > GAS_THRESHOLD && !buzzerActive) {
    digitalWrite(BUZZER_PIN, HIGH); // Bật buzzer
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, "CẢNH BÁO: Nồng độ khí ga vượt ngưỡng!");
      Blynk.logEvent("gas_alert", "Nồng độ khí ga vượt ngưỡng!");
    }
    buzzerActive = true;
    Serial.println("Khí ga vượt ngưỡng!");
  } else if (gasValue <= GAS_THRESHOLD && buzzerActive) {
    digitalWrite(BUZZER_PIN, LOW); // Tắt buzzer
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, "Bình thường");
    }
    buzzerActive = false;
    Serial.println("Khí ga bình thường");
  }
  
  // Đọc giá trị cảm biến hồng ngoại và điều khiển LED
  int irState = digitalRead(IR_SENSOR_PIN);
  if (irState == LOW && !irAlertSent) { // Phát hiện người
    digitalWrite(LED_PIN, LOW); // Bật LED (LOW vì sinking)
    if (Blynk.connected()) {
      Blynk.virtualWrite(V4, "Có người");
      Blynk.virtualWrite(V1, 1); // Đồng bộ trạng thái LED trên Blynk
      Blynk.logEvent("ir_alert", "Phát hiện người!");
    }
    irAlertSent = true;
    Serial.println("IR: Có người! LED bật");
  } else if (irState == HIGH && irAlertSent) { // Không có người
    updateLED(); // Cập nhật LED theo trạng thái Blynk
    if (Blynk.connected()) {
      Blynk.virtualWrite(V4, "Không có người");
    }
    irAlertSent = false;
    Serial.println("IR: Không có người");
  }
  
  delay(100); // Giảm tải CPU
}
