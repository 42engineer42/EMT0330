/*
 * IoT Servo example
 *
 * This example subscribe to the "servo" topic. When a message received, then it
 * change servo position
 *
 * Created 11 Sept 2017 by Heiko Pikner
 */
 
// Includes global variables and librarys that the servo motor uses
#include <Arduino.h>
// Servo library differs per platform.
#if defined(ARDUINO_ARCH_ESP32)
  #include <ESP32Servo.h> // Provides Servo class on ESP32
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <Servo.h> // ESP8266 core ships with Servo
#else
  #include <Servo.h>
#endif
//#include <ittiot.h>
 
#define MODULE_TOPIC "ESP30/servo"
#define WIFI_NAME "name"
#define WIFI_PASSWORD "password"
 
// Pin definition for the Servo.
// - ESP8266 boards often use Dx aliases (e.g., D3).
// - ESP32 boards use raw GPIO numbers.
#if defined(ARDUINO_ARCH_ESP32)
  #define SERVO_PIN 13
#else
  #define SERVO_PIN D3
#endif
 
// ESP32Servo provides the same Servo class name as the AVR Servo library.
Servo myservo;  // create servo object to control a servo
 
// Change the servo position (value between 0 and 180) when a message has been received
// mosquitto_pub -u test -P test -t "RL/esp-{ESP-ID}/servo" -m "51" = this calls servo motor to change position
/*
void iot_received(String topic, String msg)
{
  Serial.print("MSG FROM USER callback, topic: ");
  Serial.print(topic);
  Serial.print(" payload: ");
  Serial.println(msg);
 
  if(topic == MODULE_TOPIC)
  {
    myservo.write(msg.toInt());
  }
}
 
// Function started after the connection to the server is established
void iot_connected()
{
  Serial.println("MQTT connected callback");
 
  iot.subscribe(MODULE_TOPIC); // Subscribe to the topic "servo"
  iot.log("IoT Servo example!");
}
*/
void setup()
{
  Serial.begin(115200); // Setting up serial connection parameter
  delay(3000); // Short delay to ensure serial connection is established
  Serial.println("Booting");
 
  //iot.setConfig("wname", WIFI_NAME);
  //iot.setConfig("wpass", WIFI_PASSWORD);
  //iot.printConfig(); // Print json config to serial
  //iot.setup(); // Initialize IoT library
 
  myservo.attach(SERVO_PIN); // Setting the servo control pin
}
 
void loop()
{
  myservo.write(0);
  delay(150);
  myservo.write(180);
  delay(150);
  //iot.handle(); // IoT behind the plan work, it should be periodically called  
}
