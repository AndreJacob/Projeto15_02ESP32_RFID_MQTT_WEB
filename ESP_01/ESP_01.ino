#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5         // Pino SDA do RC522
#define RST_PIN 22       // Pino RST do RC522
#define LED_PIN 27       // Pino do LED

const char* ssid = "Cobertura";
const char* password = "Dev@!2024";
const char* mqtt_server = "10.0.0.97";

WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 rfid(SS_PIN, RST_PIN);

bool lampStatus = false;
byte targetUID[4] = {0x47, 0x47, 0xA9, 0x9C};
//byte targetUID[4] = {0xD7, 0x26, 0x02, 0x33};
//47 47 A9 9C

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  client.setServer(mqtt_server, 1885);
  reconnect();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao servidor MQTT...");
    if (client.connect("ESP32_01")) {
      Serial.println("Conectado ao MQTT");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    bool isTargetUID = true;

    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != targetUID[i]) {
        isTargetUID = false;
        break;
      }
    }

    // Imprimir o UID da tag lida na Serial
    Serial.print("UID da Tag: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println(); // Nova linha

    if (isTargetUID) {
      lampStatus = !lampStatus;
      digitalWrite(LED_PIN, lampStatus ? HIGH : LOW);
      client.publish("led/control", lampStatus ? "toggle_led1" : "toggle_led0");
      Serial.println(lampStatus ? "Lâmpada Ligada" : "Lâmpada Desligada");
    } else {
      client.publish("led/control", "blink_led2");
      Serial.println("Comando enviado: blink_led2");
    }

    rfid.PICC_HaltA();
    delay(500);
  }
}
