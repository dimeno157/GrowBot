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
String responseKeyboardMenu;                 // String do menu no teclado
String lightMenu;                            // String com o menu da luz
String irrigationMenu;                       // String com o enu da irrigação
String optionsMessage;
int lightPin = 27;                      // Pino da luz
int irrigationPin = 26;                 // Pino da bomba de irrigação
int lightPeriodsInHours[2];             // Intervalos em horas de luz ligada [0] e desligada [1]
int irrigationIntervalInHours;          // Intervalo em horas entre regas
int irrigationTimeInSeconds;            // Tempo em segundos que a bomba de irrigação ficará ligada em cada rega
unsigned long timeLast;                 // Tempo em milissegundos des de a ultima medição
unsigned long timeNow;                  // Tempo em milissegundos medido agora
unsigned int hoursSinceLastLightChange; // Numero de horas que se passaram des de a ultima mudança na luz
unsigned int hoursSinceLastIrrigation;  // Numero de horas que se passaram des de a ultima irrigacao
bool lightOn;                           // Indica se a luz está ligada
bool sentFirstMessage;                  // Booleana que indica se a mensagem de GrowBox Ativa foi enviada -> Indica que a placa foi reiniciada
bool irrigationMessageSent;             // Flag que indica se o lembrete de irrigação ja foi enviado
bool autoIrrigate;                      // Flaq que indica se a irrigação automatica está ligada ou desligada

//-------------------------------------------------------------------------------------------------------------

// Lê as novas mensagens e executa o comando correspondente.
void handleNewMessages(int numNewMessages);

// Seta as variaveis dos períodos de tempo (luz, irrigação, etc) de acordo com o cilclo atual.
void setTimeIntervals();

// Realiza a irrigação.
void checkAndIrrigate();

// Checa e altera (caso seja necessário) o estado da luz.
void checkAndChangeLightState();

// Checa se ja passou uma hora e acresce as variaveis de medição de tempo.
void checkAndRaiseHours();

// Connecta na rede WiFi.
void connectInNetwork();

// Envia o menu da luz.
void showLightOptions(String chat_id, bool sendStatus = true);

// Envia o menu da irrigação.
void showIrrigationOptions(String chat_id, bool lastIrrigationInfo = true, bool nextIrrigationInfo = true);

// Muda o estado da luz.
void changeLightState(bool turnOff);

// Muda o ciclo.
void changeLightCicle(String chat_id, String cicle);

// Realiza uma irrigação.
void irrigate(String chat_id);

// Liga ou desliga a irrigação automatica
void changeAutoIrrigationState(String chat_id, bool turnOff);

// Registra a irrigação.
void registerIrrigation(String chat_id);

//-------------------------------------------------------------------------------------------------------------

void setup()
{
  client.setInsecure();

  menu = "Comandos:\n";
  menu += "\n /menu - Menu inicial.\n";
  menu += "\n /luz - Abre o menu da luz.\n";
  menu += "\n /ciclo - Ciclo de luz atual.\n";
  menu += "\n /ger - Muda o ciclo atual para germinação(16/8).\n";
  menu += "\n /veg - Muda o ciclo atual para vegetativo(18/6).\n";
  menu += "\n /flor - Muda o ciclo atual para floração(12/12).\n";
  menu += "\n /irrigacao - Abre o menu da irrigação.\n";
  menu += "\n /irrigar - Realiza uma irrigação.\n";
  menu += "\n /irrigado - Registra o momento da irrigação para enviar o lembrete no intervalo correto.\n";
  menu += "\n /coolers - Abre o menu dos coolers.\n";
  menu += "\n /ligaLuz - Liga a luz. \n";
  menu += "\n /desligaLuz - Desliga.\n";
  menu += "\n /ligaAutoIrrigacao - Liga a irrigação automática.\n";
  menu += "\n /desligaAutoIrrigacao - Desiga a irrigação automática.\n";

  optionsMessage = "Escolha uma das opções do /menu ou envie /comandos para mostrar todos os comandos.";
  responseKeyboardMenu = "[[\"/luz\"],[\"/irrigacao\"],[\"/coolers\"],[\"/comandos\"]]";
  lightCicle = "veg";
  timeLast = 0;
  timeNow = 0;
  hoursSinceLastLightChange = 0;
  hoursSinceLastIrrigation = 0;
  irrigationTimeInSeconds = 15;
  lightOn = true;
  sentFirstMessage = false;
  irrigationMessageSent = false;
  autoIrrigate = false;

  setTimeIntervals();

  // Seta o pino da luz como saida e liga (O relé da luz liga em low)
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, LOW);

  // Seta o pino da irrigação como saida e desliga
  pinMode(irrigationPin, OUTPUT);
  digitalWrite(irrigationPin, LOW);

  connectInNetwork();
}

//-----------------------

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

  setTimeIntervals();
  checkAndRaiseHours();
  checkAndChangeLightState();
  checkAndIrrigate();
}

//-------------------------------------------------------------------------------------------------------------

void checkAndRaiseHours()
{
  timeNow = millis();
  if (timeLast > timeNow)
  {
    timeLast = timeNow;
  }
  if (timeNow - timeLast >= ONE_HOUR)
  {
    hoursSinceLastLightChange += 1;
    hoursSinceLastIrrigation += 1;
    timeLast = timeNow;
  }
  return;
}

//-----------------------

void setTimeIntervals()
{
  if (lightCicle.equalsIgnoreCase("ger"))
  {
    lightPeriodsInHours[0] = 16;
    lightPeriodsInHours[1] = 8;
    irrigationIntervalInHours = 5 * 24;
  }
  else if (lightCicle.equalsIgnoreCase("flor"))
  {
    lightPeriodsInHours[0] = 12;
    lightPeriodsInHours[1] = 12;
    irrigationIntervalInHours = 5 * 24;
  }
  else
  {
    lightPeriodsInHours[0] = 18;
    lightPeriodsInHours[1] = 6;
    irrigationIntervalInHours = 5 * 24;
  }
  return;
}

//-----------------------

void checkAndIrrigate()
{
  if (hoursSinceLastIrrigation >= irrigationIntervalInHours)
  {
    if (autoIrrigate)
    {
      irrigate(MY_ID);
    }
    else if (!irrigationMessageSent)
    {
      showIrrigationOptions(MY_ID, true, false);
      irrigationMessageSent = true;
    }
  }
  return;
}

//-----------------------

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
  return;
}

//-----------------------

void handleNewMessages(int numNewMessages)
{
  if (numNewMessages > 0)
  {
    for (int i = 0; i < numNewMessages; i++)
    {
      String comando = GrowBot.messages[i].text;
      if (GrowBot.messages[i].chat_id == MY_ID)
      {
        if (comando.equalsIgnoreCase("/comandos"))
        {
          GrowBot.sendMessage(GrowBot.messages[i].chat_id, menu);
        }
        else if (comando.equalsIgnoreCase("/menu"))
        {
          GrowBot.sendMessageWithReplyKeyboard(GrowBot.messages[i].chat_id, optionsMessage, "", responseKeyboardMenu, true, true);
        }
        else if (comando.equalsIgnoreCase("/ciclo"))
        {
          GrowBot.sendMessage(GrowBot.messages[i].chat_id, lightCicle);
        }
        else if (comando.equalsIgnoreCase("/irrigacao"))
        {
          showIrrigationOptions(GrowBot.messages[i].chat_id, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase("/irrigar"))
        {
          irrigate(GrowBot.messages[i].chat_id);
          showIrrigationOptions(GrowBot.messages[i].chat_id, false);
        }
        else if (comando.equalsIgnoreCase("/irrigado"))
        {
          registerIrrigation(GrowBot.messages[i].chat_id);
          showIrrigationOptions(GrowBot.messages[i].chat_id, false);
        }
        else if (comando.equalsIgnoreCase("/ligaAutoIrrigacao"))
        {
          changeAutoIrrigationState(GrowBot.messages[i].chat_id, false);
          showIrrigationOptions(GrowBot.messages[i].chat_id, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase("/desligaAutoIrrigacao"))
        {
          changeAutoIrrigationState(GrowBot.messages[i].chat_id, true);
          showIrrigationOptions(GrowBot.messages[i].chat_id, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase("/veg") && lightCicle != "veg")
        {
          changeLightCicle(GrowBot.messages[i].chat_id, "veg");
        }
        else if (comando.equalsIgnoreCase("/flor") && lightCicle != "flor")
        {
          changeLightCicle(GrowBot.messages[i].chat_id, "flor");
        }
        else if (comando.equalsIgnoreCase("/ger") && lightCicle != "ger")
        {
          changeLightCicle(GrowBot.messages[i].chat_id, "ger");
        }
        else if (comando.equalsIgnoreCase("/luz"))
        {
          showLightOptions(GrowBot.messages[i].chat_id);
        }
        else if (comando.equalsIgnoreCase("/ligaLuz") && !lightOn)
        {
          changeLightState(false);
          showLightOptions(GrowBot.messages[i].chat_id, false);
        }
        else if (comando.equalsIgnoreCase("/desligaLuz") && lightOn)
        {
          changeLightState(true);
          showLightOptions(GrowBot.messages[i].chat_id, false);
        }
        else
        {
          GrowBot.sendMessageWithReplyKeyboard(GrowBot.messages[i].chat_id, "Escolha uma das opções", "", "[[\"/menu\"],[\"/comandos\"]]", true, false, true);
        }
      }
      else
      {
        GrowBot.sendMessage(GrowBot.messages[i].chat_id, "blez...");
      }
    }
  }
  return;
}

//-----------------------

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
      GrowBot.sendMessageWithReplyKeyboard(MY_ID, "Escolha uma das opções", "", responseKeyboardMenu, true, true, true);
    }
  }
  return;
}

//-----------------------

void showLightOptions(String chat_id, bool sendStatus)
{
  if (lightOn)
  {
    if (sendStatus)
    {
      GrowBot.sendMessage(chat_id, "Luz ligada ha " + String(hoursSinceLastLightChange) + " horas\nRestam " + String(lightPeriodsInHours[0] - hoursSinceLastLightChange) + " para desligar");
    }
    lightMenu = "[[\"/desligaLuz\"],[\"/ciclo\"],[\"/ger\",\"/veg\",\"/flor\"],[\"/menu\"]]";
  }
  else
  {
    if (sendStatus)
    {
      GrowBot.sendMessage(chat_id, "Luz desligada ha " + String(hoursSinceLastLightChange) + " horas\nRestam " + String(lightPeriodsInHours[1] - hoursSinceLastLightChange) + " para ligar");
    }
    lightMenu = "[[\"/ligaLuz\"],[\"/ciclo\"],[\"/ger\",\"/veg\",\"/flor\"],[\"/menu\"]]";
  }
  GrowBot.sendMessageWithReplyKeyboard(chat_id, optionsMessage, "", lightMenu, true, true, true);
  return;
}

//-----------------------

void showIrrigationOptions(String chat_id, bool lastIrrigationInfo, bool nextIrrigationInfo)
{
  if (lastIrrigationInfo)
  {
    GrowBot.sendMessage(chat_id, "Ultima irrigação realizada ha " + String(int(hoursSinceLastIrrigation / 24)) + " dias e " + String(int(hoursSinceLastIrrigation % 24)) + " horas.");
  }
  if (nextIrrigationInfo)
  {
    GrowBot.sendMessage(chat_id, String(int((irrigationIntervalInHours - hoursSinceLastIrrigation) / 24)) + " dias e " + String(int((irrigationIntervalInHours - hoursSinceLastIrrigation) % 24)) + " horas restantes até a próxima irrigação.");
  }
  if (autoIrrigate)
  {
    irrigationMenu = "[[\"/irrigado\"],[\"/irrigar\"],[\"/desligaAutoIrrigacao\"],[\"/menu\"]]";
  }
  else
  {
    irrigationMenu = "[[\"/irrigado\"],[\"/irrigar\"],[\"/ligaAutoIrrigacao\"],[\"/menu\"]]";
  }
  GrowBot.sendMessageWithReplyKeyboard(chat_id, optionsMessage, "", irrigationMenu, true, true, true);
  return;
}

//-----------------------

void changeLightState(bool turnOff)
{
  if (turnOff)
  {
    digitalWrite(lightPin, HIGH);
    lightOn = false;
    GrowBot.sendMessage(String(MY_ID), "Luz desligada.");
  }
  else
  {
    digitalWrite(lightPin, LOW);
    lightOn = true;
    GrowBot.sendMessage(MY_ID, "Luz ligada.");
  }
  hoursSinceLastLightChange = 0;
  return;
}

//-----------------------

void changeAutoIrrigationState(String chat_id, bool turnOff)
{
  if (turnOff)
  {
    autoIrrigate = false;
    GrowBot.sendMessage(chat_id, "Irrigação automatica desligada.");
  }
  else
  {
    autoIrrigate = true;
    GrowBot.sendMessage(chat_id, "Irrigação automatica ligada.");
  }
  return;
}

//-----------------------

void changeLightCicle(String chat_id, String cicle)
{
  if (cicle == "veg")
  {
    lightCicle = "veg";
    GrowBot.sendMessage(chat_id, "Ciclo atual: Vegetativo(18/6)");
  }
  else if (cicle == "ger")
  {
    lightCicle = "ger";
    GrowBot.sendMessage(chat_id, "Ciclo atual: Germinação(16/8)");
  }
  else if (cicle == "flor")
  {
    lightCicle = "flor";
    GrowBot.sendMessage(chat_id, "Ciclo atual: Floração(12/12)");
  }
  setTimeIntervals();
  return;
}

//-----------------------

void irrigate(String chat_id)
{
  digitalWrite(irrigationPin, HIGH);
  delay(irrigationTimeInSeconds * 1000);
  digitalWrite(irrigationPin, LOW);
  hoursSinceLastIrrigation = 0;
  irrigationMessageSent = false;
  GrowBot.sendMessage(chat_id, "Irrigação relaizada.");
  return;
}

//-----------------------

void registerIrrigation(String chat_id)
{
  hoursSinceLastIrrigation = 0;
  irrigationMessageSent = false;
  GrowBot.sendMessage(chat_id, "Irrigação registrada.");
  return;
}

// -----------------------