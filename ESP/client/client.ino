#include <WiFi.h>


const char* WIFI_SSID = "Kivaplusvite";
const char* WIFI_PASS = "souponpochon";
const char* HOST      = "192.168.1.190";   // adresse du Mac
const uint16_t PORT   = 4242;

WiFiClient client;
bool subbed = false;

struct Message {
  String room, device, action, options;
};

// Analyse "PUB room/device/action[/options]".
bool parse_pub(String line, Message &m) {
  line.trim();                                  // retire \r et \n
  if (!line.startsWith("PUB ")) return false;

  String corps = line.substring(4);

  int s1 = corps.indexOf('/');
  int s2 = corps.indexOf('/', s1 + 1);
  if (s1 <= 0 || s2 < 0) return false;          // il faut au moins 3 champs

  int s3 = corps.indexOf('/', s2 + 1);          // -1 s'il n'y a pas d'options

  m.room   = corps.substring(0, s1);
  m.device = corps.substring(s1 + 1, s2);
  m.action = (s3 < 0) ? corps.substring(s2 + 1)
                      : corps.substring(s2 + 1, s3);
  m.options = (s3 < 0) ? "" : corps.substring(s3 + 1);

  return m.room.length() && m.device.length() && m.action.length();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);                    // l'ESP est cliente du Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) { // attente de la connexion Wi-Fi
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nIP de la carte : ");
  Serial.println(WiFi.localIP());

  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);

  
}

void loop() {
  // (re)connexion au serveur si nécessaire
  if (!client.connected()) {
    subbed = false;
    Serial.println("Connexion au serveur...");
    if (!client.connect(HOST, PORT)) {    // socket() + connect()
      Serial.println("Echec, nouvel essai dans 2 s");
      digitalWrite(D0, !digitalRead(D0));
      delay(1000);
      digitalWrite(D0, !digitalRead(D0));
      delay(1000);
      return;
    }
    Serial.println("Connecte.");
  }

  if (!subbed){
    client.write("SUB salon/*\n");
    client.write("SUB cuisine/*\n");

    client.write("LST\n");
    subbed = true;
  }

  if (client.available()) {                       // des octets sont arrivés
    String cmd = client.readStringUntil('\n');    // lit jusqu'au délimiteur
    cmd.trim();   // retire d'éventuels '\r'
    Serial.println(cmd);

    Message m;
    if (parse_pub(cmd, m)) {
      Serial.printf("room=%s device=%s action=%s options=%s\n",
                    m.room.c_str(), m.device.c_str(),
                    m.action.c_str(), m.options.c_str());
      if (m.room == "salon" && m.device =="lumiere1" && m.action == "toggle")  { 
        digitalWrite(D0, !digitalRead(D0));
      }
      if (m.room == "cuisine" && m.device =="lumiere1" && m.action == "toggle")  { 
        digitalWrite(D1, !digitalRead(D1));
      }
    }
  }
}
