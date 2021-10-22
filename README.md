# GrowBot

## Criando o bot no Telegram
### 1. Abra o Telegram, clique no menu no canto superior esquerdo, em seguida clique em **Contatos** e pesquise pelo contato `@BotFather`
![BotFather](/images/telegram_bot/BotFather.jpeg)
#### (Certifique-se que o Bot possui o símbolo de verificado ao lado do nome)
<br>

### 2. Abra uma conversa com o `@BotFather` e digite o comando e ele irá iniciar o processo de criação do novo bot.
    /newbot
<br>

### 3. Em seguida envie o nome e o username  (o username deve obrigatoriamente terminar com "bot") e o seu bot será criado:
![BotFather](/images/telegram_bot/name_and_username.png)
<br>

### 4. Com o Bot criado um Token é gerado, esse Token será usado mais tarde para conectar a ESP32 ao bot:
![BotFather](/images/telegram_bot/token.png)
<br>
<br>

----------
## Pegando o ID de usuário no Telegram
### 1. 
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
