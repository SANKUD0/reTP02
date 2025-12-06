#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// PID (Pour Mijoteuse)
#include <PID_v1.h>

#define PIN_ACTIVATION D1
#define PIN_TEMP A0

double Setpoint = 43.0;
double Input;
double Output;

const double Kp = 6, Ki = 0.1, Kd = 30;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

bool MIJ_STATUS = false;

unsigned long last_ms = 0;

// ---- Pour faire des requêtes HTTP ----
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

ESP8266WebServer httpd(80);

const char *ssid = "RETP022025";
const char *pwd = "RETP022025";

String GetContentType(String filename)
{
  struct Mime
  {
    String ext, type;
  } mimeTypes[] = {
      {".html", "text/html"},
      {".js", "application/javascript"},
      {".css", "text/css"},
  };

  for (unsigned int i = 0; i < sizeof(mimeTypes) / sizeof(Mime); i++)
    if (filename.endsWith(mimeTypes[i].ext))
      return mimeTypes[i].type;

  return "application/octet-stream";
}

void HandleFileRequest()
{
  String filename = httpd.uri();
  if (filename.endsWith("/"))
    filename += "index.html";
  if (!LittleFS.exists(filename))
  {
    httpd.send(404, "text/plain", "NOT FOUND");
    return;
  }

  File file = LittleFS.open(filename, "r");

  httpd.sendHeader("Cache-Control", "max-age=31536000, immutable");

  size_t sent = httpd.streamFile(file, GetContentType(filename));
  Serial.printf("[HTTP] %s: size=%u, sent=%u\n", filename.c_str(), (unsigned)file.size(), (unsigned)sent);

  file.close();
}

void HandleGetStatus()
{
  String status = MIJ_STATUS ? "on" : "off";
  httpd.send(200, "text/plain", status.c_str());
}

void applyChange(bool on)
{
  digitalWrite(PIN_ACTIVATION, on ? (int)Output : 0);
}

void HandleChangeStatus()
{
  String response;
  if (MIJ_STATUS)
  {
    MIJ_STATUS = false;
    response = "off";
    // debug
    Serial.println("Mijoteuse [OFF]");
  }
  else
  {
    MIJ_STATUS = true;
    response = "on";
    // debug
    Serial.println("Mijoteuse [ON]");
  }
  httpd.send(200, "text/plain", response.c_str());
}

double TemperatureCelcius()
{
  int valeur = analogRead(PIN_TEMP);
  double voltage = valeur * 5.0 / 1023.0;
  double resistance = 10000.0 * voltage / (5.0 - voltage);
  double temperatureKelvin = 1 / (1 / 298.15 + log(resistance / 10000.0) / 3977);
  return temperatureKelvin - 273.15;
}

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_ACTIVATION, OUTPUT);
  applyChange(false);

  
  WiFi.softAP(ssid, pwd);
  Serial.println(WiFi.softAPIP()); // Mets en claire l'IP de l'access point
  
  myPID.SetOutputLimits(0 ,1023);
  myPID.SetSampleTime(1000);
  myPID.SetMode(MANUAL);

  // Request web server
  LittleFS.begin();

  httpd.on("/GetStatus", HandleGetStatus);
  httpd.on("/ChangeStatus", HandleChangeStatus);
  // httpd.on("/Temperatures", HandleTemperature);

  last_ms = millis();

  httpd.onNotFound(HandleFileRequest);
  httpd.begin();
}

void loop()
{
  httpd.handleClient(); // le mettre au moins une fois dans le loop pour accéder au site (serveur)
  if (millis() - last_ms > 1000UL)
  {
    last_ms = millis();
    if (MIJ_STATUS)
    {
      Input = TemperatureCelcius();
      myPID.Compute();
      applyChange(true);
      myPID.SetMode(AUTOMATIC);      
      
      double pct = (Output / 1023) * 100;

      Serial.print("Temp = ");
      Serial.print(Input);
      Serial.print(" | Puissance = ");
      Serial.print(pct);
      Serial.println(" %");

    }
    else
    {
      applyChange(false);
      Serial.println("La mijoteuse en éteint !");
    }
  }
}