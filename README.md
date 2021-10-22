Criar o arquivo **code/GrowBot/src/personal_info.h** copiar o seguinte código e colar no arquivo:

```c++
#define TOKEN "Token do Bot"
#define MY_ID "Id do seu usuário do Telegram"
#define WIFI_SSID "SSID da rede Wifi na qual a placa ira se conectar"
#define WIFI_PASSWORD "Senha da rede Wifi na qual a placa ira se conectar"
```

Substituindo os conteúdos dos textos entre aspas pelos seus respectivos valores. Em seguida salve o arquivo. Essas variáveis permitem que a ESP32 se conecte tanto ao bot do telegram quanto à rede WiFi.