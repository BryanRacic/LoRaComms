#include <M5Stack.h>
#include <Wire.h>
#include "LoRa_E32.h"

#define CARDKB_ADDR 0x5F

// Global Vars
String inputText = "";
String prevInput = "";
String messageList[512];
int buttonState;
int messageListCount = 0;
int menuState = 0;
int prevCount = 1;  // Val should increment at 1
#include <SoftwareSerial.h>
SoftwareSerial mySerial(16, 17);  // e32 TX e32 RX
//LoRa_E32 e32ttl(&mySerial, 5, 7, 6);
LoRa_E32 e32ttl100(&Serial2, 2, 5, 25);
// User Customization
String userName = "BRYAN";
int bryanColor = 0x07E0;
int reaganColor = 0x001F;
int christianColor = 0xFFE0;
// Special Chars for Keyboard
int KEY_DEL = 8;
int KEY_ENTER = 13;

void setup() {
  M5.begin();
  e32ttl100.begin();
  ResponseStructContainer c;
  c = e32ttl100.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;
  configuration.ADDL = 3;
  configuration.ADDH = 0;
  configuration.CHAN = 0x04;
  configuration.OPTION.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
  configuration.OPTION.wirelessWakeupTime = WAKE_UP_250;

  configuration.OPTION.fec = FEC_1_ON;
  configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS;
  configuration.OPTION.transmissionPower = POWER_20;

  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;

  e32ttl100.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  c.close();
  Serial.begin(115200);
  delay(500);

  Wire.begin();
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);

  // For testing only
  //fillListWithDebug();
  //messageListCount = 67;
  ///////////////////
  initHomeScreen();

  // Startup all pins and UART
  e32ttl100.begin();


  // Send message
  ResponseStatus rs = e32ttl100.sendMessage("INIT MESSAGE");
  // Check If there is some problem of succesfully send
  Serial.println(rs.getResponseDescription());
}

void loop() {
  buttonState = checkButtonStatus();
  if (menuState == 0) {
    homeButtonCheck();
  } else if (menuState = 1) {
    sendButtonCheck();
    typingFunction();
  }
  messageCheck();
}

void initHomeScreen() {
  M5.Lcd.fillScreen(BLACK);
  drawUsername();
  drawMessageList();
  drawHomeMenu();
}

void drawUsername() {
  int cursorX = 300;
  int cursorY = 10;
  int fontNum = 2;
  cursorX = cursorX - (M5.Lcd.textWidth(userName));
  M5.Lcd.setTextColor(getUnameColor());
  M5.Lcd.drawString(userName, cursorX, cursorY, fontNum);
}

int getUnameColor() {
  if (userName == "BRYAN") {
    return bryanColor;
  }
  if (userName == "CHRISTIAN") {
    return christianColor;
  }
  if (userName == "REAGAN") {
    return reaganColor;
  }
  return 0xC618;
}

void drawMessageList() {
  int cursorX = 10;
  int cursorY = 10;
  int numToDisp = 12;
  int startIndex;
  int prevMenuOffset = messageListCount - (numToDisp * prevCount);
  Serial.println("Message List Count: " + String(messageListCount));
  Serial.println("Prev Menu Offset: " + String(prevMenuOffset));
  if (messageListCount < numToDisp) {
    startIndex = 0;
  } else if (prevMenuOffset < 0) {
    startIndex = 0;
  } else {
    startIndex = prevMenuOffset;
  }
  for (int i = startIndex; i < (startIndex + numToDisp); i++) {
    setLogColor(messageList[i]);
    M5.Lcd.drawString(messageList[i], cursorX, cursorY, 2);
    cursorY = cursorY + 15;
  }
}

void setLogColor(String inMessage){
  if (inMessage.indexOf("BRYAN") != -1){
    M5.Lcd.setTextColor(bryanColor);
    return;
  } else if (inMessage.indexOf("REAGAN") != -1){
    M5.Lcd.setTextColor(reaganColor);
    return;
  } else if (inMessage.indexOf("CHRISTIAN") != -1){
    M5.Lcd.setTextColor(christianColor);
    return;
  } else {
    M5.Lcd.setTextColor(WHITE);
    return;
  }
}

void drawHomeMenu() {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString("HOME", 50, 215, 2);
  M5.Lcd.drawString("SEND", 140, 215, 2);
  M5.Lcd.drawString("PREV", 240, 215, 2);
}

int checkButtonStatus() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    return 0;
  } else if (M5.BtnB.wasPressed()) {
    return 1;
  } else if (M5.BtnC.wasPressed()) {
    return 2;
  }
  return -1;
}

void homeButtonCheck() {
  switch (buttonState) {
    case 0:
      menuState = 0;
      prevCount = 1;
      initHomeScreen();
      break;
    case 1:
      menuState = 1;
      prevCount = 1;
      initSendScreen();
      break;
    case 2:
      prevCount++;
      initHomeScreen();
      break;
    default:
      break;
  }
}

void sendButtonCheck() {
  switch (buttonState) {
    case 0:
      menuState = 0;
      prevCount = 1;
      initHomeScreen();
      break;
    default:
      break;
  }
}

void initSendScreen() {
  M5.Lcd.fillScreen(BLACK);
  drawUsername();
  drawSendMenu();
}

void drawSendMenu() {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString("HOME", 50, 215, 2);
}

void typingFunction() {
  prevInput = inputText;
  Wire.requestFrom(CARDKB_ADDR, 1);
  if (Wire.available()) {
    inputText = readKB(inputText);
    if (prevInput != inputText) {
      if (inputText == "") {
        menuState = 0;
        prevCount = 1;
        initHomeScreen();
      } else {
        M5.Lcd.clear();
        initSendScreen();
        dispTypeText(inputText);
      }
    }
  } else {
    Serial.println("ERROR: Wire not available!!!");
  }
}

String readKB(String prevInpt) {
  char c = Wire.read();  // receive a byte as character
  {
    if (c == KEY_DEL) {
      prevInpt.remove(prevInpt.length() - 1);
    } else if (c == KEY_ENTER) {
      sendLoraMessage(prevInput);
      prevInpt = "";
    } else if (c != 0) {
      prevInpt.concat(c);
    }
  }
  return prevInpt;
}

void messageCheck() {
  if (e32ttl100.available() > 1) {
    ResponseContainer rs = e32ttl100.receiveMessage();
    String message = rs.data;  // First ever get the data
    Serial.println("Message Reciveved");
    Serial.println(rs.status.getResponseDescription());
    Serial.println(message);
    if(message.indexOf("ACK") != -1){
      // pass
    } else {
      sendLoraMessage("ACK");
    }
    messageList[messageListCount] = message;
    messageListCount++;
    initHomeScreen();
  }
}

void sendLoraMessage(String newMessage) {
  newMessage = userName + ": " + newMessage;
  messageList[messageListCount] = newMessage;
  messageListCount++;
  //ResponseStatus rs = e32ttl100.sendMessage(message);
  ResponseStatus rs = e32ttl100.sendMessage(newMessage);
  Serial.println("Sent message: " + newMessage);
  Serial.println(rs.getResponseDescription());
  menuState = 0;
  prevCount = 1;
  initHomeScreen();
}

void dispTypeText(String text) {
  int startX = 160;
  int startY = 100;
  int size = 4;
  int inputTextSize = text.length() * (size * 2);
  M5.Lcd.drawString(text, startX - inputTextSize, 100, size);
}
