/*
 *********************************************************************************************************
  ESTE CODIGO TEM A INTENÇÃO DE MOSTRAR UMA APLICAÇÃO USANDO UM ESP8266-12,
  E UM SHIELD RFID-RC522, NA PRIMEIRA VEZ APOS CARREGAR O CODIGO, ABRA O MONITOR SERIAL,
  CONFIGURE O MESMO PARA A VELOCIDADE DE 9600 BAUD RATE,E PRESSIONE O BOTÃO RESET DA SUA PLACA,
  NO MONITOR SERIAL VOCÊ IRA OBSERVAR A MENSAGEM: APROXIME SEU CARTAO (TAG).
  AO APROXIMAR SEU CARTAO,COMO ELE AINDA NAO ESTA CADASTRADO ELE NÃO ACIONARA O GPIO,
  MAS IMPRIMIRA A TAG DO MESMO, DESTA FORMA VOCE AGORA PODE COLOCAR NO CODIGO A TAG
  QUE FOI IMPRESSA NO MONITOR SERIAL DO SEU CARTÃO, OU CHAVEIRO, INSIRA A TAG NA LINHA:
  if (conteudo.substring(1) == "AQUI SUA TAG" || "AQUI OUTRA SE QUISER") //UID 1 - 2....
  E CARREGUE O CODIGO COM SUA TAG, OU COM AS TAGS SE FOR MAIS UMA,
  AGORA DEPOIS DE CARREGAR E PRESSIONAR O BOTÃO RESET, APROXIME O CARTÃO, E O LED
  ACENDERA POR 3 SEGUNDOS, VOCÊ PODE DEFINIR ESSE TEMPO COMO QUISER, NESTA SAIDA
  PODE USAR UM RELE PARA ABRIR UM SOLENOIDE DE UMA FECHADURA POR EXEMPLO.

  ESTE CODIGO FONTE É LIVRE E SEGUE OS PRINCIPIOS GNU, PODE SER ADAPTADO, NODIFICADO
  PARCIAL OU INTEIRAMENTE SEM PERVIO AVISO POR QUALQUER PESSOA.

  COMIGO FUNCIONOU PERFEITAMENTE SEM PROBLEMA ALGUM, MAS NÃO GARANTO SEU FUNCIONAMENTO,
  FICA POR CONTA E RISCO DE QUEM QUISER USAR, DUVIDAS FAVOR ME CONTATAR NO E-MAIL ABAIXO.

  COMIGO FUNCIONOU PERFEITAMENTE SEM PROBLEMA ALGUM, MAS NÃO GARANTO SEU FUNCIONAMENTO,
  FICA POR CIONTA E RISCO DE QUEM QUISER USAR, DUVIDAS FAZER ME CONTATAR NO E-MAIL ABAIXO.

  contato@carloskwiek.com.br

  Carlos kwiek, engenheiro eletronico amante de eletronica e programação.
  www.carloskwiek.com.br
 ************************************************************************************************************
 *************************PINAGEM****************************************************************************

  RFID-RC522   Wemos          Nodemcu
  RST/Reset RST  D3  [1*]        D3 [1*]      GPIO-0  [1*]
  SPI SS  SDA[3] D8  [2*]        D8 [2*]      GPIO-15 [2*]
  SPI MOSI MOSI  D7              D7           GPIO-13
  SPI MISO MISO  D6              D6           GPIO-12
  SPI SCK SCK    D5              D5           GPIO-14

  [1*] (1, 2) Configuracao tipica definindo como RST_PIN no sketch / programa.
  [2*] (1, 2) Configuracao, tipica definindo como SS_PIN no sketch / programa.
  [3*] O pino SDA pode ser rotulado como SS em algumas placas de MFRC522 / mais antigas, significam a mesma coisa.
******************************************************************************************************************
*/

#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include <SPI.h>
#include <MFRC522.h>

#define DEBUG
#define RST_PIN    5 //GPIO5 D1
#define SS_PIN     4 //GPIO4 D2  
#define LED        0 //GPIO0 D3

//informações da rede WIFI
const char* ssid = "LAR";                 //SSID da rede WIFI
const char* password =  "LAR@1480";    //senha da rede wifi

//informações do broker MQTT - Verifique as informações geradas pelo CloudMQTT
const char* mqttServer = "10.6.4.123";   //server
const char* mqttUser = "";              //user
const char* mqttPassword = "";      //password
const int mqttPort = 1883;                     //port
const char* mqttTopicSub = "RFID";           //tópico que sera assinado
String cartao = "";
bool repeticao = false;

WiFiClient espClient;
PubSubClient client(espClient);

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

String lerCartao(String antigo, bool repeticao);

void setup() {
  Serial.begin(9600);   // Inicia a serial
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.println("Conectando ao WiFi...\n");
#endif
  }
#ifdef DEBUG
  Serial.println("Conectado na rede WiFi\n");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.print(WiFi.localIP());// Print the IP address
#endif

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
#ifdef DEBUG
    Serial.println("Conectando ao Broker MQTT...\n");
#endif

    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
#ifdef DEBUG
      Serial.println("Conectado\n");
#endif

    } else {
#ifdef DEBUG
      Serial.print("falha estado: ");
      Serial.print(client.state());
      Serial.println();
#endif
      delay(2000);

    }
  }

  //subscreve no tópico
  client.subscribe(mqttTopicSub);

}

void callback(char* topic, byte* payload, unsigned int length) {

  //armazena msg recebida em uma sring
  payload[length] = '\0';
  String strMSG = String((char*)payload);

#ifdef DEBUG
  Serial.print("Mensagem chegou do tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.print(strMSG);
  Serial.println();
  Serial.println("___________________");
#endif

  if(strMSG == 0){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
  }
}

//função pra reconectar ao servido MQTT

void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
#ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT.\n");
#endif

    bool conectado = strlen(mqttUser) > 0 ?
                     client.connect("ESP8266Client", mqttUser, mqttPassword) :
                     client.connect("ESP8266Client");

    if (conectado) {
#ifdef DEBUG
      Serial.println("Conectado!");
#endif
      //subscreve no tópico
      client.subscribe(mqttTopicSub, 1); //nivel de qualidade: QoS 1
    } else {
#ifdef DEBUG
      Serial.println("Falha durante a conexão. Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
#endif
      //Aguarda 10 segundos
      delay(10000);
    }
  }
}

void loop() {
  String master = "";//59 2E 86 5E
  // Procura por cartao RFID
  if (!client.connected()) {
    reconect();
  }
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Seleciona o cartao RFID
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  cartao = lerCartao(cartao);
  if (!repeticao) {
    if (cartao == master) {
      Serial.println("UID: ");
      Serial.print(cartao);
      Serial.println(" Liberado!");
      Serial.println();
    }
    else {
      Serial.println("UID: ");
      Serial.print(cartao);
      Serial.println(" Sai daqui!");
      Serial.println();
    }
  }
  client.loop();
}

String lerCartao(String antigo) {

  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  if (conteudo == antigo) {
    repeticao = true;
  }
  else {
    repeticao = false;
  }
  return conteudo;
}
