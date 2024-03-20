#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Define the type of DHT sensor (DHT11)
#define DHTPIN 14           // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11      // DHT sensor type

DHT dht(DHTPIN, DHTTYPE);

const int mq136Pin = A0; // Analog pin connected to the AOUT pin of MQ136
const int mq135Pin = A3; // Analog pin connected to the AOUT pin of MQ135
const int mq7Pin = A5;   // Analog pin connected to the AOUT pin of MQ7

// Replace with your Wi-Fi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Twilio credentials
const char* account_sid = "ACba60831134cf224eef3954bdb6953fe7";
const char* auth_token = "31800f87de7bd7b336145c4d930da1d1";
const char* twilio_phone_number = "+12565883401";
const char* recipient_phone_number = "+918438596165";

// Replace with your XAMPP server IP and port
const char* server_ip = "your_server_ip";
const int server_port = 80;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(26, OUTPUT); // Initialize the DHT sensor
  connectToWiFi();
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int mq136Value = analogRead(mq136Pin);
  float mq136Voltage = mq136Value * (5.0 / 1023.0);
  float mq136PPM = map(mq136Voltage, 0.1, 4.0, 0, 100);
  int mq135Value = analogRead(mq135Pin);
  float mq135Voltage = mq135Value * (5.0 / 1023.0);
  float mq135PPM = map(mq135Voltage, 0.1, 4.0, 0, 1000);
  int mq7Value = analogRead(mq7Pin);
  float mq7Voltage = mq7Value * (5.0 / 1023.0);
  float mq7PPM = map(mq7Voltage, 0.2, 4.0, 0, 2000);

  // Display values on Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\t Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t MQ136 PPM: ");
  Serial.print(mq136PPM);
  Serial.print("\t MQ135 PPM: ");
  Serial.print(mq135PPM);
  Serial.print("\t MQ7 PPM: ");
  Serial.println(mq7PPM);

  // Send data to the server
  sendSensorData(temperature, humidity, mq136PPM, mq135PPM, mq7PPM);

  // Check for conditions and act accordingly
  if ((temperature > 38.0) || (humidity > 75.0) || (mq136PPM > 30.0) || (mq135PPM > 30.0) || (mq7PPM > 30.0)) {
    digitalWrite(26, HIGH);  // Turn on LED
    
    // Send SMS alert using Twilio
    sendTwilioSMS(temperature, humidity, mq136PPM, mq135PPM, mq7PPM);
    
    delay(500);  // Blink for 0.5 seconds
    digitalWrite(26, LOW);
  }
  
  delay(1000); // Wait for a second
}

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");
}

void sendSensorData(float temperature, float humidity, float mq136PPM, float mq135PPM, float mq7PPM) {
  if (client.connect(server_ip, server_port)) {
    Serial.println("Connected to server");
    
    // Send HTTP POST request to the PHP script with sensor data
    String data = "temperature=" + String(temperature) +
                  "&humidity=" + String(humidity) +
                  "&mq136PPM=" + String(mq136PPM) +
                  "&mq135PPM=" + String(mq135PPM) +
                  "&mq7PPM=" + String(mq7PPM);
    
    client.println("POST /store_sensor_data.php HTTP/1.1");
    client.println("Host: your_server_ip");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.println(data);
    client.println();
    
    delay(500);
    
    Serial.println("Data sent to server");
    client.stop();
  } else {
    Serial.println("Unable to connect to server");
  }
}

void sendTwilioSMS(float temperature, float humidity, float mq136PPM, float mq135PPM, float mq7PPM) {
  Serial.println("Sending SMS alert via Twilio");
  
  String message = "Alert: Sensor data exceeded threshold values!\n";
  message += "Temperature: " + String(temperature) + "°C\n";
  message += "Humidity: " + String(humidity) + "%\n";
  message += "MQ136 PPM: " + String(mq136PPM) + "\n";
  message += "MQ135 PPM: " + String(mq135PPM) + "\n";
  message += "MQ7 PPM: " + String(mq7PPM);
  
  HTTPClient http;
  http.begin("https://api.twilio.com/2010-04-01/Accounts/" + String(account_sid) + "/Messages.json");
  http.setAuthorization(String(account_sid) + ":" + String(auth_token));
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "To=" + String(recipient_phone_number) +
                    "&From=" + String(twilio_phone_number) +
                    "&Body=" + message;
  
  int httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 201) {
    Serial.println("SMS sent successfully!");
  } else {
    Serial.println("Failed to send SMS. HTTP Response Code: " + String(httpResponseCode));
  }
  
 http.end();
}
