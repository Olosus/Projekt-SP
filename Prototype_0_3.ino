#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

// Nastavenie WIFI
const char* ssid = "Zadaj wifi ssid";
const char* password = "Zadaj wifi heslo *********";

Adafruit_BME680 bme; 

int Led_Red = 18;
int Led_Green = 4;
int sound_digital = 32;
int sound_analog = 35;

float temperature;
float humidity;
float pressure;
float gasResistance;

AsyncWebServer server(80);
AsyncEventSource events("/events");

unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;  // timer na meranie

void getBME680Readings(){
  // BME 680 začne merať.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Nepodarilo sa začať meranie :("));
    return;
  }
  if (!bme.endReading()) {
    Serial.println(F("Nepodarilo sa ukončiť meranie :("));
    return;
  }
  temperature = bme.temperature;
  pressure = bme.pressure / 100.0;
  humidity = bme.humidity;
  gasResistance = bme.gas_resistance / 1000.0;
}

String processor(const String& var){
  getBME680Readings();
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  else if(var == "HUMIDITY"){
    return String(humidity);
  }
  else if(var == "PRESSURE"){
    return String(pressure);
  }
 else if(var == "GAS"){
    return String(gasResistance);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>BME680 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.gas { color: #d62246; }
    .card.sound { color: black; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Analyzator Miestnosti 2.0</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> Teplota</h4><p><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> Vlhkost</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> Tlak</h4><p><span class="reading"><span id="pres">%PRESSURE%</span> hPa</span></p>
      </div>
      <div class="card gas">
        <h4><i class="fas fa-wind"></i> Plyny</h4><p><span class="reading"><span id="gas">%GAS%</span> K&ohm;</span></p>
      </div>
      <div class="card sound">
        <h4><i class="fas fa-volume-up"></i> Zvuk</h4><p><span class="reading"><span id="sound">%SOUND%</span> dB</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);
 
 source.addEventListener('gas', function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);

 source.addEventListener('sound', function(e) {
  console.log("sound", e.data);
  document.getElementById("sound").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(9600);
  pinMode(Led_Red, OUTPUT); 
  pinMode(Led_Green, OUTPUT); 
  pinMode(sound_digital, INPUT); 
  Serial.begin(115200);

  // Station and Soft Access Point naraz
  WiFi.mode(WIFI_AP_STA);
  
  // Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Pripájam sa na wifi...");
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // BME680 sensor
  if (!bme.begin()) {
    Serial.println(F("Nenašiel sa BME680 sensor, skontrolouj pripojenie!"));
    while (1);
  }
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  int val_digital = digitalRead(sound_digital);
  int val_analog = analogRead(sound_analog);

  Serial.print(val_analog);
  Serial.print("\t");
  Serial.println(val_digital);

  if (val_digital == HIGH || val_analog == HIGH)
  {
    digitalWrite(Led_Red, HIGH);
    digitalWrite(Led_Green, LOW);
    delay(3000);
    
    }
  else
  {
    digitalWrite(Led_Red, LOW);
    digitalWrite(Led_Green, HIGH);
  }
  if ((millis() - lastTime) > timerDelay) {
    getBME680Readings();
    Serial.printf("Temperature = %.2f ºC \n", temperature);
    Serial.printf("Humidity = %.2f % \n", humidity);
    Serial.printf("Pressure = %.2f hPa \n", pressure);
    Serial.printf("Gas Resistance = %.2f KOhm \n", gasResistance);
    Serial.println();

    // Send Events to the Web Server with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(temperature).c_str(),"temperature",millis());
    events.send(String(humidity).c_str(),"humidity",millis());
    events.send(String(pressure).c_str(),"pressure",millis());
    events.send(String(gasResistance).c_str(),"gas",millis());
    
    lastTime = millis();
  }
}