#include <Arduino.h>

#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <ESPAsyncWebServer.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Web Hiển Thị Dữ Liệu</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f8ff;
            color: #333;
            text-align: center;
            padding: 20px;
        }

        h2 {
            color: #2c3e50;
        }

        .container {
            display: inline-block;
            border: 1px solid #ccc;
            padding: 20px;
            border-radius: 10px;
            background-color: #fff;
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
            margin: 20px auto;
            width: 60%;
        }

        .data-block {
            margin: 20px 0;
            text-align: left;
        }

        .data-block p {
            margin: 10px 0;
        }

        .data-block span {
            font-weight: bold;
            color: #2980b9;
        }

        #vanData, #bnoData {
            margin: 20px 0;
        }
    </style>
</head>
<body>

    <div class="container">
        <h1>ESP32 Web: Hiển Thị Dữ Liệu</h1>

        <div class="data-block">
            <h2>Độ mở van và thời gian mở van</h2>
            <div id="vanData">Đang chờ dữ liệu...</div>
        </div>

        <div class="data-block">
            <h2>Dữ Liệu BNO055</h2>
            <p>Gia tốc x: <span id="tx">0</span> (m/s<sup>2</sup>)</p>
            <p>Gia tốc y: <span id="ty">0</span> (m/s<sup>2</sup>)</p>
            <p>Gia tốc z: <span id="tz">0</span> (m/s<sup>2</sup>)</p>

            <p>Góc x: <span id="gx">0</span>&deg</p>
            <p>Góc y: <span id="gy">0</span>&deg</p>
            <p>Góc z: <span id="gz">0</span>&deg</p>
        </div>
    </div>
    <script>
        if(window.EventSource){
          var source = new EventSource('/events');
          source.addEventListener('open', function (e) {
              console.log("Events Connected");
          }, false);
          source.addEventListener('error',function(e){
              if(e.target.readyState != EventSource.OPEN){
                  console.log("Events Disconnected");
              }
          },false);
          source.addEventListener('giaToc.x',function(e){
             console.log("GiaToc.x", e.data);
             document.getElementById("tx").innerHTML=e.data;
          },false);
          source.addEventListener('giaToc.y',function(e){
             console.log("GiaToc.y", e.data);
             document.getElementById("ty").innerHTML=e.data;
          },false);
          source.addEventListener('giaToc.z',function(e){
             console.log("GiaToc.z", e.data);
             document.getElementById("tz").innerHTML=e.data;
          },false);
          source.addEventListener('goc.x',function(e){
             console.log("goc.x", e.data);
             document.getElementById("gx").innerHTML=e.data;
          },false);
          source.addEventListener('goc.y',function(e){
             console.log("goc.y", e.data);
             document.getElementById("gy").innerHTML=e.data;
          },false);
          source.addEventListener('goc.z',function(e){
             console.log("goc.z", e.data);
             document.getElementById("gz").innerHTML=e.data;
          },false);
          source.addEventListener('van',function(e){
             console.log("van", e.data);
             document.getElementById("vanData").innerHTML=e.data;
          },false);
      }
  </script>
</body>
</html>
)rawliteral";


AsyncWebServer server(80);
AsyncEventSource events("/events");

#define LED 2
#define bienTro 32
#define nutAn 23

unsigned long lastTime = 0, startTime = 0;

const char *ssid = "Tom Dong Bich";
const char *password = "hi09082015";

#define MQTT_SERVER "mqtt-dashboard.com"
#define MQTT_PORT 1883

#define MQTT_TOPIC "devID/valve"

WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

void setWiFi()
{
  Serial.print("Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("...");
  }

  Serial.println(WiFi.localIP());

}
void setBNO055()
{
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Khoi tao BNO055");
  if (!bno.begin())
  {
    Serial.println("ERROR");
    while(1);
  }
  else
  {
    Serial.println("Khoi tao thanh cong");
  }
}
void connect_to_broker()
{
  while ((!client.connected()))
  {
    Serial.println("MQTT connection");
    String clientId = "esp32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {

      Serial.println("MQTT Connected");

      client.subscribe(MQTT_TOPIC);
    }
    else
    {
      Serial.println("FAIED");
      delay(2000);
    }
  }
}
int giatri_bienTro = 0;
void read_bienTro()
{
  giatri_bienTro = map(analogRead(bienTro), 0, 4095, 0, 100);
  digitalWrite(LED, analogRead(bienTro));

}
double tx, ty, tz, gx, gy, gz;
void readBNO055()
{

  tx = -1000000, ty = -1000000, tz = -1000000, gx = -1000000, gy = -1000000, gz = -1000000;
  sensors_event_t accelerometerData, orientationData;
  bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  tx = accelerometerData.acceleration.x;
  ty = accelerometerData.acceleration.y;
  tz = accelerometerData.acceleration.z;
  gx = orientationData.orientation.x;
  gy = orientationData.orientation.y;
  gz = orientationData.orientation.z;
  events.send("ping", NULL, millis());
  events.send(String(tx).c_str(), "giaToc.x", millis());
  events.send(String(ty).c_str(), "giaToc.y", millis());
  events.send(String(tz).c_str(), "giaToc.z", millis());
  events.send(String(gx).c_str(), "goc.x", millis());
  events.send(String(gy).c_str(), "goc.y", millis());
  events.send(String(gz).c_str(), "goc.z", millis());
}

void setup()
{
  Serial.begin(9600);
  setWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  connect_to_broker();
  setBNO055();
  pinMode(bienTro, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(nutAn, INPUT_PULLUP);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html); });
  server.addHandler(&events);
  server.begin();
}
int count = 0; 
unsigned long lastTime1 = 0;

void loop()
{
 if (!client.connected()) {
    connect_to_broker();
  }
  client.loop();

  if (digitalRead(nutAn) == 0)
  {
    if(millis()- lastTime >= 100){
      read_bienTro();
      lastTime = millis();
    }
    readBNO055();

    if ((giatri_bienTro == 0) && (count == 0))
    {
      client.publish(MQTT_TOPIC, "Van Đóng");
      events.send("Van Đóng", "van", millis());
      count = 1;
    }
    else if (giatri_bienTro != 0)
    {
      if (count == 1)
      {
        startTime = millis();
      }
      JsonDocument doc;
      doc["f"] = giatri_bienTro;
      doc["t"] = millis() - startTime;

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);

      if (millis() - lastTime1 >= 500)
      {
        Serial.println(jsonBuffer);
        client.publish(MQTT_TOPIC, jsonBuffer);
        events.send(String(jsonBuffer).c_str(), "van", millis());
        lastTime1 = millis();

      }

      count = 0;
    }
  }
  else
  {

  }
}
