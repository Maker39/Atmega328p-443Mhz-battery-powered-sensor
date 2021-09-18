// Прототип беспроводного датчика на Atmega328p (Arduino NANO v3.0)c питанием от 3х1.5V
// Передача по протоколу RC Switch на ESP с прошивкой https://wifi-iot.com/
// Arduino 1.8.15
// ToDo: 
//   +   SHT31 SDA-pin ADC5, SCL-pin ADC4
//   +   пeреход на голую ATMega328P 8MHz Internal clock - работает (Atmega328p PowerDown=4.5uA)https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoToBreadboard
//   +   добавление конфигурации через дефайны
//
//    etc....
//
//
//
//
// Start on
// 08.2021
// Maker39

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <DHT.h> // можно убрать если не DHT22
#include "Adafruit_SHT31.h"
#include <RCSwitch.h>

// Настройка конфига
#define DEBUG  //+ UART, своё время сна
//#define ROOM_DHT22  // DHT22
#define OUTDOOR   //SHT31
  
/* hardware configuration */
#define ACCESSORIESPOWERPIN 3

RCSwitch mySwitch = RCSwitch();

  
#if defined (ROOM_DHT22)
#define DHTTYPE DHT22
#define DHTPIN 4
#define TCORRECTION (0)
#define HCORRECTION (0)

#elif defined (OUTDOOR)
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

#ifdef DEBUG
#define SLEEPDURATION 60 //in sec, sleep duration in seconds, shall be a factor of 8
#else
#define SLEEPDURATION 320 //sleep duration in seconds, shall be a factor of 8
#endif

int sensorPin = A0;    // select the ADC input pin
int sensorValue = 0;  // variable to store the value coming from the sensor
#ifdef ROOM_DHT22
DHT dht(DHTPIN, DHTTYPE);
#endif
void sleepFor8Secs()
{
  // disable ADC
  ADCSRA &= ~ (1 << ADEN); // Это работает
  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval 
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
  wdt_reset();  // pat the dog

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  sleep_cpu ();  

  // cancel sleep as a precaution
  sleep_disable();
}

// watchdog interrupt
ISR (WDT_vect) 
{
  wdt_disable();  // disable watchdog
}  // end of WDT_vect


void setup () 
{
  // Optional set pulse length.
  // mySwitch.setPulseLength(200);
  
  // Optional set number of transmission repetitions.
  // mySwitch.setRepeatTransmit(15);

  analogReference(INTERNAL);// internal Vref 1,1v
  mySwitch.enableTransmit(10);// pin 10 out to transmitter
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Entered setup");  
#endif

#ifdef ROOM_DHT22
  dht.begin();
#else  
  sht31.begin(0x44);  // or 
#endif
    
//  pinMode(ACCESSORIESPOWERPIN, OUTPUT);

#ifdef DEBUG
  Serial.println("Sensor id:");  
  
#endif

}

void loop()
{
//  pinMode(ACCESSORIESPOWERPIN, OUTPUT);
//  digitalWrite(ACCESSORIESPOWERPIN, HIGH); //turn on the DHT sensor and the transmitter
//  delay(2000); //sleep till the intermediate processes in the accessories are settled down

  // enable ADC
   ADCSRA |= (1 << ADEN);
#ifdef ROOM_DHT22
  float humidity = dht.readHumidity() + HCORRECTION;
  float temperature = dht.readTemperature() + TCORRECTION;
#else
  float temperature = sht31.readTemperature();
  float humidity = sht31.readHumidity();
#endif
  sensorValue = analogRead(sensorPin);
//  digitalWrite(ACCESSORIESPOWERPIN, LOW); // turn off the accessories power
//  pinMode(ACCESSORIESPOWERPIN, INPUT); //change pin mode to reduce power consumption
#ifdef ROOM_DHT22
  digitalWrite(DHTPIN, HIGH); // turn off the DHT DATA resistor попытка убрать +12uA потребления через резистор подтяжки
  pinMode(DHTPIN, INPUT); //change pin mode to reduce power consumption
#endif

#ifdef DEBUG
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("ADC:");
  Serial.println(sensorValue);
  
  /* blink the LED to indicate that the readings are done */
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
#endif

// отправка показаний 
   mySwitch.send((temperature)*10+21500, 24);//для WiFi-iot RC Temp
  delay(200);  
  mySwitch.send((humidity)*10+22000, 24);//для WiFi-iot RC Hum
  delay(200);  
  mySwitch.send((sensorValue)+23000, 24);//для WiFi-iot RC ADC
  delay(200);
  for (int i = 0; i < SLEEPDURATION / 8; i++)
    sleepFor8Secs();
}
