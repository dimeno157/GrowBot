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

// TODO: Add EEPROM storage for all important variables

//-------------------------------------------------------------------------------------------------------------

// Number of milliseconds in one hour
#define ONE_HOUR 3600000

// VARIABLES --------------------------------------------------------------------------------------------------

// Commands strings - add any new command here
struct Commands
{
  /* ENVIAR PARA O @BotFather o comando /setcommands,
  escolher o bot caso haja mais de um e enviar a seguinte mensagem:

  menu - Menu inicial.
  status - Valores do GrowBox.
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

  String menu = "/menu";
  String status = "/status";
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
  String irrigationTime = "/tempoirrigacao";
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

// 0: LED Light ON
// 1: FS Light ON
// 2: LED Light ON
// 3: Lights OFF
int currentLightStep;

// LED light pin
int lightPinLED = 27;

// Full Spectrum light pin
int lightPinFS = 28;

// Irrigation pump pin
int irrigationPin = 26;

// EEPROM address for the irrigation interval value
int irrigationIntervalAddress = 0;

// EEPROM address for the irrigation interval validity flag
int irrigationIntervalFlagAddress = 1;

// EEPROM address for the auto-irrigation flag
int autoIrrigationAddress = 2;

// EEPROM address for the irrigation time
int irrigationTimeAddress = 3;

// EEPROM address for the irrigation time validity flag
int irrigationTimeFlagAddress = 4;

// Light periods in hours:
// [0] -> LED ON (first time)
// [1] -> FS ON
// [2] -> LED ON (second time)
// [3] -> Light OFF
int lightPeriodsInHours[4];

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

// Seta as variaveis dos períodos de tempo (luz, irrigação, etc) de acordo com o ciclo atual.
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

// Get the irrigation interval current value
int getIrrigationInterval();

// Set the irrigation interval value and save in EEPROM
void setIrrigationInterval(int interval);

// Load the irrigation variables values
void initIrrigationData();

// Write data in an EEPROM address
void writeEEPROM(int address, uint8_t val);

// Update the irrigation interval from a given message
void updateIrrigationInterval(String message, String chatId);

// Update the irrigation time form a given message
void updateIrrigationTime(String message, String chatId);

// Get the irrigation time current value
int getIrrigationTime();

// Set the irrigation time value and save in EEPROM
void setIrrigationTime(int time);

// Get a value from a message:
//
// message = "/command N"
//
// With N being the returned value.
int getValueFromMessage(String command, String message);

// Send the grow status message
void sendStatusInfo(String chatId);

// Get a string with the light cycle complete name
String getLightCycleName(String cycle, bool withTimes = true);

void setLightStep(int step);

//-------------------------------------------------------------------------------------------------------------

void setup()
{
  client.setInsecure();
  EEPROM.begin(512);

  responseKeyboardMenu = "[[\"" + String(commands.light) + "\"],[\"" + String(commands.irrigation) + "\"],[\"" + String(commands.coolers) + "\"],[\"" + String(commands.status) + "\"]]";
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
  currentLightStep = 0;

  setLightIntervals();
  initIrrigationData();

  // Seta o pino da luz como saída e liga (O relé da luz liga em LOW)
  pinMode(lightPinLED, OUTPUT);
  digitalWrite(lightPinLED, LOW);

  // Seta o pino da irrigação como saída e desliga
  pinMode(irrigationPin, OUTPUT);
  digitalWrite(irrigationPin, LOW);

  connectInNetwork();
}

//-----------------------

void loop()
{
  // caso não a placa não esteja conectada a rede WiFi
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
  int timeOn = 18;
  if (lightCycle.equalsIgnoreCase("ger"))
  {
    timeOn = 16;
  }
  else if (lightCycle.equalsIgnoreCase("flor"))
  {
    timeOn = 12;
  }
  lightPeriodsInHours[0] = int(timeOn / 3);
  lightPeriodsInHours[1] = timeOn - (2 * lightPeriodsInHours[0]);
  lightPeriodsInHours[2] = lightPeriodsInHours[0];
  lightPeriodsInHours[3] = 24 - timeOn;
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
  // if the current light step period end is reached
  if (hoursSinceLastLightChange >= lightPeriodsInHours[currentLightStep])
  {
    // go to the net light step (0 -> 1 -> 2 -> 3 -> 0)
    setLightStep((currentLightStep + 1) % 4);
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
        else if (comando.equalsIgnoreCase(commands.status))
        {
          sendStatusInfo(chatId);
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
        else if (comando.indexOf(commands.irrigationTime) >= 0)
        {
          updateIrrigationTime(comando, chatId);
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
    irrigationMenu = "[[\"" + String(commands.irrigated) + "\"],[\"" + String(commands.irrigate) + "\"],[\"" + String(commands.irrigationInterval) + "\"],[\"" + String(commands.irrigationTime) + "\"],[\"" + String(commands.autoIrrigationOff) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  else
  {
    irrigationMenu = "[[\"" + String(commands.irrigated) + "\"],[\"" + String(commands.irrigate) + "\"],[\"" + String(commands.irrigationInterval) + "\"],[\"" + String(commands.irrigationTime) + "\"],[\"" + String(commands.autoIrrigationOn) + "\"],[\"" + String(commands.menu) + "\"]]";
  }
  GrowBot.sendMessageWithReplyKeyboard(chatId, "Escolha uma das opções", "", irrigationMenu, true);
  return;
}

//-----------------------

void changeLightState(bool turnOff)
{
  if (turnOff)
  {
    setLightStep(3);
  }
  else
  {
    setLightStep(0);
  }
  hoursSinceLastLightChange = 0;
  return;
}

//-----------------------

void setLightStep(int step)
{
  currentLightStep = step;
  switch (currentLightStep)
  {
  case 0:
    digitalWrite(lightPinLED, LOW);
    digitalWrite(lightPinFS, HIGH);
    GrowBot.sendMessage(MY_ID, String("Luz ligada após ") + String(hoursSinceLastLightChange) + String(" horas"));
    lightOn = true;
    break;
  case 1:
    digitalWrite(lightPinLED, HIGH);
    digitalWrite(lightPinFS, LOW);
    break;
  case 2:
    digitalWrite(lightPinLED, LOW);
    digitalWrite(lightPinFS, HIGH);
    break;
  case 3:
    digitalWrite(lightPinLED, HIGH);
    digitalWrite(lightPinFS, HIGH);
    GrowBot.sendMessage(MY_ID, String("Luz desligada após ") + String(hoursSinceLastLightChange) + String(" horas"));
    lightOn = false;
    break;
  default:
    break;
  }
}

void changeAutoIrrigationState(String chatId, bool activate)
{
  if (activate)
  {
    autoIrrigate = true;
    writeEEPROM(autoIrrigationAddress, 1);
    GrowBot.sendMessage(chatId, "Irrigação automática ligada.");
  }
  else
  {
    autoIrrigate = false;
    writeEEPROM(autoIrrigationAddress, 0);
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
    GrowBot.sendMessage(chatId, "Ciclo atual: " + getLightCycleName(cycle));
  }
  else if (cycle == "ger")
  {
    lightCycle = "ger";
    GrowBot.sendMessage(chatId, "Ciclo atual: " + getLightCycleName(cycle));
  }
  else if (cycle == "flor")
  {
    lightCycle = "flor";
    GrowBot.sendMessage(chatId, "Ciclo atual: " + getLightCycleName(cycle));
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

// -----------------------

int getIrrigationTime()
{
  if (EEPROM.read(irrigationTimeFlagAddress) == 1 && EEPROM.read(irrigationTimeAddress) > 0)
  {
    return EEPROM.read(irrigationTimeAddress);
  }
  return irrigationTimeInSeconds;
}

//-----------------------

void setIrrigationInterval(int interval)
{
  irrigationIntervalInDays = interval;
  writeEEPROM(irrigationIntervalAddress, interval);
  writeEEPROM(irrigationIntervalFlagAddress, 1);
}

// -----------------------

void setIrrigationTime(int time)
{
  irrigationTimeInSeconds = time;
  writeEEPROM(irrigationTimeAddress, time);
  writeEEPROM(irrigationTimeFlagAddress, 1);
}

//-----------------------

void initIrrigationData()
{
  // init irrigation interval
  if (EEPROM.read(irrigationIntervalFlagAddress) != 1)
  {
    setIrrigationInterval(irrigationIntervalInDays);
  }
  irrigationIntervalInDays = getIrrigationInterval();

  // init irrigation time
  if (EEPROM.read(irrigationTimeFlagAddress) != 1)
  {
    setIrrigationTime(irrigationTimeInSeconds);
  }
  irrigationTimeInSeconds = getIrrigationTime();

  // init auto irrigation
  if (EEPROM.read(autoIrrigationAddress) == 1)
  {
    autoIrrigate = true;
  }
  else
  {
    autoIrrigate = false;
  }

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
  int interval = getValueFromMessage(commands.irrigationInterval, message);
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

// -----------------------

void updateIrrigationTime(String message, String chatId)
{
  int time = getValueFromMessage(commands.irrigationTime, message);
  if (time == 0)
  {
    GrowBot.sendMessage(chatId, "Para modificar o tempo de irrigação mande a mensagem da forma:\n\n" + String(commands.irrigationTime) + " N\n\nN é o tempo de irrigação em segundos e deve ser maior que zero.");
    return;
  }
  int oldTime = irrigationTimeInSeconds;
  setIrrigationTime(time);
  GrowBot.sendMessage(chatId, "Tempo de irrigação alterado de " + String(oldTime) + " segundos para " + String(time) + " segundos.");
  return;
}

//-----------------------

int getValueFromMessage(String command, String message)
{
  String formatedCommand = String(command) + " ";
  if (message.length() <= formatedCommand.length())
  {
    return 0;
  }

  int index = message.indexOf(formatedCommand);
  if (index < 0)
  {
    return 0;
  }

  int interval = message.substring(formatedCommand.length()).toInt();
  if (interval <= 0)
  {
    return 0;
  }

  return interval;
}

//-----------------------

void sendStatusInfo(String chatId)
{
  String message = "Status:\n\n";

  // light status
  message += "LUZ \xF0\x9F\x92\xA1 \n";
  message += "Ciclo de luz: " + getLightCycleName(lightCycle) + "\n";
  message += "Status da luz: " + String(lightOn ? "ligada" : "desligada") + "\n";
  message += "Tempo dês de a ultima mudança na luz: " + String(hoursSinceLastLightChange) + " horas\n";
  // add new light status here
  message += "\n";

  // irrigation status
  message += "IRRIGAÇÃO \xF0\x9F\x9A\xBF \n";
  message += "Intervalo entre irrigações: " + String(irrigationIntervalInDays) + " dias\n";
  message += "Tempo de irrigação: " + String(irrigationTimeInSeconds) + " segundos\n";
  message += "Status da auto-irrigação: " + String(autoIrrigate ? "ligada" : "desligada") + "\n";
  // add new irrigation status here
  message += "\n";

  GrowBot.sendMessage(chatId, message);
}

//-----------------------

String getLightCycleName(String cycle, bool withTimes)
{
  if (cycle == "veg")
  {
    return "Vegetativo" + String(withTimes ? " (18/6)" : "");
  }
  else if (cycle == "ger")
  {
    return "Germinação" + String(withTimes ? " (16/8)" : "");
  }
  else if (cycle == "flor")
  {
    return "Floração" + String(withTimes ? " (12/12)" : "");
  }
  return "Unknown";
}