#include <Arduino.h>
#include <NewPing.h>
#include "WiFi.h"
#include "ESPmDNS.h"
#include "ESPAsyncWebServer.h"
#include "WebSocketsServer.h"
#include "ArduinoJson.h"

#define DEBUG 1

#if DEBUG == 1
#define debugstart(x) Serial.begin(9600)
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debugstart(x)
#define debug(x)
#define debugln(x)
#endif

#define MAX_DISTANCE 400
const int ECO = 26, TRIGGER = 27, INP = 14, CH1 = 21, CH2 = 22, LED = 2;
const int MAX = 30, MIN = 10;
float dnt, dnt2 = 0;

unsigned long previousTime = 0, currentTime;
long timeInterval = 5000;
int txt = 0;

char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>Ultrasonic</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="shortcut icon" type="image/png" href="D:\Esp\favicon.png" />
    <link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">


    <script>
        var connection = new WebSocket('ws://' + location.hostname + ':81/');

        var button_1_status = 0;
        var dis_data1 = 0;
        var dis_data2 = 0;
        var text_data = 0;
        var string1 = "tank is string1";
        var string2 = "tank is string2";


        function update_log() {
            console.log("client requsted to update");
            var full_data = '{"refresh" :"1"}';
            connection.send(full_data);
        }

        connection.onmessage = function (event) {
            var full_data = event.data;
            console.log(full_data);
            var data = JSON.parse(full_data);
            console.log(data);
            dis_data1 = data.temp;
            dis_data2 = data.cemp;
            text_data = data.text;

            document.getElementById("dis_meter1").value = dis_data1;
            document.getElementById("dis_value1").innerHTML = dis_data1;
            document.getElementById("dis_meter2").value = dis_data2;
            document.getElementById("dis_value2").innerHTML = dis_data2;

            if (text_data == 1) {
                document.getElementById("text_data_from").innerHTML = string1;
            }
            else {
                document.getElementById("text_data_from").innerHTML = string2;
            }

        }



    </script>


    <style>
        body {
            animation: fadeInAnimation ease 3s;
            animation-iteration-count: 1;
            animation-fill-mode: forwards;
        }

        @keyframes fadeInAnimation {
            0% {
                opacity: 0;
            }

            100% {
                opacity: 1;
            }
        }


        new meter,
        meter,
        h3 {
            display: inline
        }

        title h3,
        h2 {
            display: inline;
        }
    </style>
</head>

<body>
    <center>
        <h1>Ultasonic sensor reading!</h1>

        <p>update the readings of sensor by clicking below button</p>
        <button onclick="update_log()"> Update </button>

        <div style="text-align: center;">
            <div class="title">
                <h3>Tank_vol--------</h3>
                <h3>Tank_Vo2</h3>
            </div>
            <div class="new">
                <meter value="100" min="0" max="30" id="dis_meter1"> </meter>
                <h3>--------</h3>
                <meter value="100" min="0" max="30" id="dis_meter2"> </meter>
            </div>
            <h3 id="dis_value1"> 15</h3>
            <h3>cm</h3>
            <h3>-----------</h3>
            <h3 id="dis_value2"> 30</h3>
            <h3>cm</h3>
        </div>
        <h3 id="text_data_from"></h3>
    </center>
</body>

</html>

)rawliteral"; // wrtite "   R"rawliteral( __________ )rawliteral"   " for not convert c+ string

AsyncWebServer server(80); //http protocol prt 80 define, for diffrent protocol https://www.meridianoutpost.com/resources/articles/well-known-tcpip-ports.php
WebSocketsServer websockets(81);

const char *ssid = "upside"; // define the char or string for your wifi credentials
const char *password = "192.168.0.1";

NewPing sonar(TRIGGER, ECO, MAX_DISTANCE);

void blink_led()
{
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  delay(1000);
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  delay(1000);
}

void settingup()
{
  debugln("function Setting up");
  blink_led();
}

float ultra_send()
{
  int iterations = 5;
  float duration = sonar.ping_median(iterations);
  //duration = sonar.ping();  // this thing use for decimal number or acuurate countinous readings.
  float distance = (duration / 2) * 0.0343;
  debugln("function: ultra send and distance ");
  debugln(distance);
  return distance;
}

void send_sensor()
{
  dnt = ultra_send();
  if (dnt < MIN)
  {
    dnt = MAX;
    txt = 1;
  }
  else if (dnt > MAX)
  {
    dnt = MAX;
  }

  else
  {
    dnt = (int)ultra_send();
  }

  if (dnt2 < 30)
  {
    dnt2++;
  }
  else
  {
    dnt2 = 0;
  }
  String JSON_Data = "{\"temp\":";
  JSON_Data += dnt;
  JSON_Data += ",\"cemp\":";
  JSON_Data += dnt2;
  JSON_Data += ",\"text\":";
  JSON_Data += txt;
  JSON_Data += "}";
  debugln(JSON_Data);
  websockets.broadcastTXT(JSON_Data);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = websockets.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    websockets.sendTXT(num, "Connected from server");
  }
  break;
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    String message = String((char *)(payload));
    debugln(message);

    DynamicJsonDocument doc(200);
    // deserialize the data
    DeserializationError error = deserializeJson(doc, message);
    // parse the parameters we expect to receive (TO-DO: error handling)
    // Test if parsing succeeds.
    if (error)
    {
      debugln("deserializeJson() failed: ");
      debugln(error.c_str());
      return;
    }

    int refresh_status = doc["refresh"];
    if (refresh_status == 1)
    {
      send_sensor();
    }
    break;
  }
}

void fill_tank()
{
  int status = 0;
  while (ultra_send() > MIN)
  {
    if (digitalRead(INP) == LOW && status == 0)
    {
      status = 1;
    }
    if (status == 1)
    {
      digitalWrite(CH1, LOW);
      debugln("motor1 is off");
      delay(1000);
      digitalWrite(CH2, HIGH);
      debugln("motor2 is on");
      break;
    }
    else
    {
      digitalWrite(CH1, HIGH);
      debugln("motor1 is on");
    }
  }

  if (status == 1)
  {
    while (ultra_send() > MIN)
    {
      digitalWrite(CH2, HIGH);
      debugln("motor2 is on");
    }
  }

  digitalWrite(CH1, LOW);
  digitalWrite(CH2, LOW);
  status = 0;
  debugln("motors is OFF");
}

void setup()
{
  // put your setup code here, to run once:
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);    //turn off the brownout in esp32
  pinMode(INP, INPUT);
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(LED, OUTPUT);
  debugstart(9600);
  settingup();
  debug("\n\n");
  WiFi.mode(WIFI_STA);        // creating sta mode for esp
  WiFi.begin(ssid, password); // starting wifi connection
  debugln("conecting to :");
  debug(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    debug(".");
  }

  debugln("");
  debugln("WiFi connected.");
  debugln("IP address: ");
  debugln(WiFi.localIP());
  server.begin();
  if (MDNS.begin("home"))
  { //esp.local/
    debugln("MDNS responder started");
  }

  server.on("/", [](AsyncWebServerRequest *request)
            {
              //String msg = "helloworld";
              request->send_P(200, "text/html", webpage); //here 200 and 404 repesent the status code
            });

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
                      String msg = "page not found";
                      request->send(404, "text/plain", msg);
                    });

  websockets.begin();
  websockets.onEvent(webSocketEvent);
}

void loop()
{
  // put your main code here, to run repeatedly:
  currentTime = millis();
  if (currentTime - previousTime > timeInterval)
  {
    previousTime = currentTime;
    float tank_level = ultra_send();
    if (tank_level > MAX)
    {
      debugln("tank need to fill");
      fill_tank();
      debugln("tank is fill");
    }
    if (tank_level < MIN)
    {
      debugln("Warning");
    }
  }

  websockets.loop();
}