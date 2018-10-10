/*
  Done by TATCO.

  Contact us:
  info@tatco.cc

  Release Notes:
  - V1 Created 10 Oct 2015
  - V2 Updated 30 Apr 2016
  - V3 Updated 10 Dec 2016
  - V4 Updated 27 Sep 2017
  - V4.5 Updated 07 Oct 2018

  Tested on:
  - Arduino Yun.
  - Arduino Yun Shield with any of arduino boards(Uno, Leonardo or Mega).
  - Dragino Shield with any of arduino boards(Uno, Leonardo or Mega).

*/

#include <EEPROM.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#include <Process.h>
#include <Servo.h>

#define lcdSize 3 //this will define number of LCD on the phone app
String protectionPassword = ""; //This will not allow anyone to add or control your board.
String boardType;

BridgeServer server;

char pinsMode[54];
int pinsValue[54];
Servo servoArray[54];
String lcd[lcdSize];

unsigned long serialTimer = millis();
byte digitalArraySize,analogArraySize;

void setup() {
  Bridge.begin();
  boardInit();
  server.listenOnLocalhost();
  server.begin();
}

void loop() {
  lcd[0] = "Test 1 LCD";// you can send any data to your mobile phone.
  lcd[1] = "Test 2 LCD";// you can send any data to your mobile phone.
  lcd[2] = analogRead(1);//  send analog value of A1

  BridgeClient client = server.accept();
  if (client) {
    process(client);
    client.stop();
  }
  delay(50);
  update_input();
  print_wifiStatus();
}

void process(BridgeClient client) {
  String command = client.readStringUntil('/');

  if (command == "digital") {
    digitalCommand(client);
  }

  if (command == "pwm") {
    pwmCommand(client);
  }

  if (command == "servo") {
    servoCommand(client);
  }

  if (command == "terminal") {
    terminalCommand(client);
  }

  if (command == "mode") {
    modeCommand(client);
  }

  if (command == "allonoff") {
    allonoff(client);
  }

  if (command == "password") {
    changePassword(client);
  }

  if (command == "allstatus") {
    allstatus(client);
  }
}

void terminalCommand(BridgeClient client) {//Here you recieve data form app terminal
  client.print( "Ok from Arduino " + String(random(1, 100)));
//  String data = client.readStringUntil('/');
//  SerialUSB.println(data);
}

void digitalCommand(BridgeClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
    pinsValue[pin] = value;
    client.print( value);
  }
}

void pwmCommand(BridgeClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    analogWrite(pin, value);
    pinsValue[pin] = value;
    client.print( value);
  }
}

void servoCommand(BridgeClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    servoArray[pin].write(value);
    pinsValue[pin] = value;
    client.print( value);
  }
}

void modeCommand(BridgeClient client) {
  String  pinString = client.readStringUntil('/');
  int pin = pinString.toInt();
  String mode = client.readStringUntil('\r');

  if (mode != "servo") {
    servoArray[pin].detach();
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    pinsMode[pin] = 'o';
    pinsValue[pin] = 0;
    allstatus(client);
  }
  if (mode == "push") {
    pinsMode[pin] = 'm';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus(client);
  }
  if (mode == "schedule") {
    pinsMode[pin] = 'c';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus(client);
  }

  if (mode == "input") {
    pinsMode[pin] = 'i';
    pinsValue[pin] = 0;
    pinMode(pin, INPUT);
    allstatus(client);
  }

  if (mode == "pwm") {
    pinsMode[pin] = 'p';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
    allstatus(client);
  }

  if (mode == "servo") {
    pinsMode[pin] = 's';
    pinsValue[pin] = 0;
    servoArray[pin].attach(pin);
    servoArray[pin].write(0);
    allstatus(client);
  }
}

void changePassword(BridgeClient client) {
  String data = client.readStringUntil('\r');
  protectionPassword = data;
  client.println();
}

void allonoff(BridgeClient client) {
  int value = client.parseInt();
  client.println();

  for (byte i = 0; i <= digitalArraySize; i++) {
    if (pinsMode[i] == 'o') {
      digitalWrite(i, value);
      pinsValue[i] = value;
    }
  }
}
void allstatus(BridgeClient client) {
  String dataResponse;
  dataResponse += F("Status:200 \r\n");
  dataResponse += F("content-type:application/json \r\n\r\n");
  dataResponse += "{";

  dataResponse += "\"m\":[";//m for mode
  for (byte i = 0; i <= digitalArraySize; i++) {
    dataResponse += "\"";
    dataResponse += pinsMode[i];
    dataResponse += "\"";
    if (i != digitalArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"v\":[";//v for value
  for (byte i = 0; i <= digitalArraySize; i++) {
    dataResponse += pinsValue[i];
    if (i != digitalArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"a\":[";//a for analog value
  for (byte i = 0; i <= analogArraySize; i++) {
    dataResponse += analogRead(i);
    if (i != analogArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"l\":[";//l for LCD value
  for (byte i = 0; i <= lcdSize - 1; i++) {
    dataResponse += "\"";
    dataResponse += lcd[i];
    dataResponse += "\"";
    if (i != lcdSize - 1)dataResponse += ",";
  }
  dataResponse += "],";
  dataResponse += "\"t\":\""; //t for Board Type .
  dataResponse += boardType;
  dataResponse += "\",";
  dataResponse += "\"p\":\""; // p for Password.
  dataResponse += protectionPassword;
  dataResponse += "\"";
  dataResponse += "}";
  client.print(dataResponse);
}

void boardInit() {
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
  boardType = "uno";
  digitalArraySize=13;
  analogArraySize=5;
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__SAM3X8E__)
  boardType = "mega";
  digitalArraySize=53;
  analogArraySize=15;
#elif defined(__AVR_ATmega32U4__)
  boardType = "leo";
  digitalArraySize=13;
  analogArraySize=5;
#else
  boardType = "uno";
  digitalArraySize=13;
  analogArraySize=5;
#endif

  for (byte i = 0; i <= digitalArraySize; i++) {
    if (i == 0 || i == 1 ) {
      pinsMode[i] = 'x';
      pinsValue[i] = 'x';
    }
    else {
      pinsMode[i] = 'o';
      pinsValue[i] = 0;
      pinMode(i, OUTPUT);
    }
  }
}

void print_wifiStatus() {
  if (Serial.read() > 0) {
    if (millis() - serialTimer > 2000) {
      Process wifiCheck;
      wifiCheck.runShellCommand("/usr/bin/pretty-wifi-info.lua");
      while (wifiCheck.available() > 0) {
        char c = wifiCheck.read();
        SerialUSB.print(c);
      }
      SerialUSB.println();
    }
    serialTimer = millis();
  }
}

void update_input() {
  for (byte i = 0; i <= digitalArraySize; i++) {
    if (pinsMode[i] == 'i') {
      pinsValue[i] = digitalRead(i);
    }
  }
}
