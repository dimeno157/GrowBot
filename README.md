# GrowBot #
## Tutorial de criar bot no telegram ##
TODO
<br>
<br>

## Como pegar o User ID do telegram ##
TODO 
<br>
<br>

## Configurações Inicias ##
Criar o arquivo **code/GrowBot/src/personal_info.h** copiar o seguinte código e colar no arquivo:

```c++
#define TOKEN "Token do Bot"
#define MY_ID "Id do seu usuário do Telegram"
#define WIFI_SSID "SSID da rede Wifi na qual a placa ira se conectar"
#define WIFI_PASSWORD "Senha da rede Wifi na qual a placa ira se conectar"
```

Substituindo os conteúdos dos textos entre aspas pelos seus respectivos valores. Em seguida salve o arquivo. Essas variáveis permitem que a ESP32 se conecte tanto ao bot do telegram quanto à rede WiFi.
<br>
<br>

## Circuitos ##
Os circuitos foram criados separadamente no TinkerCAD.:

### 1. Circuito da bomba d'água DC 12V para irrigação: <br>
- Componentes: <br>
    Mini Bomba D'água 12V RS-385<br>
    Resistor 1k<br>
    Transistor NPN (BJT) BC337<br>
    Fonte DC 12V, 0.5 - 0.7A<br>
- Link: https://www.tinkercad.com/things/khrkiLc9ZsU
<br/>
<br/>

## Modelos ##
TODO
<br/>
<br/>
