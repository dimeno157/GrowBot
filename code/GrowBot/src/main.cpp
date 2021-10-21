#include <Arduino.h>              //Biblioteca do PlateformIO que adiciona em C++ as funções do Arduino
#include <WiFi.h>                 // Biblioteca para conexão WiFi no ESP32
#include <WiFiClientSecure.h>     // Biblioteca que gera uma conexão segura na rede
#include <UniversalTelegramBot.h> // Biblioteca com as funçoes do Bot
#include "personal_info.h"

//-------------------------------------------------------------------------------------------------------------

// Numero de milisegundos em uma hora
#define ONE_HOUR 3600000

//-------------------------------------------------------------------------------------------------------------

WiFiClientSecure client;                     // Cliente para conexões seguras
UniversalTelegramBot GrowBot(TOKEN, client); // Objeto que permite a comunicação via Telegram
String lightCicle;                           // Ciclo de luz -> 'veg', 'flor', 'ger'
String menu;                                 // String com o menu
int lightPin = 27;                           // Pino da luz
int irrigationPin = 0;                       //TODO: Setar o pino certo // Pino da bomba de irrigação
int lightPeriodsInHours[2];                  // Intervalos em horas de luz ligada [0] e desligada [1]
int irrigationIntervalInHours;               // Intervalo em horas entre regas
int irrigationTimeInSeconds;                 // Tempo em segundos que a bomba de irrigação ficará ligada em cada rega
unsigned long timeLast;                      // Tempo em milissegundos des de a ultima medição
unsigned long timeNow;                       // Tempo em milissegundos medido agora
unsigned int hoursSinceLastLightChange;      // Numero de horas que se passaram des de a ultima mudança na luz
unsigned int hoursSinceLastIrrigation;       // Numero de horas que se passaram des de a ultima irrigacao
bool lightOn;                                // Indica se a luz está ligada
bool sentFirstMessage;                       // Booleana que indica se a mensagem de GrowBox Ativa foi enviada -> Indica que a placa foi reiniciada

//-------------------------------------------------------------------------------------------------------------

void handleNewMessages(int numNewMessages); // Função que lê as novas mensagens e executa o comando correspondente
void setTimeIntervals();                    // Função que seta as variaveis que marcam periodos de acordo com o cilclo atual
void checkAndIrrigate();                    // Função que realiza a irrigação da planta
void checkAndChangeLightState();            // Função que checa se o estado da luz pode ser alterado e se puder altera ele
void checkAndRaiseHours();                  // Função que checa se ja passou uma hora e acresce as variaveis de medição de tempo
void connectInNetwork();                    //Função que connecta na rede WiFi

//-------------------------------------------------------------------------------------------------------------

void setup()
{
  client.setInsecure();

  menu = "Comandos:\n";
  menu += "\nciclo - ciclo de luz atual.\n";
  menu += "\nger - Muda o ciclo atual para germinação(16/8).\n";
  menu += "\nveg - Muda o ciclo atual para vegetativo(18/6).\n";
  menu += "\nflor - Muda o ciclo atual para floração(12/12).\n";
  menu += "\nluz - Indica se a luz esta acesa ou apagada.\n";
  menu += "\nliga - Liga a luz caso esteja desligada. \n";
  menu += "\ndesliga - Desliga a luz caso esteja ligada.";

  // Seta os valores iniciais das variáveis
  lightCicle = "veg";
  timeLast = 0;
  timeNow = 0;
  hoursSinceLastLightChange = 0;
  hoursSinceLastIrrigation = 0;
  irrigationTimeInSeconds = 5;
  lightOn = true;
  sentFirstMessage = false;

  setTimeIntervals();

  // Seta o pino da luz como saida e liga ele (O relé da luz liga em low)
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, LOW);

  // Seta o pino da irrigação como saida e desliga ele
  pinMode(irrigationPin, OUTPUT);
  digitalWrite(irrigationPin, LOW);

  connectInNetwork();
}

//-------------------------------------------------------------------------------------------------------------

void loop()
{
  // Se o wifi estiver desconectado tenta se conectar, caso contrario le os comandos no bot
  if (WiFi.status() != WL_CONNECTED)
  {
    connectInNetwork();
  }
  else
  {
    int numNewMessages = GrowBot.getUpdates(GrowBot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }
  // Coloca os valores corretos nas variáveis de tempo
  setTimeIntervals();
  // Checa se ja passou uma hora pra acrescer as variaveis de medição
  checkAndRaiseHours();
  // Checa se o estado da luz pode ser mudado e muda em caso positivo
  checkAndChangeLightState();
  // Checa se ja pode irrigar e irriga em caso positivo
  checkAndIrrigate();
}

//-------------------------------------------------------------------------------------------------------------

void checkAndRaiseHours()
{
  // Checa se ja passou uma hora des de a ultima medição de tempo
  timeNow = millis();
  if (timeNow - timeLast >= ONE_HOUR)
  {
    hoursSinceLastLightChange += 1;
    hoursSinceLastIrrigation += 1;
    timeLast = timeNow;
  }
}

//-------------------------------------------------------------------------------------------------------------

void setTimeIntervals()
{
  if (lightCicle.equalsIgnoreCase("ger"))
  {
    lightPeriodsInHours[0] = 16;
    lightPeriodsInHours[1] = 8;
    irrigationIntervalInHours = 48;
  }
  else if (lightCicle.equalsIgnoreCase("flor"))
  {
    lightPeriodsInHours[0] = 12;
    lightPeriodsInHours[1] = 12;
    irrigationIntervalInHours = 48;
  }
  else
  {
    lightPeriodsInHours[0] = 18;
    lightPeriodsInHours[1] = 6;
    irrigationIntervalInHours = 48;
  }
}

//-------------------------------------------------------------------------------------------------------------

void checkAndIrrigate()
{
  if (hoursSinceLastIrrigation >= irrigationIntervalInHours)
  {
    digitalWrite(irrigationPin, HIGH);
    delay(irrigationTimeInSeconds * 1000);
    digitalWrite(irrigationPin, LOW);
    hoursSinceLastIrrigation = 0;
  }
}

//-------------------------------------------------------------------------------------------------------------

void checkAndChangeLightState()
{
  // Liga a luz caso ela esteja desligada e ja tiver dado o tempo para ligar
  if (!lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[1])
  {
    digitalWrite(lightPin, LOW);
    GrowBot.sendMessage(MY_ID, String("Luz ligada após ") + String(hoursSinceLastLightChange) + String(" horas"));
    lightOn = true;
    hoursSinceLastLightChange = 0;
  }
  // Liga a luz caso ela esteja ligada e ja tiver dado o tempo para desligar
  else if (lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[0])
  {
    digitalWrite(lightPin, HIGH);
    GrowBot.sendMessage(MY_ID, String("Luz desligada após ") + String(hoursSinceLastLightChange) + String(" horas"));
    lightOn = false;
    hoursSinceLastLightChange = 0;
  }
}

//-------------------------------------------------------------------------------------------------------------

void handleNewMessages(int numNewMessages)
{
  if (numNewMessages > 0)
  {
    for (int i = 0; i < numNewMessages; i++)
    {
      String comando = GrowBot.messages[i].text;
      if (GrowBot.messages[i].chat_id == MY_ID)
      {
        if (comando.equalsIgnoreCase("menu"))
        {
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), menu);

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("ciclo"))
        {
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), lightCicle);

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("veg") && lightCicle != "veg")
        {
          lightCicle = "veg";
          // Se mudar e tiver ultrapassado um dos tempos, reseta o contador de tempo
          if ((!lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[1]) || (lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[0]))
          {
            timeLast = timeNow;
          }
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Ciclo atual: Vegetativo(18/6)");

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("flor") && lightCicle != "flor")
        {
          lightCicle = "flor";
          // Se mudar e tiver utrapassado um dos tempos reseta o contador de tempo
          if ((!lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[1]) || (lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[0]))
          {
            timeLast = timeNow;
          }
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Ciclo atual: Floração(12/12)");

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("ger") && lightCicle != "ger")
        {
          lightCicle = "ger";
          // Se mudar e tiver utrapassado um dos tempos reseta o contador de tempo
          if ((!lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[1]) || (lightOn && hoursSinceLastLightChange >= lightPeriodsInHours[0]))
          {
            timeLast = timeNow;
          }
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Ciclo atual: Germinação(16/8)");

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("luz"))
        {
          if (lightOn)
          {
            GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Luz ligada");
          }
          else
          {
            GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Luz desligada");
          }

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("liga") && !lightOn)
        {
          digitalWrite(lightPin, LOW);
          lightOn = true;
          hoursSinceLastLightChange = 0;
          timeLast = timeNow;
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Luz ligada");

          //------------------------------
        }
        else if (comando.equalsIgnoreCase("desliga") && lightOn)
        {
          digitalWrite(lightPin, HIGH);
          lightOn = false;
          hoursSinceLastLightChange = 0;
          timeLast = timeNow;
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "Luz desligada");

          //------------------------------
        }
        else
        {
          GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), menu);
        }
      }
      else
      {
        GrowBot.sendMessage(String(GrowBot.messages[i].chat_id), "blez...");
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------------

void connectInNetwork()
{
  // Inicia em modo station (mais um dispositivo na rede, o outro modo é o Access Point)
  WiFi.mode(WIFI_STA);
  // Conecta na rede com o ssid e senha
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(10000);
  // Se ja tiver conectado
  if (WiFi.status() == WL_CONNECTED)
  {
    // Se ja tiver enviado a primeira mensagem significa que a conexão caiu
    if (sentFirstMessage)
    {
      GrowBot.sendMessage(MY_ID, "Conexão reestabelecida");
    }
    // Se não tiver enviado a primeira mensagem significa que acabou de ligar
    else
    {
      sentFirstMessage = GrowBot.sendMessage(MY_ID, "--- GrowBox ativa ---");
      GrowBot.sendMessage(MY_ID, menu);
    }
  }
}
