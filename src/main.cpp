#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include <string>

#include "types.h"

#define SETUP_TRIGGER_PIN 5
#define AHC_VERSION "0.1.2"

int isConfigMode = 4;
eepConfigData_t cfg;

WiFiClient espClient;
PubSubClient client(espClient);

void saveConfigCallback () {

}

int cfgStart = 256;

void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  EEPROM.begin(4095);
  for (int i = cfgStart ; i < sizeof(cfg) ; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();
}

void saveConfig() {
  // Save configuration from RAM into EEPROM
  EEPROM.begin(4095);
  EEPROM.put( cfgStart, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
  EEPROM.end();                         // Free RAM copy of structure
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.begin(4095);
  EEPROM.get( cfgStart, cfg );
  EEPROM.end();
}

void sendToOutChannel(String message)
{
  char channel_out[69];
  strcpy(channel_out, cfg.MQTTChannel);
  strcat(channel_out, "/output");
  client.publish(channel_out, message.c_str());
}

void MQTTCallback(char* topic, byte* payload, unsigned int length) {

  char msg[length+1];
  for(int i=0; i<length; i++)
  {
    msg[i] = payload[i];
  }
  msg[length] = '\0';

  short device = msg[1] - 48;
	device += ( msg[0] - 48 ) * 10;

  Serial.println(device);

  if (msg[2] == 'o' && msg[3] == 'n')
  {
    if (device == 0)
    {
      digitalWrite(13, HIGH);
    }
    if (device == 1)
    {
      digitalWrite(12, HIGH);
    }
    Serial.println("Putting output for device on.");
  }
  delay(5);
  if (msg[2] == 'o' && msg[3] == 'f')
  {
    if (device == 0)
    {
      digitalWrite(13, LOW);
    }
    if (device == 1)
    {
      digitalWrite(12, LOW);
    }
    Serial.println("Putting output for device off.");
  }
  delay(5);
  if (msg[2] == 's' && msg[3] == 't')
  {
    sendToOutChannel("Version:");
    sendToOutChannel(AHC_VERSION);
    char stat [1];

    char dev0 [10];
    strcpy(dev0, "dev0:");
    itoa(digitalRead(13), stat, 10);
    strcat(dev0, stat);
    sendToOutChannel(dev0);

    char dev1 [10];
    strcpy(dev1, "dev1:");
    itoa(digitalRead(12), stat, 10);
    strcat(dev1, stat);
    sendToOutChannel(dev1);
  }

  delay(5);
}

// Blocking funktion until wifi is connected!
void setup_wifi() {
	//WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Autoconnect to WLAN.");
    WiFi.begin();
		delay(5000);
	}
  Serial.println("Autoconnect Successed.");
}

void reconnect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		// Attempt to connect
	    char name [31];
	    itoa(ESP.getChipId(), name, 10);
	    char channel_out[69];
	    strcpy(channel_out, cfg.MQTTChannel);
	    strcat(channel_out, "/status");
	
		if (client.connect(name), channel_out, 2, true, "device offline (testament)") {
		    delay(50);
		
			// Once connected, publish an announcement...
		    client.publish(channel_out, "device online");
		    delay(50);
			// ... and resubscribe
			client.subscribe(cfg.MQTTChannel);
		}
		else {
			// Wait 2 seconds before retrying
			delay(2000);
		}
	}
}

void setup()
{
  Serial.begin(115200);
  Serial.println("starting...");
  pinMode(SETUP_TRIGGER_PIN, INPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.println(digitalRead(SETUP_TRIGGER_PIN));

  delay(500);
  // is configuration portal requestetd?
  if (digitalRead(SETUP_TRIGGER_PIN) == HIGH)
  {
    WiFi.persistent(true);
    Serial.println("entering setup...");
    eraseConfig();
    Serial.println("Configuration erased..");
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", cfg.MQTTHost, 16);
    WiFiManagerParameter custom_mqtt_channel("channel", "mqtt channel", cfg.MQTTChannel, 62);

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_channel);

    if (!wifiManager.startConfigPortal("AderetHomeControl Config")) {
      //TODO: error handling if config cannot be loadet. can this happen?
    }
    //TODO: save and reboot
    Serial.println("Configuration done! saving and reboot...");
    strcpy(cfg.MQTTHost, custom_mqtt_server.getValue());
    strcpy(cfg.MQTTChannel, custom_mqtt_channel.getValue());


    saveConfig();
    ESP.restart();
  }
  else
  {
    loadConfig();
    Serial.println("loadet Config...");
    Serial.print("MQTT Host: ");
    Serial.println(cfg.MQTTHost);
    Serial.print("MQTT Channel: ");
    Serial.println(cfg.MQTTChannel);
    setup_wifi();
    client.setServer(cfg.MQTTHost, 1883);
    client.setCallback(MQTTCallback);
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    setup_wifi();
  //Serial.println(digitalRead(SETUP_TRIGGER_PIN));
  //delay(500);
  if (!client.connected()) {
		reconnect();
	}
	client.loop();

}
