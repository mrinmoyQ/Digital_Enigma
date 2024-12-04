#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define totalWheels 3
#define totalElements 26
#define debug false

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int wheels[3][26] = {
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 }
};

String seeds[3] = {
  "mantu",
  "loves",
  "girls"
};

int btED = 13;
int btABC = 12;
int btDEF = 11;
int btGHI = 10;
int btJKL = 9;
int btMNO = 8;
int btPQRS = 7;
int btTUV = 6;
int btWXYZ = 5;

int mode = 1;               // 1 - encode | 2 - decode
int lastButtonPressed = 0;  // time

int lastButtonNo = 0;
int buttonIndex = 0;

char alphabets[8][4] = {
  { 'A', 'B', 'C', '-' },  // 2
  { 'D', 'E', 'F', '-' },  // 3
  { 'G', 'H', 'I', '-' },  // 4
  { 'J', 'K', 'L', '-' },  // 5
  { 'M', 'N', 'O', '-' },  // 6
  { 'P', 'Q', 'R', 'S' },  // 7
  { 'T', 'U', 'V', '-' },  // 8
  { 'W', 'X', 'Y', 'Z' }   // 9
};


int buttonStatus = 0;
bool handlingISR = false;
int lastDisplayRefreshed = 0;
char lastResult = ' ';


void setup() {
  pinMode(btED, INPUT_PULLUP);
  pinMode(btABC, INPUT);
  pinMode(btDEF, INPUT);
  pinMode(btGHI, INPUT);
  pinMode(btJKL, INPUT);
  pinMode(btMNO, INPUT);
  pinMode(btPQRS, INPUT);
  pinMode(btTUV, INPUT);
  pinMode(btWXYZ, INPUT);

  Serial.begin(115200);


  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();

  PCICR |= B00000101;   // Enable Port B & D
  PCMSK0 |= B00111111;  // Enable D13 -> D8
  PCMSK2 |= B11100000;  // Enable D7 -> D5

  lastButtonPressed = millis();
  lastDisplayRefreshed = millis();

  generateSeeds();
  
  printWheels();
}

void loop() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

  display.setCursor(24, 1);
  display.write("DIGITAL ENIGMA");
  display.display();

  refreshScreen();
  if (handlingISR) return;
  int timeElapsed = millis() - lastButtonPressed;
  if (timeElapsed > 1200) {
    int copyBtn = lastButtonNo;
    int copyIndex = buttonIndex;
    lastButtonNo = 0;
    buttonIndex = 0;
    if (copyBtn != 0) {
      char character = alphabets[copyBtn - 2][copyIndex];
      processInput(character);
    }
  }
  if (Serial.available() == 0) {
    return;
  }
  char character = Serial.read();
  int characterInt = int(character);
  if (!(character >= 65 && character <= 90)) {
    return;
  }
  characterInt = characterInt - 65;
  // encode(characterInt);
  decode(characterInt);
}

void generateSeeds(){
  int dumpWheels[3][26] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 }
  };
  for (int i=0; i<totalWheels; i++){
    for (int j=0; j<totalElements; j++) {
      wheels[i][j] = dumpWheels[i][j];
    }
  }
  for (int i = 0; i < totalWheels; i++) {
    srand(generateSeed(seeds[i]));

    for (int j = 0; j < totalElements; j++) {
      int k = rand() % (j + 1);
      int temp = wheels[i][j];
      wheels[i][j] = wheels[i][k];
      wheels[i][k] = temp;
    }
  }
}
void printWheels() {
  if (!debug) {
    return;
  }
  for (int i = 0; i < totalWheels; i++) {
    for (int j = 0; j < totalElements; j++) {
      Serial.print(wheels[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void rotateWheel() {
  for (int j = 0; j < totalWheels; j++) {
    int firstElement = wheels[j][0];
    for (int i = 0; i < totalElements - 1; i++) {
      wheels[j][i] = wheels[j][i + 1];
    }
    wheels[j][totalElements - 1] = firstElement;
  }
  printWheels();
}

int generateSeed(const String& word) {
  int seed = 0;
  for (int i = 0; i < word.length(); i++) {
    seed = seed * 50 + word[i];
  }
  return seed;
}

void showLetter(int characterInt) {
  char c = (characterInt + 65);
  lastResult = c;
}

void encode(int userInput) {
  int result = userInput;
  for (int i = 0; i < totalWheels; i++) {
    result = wheels[i][result];
  }
  showLetter(result);
  rotateWheel();
}

void decode(int userInput) {
  int position = userInput;
  for (int j = totalWheels - 1; j >= 0; j--) {

    for (int i = 0; i < totalElements; i++) {
      if (position == wheels[j][i]) {
        position = i;
        break;
      }
    }
  }
  showLetter(position);
  rotateWheel();
}


void processInput(char character) {

  int a = int(character) - 65;
  if (mode == 1) {
    encode(a);
  }else if(mode == 2){
    decode(a);
  }
}

// keyboard and display-oled handler

void refreshScreen() {
  if (millis() - lastDisplayRefreshed < 200) {
    return;
  }

  // mode
  display.setCursor(0 , 14);
  display.setTextColor(WHITE, BLACK);
  if (mode == 1) {
    display.write("ENCODE : ");
  } else if (mode == 2) {
    display.write("DECODE : ");
  }
  display.display();
  display.setCursor(0 , 25);
  display.write("CODE : ");
  display.display();
  // tmp character
  if (lastButtonNo >= 2) {
    display.setCursor(55 , 14);
    display.write(alphabets[lastButtonNo - 2][buttonIndex]);
  }
  // print result
  if (lastResult != ' ') {
    display.setCursor(43 , 25);
    display.write(lastResult);
  }
  lastDisplayRefreshed = millis();
}


ISR(PCINT0_vect) {  // D13 -> D8
  handleButtonISR();
}

ISR(PCINT2_vect) {  // D7 -> D0
  handleButtonISR();
}

void handleButtonISR() {
  handlingISR = true;
  int buttonNo = whichButtonPressed();
  if (buttonNo == -1) {
    handlingISR = false;
    return;
  }
  int currentTime = millis();
  int totalTimeEllapsed = currentTime - lastButtonPressed;
  if (totalTimeEllapsed < 200) {
    handlingISR = false;
    return;
  }
  if (buttonNo == 1) {
    if (mode == 1) {
      mode = 2;
    } else if (mode == 2) {
      mode = 1;
    }
    generateSeeds();
    // TODO reset state
    lastButtonNo = 0;
    handlingISR = false;
    return;
  }

  if (buttonNo == lastButtonNo && totalTimeEllapsed < 1000) {
    buttonIndex += 1;
    if (buttonIndex >= 4) {
      buttonIndex = 0;
    }
    if (buttonIndex == 3 && !(lastButtonNo == 7 || lastButtonNo == 9)) {
      buttonIndex = 0;
    }
  } else {
    lastButtonNo = buttonNo;
    buttonIndex = 0;
  }
  lastButtonPressed = currentTime;
  handlingISR = false;
}

int whichButtonPressed() {
  if (digitalRead(btED) == 1) {
    return 1;
  } else if (digitalRead(btABC) == 1) {
    return 2;
  } else if (digitalRead(btDEF) == 1) {
    return 3;
  } else if (digitalRead(btGHI) == 1) {
    return 4;
  } else if (digitalRead(btJKL) == 1) {
    return 5;
  } else if (digitalRead(btMNO) == 1) {
    return 6;
  } else if (digitalRead(btPQRS) == 1) {
    return 7;
  } else if (digitalRead(btTUV) == 1) {
    return 8;
  } else if (digitalRead(btWXYZ) == 1) {
    return 9;
  } else {
    return -1;
  }
}