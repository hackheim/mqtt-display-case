#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_RMT_MAX_CHANNELS 1
#define NUM_STRIPS 5
#define NUM_LEDS_PER_STRIP 95

#include <WiFi.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include "FastLED.h"
#include "secrets.h"

//declare array of strips 
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
int value = 0;
char msg[100];

CHSV colorStart = CHSV(128, 255, 255);
CHSV colorTarget = CHSV(1, 255, 255);
CHSV colorCurrent = colorStart;
uint8_t blendRate = 4;

CHSV messageColors[6];
CHSV targetColors[6];
CHSV startColors[6];
CHSV currentColors[6];
CHSV kValues[6];

void test(){

  static uint8_t h = 0;

  EVERY_N_MILLISECONDS(blendRate){

    for(int i = 0; i < NUM_STRIPS; i++){

      fill_solid( leds[i], NUM_LEDS_PER_STRIP, CHSV(h,255,100));

    FastLED.show();
    
    }

    h++;
  } 
}

void colorFade(){

  EVERY_N_MILLISECONDS(blendRate){

    static uint8_t k[NUM_STRIPS];  // the amount to blend [0-255]
    static uint8_t test = 2;

    for(int i = 0; i < NUM_STRIPS; i++){

      if ( currentColors[i].h == targetColors[i].h ) {  // Check if target has been reached
        startColors[i] = currentColors[i];
        targetColors[i] = messageColors[i];
        k[i] = 0;  // reset k value
        
      }

      currentColors[i] = blend(startColors[i], targetColors[i], k[i], SHORTEST_HUES);
      fill_solid( leds[i], NUM_LEDS_PER_STRIP, currentColors[i]);
      k[i]++;
    }

  FastLED.show();
  }
}

void callback(char* topic, byte* message, unsigned int length) {

  for(int i = 0; i < 6; i++){

    messageColors[i].h = message[i * 3 + 0];
    messageColors[i].s = message[i * 3 + 1] - 20;
    messageColors[i].v = message[i * 3 + 2];
  }   
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {

  //Delcare LED-strips and GPIO pins they are connected at
  FastLED.addLeds<WS2812B, GPIO_NUM_32, GRB>(leds[0], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, GPIO_NUM_33, GRB>(leds[1], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, GPIO_NUM_25, GRB>(leds[2], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, GPIO_NUM_26, GRB>(leds[3], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, GPIO_NUM_27, GRB>(leds[4], NUM_LEDS_PER_STRIP);

  //Set the brightness to something sensible
  FastLED.setBrightness(100);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  for(int i = 0; i < 6; i++){
    startColors[i] = CHSV(255, 0, 255);
    currentColors[i] = startColors[i];
    targetColors[i] = CHSV(120,180,255);
    messageColors[i] = targetColors[i];
  }

  FastLED.show();
  delay(1000);
  
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Lighting_Controller", mqtt_user, mqtt_pass)) {

      Serial.println("connected");

      // Subscribe
      client.subscribe("hackheim/lights/hue");
    } 
    
    else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!client.connected()) {

    reconnect();
  }

  client.loop();
  colorFade();

}