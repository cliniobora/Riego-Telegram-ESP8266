#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiUdp.h>
#include <EEPROM.h>


#define EEPROM_SIZE 512
// Wifi network station credentials
#define WIFI_SSID "*****************"
#define WIFI_PASSWORD "*******************"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "********************************"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ar.pool.ntp.org", -10800);

#define pinLed 12
int segundos = 0;
int estado = 0;
int horas;
int minutos;
int horaderiego = 8;
int minutosderiego = 10;
int horadeinicio = 0;
int minutodeinicio = 0;
int contadordetiempo = 0;
int tiemporestante;
int contadordeminutos;
int contadordesegundos;

boolean actualizar = true;
boolean cdRiegoHora = false;

const unsigned long BOT_MTBS = 10;  // mean time between scan messages
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;  // last time messages' scan has been done


void handleNewMessages(int numNewMessages) {

  for (int i = 0; i < numNewMessages; i++) {

    //Inline buttons with callbacks when pressed will raise a callback_query message
    String chat_id = bot.messages[i].chat_id;
    String from_id = bot.messages[i].from_id;

    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    //Serial.println(chat_id);

    if (bot.messages[i].chat_id == "******************") { //////////replace ****** with your chat_id
                                                            //        bot only asnwer to chat_id

      if (bot.messages[i].type == "callback_query") {
        Serial.print("Call back button pressed by: ");
        Serial.println(bot.messages[i].from_id);
        Serial.print("Data on the button: ");
        Serial.println(bot.messages[i].text);


        if (bot.messages[i].text == "regarahora") {
          tiemporestante = minutosderiego;
          bot.sendMessage(bot.messages[i].from_id, "Regando...");
        }
        if (bot.messages[i].text == "mastiemporiego") {
          if (minutosderiego >= 200) {
            minutosderiego = 200;
            bot.sendMessage(bot.messages[i].from_id, "No puedo regar mas de 200 minutos, cuida el agua hdp");
          } else {
            minutosderiego += 5;
            bot.sendMessage(bot.messages[i].from_id, "Has aumentado el tiempo de riego ahora es de: " + String(minutosderiego) + " minutos");
          }
        }
        if (bot.messages[i].text == "menostiemporiego") {
          if (minutosderiego > 0) {
            minutosderiego -= 5;
            bot.sendMessage(bot.messages[i].from_id, "Has disminuido el tiempo de riego ahora es de: " + String(minutosderiego) + " minutos");
          } else {
            bot.sendMessage(bot.messages[i].from_id, "No puedo disminuir mas el tiempo de riego, se encuentra en 0 y por eso no regare nunca");
          }
          //bot.sendMessage(bot.messages[i].from_id, bot.messages[i].text, "");
        }
        if (bot.messages[i].text == "cambiarhorario") {
          horaderiego++;
          horaderiego = horaderiego % 24;
          bot.sendMessage(bot.messages[i].from_id, "Has cambiado el horario de riego ahora es a las: " + String(horaderiego) + " horas");
        }
        if (bot.messages[i].text == "save") {

          EEPROM.put(0, horaderiego);
          EEPROM.put(10, minutosderiego);
          EEPROM.commit();
          bot.sendMessage(bot.messages[i].from_id, "Configuracion guardada");
        }

        break;
      } else {
        // String chat_id = bot.messages[i].chat_id;

        // String text = bot.messages[i].text;

        // String from_name = bot.messages[i].from_name;
        if (from_name == "")
          from_name = "Guest";
        //Serial.println(chat_id);
        if (text == "opciones") {
          String keyboardJson = "[[{ \"text\" : \"Regar ahora\", \"callback_data\" : \"regarahora\" }],[{ \"text\" : \"Mas Tiempo de riego\", \"callback_data\" : \"mastiemporiego\" }],[{ \"text\" : \"Menos tiempo de riego\", \"callback_data\" : \"menostiemporiego\" }],[{ \"text\" : \"Cambiar horario de Riego\", \"callback_data\" : \"cambiarhorario\" }],[{ \"text\" : \"Guardar ajustes\", \"callback_data\" : \"save\" }]]";
          bot.sendMessageWithInlineKeyboard(chat_id, "Podes usar las siguientes opciones: ", "", keyboardJson);
          break;
        }


        if (text.equalsIgnoreCase("estado")) {
          String welcome = "Hola " + from_name + ".\n";
          welcome += "Te envio informacion sobre el estado del sistema de riego.\n\n";
          welcome += "Regare todos los dias a las " + String(horaderiego) + " horas.\n";
          welcome += "Regare durante " + String(minutosderiego) + " minutos.\n";

          if (tiemporestante > 0) {

            welcome += "Actualmente estoy regando, seguire asi por " + String(tiemporestante) + " minutos mas.\n";
          }
          welcome += "Hora actual: " + String(horas) + ":" + String(minutos) + ".\n";
          bot.sendMessage(chat_id, welcome, "Markdown");
        }

        else {
          String noption = "Bienvenido al sistema de riego " + from_name + ".\n";
          noption += "No reconozco tu mensaje\n";
          noption += "Proba enviando 'opciones', 'estado'";
          bot.sendMessage(chat_id, noption);
        }
      }
    }
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(pinLed, OUTPUT);  // initialize digital ledPin as an output.
  digitalWrite(pinLed, HIGH);  // initialize pin as off (active LOW)

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");       // get UTC time via NTP
  secured_client.setTrustAnchors(&cert);  // Add root certificate for api.telegram.org
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  EEPROM.begin(EEPROM_SIZE);
  // EEPROM.put(0, horaderiego);
  // EEPROM.put(10, minutosderiego);
  // EEPROM.commit();
  EEPROM.get(0, horaderiego);
  EEPROM.get(10, minutosderiego);
  digitalWrite(pinLed, HIGH);  // initialize pin as off (active LOW)

}

void loop() {

  timeClient.update();
  String tiempo = timeClient.getFormattedTime();
  //int tiempoepoch = timeClient.getEpochTime();
  horas = timeClient.getHours();
  minutos = timeClient.getMinutes();
/////////////////////////////////////////////////////////////////
  if (tiemporestante > 0) {
    estado = 0;
    if (minutos != contadordeminutos) {
      contadordeminutos = minutos;
      tiemporestante--;
    }

  } else {
    estado = 1;
  }
  digitalWrite(pinLed, estado);

  //////////////////////ENCIENDE RIEGO SI SE CUMPLE HORARIO/////////////////////////////////////
  if (horas == horaderiego) {
    if (cdRiegoHora == 0) {
      tiemporestante = minutosderiego;
      cdRiegoHora = 1;
    }
  } else {
    cdRiegoHora = false;
  }
  //////////////////////////////////////////////////////////////////////////

  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}
