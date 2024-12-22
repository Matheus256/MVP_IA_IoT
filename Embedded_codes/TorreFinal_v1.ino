#include <Arduino.h>
#include <WiFi.h> //Biblioteca para o WiFi
#include <PubSubClient.h> //Biblioteca para o MQTT
#include <stdio.h>
#include <OneWire.h> 
#include <DallasTemperature.h> //Biblioteca para o sensor de temperatura
#include <DHT.h> //Biblioteca para o sensor de temperatura e umidade
#include <GravityTDS.h>
//#include "GravityTDS.h" //Biblioteca para trabalhar com o sensor de TDS (Total Dissolved Solids) da Gravity.

//Para o pH =======================
#include <SPI.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

Adafruit_ADS1115 ads;

int buffer_arr[10], temp;
unsigned long int avgval;
float ph_act;
float calibration_value = 13.213;

//========================

#include <EEPROM.h>

#define ONE_WIRE_BUS 14 // Pino para ler a temperatura corresponde ao GPIO 14
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Para os sensores de umidade
#define DHTPIN 18      // Define o pino de sinal do DHT11 para GPIO 25
#define DHTTYPE DHT11  // Define o tipo de sensor como DHT11
DHT dht1(DHTPIN, DHTTYPE);

//Outro dht no pino 19
#define DHTPIN2 19
DHT dht2(DHTPIN2, DHTTYPE);


#define TdsSensorPin 33 // Sensor de condutividade no GPIO 33
GravityTDS gravityTds; //Criar uma instância do objeto GravityTDS
//ATENÇÃO PARA CALIBRA DIGITE "ENTER" E EM SEGUIDA "CAL:84" ("CAL:94")

const int IntermediaryBuoySensor = 35; //Sensor de nível de água (Boia Intermediaria)
const int LowBuoySensor = 34; //Sensor de nível de água (Boia que está na base)

//Para o semaforo binario que vai gerencias as tarefas
SemaphoreHandle_t xSemaphore;

// Configurações de Wi-Fi
const char* ssid = "nome_da_rede_wifi";
const char* password = "...";

// Configurações MQTT
const char* mqttServer = "...";
const int mqttPort = ...;  // Porta padrão MQTT
const char* mqttUser = "user_name:device_id";  // Apenas o usuário, sem senha
const char* mqttTopic = "user_name:device_id/attrs";  // Substitua <device_id> pelo ID do dispositivo no Dojot

// Cria instâncias para cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//Variaveis globais que guardam os valores medidos pelos sensores
float tempC = 0.00; //Temperatura
float hum = 0; //Sensor de Umidade 1
float hum2 = 0; //Sensor de Umidade 2
float conductivity = 0; //Condutividade elétrica
int intermediary_buoy = 0; //Nível da boia alta
int low_buoy = 0; //Nível da boia baixa
float pH = 0;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  //Conexão com o WiFi
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Função de callback MQTT (não será usada neste exemplo, mas pode ser útil para receber mensagens)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Função para conectar ao broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT... ");
    // Conecta apenas com o id do cliente e usuário, o terceiro parâmetro é para senha
    if (client.connect("ESP32Client", mqttUser, "")) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

float get_humidity(DHT &dht){
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Verifica se a leitura falhou
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Falha ao ler do sensor DHT11!");
    //return;
  }

  // Exibe a leitura no monitor serial
  Serial.print("Umidade: ");
  Serial.println(humidity);

  return humidity;
}

float get_tds_value(float temp){
  gravityTds.setTemperature(temp);
  gravityTds.update();

  //Leitura e escrita do valor TDS
  float tdsValue = gravityTds.getTdsValue();
  Serial.print("Condutividade: ");
  Serial.print(tdsValue,0); //Imprime na tela no valor lido pelo sensor de condutividade com 0 casas decimais
  Serial.println("ppm");

  return tdsValue;
}

int get_buoy_level(const int &buoy_pin){
  int buoy_level = digitalRead(buoy_pin);

  Serial.print("Nivel da Boia: ");
  if(buoy_level == 1){
    Serial.println("ALTO");
  } else if(buoy_level == 0){
    Serial.println("BAIXO");
  } else {
    Serial.println("Ocorreu algum erro!");
  }

  return buoy_level;
}

void setup_ph(){
  Wire.begin(22, 23); // SDA no GPIO 22 e SCL no GPIO 23

    ads.setGain(GAIN_ONE);

    if(!ads.begin())
    {
        Serial.println("Falha ao iniciar o ADS.");
        while(1)
            ;
    }
}

float get_ph(){
  avgval = 0;
    float voltage = 0.0;
    
    // Ler o ADC do canal especificado
    int16_t adc0 = ads.readADC_SingleEnded(0);
    
    // Calcular a tensão
    float volts0 = ads.computeVolts(adc0);

    // Armazenar os valores de tensão em um buffer
    for(int i = 0; i < 10; i++)
    {
        buffer_arr[i] = volts0;
        delay(30);
    }

    // Ordenar os valores do buffer
    for(int i = 0; i < 9; i++)
    {
        for(int j = i + 1; j < 10; j++)
        {
            if(buffer_arr[i] > buffer_arr[j])
            {
                temp = buffer_arr[i];
                buffer_arr[i] = buffer_arr[j];
                buffer_arr[j] = temp;
            }
        }
    }

    // Calcular a média dos valores do buffer
    for(int i = 2; i < 8; i++)
        avgval += buffer_arr[i];

    float volt = (float)avgval * 3.3 / 4096 / 6;

    // Exibir a tensão no monitor serial
    //Serial.print("Tensão: ");
    //Serial.println(volts0);
    Serial.println("");
    
    // Calcular o valor de pH com base na tensão lida e no valor de calibração
    ph_act = -5.70 * volts0 + calibration_value;

    // Exibir o valor de pH no monitor serial
    Serial.print("Valor de pH: ");
    Serial.println(ph_act);

    return ph_act;
}

void get_sensors_values_task(void *pvParameters) {
  while (1) {
    if (xSemaphoreGive(xSemaphore) == pdTRUE){
      //Fazendo as leituras dos dados dos sensores
      //Serial.println("Reconhecendo os valores dos sensores..."); //Printa

      sensors.requestTemperatures(); //Comando para obter a temperatura
      tempC = sensors.getTempCByIndex(0);

      // Checando se a temperatura foi realizada com sucesso
      if (tempC != DEVICE_DISCONNECTED_C){
        Serial.print("A temperatura é: ");
        Serial.println(tempC);
      } else {
        Serial.println("Ops! Nenhuma leitura de temperatura foi realizada");
      }

      Serial.println("");
      //Atualizando oas variaveis com os novos valores lidos pelos sensores
      hum = get_humidity(dht1);
      hum2 = get_humidity(dht2);

      intermediary_buoy = get_buoy_level(IntermediaryBuoySensor);
      low_buoy = get_buoy_level(LowBuoySensor);

      pH = get_ph();

      conductivity = get_tds_value(tempC);
    }
    //Aguarda alguns segundos
    vTaskDelay(15000 / portTICK_PERIOD_MS);
  }
}

void send_data_task(void *pvParameters) {
  while (1) {

    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    
    // Aguarda os dados dos sensores serem atualizados
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
      //if (!client.connected()) {
      //  reconnect();
      //}
      //client.loop();
  
      //Criacão do payload para ser enviadoã
      char payload[182];
      sprintf(payload, "{\"Temperatura\": %.2f, \"Umidade1\": %.2f, \"Umidade2\": %.2f, \"CondutividadeEletrica\": %.0f, \"NivelBoiaAlta\": %d, \"NivelBoiaBaixa\": %d, \"pH\": %.2f}", tempC, hum, hum2, conductivity, intermediary_buoy, low_buoy, pH);

      Serial.print("Enviando payload: ");
      Serial.println(payload);

      //Publica o payload no tópico
      if (client.publish(mqttTopic, payload)) {
        Serial.println("Mensagem publicada com sucesso");
      } else {
        Serial.println("Falha ao publicar a mensagem");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  //Iniciando o WiFi e a cenxão com o MQTT
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  //Inicialização do senosor de temperatura
  //Serial.println("");
  //Serial.println("Inicio do sensor de Temperatura");

  sensors.begin(); //Sensor de temperatura

  //Sensores de temperatura e umidade
  dht1.begin(); 
  dht2.begin(); 

  //Para o sensor TDS
  gravityTds.setPin(TdsSensorPin); //Definindo o pino do sensor através do TdsSensorPin definido pelo #define do início 
  gravityTds.setAref(3.3); //Garantirá que o sensor de TDS utilize a tensão correta do ESP8266 para suas medições.
  gravityTds.setAdcRange(1024); //1024 é a resolução correta para o conversor analógico-digital de 10 bits do ESP8266.
  gravityTds.begin();

  setup_ph();

  //Sensores boia nível de água
  pinMode(IntermediaryBuoySensor, INPUT);
  pinMode(LowBuoySensor, INPUT);

  //Criação do semáforo binario
  xSemaphore = xSemaphoreCreateBinary();

  //Criação das tarefas
  xTaskCreate(get_sensors_values_task, "Update sensors values", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY, NULL);
  xTaskCreate(send_data_task, "Publish values", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY, NULL);
}

void loop() {
  //Loop vazio pois as tarefas já foram criadas
}
