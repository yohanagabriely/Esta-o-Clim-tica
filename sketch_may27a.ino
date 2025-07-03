#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// --- Definição de pinos ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define RAIN_SENSOR_PIN 15
#define UV_SENSOR_PIN 34
#define UV_ENABLE_PIN 33  // Opcional: para ativar/desativar o ML8511

// --- Inicialização do DHT ---
DHT dht(DHTPIN, DHTTYPE);

// --- Configurações WiFi e Ubidots ---
#define WIFISSID "seu wifi"
#define PASSWORD "sua senha"
#define TOKEN "seu token"
#define DEVICE_LABEL "seu device label"

void setup() {
  Serial.begin(115200);
  
  // Inicializa o DHT
  dht.begin();
  
  // Configura pinos
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(UV_ENABLE_PIN, OUTPUT);
  
  digitalWrite(UV_ENABLE_PIN, HIGH);  // Ativa o ML8511

  // Conectar ao WiFi
  WiFi.begin(WIFISSID, PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// === Função para enviar dados ao Ubidots ===
void sendToUbidots(float temperature, float humidity, int rainStatus, float uvIndex, float uvVoltage) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String serverName = "https://industrial.api.ubidots.com/api/v1.6/devices/" + String(DEVICE_LABEL) + "/";
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Auth-Token", TOKEN);

    String payload = "{\"temperatura\": {\"value\": " + String(temperature, 2) + "},";
    payload += "\"umidade\": {\"value\": " + String(humidity, 2) + "},";
    payload += "\"chuva\": {\"value\": " + String(rainStatus) + "},";
    payload += "\"indice_uv\": {\"value\": " + String(uvIndex, 2) + "},";
    payload += "\"tensao_uv\": {\"value\": " + String(uvVoltage, 2) + "}}";

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro HTTP: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    Serial.println("Payload: " + payload);
    http.end();
  } else {
    Serial.println("WiFi desconectado!");
  }
}

void loop() {
  // --- Leitura do DHT11 ---
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // --- Leitura do sensor de chuva ---
  int rainStatus = digitalRead(RAIN_SENSOR_PIN);
  
  // --- Leitura do sensor UV (ML8511) ---
  int uvRaw = analogRead(UV_SENSOR_PIN);
  float uvVoltage = (uvRaw / 4095.0) * 3.3;  // Conversão para tensão (3.3V)
  
  // Conversão aproximada da tensão para índice UV
  float uvIndex = map(uvRaw, 0, 4095, 0, 15);  // Ajuste conforme necessidade

  // --- Exibe os dados no Serial ---
  Serial.println("===== Leitura de Sensores =====");
  
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Status de Chuva: ");
  Serial.println(rainStatus ? "Sem chuva" : "Chuva detectada");
  
  Serial.print("Tensão UV: ");
  Serial.print(uvVoltage, 2);
  Serial.println(" V");
  
  Serial.print("Índice UV aproximado: ");
  Serial.println(uvIndex);
  
  Serial.println("===============================");

  // --- Enviar dados ao Ubidots ---
  sendToUbidots(temperature, humidity, rainStatus, uvIndex, uvVoltage);

  delay(5000);  // Aguarda 5 segundos entre leituras
}
