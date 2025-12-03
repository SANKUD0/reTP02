#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// PID (Pour Mijoteuse)
#include <PID_v1.h>
double Setpoint;
double Input;
double Output;

const double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

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

void setup()
{
  Serial.begin(115200);

  WiFi.softAP(ssid, pwd);
  Serial.print(WiFi.softAPIP()); // Mets en claire l'IP de l'access point

  LittleFS.begin();
  httpd.onNotFound(HandleFileRequest);
  httpd.begin();
}

void loop()
{
  httpd.handleClient(); // le mettre au moins une fois dans le loop pour accéder au site (serveur)
}