#include <WiFi.h>
#include <NetworkUdp.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HMC5883_U.h>
#include "ArduinoJson.h"
#include "ESPAsyncWebServer.h"
#include "esp_eap_client.h"
#include "DHT.h"

// Initialize the LCD using pins 4, 23, 32, 2, 25, 26
LiquidCrystal lcd(4, 23, 32, 2, 25, 26);

// DHT22 setup
#define DHTPIN 13        // Pin where the DHT22 is connected
#define DHTTYPE DHT22    // DHT 22 (AM2302), AM2321

#define WIND_SPEED_PIN 19

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP3XX bmp;
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// UV sensor pin (A4 for Mega or pin 18 as per your setup)
#define UV_PIN 18

const char* UNIQUE_DEVICE_ID = "0a1c3bbf72";

const char* SSID = "ECTE250 Weather Station";
const char* PASSWORD = "test123456";

IPAddress localIp(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

DNSServer dnsServer;
AsyncWebServer server(80);
NetworkUDP udp;

// Weather data
float humidity = 0.0;
float temperature = 0.0;
float uvIndex = 0.0;
float pressure = 0.0;
float windSpeed = 0.0;
float headingDegrees = 0.0;
String direction = "null";

int photointerrupter = LOW;
int photointerruptercounter = 0;

// Weather station status
bool isSetupMode = true;
bool needsWiFiscan = true;

// Current wifi
struct WifiNetworkInfo {
  String ssid;
  int type;
  String identity;
  String username;
  String password;
} current_network;

struct WifiNetworkListItem {
  String ssid;
  int type;
};

std::vector<WifiNetworkListItem> scannedNetworks;

// Timers
unsigned long timerScreenUpdate = 0;
unsigned long timerDHT22Read = 0;
unsigned long timerBMPRead = 0;
unsigned long timerWindSpeedCalculate = 0;
unsigned long timerMagnetRead = 0;
unsigned long timerSendDataToServer = 0;
unsigned long needsWiFiConnect = 0;

const char* captiveHtmlContent = R"delim(
<!DOCTYPE html><html lang="en"><head> <meta charset="UTF-8"> <meta name="viewport" content="width=device-width, initial-scale=1.0"> <title>Wireless Weather Station</title> <style>body{font-family: Arial, sans-serif; background-color: #f0f8ff; color: #333; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh;}.container{background-color: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); max-width: 400px; width: 90%;}.header{text-align: center; margin-bottom: 20px;}.header h1{margin: 0; font-size: 24px; color: #007acc;}.stats, .controls{display: flex; flex-direction: column; gap: 10px;}.stat-item{display: flex; justify-content: space-between; font-size: 18px; padding: 10px; border-radius: 5px; background-color: #f7f9fa;}.stat-item span{font-weight: bold;}@media (max-width: 600px){.header h1{font-size: 20px;}.stat-item{font-size: 16px;}}#toggleLedButton{width: 100%;}#textInput{width: 100%;}#error{font-size: 0.75rem; color: red; margin-top: 1rem;}#error:not(.visible){display: none;}#connected:not(.visible){display: none;}</style></head><body> <div class="container"> <div class="header"> <h1>Wireless Weather Station</h1> </div><div class="settings-section"> <div style="font-weight: 900;"> Change network </div><div style="margin-top: 0.5rem; margin-bottom: 0.5rem; font-size: 0.75rem;"> In here you can change the WiFi network the weather station connects to when sending the data to our servers. </div><div style="margin-top: 0.5rem; margin-bottom: 0.5rem; font-size: 0.75rem;"> Currently connected to <b id="currentNetwork">UOW</b> <span id="connected">&nbsp;with IP address <b id="currentIp"></b></span> </div><select id="wifiDropdown" style="width: 100%; padding: 0.5rem; margin-top: 0.5rem;" disabled> </select> <button id="scanButton" style="margin-top: 1rem; padding: 0.5rem;" disabled onclick="scanNetworks()">Scan</button> </div><div id="error">Error text blah</div></div><script>const scanButton=document.getElementById("scanButton"); const wifiDropdown=document.getElementById("wifiDropdown"); const currentNetwork=document.getElementById("currentNetwork"); const errorDisplay=document.getElementById("error"); const isConnected=document.getElementById("connected"); const currentIp=document.getElementById("currentIp"); let errorNumber=0; async function scanNetworks(){const scanningText=document.createElement("option"); scanningText.innerText="Scanning..."; scanButton.setAttribute("disabled", ""); wifiDropdown.setAttribute("disabled", ""); Array.from(wifiDropdown.children).forEach(child=>{wifiDropdown.removeChild(child);}); wifiDropdown.appendChild(scanningText); await scanNetworks_() .then(()=>{wifiDropdown.removeChild(scanningText); errorDisplay.classList.remove("visible"); wifiDropdown.removeAttribute("disabled"); errorNumber=0;}) .catch(e=>{Array.from(wifiDropdown.children).forEach(child=>{wifiDropdown.removeChild(child);}); errorDisplay.innerText=`(${errorNumber}) ${e}`; errorDisplay.classList.add("visible"); errorNumber=errorNumber + 1;}) .finally(()=>{scanButton.removeAttribute("disabled");});}async function scanNetworks_(){const scanResponse=await fetch(`/api/scan`,{method: "POST"}); if (!scanResponse.ok){throw "Failed to scan";}let sleepDuration; if (errorNumber==0){sleepDuration=5000;}else{sleepDuration=8000;}await new Promise(r=> setTimeout(r, sleepDuration)); const dataResponse=await fetch(`/api/get_network_data`,{method: "GET"}); const json=await dataResponse.json(); if (json.current_ip !=null){isConnected.classList.add("visible"); currentIp.innertext=`${json.currentIp}`;}else{isConnected.classList.remove("visible"); currentIp.innerText="";}const networks=Array.from(json.networks); if (networks.length==0){const nothingFound=document.createElement("option"); nothingFound.innerText="No networks found"; wifiDropdown.appendChild(nothingFound); return;}const selectOption=document.createElement("option"); selectOption.innerText="--Select a new network--"; wifiDropdown.appendChild(selectOption); networks.forEach(network=>{const wifiOption=document.createElement("option"); wifiOption.innerText=network.ssid; wifiOption.wifiType=network.type; wifiDropdown.appendChild(wifiOption);}); /*{"current_network": "UOW", "current_ip": null, "networks": [{"ssid": "", "type": 1}]}*/}window.addEventListener("load", (event)=>{scanNetworks();}); </script></body></html>
)delim";

const char* captiveHtmlContentConnect = R"delim(
<!DOCTYPE html><html lang="en"><head> <meta charset="UTF-8"> <meta name="viewport" content="width=device-width, initial-scale=1.0"> <title>Wireless Weather Station</title> <style>body{font-family: Arial, sans-serif; background-color: #f0f8ff; color: #333; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh;}.container{background-color: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); max-width: 400px; width: 90%;}.header{text-align: center; margin-bottom: 20px;}.header h1{margin: 0; font-size: 24px; color: #007acc;}.stats, .controls{display: flex; flex-direction: column; gap: 10px;}.stat-item{display: flex; justify-content: space-between; font-size: 18px; padding: 10px; border-radius: 5px; background-color: #f7f9fa;}.stat-item span{font-weight: bold;}@media (max-width: 600px){.header h1{font-size: 20px;}.stat-item{font-size: 16px;}}#toggleLedButton{width: 100%;}#textInput{width: 100%;}#error{font-size: 0.75rem; color: red; margin-top: 1rem;}#error:not(.visible){display: none;}#connected:not(.visible){display: none;}.input{width: 100%; padding: 0.5rem; margin-top: 0.5rem; box-sizing: border-box;}.wifiCred:not(.visible){display: none;}</style></head><body> <div class="container"> <div class="header"> <h1>Wireless Weather Station</h1> </div><div class="settings-section"> <div style="font-weight: 900;"> Connect to UOW </div><div style="margin-top: 0.5rem; margin-bottom: 0.5rem; font-size: 0.75rem;"> Encryption type: <span id="encType"></span> </div><input id="wifiIdent" type="text" class="input wifiCred" placeholder="Identity" required/> <input id="wifiUser" type="text" class="input wifiCred" placeholder="Username" required/> <input id="wifiPass" type="password" class="input wifiCred" placeholder="Password" required/> <button id="connectButton" style="margin-top: 1rem; padding: 0.5rem;" onclick="connect()">Connect</button> </div><div id="error">Error text blah</div></div><script>const connectButton=document.getElementById("connectButton"); const wifiIdent=document.getElementById("wifiIdent"); const wifiUser=document.getElementById("wifiUser"); const wifiPass=document.getElementById("wifiPass"); const errorDisplay=document.getElementById("error"); const wifiCreds=[wifiIdent, wifiUser, wifiPass]; let errorNumber=0; let ssid; let type; async function connect(){connectButton.setAttribute("disabled", ""); wifiCreds.forEach(elem=> elem.setAttribute("disabled", "")); await connect_() .then(()=>{errorDisplay.classList.remove("visible"); errorNumber=0;}) .catch(e=>{errorDisplay.innerText=`(${errorNumber}) ${e}`; errorDisplay.classList.add("visible"); errorNumber=errorNumber + 1;}) .finally(()=>{wifiCreds.forEach(elem=> elem.removeAttribute("disabled")); scanButton.removeAttribute("disabled");});}async function connect_(){let body; switch (type){case 1: body={type}; break; case 2: wifiPass.reportValidity(); body={type, password: wifiPass.value}; break; case 3: wifiCreds.forEach(elem=> elem.reportValidity()); body={type, identity: wifiIdent.value, username: wifiUser.value, password: wifiPass.value}; break;}const connectResponse=await fetch(`/api/connect`,{method: "POST", headers:{"content-type": "application/json"}, body: JSON.stringify(body)}); if (!connectResponse.ok){throw "Failed to connect";}let iterations=0; while (true){await new Promise(r=> setTimeout(r, 2000)); const dataResponse=await fetch(`/api/get_connecting_status`,{method: "GET"}); if (!dataResponse.ok){throw await dataResponse.text;}const json=await dataResponse.json(); if (json.connected==true){break;}iterations +=1; if (iterations==5){throw "Failed to connect";}}}window.addEventListener("load", (event)=>{const urlParams=new URLSearchParams(window.location.search); ssid=urlParams.get("ssid"); type=parseInt(urlParams.get("type")); if (ssid==null || type==null){alert("WiFi network not specified"); return;}encType.innerText=urlParams.get("encType"); switch (type){case 1: break; case 2: wifiPass.classList.add("visible"); break; case 3: wifiIdent.classList.add("visible"); wifiUser.classList.add("visible"); wifiPass.classList.add("visible"); break;}}); </script></body></html>
)delim";

void updateScannedNetworks(uint16_t networksFound);
String generateItemList();
void removeDuplicateNetworks(std::vector<WifiNetworkListItem>& scannedNetworks);
void updateScreen();
void windSpeedCounter();
void initiateWiFiConnect();

#define EAP_IDENTITY "maa547"
#define EAP_PASSWORD "<redacted>"

const uint16_t serverPort = 1337;
const char *serverHost = "51.161.130.58";
bool needsToSendData = false;

// Network connection state
NetworkClient client;
int networkConnState = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  pinMode(UV_PIN, INPUT);
  pinMode(WIND_SPEED_PIN, INPUT_PULLUP);

  if (!bmp.begin_I2C()) {
    Serial.println("Could not find a valid BMP388 sensor, check wiring!");
    while (1);
  }
  
  if(!mag.begin()) {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }

  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  Serial.println("BMP388 sensor initialized successfully");

  lcd.begin(16, 2);       // Initialize 16x2 LCD display

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  Serial.println("Connecting to WiFi");

  esp_eap_client_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));  //provide identity
  esp_eap_client_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));  //provide username
  esp_eap_client_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));  //provide password
  esp_eap_client_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_PAP);
  esp_wifi_sta_enterprise_enable();

  WiFi.begin(
    "eduroam"
  );

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Setting up soft AP");

  WiFi.softAP(SSID, PASSWORD);
  Serial.print("Access Point \"");
  Serial.print(SSID);
  Serial.println("\" started");
  
  WiFi.softAPConfig(localIp, localIp, subnet);

  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", captiveHtmlContent);
  });

  server.on("/connect.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", captiveHtmlContentConnect);
  });

  server.on("/api/get_network_data", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Got get_network_data request");
    
    String responseJson = R"(
      {
         "current_network": "$CURRENT_NETWORK",
         "current_ip_address": $CURRENT_IP,
         "networks": [
             $NETWORKS_LIST
         ]
      }
    )";

    if (WiFi.status() == WL_CONNECTED) {
      responseJson.replace("$CURRENT_IP", "\"" + WiFi.localIP().toString() + "\"");
    } else {
      responseJson.replace("$CURRENT_IP", "null");
    }

    if (current_network.type = -1) {
      responseJson.replace("$CURRENT_NETWORK", "null");
    } else {
      responseJson.replace("$CURRENT_NETWORK", current_network.ssid);
    }

    String itemList = generateItemList();

    responseJson.replace("$NETWORKS_LIST", itemList);

    Serial.println(responseJson);

    request->send(200, "application/json", responseJson);
  });

  server.on("/api/scan", HTTP_POST, [](AsyncWebServerRequest *request){
    WiFi.scanNetworks(true);
    needsWiFiscan = true;

    request->send(200);
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if (request->url() == "/api/connect") {
      Serial.println("Got connect_wifi request");
    
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data);
      uint8_t TYPE = doc["type"].as<uint8_t>();

      current_network.ssid = doc["ssid"].as<String>();

      switch (TYPE) {
        case 1:
          current_network.type = 1;
          current_network.identity = "";
          current_network.username = "";
          current_network.password = "";
          break;
        case 2:
          current_network.type = 2;
          current_network.identity = "";
          current_network.username = "";
          current_network.password = doc["password"].as<String>();
          break;
        case 3:
          current_network.type = 3;
          current_network.identity = doc["identity"].as<String>();
          current_network.username = doc["username"].as<String>();
          current_network.password = doc["password"].as<String>();
          break;
      }

      initiateWiFiConnect();
      
      request->send(200, "text/plain", "true");
    }
  });

  server.on("/api/get_connecting_status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Got get_connecting_status request");

    if (WiFi.status() == WL_CONNECTED ||
        WiFi.status() == WL_DISCONNECTED ||
        WiFi.status() == WL_IDLE_STATUS) {
      
        String responseJson = R"(
          {
            "connected": $CONNECTED
          }
        )";

        if (WiFi.status() == WL_CONNECTED) {
          responseJson.replace("$CONNECTED", "true");
        } else {
          responseJson.replace("$CONNECTED", "false");
        }

        Serial.println(responseJson);
        
        request->send(200, "application/json", responseJson);
    } else {
        String responseJson = R"(
          {
            "error": "$ERROR"
          }
        )";
        
        if (WiFi.status() == WL_CONNECT_FAILED ||
               WiFi.status() == WL_CONNECTION_LOST) {
          responseJson.replace("$ERROR", "Connection failed");
        } else {
          Serial.println(WiFi.status());
          responseJson.replace("$ERROR", "Unexpected WiFi status");
        }

        Serial.println(responseJson);
      
        request->send(500, "application/json", responseJson);
    }
  });

  
  current_network.type = -1;
  
  server.begin();

  delay(2000);

  timerWindSpeedCalculate = millis();
  timerDHT22Read = millis();
  timerBMPRead = millis();
  timerScreenUpdate = millis();
  timerMagnetRead = millis();
  timerSendDataToServer = millis();

  randomSeed(millis());
  
  Serial.print("Test connecting to ");
  Serial.println(serverHost);

  client = NetworkClient();

  if (!client.connect(serverHost, serverPort)) {
    Serial.println("Connection failed");
  } else {
    Serial.println("Connection succeeded");
    client.print("esp32:test_connection\n");
    client.stop();
  }

  WiFi.scanNetworks(true);

  dht.begin();            // Initialize DHT sensor
}

void loop() {
  // Send data to server
  if (millis() - timerSendDataToServer > 10000) { 
    timerSendDataToServer = millis();

    needsToSendData = true;

    if (networkConnState == 1) {
      client.stop();
    }

    return; // continue the loop
  }

  if (needsToSendData) {
    if (WiFi.status() == WL_CONNECTED) {
      needsToSendData = false;

      Serial.print("Connecting to ");
      Serial.println(serverHost);

      client = NetworkClient();

      if (!client.connect(serverHost, serverPort)) {
        Serial.println("Connection failed");
      } else {
        Serial.println("Connection succeeded");
        networkConnState = 1;
        client.printf("%s;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
           UNIQUE_DEVICE_ID, humidity, temperature, uvIndex, pressure, windSpeed, headingDegrees);
        client.stop();
      }
    }
  }

//  if (networkConnState == 1) {
//    if (WiFi.status() == WL_CONNECTED) {
//      if (client.available()) {
//        String line = client.readStringUntil('\n');
//        Serial.print("Response from server: ");
//        Serial.println(line);
//
//        networkConnState = 0;
//        client.stop();
//      }
//    } else {
//      networkConnState = 0;
//      client.stop();
//    }
//  }
  
  // Read magnetometer
  if (millis() - timerMagnetRead > 557) {
    timerMagnetRead = millis();

    sensors_event_t event; 
    mag.getEvent(&event);

    float calibratedX = event.magnetic.x - - 14;
    float calibratedY = event.magnetic.y - - 43;

    float heading = atan2(calibratedY, calibratedX);

    float declinationAngle = 0.216;
    heading += declinationAngle;
  
    // Correct for when signs are reversed.
    if(heading < 0)
      heading += 2*PI;

    // Check for wrap due to addition of declination.
    if(heading > 2*PI)
      heading -= 2*PI;
   
    // Convert radians to degrees for readability.
    headingDegrees = heading * 180/M_PI; 

    if (headingDegrees >= 337.5 || headingDegrees < 22.5) {
      direction = "N";
    } else if (headingDegrees >= 22.5 && headingDegrees < 67.5) {
      direction = "NE";
    } else if (headingDegrees >= 67.5 && headingDegrees < 112.5) {
      direction = "E";
    } else if (headingDegrees >= 112.5 && headingDegrees < 157.5) {
      direction = "SE";
    } else if (headingDegrees >= 157.5 && headingDegrees < 202.5) {
      direction = "S";
    } else if (headingDegrees >= 202.5 && headingDegrees < 247.5) {
      direction = "SW";
    } else if (headingDegrees >= 247.5 && headingDegrees < 292.5) {
      direction = "W";
    } else if (headingDegrees >= 292.5 && headingDegrees < 337.5) {
      direction = "NW";
    }

    // Print the direction and heading
//    Serial.print("Heading (degrees): ");
//    Serial.print(headingDegrees);
//    Serial.print(" - Direction: ");
//    Serial.println(direction);
  }
  
  int windspeedval = digitalRead(WIND_SPEED_PIN);
  
  if (windspeedval != photointerrupter) {
    photointerrupter = windspeedval;

    // Trigger counter
    photointerruptercounter += 1;
  }

  // Recalculate wind speed
  if (millis() - timerWindSpeedCalculate > 5000) {
    timerWindSpeedCalculate = millis();

    float rpm = ((((float) photointerruptercounter) / 16) / 5.0) * 12.0;

    photointerruptercounter = 0;

//    Serial.print("RPM: ");
//    Serial.println(rpm);

    float pi = 3.141592653589;
    float diameter = 7.87;

    windSpeed = ((pi * diameter * rpm * 60.0) / 5280.0) * 1.609;
  }
  
  if (isSetupMode) {
    dnsServer.processNextRequest();
  }
  
  if (needsWiFiscan) {
    int16_t WiFiScanStatus = WiFi.scanComplete();

    if (WiFiScanStatus < 0) {  // it is busy scanning or got an error
      if (WiFiScanStatus == WIFI_SCAN_FAILED) {
        Serial.println("WiFi Scan has failed. Starting again.");
        WiFi.scanNetworks(true);
      }
    } else {
      updateScannedNetworks(WiFiScanStatus);
      needsWiFiscan = false;
    }
  }

  // Update DHT22 values
  if (millis() - timerDHT22Read > 2500) {
    timerDHT22Read = millis();

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    int uvValue = analogRead(UV_PIN);
    uvIndex = map(uvValue, 0, 1023, 0, 15);
  }

  // Update BMP values
  if (millis() - timerBMPRead > 500) {
    timerBMPRead = millis();

    if (!bmp.performReading()) {
      //Serial.println("Failed to perform BMP reading");
      return;
    }

    pressure = bmp.pressure / 100.0;
  }
  
  // Update screen
  if (millis() - timerScreenUpdate > 5000) {
    timerScreenUpdate = millis();

    updateScreen();
  }
}

void initiateWiFiConnect() {
  Serial.println("initiateWifiConnect called");

  return;
  
  WiFi.disconnect();

  switch (current_network.type) {
    case 1:
      WiFi.begin(current_network.ssid);
      break;
    case 2:
      WiFi.begin(current_network.ssid, current_network.password);
      break;
    case 3:
      Serial.println("Connecting with WPA2 enterprise");
      Serial.print("Username: ");
      Serial.println(current_network.username);
    
      esp_eap_client_set_identity((uint8_t *)current_network.identity.c_str(), strlen(current_network.identity.c_str()));  //provide identity
      esp_eap_client_set_username((uint8_t *)current_network.username.c_str(), strlen(current_network.username.c_str()));  //provide username
      esp_eap_client_set_password((uint8_t *)current_network.password.c_str(), strlen(current_network.password.c_str()));  //provide password
      esp_eap_client_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_PAP);
      esp_wifi_sta_enterprise_enable();
      
      WiFi.begin(current_network.ssid);

      break;
  }
}

void windSpeedCounter() {
  static int counter = 0;

  counter += 1;
}

int alternator = 0;

void updateScreen() {
//  Serial.print("Updating screen with: ");
//  Serial.print(temperature);
//  Serial.print(" ");
//  Serial.print(humidity);
//  Serial.print(" ");
//  Serial.print(uvIndex);
//  Serial.print(" ");
//  Serial.println(pressure);
  
//  if (isnan(humidity) || isnan(temperature) || isnan(uvIndex)) {
//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print("Error reading DHT22");
//
//    return;
//  }
  
  lcd.clear();

  switch (alternator) {
    case 0:
      alternator = 1;

      lcd.setCursor(0, 0);

      lcd.print("T: ");
      lcd.print(temperature, 1);
      lcd.print(" C; ");

      lcd.print("H: ");
      lcd.print(humidity, 1);
      lcd.print(" %; ");

      lcd.setCursor(0, 1);

      lcd.print("P: ");
      lcd.print(pressure, 1);
      lcd.print(" hPa");
  
      break;
    case 1:
      alternator = 0;

      lcd.setCursor(0, 0);

      lcd.print("UV: ");
      lcd.print(uvIndex, 1);
      lcd.print("; ");

      lcd.print("WD: ");
      lcd.print(direction);
      lcd.print(" ");

      lcd.setCursor(0, 1);
      
      lcd.print("WS: ");
      lcd.print(windSpeed, 1);
      lcd.print(" km/h");

      break;
  }
}

void updateScannedNetworks(uint16_t networksFound) {
  uint16_t n = networksFound;

  scannedNetworks.clear();

  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found\n");
      
    for (int i = 0; i < n; ++i) {
      // Save SSID and RSSI for each network found
      
      Serial.println(WiFi.SSID(i));
      Serial.println(WiFi.RSSI(i));

      int type;
      
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:            type = 1; break;
        case WIFI_AUTH_WEP:             type = 2; break;
        case WIFI_AUTH_WPA_PSK:         type = 2; break;
        case WIFI_AUTH_WPA2_PSK:        type = 2; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: type = 3; break;
        default:                        continue;
      }

      struct WifiNetworkListItem item;

      item.ssid = WiFi.SSID(i);
      item.type = type;

      scannedNetworks.push_back(item);
      
      Serial.println("\n");
    }

    removeDuplicateNetworks(scannedNetworks);
  }
}

String escapeQuotes(const String& input) {
    String escaped = input;
    escaped.replace("\"", "\\\"");  // Escape any double quotes in the SSID
    return escaped;
}

String generateItemList() {
    String itemList;
    
    for (size_t i = 0; i < scannedNetworks.size(); i++) {
        String item;
        
        if (i == 0) {
            item = R"({ "ssid": "$SSID", "type": "$TYPE" })";
        } else {
            item = R"(,{ "ssid": "$SSID", "type": "$TYPE" })";
        }

        String escapedSSID = escapeQuotes(scannedNetworks[i].ssid);

        item.replace("$SSID", escapedSSID);
        item.replace("$TYPE", (String)scannedNetworks[i].type);

        itemList += item;
    }

    return itemList;
}

void removeDuplicateNetworks(std::vector<WifiNetworkListItem>& scannedNetworks) {
    // Step 1: Sort by SSID first, then by type (descending) for equal SSIDs
    std::sort(scannedNetworks.begin(), scannedNetworks.end(), [](const WifiNetworkListItem& a, const WifiNetworkListItem& b) {
        if (a.ssid == b.ssid) {
            return a.type > b.type;  // For equal SSIDs, sort by type descending
        }
        return a.ssid < b.ssid;  // Otherwise, sort by SSID
    });

    // Step 2: Remove duplicates (SSID-based) while keeping the highest type
    auto last = std::unique(scannedNetworks.begin(), scannedNetworks.end(), [](const WifiNetworkListItem& a, const WifiNetworkListItem& b) {
        return a.ssid == b.ssid;  // Mark as duplicate if SSID matches
    });

    // Step 3: Erase the duplicates from the vector
    scannedNetworks.erase(last, scannedNetworks.end());
}
