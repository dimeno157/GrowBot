// PlatformIO library that adds the Arduino library in C++
#include <Arduino.h>

// Library to connect the ESP32 in the WIFi network
#include <WiFi.h>

// Library to generate a secure network connection
#include <WiFiClientSecure.h>

// Library for the Telegram Bot
#include <UniversalTelegramBot.h>

// Library to access the ESP32 EEPROM memory
#include <EEPROM.h>

// File with the personal info - Intructions to crete in https://github.com/dimeno157/GrowBot
#include "personal_info.h"

//-------------------------------------------------------------------------------------------------------------

// Number of milliseconds in one hour
#define ONE_HOUR 3600000

// VARIABLES --------------------------------------------------------------------------------------------------

// Commands strings - add any new command here
struct Commands
{
  String menu = "/menu";
  String light = "/luz";
  String lightOn = "/ligaluz";
  String lightOff = "/desligaLuz";
  String lightCycle = "/ciclo";
  String ger = "/ger";
  String veg = "/veg";
  String flor = "/flor";
  String irrigation = "/irrigacao";
  String irrigate = "/irrigar";
  String irrigated = "/irrigado";
  String autoIrrigationOn = "/ligaautoirrigacao";
  String autoIrrigationOff = "/desligaautoirrigacao";
  String irrigationInterval = "/intervaloirrigacao";
  String coolers = "/coolers";
} commands;

// Client for secure WiFi connections
WiFiClientSecure client;

// Object for connecting in the Telegram Bot
UniversalTelegramBot GrowBot(TOKEN, client);

// light cycle -> 'veg', 'flor', 'ger'
String lightCycle;

// Main menu string
String responseKeyboardMenu;

// Light menu string
String lightMenu;

// Irrigation menu string
String irrigationMenu;

// Light pin
int lightPin = 27;

// Irrigation pump pin
int irrigationPin = 26;

// EEPROM address for the irrigation interval value
int irrigationIntervalAddress = 0;

// EEPROM address for the irrigation interval validity flag
int irrigationIntervalFlagAddress = 1;

// Intervals in hours for light on[0] and light off[1]
int lightPeriodsInHours[2];

// Interval between irrigations in days
int irrigationIntervalInDays;

// Time in seconds for the irrigation pump to be on during one irrigation
int irrigationTimeInSeconds;

// Time in milliseconds of the last time measure
unsigned long timeLast;

// Time in milliseconds measured now
unsigned long timeNow;

// Hours since last light change
unsigned int hoursSinceLastLightChange;

// Hours since last irrigation
unsigned int hoursSinceLastIrrigation;

// Indicates that the light is on
bool lightOn;

// Indicates that the GrowBox init message was already sent -> If ESP32 restars it will be false
bool sentFirstMessage;

// Indicates that the irrigation reminder message was already sent
bool irrigationMessageSent;

// Indicates that the auto irrigation is on
bool autoIrrigate;

// FUNCTIONS ----------------------------------------------------------------------------------------------------

// Lê as novas mensagens e executa o comando correspondente.
void handleNewMessages(int numNewMessages);
// Seta as variaveis dos períodos de tempo (luz, irrigação, etc) de acordo com o cilclo atual.
void setLightIntervals();
// Realiza a irrigação (auto-irrigação ativada) ou envia uma mensagem lembrando da irrigação (auto-irrigação desativada).
void checkAndIrrigate();
// Checa e altera (caso seja necessário) o estado da luz.
void checkAndChangeLightState();
// Checa se ja passou uma hora e acresce as variaveis de medição de tempo.
void checkAndRaiseHours();
// Conecta na rede WiFi.
void connectInNetwork();
// Envia o menu da luz.
void showLightOptions(String chatId, bool sendStatus = true);
// Envia o menu da irrigação.
void showIrrigationOptions(String chatId, bool lastIrrigationInfo = true, bool nextIrrigationInfo = true);
// Muda o estado da luz.
void changeLightState(bool turnOff);
// Muda o ciclo.
void changelightCycle(String chatId, String cycle);
// Realiza uma irrigação.
void irrigate(String chatId);
// Liga ou desliga a irrigação automatica
void changeAutoIrrigationState(String chatId, bool activate);
// Registra a irrigação.
void registerIrrigation(String chatId);
int getIrrigationInterval();
void setIrrigationInterval(int interval);
void initIrrigationInterval();
void initIrrigationInterval();
void writeEEPROM(int address, uint8_t val);
void updateIrrigationInterval(String message, String chatId);
int getIrrigationIntervalFromMessage(String message, String chatId);

//-------------------------------------------------------------------------------------------------------------

/*
ENVIAR PARA O @BotFather o comando /setcommands,
escolher o bot caso haja mais de um e enviar a seguinte mensagem:

menu - Menu inicial.
luz - Menu da luz.
ligaluz - Liga a luz.
desligaluz - Desliga a luz.
ciclo - Ciclo de luz atual.
ger - Muda para germinação(16/8).
veg - Muda para vegetativo(18/6).
flor - Muda para floração(12/12).
irrigacao - Menu da irrigação.
irrigar - Realiza uma irrigação.
irrigado - Registra o momento da irrigação.
ligaautoirrigacao - Liga a irrigação automática.
desligaautoirrigacao - Desiga a irrigação automática.
intervaloirrigacao - Muda o intervalo entre irrigações.
coolers - Menu dos coolers.

para criar o menu (que fica no canto superior esquerdo do teclado) do bot
Modifique de acordo com os seus comandos.
Os comandos não podem conter letras maiúsculas.
*/

void setup()
{
  client.setInsecure();
  EEPROM.begin(512);

  responseKeyboardMenu = "[[\"" + String(commands.light) + "\"],[\"" + String(commands.irrigation) + "\"],[\"" + String(commands.coolers) + "\"]]";
  lightCycle = "veg";
  timeLast = 0;
  timeNow = 0;
  hoursSinceLastLightChange = 0;
  hoursSinceLastIrrigation = 0;
  irrigationTimeInSeconds = 15;
  irrigationIntervalInDays = 5;
  lightOn = true;
  sentFirstMessage = false;
  irrigationMessageSent = false;
  autoIrrigate = false;

  setLightIntervals();
  initIrrigationInterval();

  // Seta o pino da luz como saida e liga (O relé da luz liga em LOW)
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
  // caso não a placa não esteja conectada a rede Wifi
  if (WiFi.status() != WL_CONNECTED)
  {
    connectInNetwork();
  }
  // caso a placa esteja conectada a rede WIfi
  else
  {
    // pega o numero de novas mensagens des de a ultima checagem
    int numNewMessages = GrowBot.getUpdates(GrowBot.last_message_received + 1);

    handleNewMessages(numNewMessages);
  }

  setLightIntervals();

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

void setLightIntervals()
{
  if (lightCycle.equalsIgnoreCase("ger"))
  {
    lightPeriodsInHours[0] = 16;
    lightPeriodsInHours[1] = 8;
  }
  else if (lightCycle.equalsIgnoreCase("flor"))
  {
    lightPeriodsInHours[0] = 12;
    lightPeriodsInHours[1] = 12;
  }
  else
  {
    lightPeriodsInHours[0] = 18;
    lightPeriodsInHours[1] = 6;
  }
  return;
}

//-----------------------

void checkAndIrrigate()
{
  if (hoursSinceLastIrrigation >= irrigationIntervalInDays * 24)
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
      String chatId = GrowBot.messages[i].chat_id;

      if (chatId == MY_ID)
      {
        if (comando.equalsIgnoreCase(commands.menu))
        {
          GrowBot.sendMessageWithReplyKeyboard(chatId, "Escolha uma das opções", "", responseKeyboardMenu, true);
        }
        else if (comando.equalsIgnoreCase(commands.lightCycle))
        {
          GrowBot.sendMessage(chatId, lightCycle);
        }
        else if (comando.equalsIgnoreCase(commands.irrigation))
        {
          showIrrigationOptions(chatId, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase(commands.irrigate))
        {
          irrigate(chatId);
          showIrrigationOptions(chatId, false);
        }
        else if (comando.equalsIgnoreCase(commands.irrigated))
        {
          registerIrrigation(chatId);
          showIrrigationOptions(chatId, false);
        }
        else if (comando.indexOf(commands.irrigationInterval) >= 0)
        {
          updateIrrigationInterval(comando, chatId);
        }
        else if (comando.equalsIgnoreCase(commands.autoIrrigationOn) && !autoIrrigate)
        {
          changeAutoIrrigationState(chatId, true);
          showIrrigationOptions(chatId, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase(commands.autoIrrigationOff) && autoIrrigate)
        {
          changeAutoIrrigationState(chatId, false);
          showIrrigationOptions(chatId, true, autoIrrigate);
        }
        else if (comando.equalsIgnoreCase(commands.veg) && lightCycle != "veg")
        {
          changelightCycle(chatId, "veg");
          showLightOptions(chatId);
        }
        else if (comando.equalsIgnoreCase(commands.flor) && lightCycle != "flor")
        {
          changelightCycle(chatId, "flor");
          showLightOptions(chatId);
        }
        else if (comando.equalsIgnoreCase(commands.ger) && lightCycle != "ger")
        {
          changelightCycle(chatId, "ger");
          showLightOptions(chatId);
        }
        else if (comando.equalsIgnoreCase(commands.light))
        {
          showLightOptions(chatId);
        }
        else if (comando.equalsIgnoreCase(commands.lightOn) && !lightOn)
        {
          changeLightState(false);
          showLightOptions(chatId, false);
        }
        else if (comando.equalsIgnoreCase(commands.lightOff) && lightOn)
        {
          changeLightState(true);
          showLightOptions(chatId, false);
        }
        else if (comando.equalsIgnoreCase(commands.coolers))
        {
          // TODO: Fazer lógica dos coolers.
        }
      }
      else
      {
        GrowBot.sendMessage(chatId, "blez...");
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
      GrowBot.sendMessage(MY_ID, "--- Conexão reestabelecida ---");
    }
    // Se não tiver enviado a primeira mensagem significa que acabou de ligar
    else
    {
      sentFirstMessage = GrowBot.sendMessage(MY_ID, "--- GrowBox ativa ---");
      GrowBot.sendMessageWithReplyKeyboard(MY_ID, "Escolha uma das opções", "", responseKeyboardMenu, true);
    }
  }
  return;
}

//-----------------------

void showLightOptions(String chatId, bool sendStatus)
{
  if (lightOn)
  {
    if (sendStatus)
    {
      GrowBot.sendMessage(chatId, "Luz ligada ha " + String(hoursSinceLastLightChange) + " horas\nRestam " + String(lightPeriodsInHours[0] - hoursSinceLastLightChange) + " para desligar");
    }
    lightMenu = "[[\"" + String(commands.lightOff) + "\"],[\"" + String(commands.lightCycle) + "\"],[\"" + String(commands.ger) + "\",\"" + String(commands.veg) + "\",\"" + String(commands.flor) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  else
  {
    if (sendStatus)
    {
      GrowBot.sendMessage(chatId, "Luz desligada ha " + String(hoursSinceLastLightChange) + " horas\nRestam " + String(lightPeriodsInHours[1] - hoursSinceLastLightChange) + " para ligar");
    }
    lightMenu = "[[\"" + String(commands.lightOn) + "\"],[\"" + String(commands.lightCycle) + "\"],[\"" + String(commands.ger) + "\",\"" + String(commands.veg) + "\",\"" + String(commands.flor) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  GrowBot.sendMessageWithReplyKeyboard(chatId, "Escolha uma das opções", "", lightMenu, true);
  return;
}

//-----------------------

void showIrrigationOptions(String chatId, bool lastIrrigationInfo, bool nextIrrigationInfo)
{
  if (lastIrrigationInfo)
  {
    GrowBot.sendMessage(chatId, "Ultima irrigação realizada ha " + String(int(hoursSinceLastIrrigation / 24)) + " dias e " + String(int(hoursSinceLastIrrigation % 24)) + " horas.");
  }
  if (nextIrrigationInfo)
  {
    GrowBot.sendMessage(chatId, String(int(((irrigationIntervalInDays * 24) - hoursSinceLastIrrigation) / 24)) + " dias e " + String(int(((irrigationIntervalInDays * 24) - hoursSinceLastIrrigation) % 24)) + " horas restantes até a próxima irrigação.");
  }
  if (autoIrrigate)
  {
    irrigationMenu = "[[\"" + String(commands.irrigated) + "\"],[\"" + String(commands.irrigate) + "\"],[\"" + String(commands.irrigationInterval) + "\"],[\"" + String(commands.autoIrrigationOff) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  else
  {
    irrigationMenu = "[[\"" + String(commands.irrigated) + "\"],[\"" + String(commands.irrigate) + "\"],[\"" + String(commands.irrigationInterval) + "\"],[\"" + String(commands.autoIrrigationOn) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  GrowBot.sendMessageWithReplyKeyboard(chatId, "Escolha uma das opções", "", irrigationMenu, true);
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

void changeAutoIrrigationState(String chatId, bool activate)
{
  if (activate)
  {
    autoIrrigate = true;
    GrowBot.sendMessage(chatId, "Irrigação automática ligada.");
  }
  else
  {
    autoIrrigate = false;
    GrowBot.sendMessage(chatId, "Irrigação automática desligada.");
  }
  return;
}

//-----------------------

void changelightCycle(String chatId, String cycle)
{
  if (cycle == "veg")
  {
    lightCycle = "veg";
    GrowBot.sendMessage(chatId, "Ciclo atual: Vegetativo(18/6)");
  }
  else if (cycle == "ger")
  {
    lightCycle = "ger";
    GrowBot.sendMessage(chatId, "Ciclo atual: Germinação(16/8)");
  }
  else if (cycle == "flor")
  {
    lightCycle = "flor";
    GrowBot.sendMessage(chatId, "Ciclo atual: Floração(12/12)");
  }
  setLightIntervals();
  return;
}

//-----------------------

void irrigate(String chatId)
{
  digitalWrite(irrigationPin, HIGH);
  delay(irrigationTimeInSeconds * 1000);
  digitalWrite(irrigationPin, LOW);
  hoursSinceLastIrrigation = 0;
  irrigationMessageSent = false;
  GrowBot.sendMessage(chatId, "Irrigação realizada.");
  return;
}

//-----------------------

void registerIrrigation(String chatId)
{
  hoursSinceLastIrrigation = 0;
  irrigationMessageSent = false;
  GrowBot.sendMessage(chatId, "Irrigação registrada.");
  return;
}

// -----------------------

int getIrrigationInterval()
{
  if (EEPROM.read(irrigationIntervalFlagAddress) == 1 && EEPROM.read(irrigationIntervalAddress) > 0)
  {
    return EEPROM.read(irrigationIntervalAddress);
  }
  return irrigationIntervalInDays;
}

//-----------------------

void setIrrigationInterval(int interval)
{
  irrigationIntervalInDays = interval;
  writeEEPROM(irrigationIntervalAddress, interval);
  writeEEPROM(irrigationIntervalFlagAddress, 1);
}

//-----------------------

void initIrrigationInterval()
{
  if (EEPROM.read(irrigationIntervalFlagAddress) != 1)
  {
    setIrrigationInterval(irrigationIntervalInDays);
    return;
  }
  irrigationIntervalInDays = getIrrigationInterval();
  return;
}

//-----------------------

void writeEEPROM(int address, uint8_t val)
{
  EEPROM.write(address, val);
  EEPROM.commit();
}

//-----------------------

void updateIrrigationInterval(String message, String chatId)
{
  int interval = getIrrigationIntervalFromMessage(message, chatId);
  if (interval == 0)
  {
    GrowBot.sendMessage(chatId, "Para modificar o intervalo de irrigação mande a mensagem da forma:\n\n" + String(commands.irrigationInterval) + " N\n\nN é o intervalo de irrigação em dias e deve ser maior que zero.");
    return;
  }
  int oldInterval = irrigationIntervalInDays;
  setIrrigationInterval(interval);
  GrowBot.sendMessage(chatId, "Intervalo de irrigação alterado de " + String(oldInterval) + " dias para " + String(interval) + " dias.");
  return;
}

//-----------------------

int getIrrigationIntervalFromMessage(String message, String chatId)
{
  String command = String(commands.irrigationInterval) + " ";
  if (message.length() <= command.length())
  {
    return 0;
  }

  int index = message.indexOf(command);
  if (index < 0)
  {
    return 0;
  }

  int interval = message.substring(command.length()).toInt();
  if (interval <= 0)
  {
    return 0;
  }

  return interval;
}