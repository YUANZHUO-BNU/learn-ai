 /*
   MQTT RGB Light for Home-Assistant - NodeMCU (ESP8266)
   https://home-assistant.io/components/light.mqtt/
   Libraries :
    - ESP8266 core for Arduino : https://github.com/esp8266 /Arduino
    - PubSubClient : https://github.com/knolleary/pubsubclient
   Sources :
    - File > Examples > ES8266WiFi > WiFiClient
    - File > Examples > PubSubClient > mqtt_auth
    - File > Examples > PubSubClient > mqtt_esp8266
    - http://forum.arduino.cc/index.php?topic=272862.0
   Schematic :
    - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_rgb_light/Schematic.png
    - LED leg 1 - Resistor 270 Ohms - D1/GPIO5
    - LED leg 2 (longuest leg) - GND
    - LED leg 3 - Resistor 270 Ohms - D2/GPIO4
    - LED leg 4 - Resistor 270 Ohms - D3/GPIO0
   Configuration (HA) : 
    light:
      platform: mqtt
      name: 'classroom RGB light'
      state_topic: 'classroom/rgb1/light/status'
      command_topic: 'classroom/rgb1/light/switch'
      brightness_state_topic: 'classroom/rgb1/brightness/status'
      brightness_command_topic: 'classroom/rgb1/brightness/set'
      rgb_state_topic: 'classroom/rgb1/rgb/status'
      rgb_command_topic: 'classroom/rgb1/rgb/set'
      brightness_scale: 100
      optimistic: false
   Samuel M. - v1.1 - 08.2016
   If you like this example, please add a star! Thank you!
   https://github.com/mertenats/open-home-automation
*/
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <FS.h>
#include "FastLED.h"
#include "server.h"
#define NUM_LEDS 3
// pins used for the rgb led


CRGB leds[NUM_LEDS];
void ledInit(){
   FastLED.addLeds<NEOPIXEL, 8>(leds, NUM_LEDS);
  };
void setColor(int red,int green,int blue){
  char i;
  for(i=0;i<NUM_LEDS;i++){
    leds[i] = CRGB(red,green,blue);
    }
    FastLED.show();
  };

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const char* WIFI_SSID = "AI";
const char* WIFI_PASSWORD = "raspberry";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "classroom_rgb_light";
const PROGMEM char* MQTT_SERVER_IP = "192.168.123.45";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "mqttuser";
const PROGMEM char* MQTT_PASSWORD = "bnubnu";

// MQTT: topics
// state
const PROGMEM char* MQTT_LIGHT_STATE_TOPIC = "classroom/rgb1/light/status";
const PROGMEM char* MQTT_LIGHT_COMMAND_TOPIC = "classroom/rgb1/light/switch";

// brightness
const PROGMEM char* MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC = "classroom/rgb1/brightness/status";
const PROGMEM char* MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC = "classroom/rgb1/brightness/set";

// colors (rgb)
const PROGMEM char* MQTT_LIGHT_RGB_STATE_TOPIC = "classroom/rgb1/rgb/status";
const PROGMEM char* MQTT_LIGHT_RGB_COMMAND_TOPIC = "classroom/rgb1/rgb/set";

// payloads by default (on/off)
const PROGMEM char* LIGHT_ON = "ON";
const PROGMEM char* LIGHT_OFF = "OFF";

// variables used to store the state, the brightness and the color of the light
boolean m_rgb_state = false;
uint8_t m_rgb_brightness = 100;
uint8_t m_rgb_red = 255;
uint8_t m_rgb_green = 255;
uint8_t m_rgb_blue = 255;


// buffer used to send/receive data with MQTT
const uint8_t MSG_BUFFER_SIZE = 20;
char m_msg_buffer[MSG_BUFFER_SIZE]; 

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// function called to publish the state of the led (on/off)
void publishRGBState() {
  if (m_rgb_state) {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_ON, true);
  } else {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_OFF, true);
  }
}

// function called to publish the brightness of the led (0-100)
void publishRGBBrightness() {
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_rgb_brightness);
  client.publish(MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC, m_msg_buffer, true);
}

// function called to publish the colors of the led (xx(x),xx(x),xx(x))
void publishRGBColor() {
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_rgb_red, m_rgb_green, m_rgb_blue);
  client.publish(MQTT_LIGHT_RGB_STATE_TOPIC, m_msg_buffer, true);
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  // handle message topic
  if (String(MQTT_LIGHT_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(LIGHT_ON))) {
      if (m_rgb_state != true) {
        m_rgb_state = true;
        setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
        publishRGBState();
      }
    } else if (payload.equals(String(LIGHT_OFF))) {
      if (m_rgb_state != false) {
        m_rgb_state = false;
        setColor(0, 0, 0);
        publishRGBState();
      }
    }
  } else if (String(MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC).equals(p_topic)) {
    uint8_t brightness = payload.toInt();
    if (brightness < 0 || brightness > 100) {
      // do nothing...
      return;
    } else {
      m_rgb_brightness = brightness;
      setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
      publishRGBBrightness();
    }
  } else if (String(MQTT_LIGHT_RGB_COMMAND_TOPIC).equals(p_topic)) {
    // get the position of the first and second commas
    uint8_t firstIndex = payload.indexOf(',');
    uint8_t lastIndex = payload.lastIndexOf(',');
    
    uint8_t rgb_red = payload.substring(0, firstIndex).toInt();
    if (rgb_red < 0 || rgb_red > 255) {
      return;
    } else {
      m_rgb_red = rgb_red;
    }
    
    uint8_t rgb_green = payload.substring(firstIndex + 1, lastIndex).toInt();
    if (rgb_green < 0 || rgb_green > 255) {
      return;
    } else {
      m_rgb_green = rgb_green;
    }
    
    uint8_t rgb_blue = payload.substring(lastIndex + 1).toInt();
    if (rgb_blue < 0 || rgb_blue > 255) {
      return;
    } else {
      m_rgb_blue = rgb_blue;
    }
    
    setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
    publishRGBColor();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("INFO: connected");
      
      // Once connected, publish an announcement...
      // publish the initial values
      publishRGBState();
      publishRGBBrightness();
      publishRGBColor();

      // ... and resubscribe
      client.subscribe(MQTT_LIGHT_COMMAND_TOPIC);
      client.subscribe(MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC);
      client.subscribe(MQTT_LIGHT_RGB_COMMAND_TOPIC);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(115200);

  // init the RGB led
  ledInit();

  //init the server
  SPIFFS.begin();
  server.serveStatic("/", SPIFFS, "/index.html");
  server.on("/set-color", [](){
    String uri = server.uri();
    Serial.println(uri);
    String red = server.arg("red");
    String green = server.arg("green");
    String blue = server.arg("blue");
    Serial.println("red="+red+" green="+green+" blue="+blue);
    setColor(red.toInt(),green.toInt(),blue.toInt());
    server.send(200, "text/plain", String("set to ")+"red="+red+" green="+green+" blue="+blue);
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  
  analogWriteRange(255);
  setColor(0, 0, 0);

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();
}
