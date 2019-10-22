#include "DHT.h"

#include <SoftwareSerial.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define TOPICO_SUBSCRIBE "lampReceiving"
#define TOPICO_PUBLISH   "lampSending"

#define ID_MQTT  "krpgdhec"

#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

#define DHTTYPE DHT11
#define DHTPIN D4

// DHT11
DHT dht(DHTPIN, DHTTYPE);

// WIFI
const char* SSID = "WIN-NE7NN0E6UGH 3672";
const char* PASSWORD = "12345678";

//const char* SSID = "WLL-Inatel";
//const char* PASSWORD = "inatelsemfio";

// MQTT
const char* BROKER_MQTT = "soldier.cloudmqtt.com";
int BROKER_PORT = 16838;
const char* mqttUser = "krpgdhec";
const char* mqttpassword = "1Us4LQZgRFWl";

//Variáveis e objetos globais
WiFiClient espClient;
PubSubClient MQTT(espClient);
String ventoinha = "0";
char Buf[50];

//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void webRequest();

// D9 (RX) e D10 (TX)
SoftwareSerial minhaSerial(D9 , D10);

//DHT dht11;

void setup()
{
  Serial.begin(9600);
  Serial.println("Hello");
  pinMode(D1, OUTPUT);
  digitalWrite(D1, LOW);
  initSerial();
  initWiFi();
  webRequest();
  initMQTT();
  dht.begin();
}

void initSerial()
{
  Serial.begin(9600);
}

void initWiFi()
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconectWiFi();
}

void initMQTT()
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT, mqttUser, mqttpassword))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      /*Serial.println("Havera nova tentatica de conexao em 2s");*/
      delay(2000);
    }
  }
}

void reconectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    // Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT();

  reconectWiFi();
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String msg;

  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }

  if (msg.equals("ON"))
  {
    digitalWrite(D1, HIGH);
    //Serial.println("1");
  }
  if (msg.equals("OFF")) {
    digitalWrite(D1, LOW);
  }

}

void EnviaEstadoOutputMQTT(void)
{
  MQTT.publish(TOPICO_PUBLISH, Buf);
}

String leStringSerial() {
  String conteudo = "";
  char caractere;

  while (Serial.available() > 0) {
    caractere = Serial.read();
    // Serial.println(caractere);
    if (caractere != '#') {
      conteudo.concat(caractere);
    }
    delay(10);
  }

  // Serial.println(conteudo);
  return conteudo;
}

void webRequest() {

  Serial.println("Web Request");
  
  int httpCode = 1;
  if (WiFi.status() == WL_CONNECTED && httpCode != 200) { //Check WiFi connection status

    HTTPClient http;    //Declare object of class HTTPClient

    http.begin("http://10.0.29.59:8080/device");      //Specify request destination
    //http.begin("http://" + String(SSID) + ":8080/device");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header

    httpCode = http.POST("{\"name\":\"Lamp\",\"type\":\"lamp\",\"subscriptionTopic\":\"lampReceiving\",\"publishingTopic\":\"lampSending\",\"turnOn\":\"ON\",\"turnOff\":\"OFF\"}");   //Send the request
                             
    String payload = http.getString();                  //Get the response payload

    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload

    http.end();  //Close connection

  } else {

    Serial.println("Error in WiFi connection");

  }
}


void loop()
{

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();


  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  String json = String("{\"humidity\": \"") + String(h) + String("%\", \"temperature\": \"") + String(t) + String("°C\"}\"");

  VerificaConexoesWiFIEMQTT();

  MQTT.loop();

  json.toCharArray(Buf, 50);
  EnviaEstadoOutputMQTT();
  delay(200);

}
