#include <OneWire.h> //INCLUSÃO DE BIBLIOTECA
#include <DallasTemperature.h> //INCLUSÃO DE BIBLIOTECA
#include <Wire.h>  //INCLUSÃO DE BIBLIOTECA
#include <LiquidCrystal_I2C.h> //INCLUSÃO DE BIBLIOTECA
#include <PZEM004Tv30.h> //INCLUSÃO DE BIBLIOTECA
#include <ESP8266WiFi.h> //INCLUSÃO DE BIBLIOTECA
#include <PubSubClientMQTT.h> //INCLUSÃO DE BIBLIOTECA

#include "src/iotc/common/string_buffer.h" //INCLUSÃO DE BIBLIOTECA NA PASTA
#include "src/iotc/iotc.h" //INCLUSÃO DE BIBLIOTECA NA PASTA

// RELE
#define RelayPin1 D1  

// SWITCH
#define SwitchPin1 10  

//LED STATUS DO WIFI
#define wifiLed    D0   

int toggleState_1 = 1;

const char* ssid = "Cavalcante"; //REDE WIFI
const char* password = "15052406"; //SENHA WIFI
const char* mqttServer = "driver.cloudmqtt.com"; //SERVER MQTT
const char* mqttUserName = "xxbwkdyp"; // USUARIO MQTT
const char* mqttPwd = "HLSgAt5PZnRw"; // SENHA MQTT
const char* clientID = "xxbwkdyp"; // client id

const char* SCOPE_ID = "0ne005AD578";
const char* DEVICE_ID = "NodeMCU1";
const char* DEVICE_KEY = "+SrTso7qKYUNBG+3p9l0Q9QNym6dwoa5qCIn9oEcjOE=";

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h" //INCLUSÃO DE BIBLIOTECA NA PASTA

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // STATUS DE CONEXAO
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}

/*
-------------------------------------------------
NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
D0 = 16            |  D6 = 12            |
D1 = 5             |  D7 = 13            |
D2 = 4             |  D8 = 15            |
D3 = 0             |  D9 = 3             |
D4 = 2             |  D10 = 1            |
D5 = 14            |                     |
-------------------------------------------------
*/

LiquidCrystal_I2C lcd(0x27, 16, 2);
const int SensorDataPin = 14;  
PZEM004Tv30 pzem(&Serial);

#define sub1 "switch1"

#define pub1 "switch1_status"

WiFiClient espClient;
PubSubClientMQTT client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (80)
char msg[MSG_BUFFER_SIZE];
int value = 0;

OneWire oneWire(SensorDataPin);
 
DallasTemperature sensors(&oneWire);

void setup_wifi() {
 delay(10);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial1.print(".");
 }
 Serial1.println("");
 Serial1.println("WiFi conectado");
 Serial1.println("Endereco IP: ");
 Serial1.println(WiFi.localIP());
}

void reconnect() {
 while (!client.connected()) {
 if (client.connect(clientID, mqttUserName, mqttPwd)) {
 Serial1.println("MQTT conectado");
  
      client.subscribe(sub1);
    } 
    else {
      Serial1.print("falhou, rc=");
      Serial1.print(client.state());
      Serial1.println(" tentando novamente em 5 segundos");
    
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.print("] ");

  if (strstr(topic, sub1))
  {
    for (int i = 0; i < length; i++) {
      Serial1.print((char)payload[i]);
    }
    Serial1.println();
 
    if ((char)payload[0] == '0') {
      digitalWrite(RelayPin1, LOW);   
      toggleState_1 = 0;
      client.publish(pub1, "0");
    } else {
      digitalWrite(RelayPin1, HIGH); 
      toggleState_1 = 1;
      client.publish(pub1, "1");
    }    
  }
  else
  {
    Serial1.println("unsubscribed topic");
  }
}

void manual_control(){
    // Controle manual do rele
    if (digitalRead(SwitchPin1) == LOW){
      delay(200);
      if(toggleState_1 == 1){
        digitalWrite(RelayPin1, LOW); 
        toggleState_1 = 0;
        client.publish(pub1, "0");
        Serial1.println("Device1 ON");
        }
      else{
        digitalWrite(RelayPin1, HIGH); 
        toggleState_1 = 1;
        client.publish(pub1, "1");
        Serial1.println("Device1 OFF");
        }
     }
    delay(100);
}

void setup() {
  Serial1.begin(115200);
  pinMode(RelayPin1, OUTPUT);
  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(wifiLed, HIGH);
  Wire.begin(0, 4);
  lcd.init();  
  lcd.backlight();                                        
  lcd.setCursor(0, 0); 
  lcd.print("SMART C BREAKER");
  lcd.setCursor(0, 1); 
  lcd.print("MARCOS,LUIZ&KAUE");
  sensors.begin();
  setup_wifi();
    client.setServer(mqttServer, 18561);
    client.setCallback(callback);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);


  if (context != NULL) {
    lastTick = 0;  
  }
}

void loop() {
  if (!client.connected()) {
    digitalWrite(wifiLed, HIGH);
    reconnect();
  }
  else{
    digitalWrite(wifiLed, LOW);
    manual_control();
  }
  client.loop();
  
  sensors.requestTemperatures(); 
  float temperature_Celsius = sensors.getTempCByIndex(0);
  float temperature_Fahrenheit = sensors.getTempFByIndex(0);
  Serial1.print("Temperature: ");
  Serial1.print(temperature_Celsius);
  Serial1.println(" ºC");
  Serial1.print("Temperature: ");
  Serial1.print(temperature_Fahrenheit);
  Serial1.println(" ºF");
  Serial1.println("");
  if (temperature_Celsius <= -127){
   temperature_Celsius = 0;
  }
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("SMART C BREAKER");
  lcd.setCursor(0, 1);     
  lcd.print("Temp: ");
  lcd.print(temperature_Celsius); 
  lcd.print(" C"); 
  client.loop();
  if (temperature_Celsius > 35){
    Serial1.print("Alerta! Temperatura do disjuntor elevada!! ");
    Serial1.print(temperature_Celsius);
    Serial1.println(" ºC");
    digitalWrite(RelayPin1, HIGH); 
  }
  delay(2000);

float voltage = pzem.voltage();
if(voltage != NAN){
Serial1.print("Voltage: "); Serial1.print(voltage); Serial1.println("V");
  if (voltage>283){
    Serial1.print("Alerta! Tensao elevada!! ");
    digitalWrite(RelayPin1, HIGH); 
  }
client.loop();
} else {
Serial1.println("erro voltagem");
client.loop();
}
float current = pzem.current();
if(current != NAN){
Serial1.print("Current: "); Serial1.print(current); Serial1.println("A");
client.loop();
} else {
Serial1.println("erro corrente");
client.loop();
}
float power = pzem.power();
if(power != NAN){
Serial1.print("Power: "); Serial1.print(power); Serial1.println("W");
client.loop();
} else {
Serial1.println("erro power");
client.loop();
}
float energy = pzem.energy();
if(energy != NAN){
Serial1.print("Energy: "); Serial1.print(energy,3); Serial1.println("kWh");
client.loop();
} else {
Serial1.println("erro energia");
client.loop();
}
float frequency = pzem.frequency();
if(frequency != NAN){
Serial1.print("Frequencia: "); Serial1.print(frequency, 1); Serial1.println("Hz");
client.loop();
} else {
Serial1.println("erro frequencia");
client.loop();
}
float pf = pzem.pf();
if(pf != NAN){
Serial1.print("PF: "); Serial1.println(pf);
client.loop();
} else {
Serial1.println("erro fator de potencia");
client.loop();
}
float gasto;
float taxa = 0.92;
if(energy != NAN) {
  gasto = energy * taxa;
  }
Serial1.println();
delay(2000);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(voltage); 
lcd.print(" V");
lcd.setCursor(7, 0); 
lcd.print(current);
lcd.print(" A");
lcd.setCursor(0, 1); 
lcd.print(power); 
lcd.print(" W");
lcd.setCursor(7, 1); 
lcd.print(frequency); 
lcd.print(" Hz");
client.loop();

  if (isConnected) {

    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  
      char msg[64] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;
      if (loopId++ % 2 == 0) {  
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Temperature\": %f}",
                       temperature_Celsius);
        errorCode = iotc_send_telemetry(context, msg, pos);
        
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Voltage\":%f}",
                       voltage);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"Current\":%f}",
                       current);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"Power\":%f}",
                       power);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"Energy\":%f}",
                       energy);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"Frequency\":%f}",
                       frequency);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"PF\":%f}",
                       pf);
        errorCode = iotc_send_telemetry(context, msg, pos);
        
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Gasto\":%f}",
                      gasto);
        errorCode = iotc_send_telemetry(context, msg, pos);
        client.loop();  
      } else {  
        client.loop();
      } 
  
      msg[pos] = 0;

      if (errorCode != 0) {
        LOG_ERROR("Houve falha no envio da mensagem: %d", errorCode);
        client.loop();
      }
    }

    iotc_do_work(context); 
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
    client.loop();
  }

}
