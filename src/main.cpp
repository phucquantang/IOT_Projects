#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define DHTPIN 12
#define LED 26
#define MQ2_PIN 33
#define BUZZER_PIN 25
#define DHTTYPE DHT22

DHT_Unified dht(DHTPIN, DHTTYPE);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "43b77b57cb804196841655508e884c99.s1.eu.hivemq.cloud";
const char* mqttUser = "Nhom1";
const char* mqttPassword = "Nhom1MKTest";
const char* clientID = "esp32_dht22_client_001";

const char* topic_data = "nhom1home/nha_bep/fire_alarm/data";
const char* topic_audio = "nhom1home/nha_bep/fire_alarm/alert";
const char* topic_led = "nhom1home/nha_bep/fire_alarm/led_control";

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0;
const long interval = 1000;

float temp = 0, hum = 0;
int gasValue = 0;

bool isFire = false;
bool lastFireState = false;

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect(clientID, mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(topic_led); 
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == topic_led) { 
    if (message == "true") {
      digitalWrite(LED, HIGH);
    } else {
      digitalWrite(LED, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  setup_wifi();

  espClient.setInsecure(); 
  
  client.setServer(mqttServer, 8883); 
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    gasValue = analogRead(MQ2_PIN);
    sensors_event_t event;

    dht.temperature().getEvent(&event);
    temp = isnan(event.temperature) ? temp : event.temperature;

    dht.humidity().getEvent(&event);
    hum = isnan(event.relative_humidity) ? hum : event.relative_humidity;

    isFire = (temp > 50 && gasValue > 2000);

    String csv = String(temp) + "," + String(hum) + "," + String(gasValue);
    client.publish(topic_data, csv.c_str()); 
    Serial.println("Thông tin gửi:" + csv);

    if (isFire != lastFireState) {
      if (isFire) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED, HIGH);
        client.publish(topic_audio, "true"); 
        Serial.println("Có lửa");
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED, LOW);
        client.publish(topic_audio, "false"); 
        Serial.println("Lửa tắt");
      }
      lastFireState = isFire;
    }
  }
}