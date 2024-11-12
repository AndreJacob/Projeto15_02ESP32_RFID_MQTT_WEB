#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>

const char* ssid = "Cobertura";
const char* password = "Dev@!2024";
const char* mqtt_server = "10.0.0.97";  // Endereço do servidor MQTT

#define LED_PIN_1 27  // LED principal
#define LED_PIN_2 33  // LED que pisca

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);  // Inicia o servidor web na porta 80

bool tagNotIdentified = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);

  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  // Configura o servidor MQTT
  client.setServer(mqtt_server, 1885);
  client.setCallback(callback);
  reconnect();

  // Configura o servidor web
  server.on("/", handleRoot);            // Página inicial
  server.on("/status", handleStatus);    // Endpoint para o status
  server.begin();

  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao servidor MQTT...");
    if (client.connect("ESP32_02")) {
      Serial.println("Conectado ao MQTT");
      client.subscribe("led/control");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  if (command == "toggle_led1") {
    bool ledState = digitalRead(LED_PIN_1);
    digitalWrite(LED_PIN_1, !ledState);
    Serial.println(ledState ? "Lâmpada desligada" : "Lâmpada ligada");
  } 
  else if (command == "blink_led2") {
    Serial.println("Pisca LED 2");
    tagNotIdentified = true;  // Ativa o estado "Tag não identificada"
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN_2, HIGH);
      delay(250);  // Essa parte pode ser otimizada para evitar delays longos
      digitalWrite(LED_PIN_2, LOW);
      delay(250);  // Ajuste a duração do delay se necessário
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>Status da Lâmpada</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #2c2c2c; color: #ddd; text-align: center; margin-top: 50px; }";
  html += ".status-container { border: 2px solid #444; display: inline-block; padding: 20px; border-radius: 10px; background-color: #3a3a3a; }";
  html += ".status-text { font-size: 24px; color: #ffcc00; margin: 20px 0; }";
  html += ".notification { font-size: 20px; color: #ff4444; display: none; }";
  html += "</style>";
  html += "<script>";
  html += "function updateStatus(state) { document.getElementById('ledStatus').textContent = state; }";
  html += "function showNotification() {";
  html += "  var notification = document.getElementById('notification');";
  html += "  notification.style.display = 'block';";
  html += "  setTimeout(function() { notification.style.display = 'none'; }, 3000);";  // Exibir por 3 segundos
  html += "}";  
  html += "setInterval(function() {";
  html += "  fetch('/status').then(response => response.text()).then(state => {";
  html += "    if (state === 'Tag não identificada') { showNotification(); }";  // Aciona a notificação
  html += "    updateStatus(state);";
  html += "  });";
  html += "}, 1000);";  // Atualização a cada segundo
  html += "</script></head><body>";
  html += "<div class='status-container'>";
  html += "<h1>Status da Lâmpada</h1>";
  html += "<p><strong>Estado: </strong><span class='status-text' id='ledStatus'>Carregando...</span></p>";
  html += "<p class='notification' id='notification'>Tag não identificada</p>";
  html += "</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleStatus() {
  if (tagNotIdentified) {
    server.send(200, "text/plain", "Tag não identificada");
    tagNotIdentified = false;  // Reseta o estado após enviar a notificação
  } else {
    bool ledState = digitalRead(LED_PIN_1);
    String state = ledState ? "Lâmpada ligada" : "Lâmpada desligada";
    server.send(200, "text/plain", state);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();  // Processa requisições HTTP
}
