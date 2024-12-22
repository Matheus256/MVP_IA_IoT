//#include <GravityTDS.h>
#include <OneWire.h> //Biblioteca para sensores que utilizam protocolo 1-Wire (único barramento para dados)
#include <DallasTemperature.h> // Bibliotecas para sensor de temperatura especificamente

#include <WiFi.h> //Biblioteca para se conectar ao wifi

#include <EEPROM.h> //Biblioeteca para ler e escrever dados na memória EEPROM do microcontrolador
//A memória EEPROM (Electrically Erasable Programmable Read-Only Memory) é um tipo de memória não volátil usada em dispositivos eletrônicos para armazenar dados que precisam ser preservados mesmo quando o dispositivo é desligado.

//#include "GravityTDS.h" //Biblioteca para trabalhar com o sensor de TDS (Total Dissolved Solids) da Gravity. Essa biblioteca facilita a leitura e o processamento dos dados do sensor, permitindo que você obtenha as medições de sólidos dissolvidos em água de forma simples.


//#define PH_SENSOR_PIN 32 // Sensor de pH no GPIO 32
//#define TdsSensorPin 33 // Sensor de condutividade no GPIO 32

//GravityTDS gravityTds; //Criar uma instância do objeto GravityTDS da biblioteca Gravity TDS, que permite interagir com o sensor de TDS.

#define ONE_WIRE_BUS 14 // D0 no ESP8266 corresponde ao GPIO 14

OneWire oneWire(ONE_WIRE_BUS);
//Essa linha de código inicializa o sistema 1-Wire, permitindo que o ESP8266 comece a comunicar-se com o sensor DS18B20 através do pino definido (neste caso, o pino D0 ou GPIO16). 

DallasTemperature sensors(&oneWire);
//Essa linha inicializa a comunicação com o sensor de temperatura usando o protocolo 1-Wire. 
//Ao passar a instância oneWire para a classe DallasTemperature, você está dizendo à biblioteca que o sensor de temperatura está conectado àquele pino específico do ESP8266 (definido anteriormente como ONE_WIRE_BUS), permitindo que o programa leia a temperatura a partir desse sensor.

//Definião do nome da rede e da senha
const char *ssid = "nome_da_rede_wifi";
const char *password = "...";

void setup() {
  // Começando Serial
  Serial.begin(9600); //9600 é o número de bits por segundo que serão transmitidos pela comunicação serial entre o ESP8266 e o monitor serial no seu computador.
  Serial.println("Temperatura do sensor");

  // Começando a biblioteca
  sensors.begin();
  //A função sensors.begin(); inicializa a biblioteca DallasTemperature para que o ESP8266 possa começar a se comunicar com os sensores de temperatura DS18B20 conectados ao pino definido.

  //gravityTds.setPin(TdsSensorPin); //Definindo o pino do sensor através do TdsSensorPin definido pelo #define do início 

  //gravityTds.setAref(3.3); //Garantirá que o sensor de TDS utilize a tensão correta do ESP8266 para suas medições.

  //gravityTds.setAdcRange(1024);  //1024 é a resolução correta para o conversor analógico-digital de 10 bits do ESP8266.

  //gravityTds.begin();
  //O begin() garante que o ADC esteja configurado corretamente com base nas definições de tensão de referência (setAref()) e na resolução do ADC (setAdcRange()).
  //A função também verifica se o pino correto foi definido por meio de setPin() e se o sensor está corretamente conectado ao microcontrolador
  //Pode executar uma calibração inicial com base nas condições do ambiente
  //A função também pode inicializar variáveis internas da biblioteca, como buffers de dados e parâmetros para o cálculo do TDS.

  WiFi.begin(ssid, password); //conecta a rede wifi

  //Loop para ficar tentando conectar ao WiFi até conseguir (vai ficar imprimindo pontinhos ".")
  Serial.print("Conectando a rede WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  //Quando a conexão é estabelecida então endeço IP é informadoé
  Serial.println("Conectado!");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void loop() {

  Serial.print("Reconhecendo temperaturas..."); //Printa

  sensors.requestTemperatures(); //Comando para obter a temperatura
  //Essa função apenas solicita a medição, não fornece o valor imediatamente.

  float tempC = 0.00; //Cria uma variável chamada tempC do tipo float para armazenar a temperatura

  tempC = sensors.getTempCByIndex(0); //A variável float tempC recebe retorno da função getTempCByIndex
  //Essa função acessa o primeiro sensor que está conectado ao barramento 1-Wire (que você inicializou com sensors.begin();).
  //O índice 0 refere-se ao primeiro sensor. Se você tivesse mais de um sensor conectado, você poderia usar 1, 2, etc., para acessar os sensores subsequentes.
  //A função retorna a temperatura medida pelo sensor em graus Celsius (°C).
  //Se não houver um sensor conectado ou se houver um erro, a função pode retornar -127.00, que é um valor padrão indicando que a temperatura não pôde ser lida.

  // Checando se a temperatura foi realizada com sucesso
  if (tempC != DEVICE_DISCONNECTED_C)
  {
    Serial.print("A temperatura é: ");
    Serial.println(tempC);
  }
  else
  {
    Serial.println("Ops! Nenhuma leitura de temperatura foi realizada");
  }

  //gravityTds.setTemperature(tempC);  //Esta função define a temperatura da água lida pelo sensor de temperatura, para que o valor do TDS seja ajustado corretamente, já que a concentração de sólidos dissolvidos varia conforme a temperatura. 

  //gravityTds.update();  //Esta função coleta amostras de dados do sensor de TDS, realiza os cálculos internos e atualiza os valores necessários para a leitura.
  //O que acontece dentro de gravityTds.update()?
    //Leitura da tensão no pino analógico
    //Conversão da leitura para condutividade elétrica (EC)
    //Compensação de temperatura
    //Cálculo do valor de TDS (ppm)=Condutividade(μS/cm)×fator de conversão (0.5 a 0.7)
    //Atualização do valor de TDS
  
  //float tdsValue = gravityTds.getTdsValue(); //O valor de tdsValue é resultado da função gravityTds.update()
  //A linha tdsValue = gravityTds.getTdsValue(); obtém o valor de TDS (sólidos dissolvidos totais) em ppm, já calculado após a função gravityTds.update(), e o armazena na variável tdsValue para exibir ou usar no código.

  //Serial.print(tdsValue,0); //Imprime na tela no valor lido pelo sensor de condutividade com 0 casas decimais
  //Serial.println("ppm"); //Imprime na tela a palavra "ppm" após o valor
  delay(25000); //Espera 10000ms -> 10 seg

}
