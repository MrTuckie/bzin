#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

////////////////////////////////////// DEFINIÇÕES //////////////////////////////////////////

#define TOPICO_SUBSCRIBE "bzmanda"     //tópico MQTT de escuta
#define TOPICO_PUBLISH   "bzrecebe"    //tópico MQTT de envio de informações para Broker

#define ID_MQTT  "bzBotao1"     //id mqtt (para identificação de sessão)

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2 // PINO DO LED ONBOARD
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1
#define pino  5 // Vai acabar sendo o D1 de qualquer jeito.

// WIFI
const char* SSID = ""; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = ""; // Senha da rede WI-FI que deseja se conectar

// MQTT
const char* BROKER_MQTT = "broker.emqx.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
bool statusAnt = 0;

////////////////////////////////////// DEFINIÇÕES //////////////////////////////////////////

//////////////////////////////////////   FUNÇÕES  //////////////////////////////////////////

void initSerial() // Inicializa comunicação serial com baudrate 115200
{
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while(!Serial) { }
  Serial.println("Acordado...");
}

void reconectWiFi()  //Reconecta-se ao WiFi
{
 //se já está conectado a rede WI-FI, nada é feito. 
 //Caso contrário, são efetuadas tentativas de conexão
 if (WiFi.status() == WL_CONNECTED)
      return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(100);
      Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
} 

void initWiFi() //Inicializa e conecta-se na rede WI-FI desejada
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconectWiFi();
}

void initMQTT() //inicializa parâmetros de conexão MQTT
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  //MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

// Por enquanto a callback não vai ser setada.
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  String msg;

  //obtem a string do payload recebido
  for(int i = 0; i < length; i++) 
  {
      char c = (char)payload[i];
      msg += c;
  }

  //toma ação dependendo da string recebida:
  //verifica se deve colocar nivel alto de tensão na saída D0:
  //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
  //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
  if (msg.equals("L"))
  {
      digitalWrite(D0, LOW);
      EstadoSaida = '1';
  }

  //verifica se deve colocar nivel alto de tensão na saída D0:
  if (msg.equals("D"))
  {
       digitalWrite(D0, HIGH);
       EstadoSaida = '0';
  }

}

void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
      Serial.print("* Tentando se conectar ao Broker MQTT: ");
      Serial.println(BROKER_MQTT);
      if (MQTT.connect(ID_MQTT)) 
      {
          Serial.println("Conectado com sucesso ao broker MQTT!");
          MQTT.subscribe(TOPICO_SUBSCRIBE); 
      } 
      else
      {
          Serial.println("Falha ao reconectar no broker.");
          Serial.println("Havera nova tentatica de conexao em 2s");
          delay(2000);
      }
    }
}

void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

    reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
// Posso usar ela para monitorar energia...
void EnviaEstadoOutputMQTT(void)
{
    if (EstadoSaida == '0'){
      MQTT.publish(TOPICO_PUBLISH, "L");
      Serial.println("- L enviado ao broker!");
    }
    if (EstadoSaida == '1'){
      MQTT.publish(TOPICO_PUBLISH, "D");
      Serial.println("- D enviado ao broker!");
    }
    //Serial.println("- Estado da saida D0 enviado ao broker!");
    delay(1000);
}

void InitInput() 
{
//Fazendo o pino ser de entrada.
pinMode(pino, INPUT_PULLUP); // ELE COMEÇA HIGH

}

void publicaComando() 
{
    if (!MQTT.connected()) 
    {
    Serial.println("MQTT desconectado! Tentando reconectar...");
    reconnectMQTT();
    }

    MQTT.loop();

    //Publicando no MQTT
    Serial.println("Fazendo a publicacao...");
    MQTT.publish(TOPICO_PUBLISH,"L");
    Serial.println("L enviado...");

}


void setup() {
  delay(1000);
  initSerial();
  InitInput();
  
  if(digitalRead(pino) == LOW){
    // Se o botão estiver pressionado, vai fazer o que tem que fazer
    initWiFi();
    initMQTT();

    VerificaConexoesWiFIEMQTT();
  
    delay(100);
    
    MQTT.loop();
    publicaComando();
  }
  
  Serial.println("Entrando no sono por 7 segundos...");
  delay(500);
  ESP.deepSleep(7000000); // Dorme por 7 segundos, checa a condição, volta a dormir
}

void loop() {

}
