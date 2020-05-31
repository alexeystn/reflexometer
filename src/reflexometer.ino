#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

#define LED_A 9
#define LED_B 11

#define BUTTON_A 8
#define BUTTON_B 10
#define BUTTON_C 12

#define ANSWER_A 0
#define ANSWER_B 1

#define ANSWER_EMPTY 0
#define ANSWER_RIGHT 1
#define ANSWER_WRONG 2

#define REACTION_THRESHOLD 1000
#define REACTION_WAIT_MIN  2000
#define REACTION_WAIT_MAX  4000 

#define SMALL_FONT &FreeSans9pt7b
#define LARGE_FONT &FreeSans12pt7b

#define DELAY     delay(duration)
#define LED_A_ON  digitalWrite(LED_A, 1);
#define LED_A_OFF digitalWrite(LED_A, 0);
#define LED_B_ON  digitalWrite(LED_B, 1);
#define LED_B_OFF digitalWrite(LED_B, 0);

enum program_t { P_REACTION, P_TIME_SHIFT, P_ON_TIME, P_OFF_TIME };

#define STATS_LENGTH 10
uint16_t statsReaction[STATS_LENGTH] = {0};
uint8_t statsDelay[STATS_LENGTH] = {0};
uint8_t statsPointer = 0;

uint16_t durationList[] = {200, 150, 120, 100, 80, 70, 50, 
                            40, 35, 30, 25, 20, 15, 12, 10, 
                            8, 6, 5, 4, 3, 2, 1};
uint8_t durationPointer = 6;
#define DURATION_LIST_LENGTH sizeof(durationList)/sizeof(durationList[0])

typedef struct {
  char name[20];
  program_t program;
} menu_item_t;

#define MENU_LENGTH       4

menu_item_t menu[MENU_LENGTH] = {
  {"Reaction", P_REACTION},
  {"Time shift", P_TIME_SHIFT}, 
  {"On duration", P_ON_TIME}, 
  {"Off duration", P_OFF_TIME}  
};

program_t currentProgram;

Adafruit_SSD1306 display(128, 64, &Wire, -1);


void setup(void) {

  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BUTTON_A, INPUT);
  pinMode(BUTTON_B, INPUT);
  pinMode(BUTTON_C, INPUT);

  digitalWrite(BUTTON_A, 1);
  digitalWrite(BUTTON_B, 1);
  digitalWrite(BUTTON_C, 1);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.display();

}


void loop(void) {
  currentProgram = navigateMenu();  
  switch (currentProgram) {
    case P_REACTION:
      runReactionTest();
      break;
    default:
      runDurationTest();
      break;
  }
  LED_A_OFF;
  LED_B_OFF;
  display.clearDisplay();
  display.display();
  while (!allButtonsRelased()) {
    delay(100);
  }
}


program_t navigateMenu(void) {
  static uint8_t menuPointer = 0;
  while (1) {
    if (isButtonPressed(BUTTON_A) && (menuPointer > 0))
      menuPointer--;
    if (isButtonPressed(BUTTON_B) && (menuPointer < (MENU_LENGTH - 1)))
      menuPointer++;
    if (isButtonPressed(BUTTON_C)) {
      while (isButtonPressed(BUTTON_C))
        delay(100);
      return menu[menuPointer].program;
    }
    displayMenu(menuPointer);
    waitReleaseAllButtons();
  }
}


void runReactionTest(void) {

  uint32_t timeTrigger, timeStart, timeStop, timeReaction;
  bool isFalseStart;
  statsPointer = 0;

  while (1) {
    displayReactionReady();
    isFalseStart = false;
    timeTrigger = millis() + random(REACTION_WAIT_MIN, REACTION_WAIT_MAX);
    while (millis() < timeTrigger) { 
      if (isButtonPressed(BUTTON_C)) {
        isFalseStart = true;
        if (checkExitCommand()) 
          return;
        break;
      }
    }
    if (isFalseStart) {
      displayFalseStart();
      delay(1000);
    } else {
      LED_A_ON;
      timeStart = millis();
      while (!isButtonPressed(BUTTON_C)) {};
      timeStop = millis();
      timeReaction = timeStop - timeStart;

      if (timeReaction > REACTION_THRESHOLD) {
        timeReaction = REACTION_THRESHOLD; 
      } else {
        statsReaction[statsPointer] = timeReaction;
        statsPointer++;
      }
      
      displayReactionResult(timeReaction);
      delay(1500);
      LED_A_OFF;

      if (statsPointer == STATS_LENGTH) {
        displayReactionStats();
        delay(1000);
        while (!isButtonPressed(BUTTON_C)) delay(100);
        if (checkExitCommand())
          return;
        statsPointer = 0;
      }
    }
  }
}


void runDurationTest() {
  while (1) {
    if (isButtonPressed(BUTTON_A) && (durationPointer > 0)) {
      durationPointer--;
    }
    if (isButtonPressed(BUTTON_B) && (durationPointer < (DURATION_LIST_LENGTH - 1))) {
      durationPointer++;
    }
    if (isButtonPressed(BUTTON_C)) {
      if (checkExitCommand()) {
        break;
      } else {
        uint8_t res;
        res = runDurationTestCycle(durationList[durationPointer]); 
        if (res)
          break;
      }
    }
    displaySelectDelay(durationList[durationPointer]);
    waitReleaseAllButtons();
  }
}


uint8_t runDurationTestCycle(uint16_t duration) {

  uint8_t exitFlag = 0;
  uint8_t rightAnswer = 0;
  bool isAnswerCorrect = false;
  uint32_t nextTimestamp;
  
  memset(statsDelay, 0, sizeof(statsDelay));
  statsPointer = 0;

  if (currentProgram == P_OFF_TIME) {
    LED_A_ON;
    LED_B_ON;
  }

  #define TIME_STEP 500

  while (1) {
    
    displayDelayQuestion();
    rightAnswer = random(2);
    isAnswerCorrect = false;
    exitFlag = 0;
    nextTimestamp = millis();
    uint8_t currentPhase = 0;

    while (!exitFlag) { // poll cycle

      if (millis() > nextTimestamp) {
        nextTimestamp = millis() + TIME_STEP;
        if (currentPhase == 0)
          firstPhaseAction(duration, rightAnswer); 
        else
          secondPhaseAction(duration, rightAnswer);
        currentPhase ^= 1;
      }
      if (isButtonPressed(BUTTON_A)) {
        if (rightAnswer == ANSWER_A) {
          isAnswerCorrect = true;
        } else {
          isAnswerCorrect = false;
        }
        exitFlag = 1;
      }
      if (isButtonPressed(BUTTON_B)) {
        if (rightAnswer == ANSWER_B) {
          isAnswerCorrect = true;
        } else {
          isAnswerCorrect = false;
        }
        exitFlag = 1;
      }

      if (isButtonPressed(BUTTON_C)) {
        LED_A_OFF;
        LED_B_OFF;
        if (checkExitCommand()) 
          return 1;
        else 
          return 0;
      }
    }
    LED_A_OFF;
    LED_B_OFF;

    if (isAnswerCorrect) {
      statsDelay[statsPointer] = ANSWER_RIGHT;
    } else {
      statsDelay[statsPointer] = ANSWER_WRONG;
    }
    displayAnswer(isAnswerCorrect);
    delay(1000);

    statsPointer++;
    if (statsPointer == STATS_LENGTH) {
      displayDelayStats();
      while (!isButtonPressed(BUTTON_C))
        delay(100);
      while (isButtonPressed(BUTTON_C)) 
        delay(100);
      return 0;
    }
  }
}


void firstPhaseAction(uint16_t duration, uint8_t rightAnswer) {
  if (currentProgram == P_TIME_SHIFT) {
    if (rightAnswer == ANSWER_A) {
      LED_A_ON; 
      DELAY; 
      LED_B_ON;
    } else {
      LED_B_ON; 
      DELAY; 
      LED_A_ON;
    }
  } else if (currentProgram == P_ON_TIME) {
    if (rightAnswer == ANSWER_A) duration *= 2; 
    {  
      LED_A_ON;
      DELAY;
      LED_A_OFF;
    }
  } else if (currentProgram == P_OFF_TIME) {
    if (rightAnswer == ANSWER_A) duration *= 2; 
    {  
      LED_A_OFF;
      DELAY;
      LED_A_ON;
    }
  }
}


void secondPhaseAction(uint16_t duration, uint8_t rightAnswer) {
  if (currentProgram == P_TIME_SHIFT) {
    if (rightAnswer == ANSWER_A) {
      LED_A_OFF;
      DELAY;
      LED_B_OFF;
    } else {
      LED_B_OFF;
      DELAY;
      LED_A_OFF;
    }
  } else if (currentProgram == P_ON_TIME) {
    if (rightAnswer == ANSWER_B) duration *= 2;
    LED_B_ON;
    DELAY;
    LED_B_OFF;
  } else if (currentProgram == P_OFF_TIME) {
    if (rightAnswer == ANSWER_B) duration *= 2;
    LED_B_OFF; 
    DELAY;
    LED_B_ON;
  }
}


uint8_t isButtonPressed(uint8_t btn) {
  return (!digitalRead(btn));
}


uint8_t allButtonsRelased(void) {
  return ((digitalRead(BUTTON_A) && digitalRead(BUTTON_B)) && digitalRead(BUTTON_C));
}


bool checkExitCommand(void) {
  uint8_t cnt = 0;
  while (isButtonPressed(BUTTON_C)) {
    delay(100);
    cnt++;
    if (cnt == 10)
      return true;
  }
  return false;
}


void waitReleaseAllButtons(void) {
  while (!allButtonsRelased()) {
    delay(100);
  }
  while (allButtonsRelased()) {
    delay(100);
  };
}


int getTextWidth(char *text) {
  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x, &y, &w, &h);
  return w;
}


void drawCheckMark(uint8_t x, uint8_t y, uint8_t ch) {
  if (ch == ANSWER_RIGHT) { // tick
    display.drawLine(x-1, y+1, x+5, y-5, SSD1306_WHITE); 
    display.drawLine(x-1, y+3, x+5, y-5, SSD1306_WHITE);
    display.drawLine(x-1, y+3, x-3, y-1, SSD1306_WHITE);
    display.drawLine(x-1, y+2, x-0, y+1, SSD1306_WHITE);
  }
  if (ch == ANSWER_WRONG) { // cross
    display.drawLine(x-3, y+3, x+4, y-4, SSD1306_WHITE); 
    display.drawLine(x-2, y+1, x+2, y-3, SSD1306_WHITE);
    display.drawLine(x-3, y-3, x+3, y+3, SSD1306_WHITE);
    display.drawLine(x-1, y-2, x+2, y+1, SSD1306_WHITE);
  }
}


void drawCursor(uint8_t x, uint8_t y) {
  for (uint8_t i = 0; i < 5; i++)
    display.drawLine(x-i, y-i, x-i, y+i, SSD1306_WHITE);
}


void printDelayProgressBar(void) {
  for (uint8_t i = 0; i < STATS_LENGTH; i++) {
    drawCheckMark((i*128)/STATS_LENGTH + 128/STATS_LENGTH/2, 60, statsDelay[i]);
  }
}


void printReactionProgressBar(void) {
  if (statsPointer > 0) 
    display.drawRect(0, 62, (statsPointer * 128) /STATS_LENGTH, 2, SSD1306_WHITE);
}


void displayDelayQuestion(void) {
  display.clearDisplay();
  display.setFont(SMALL_FONT);
  if (currentProgram == P_TIME_SHIFT) {
    display.setCursor(20, 16);
    display.print("Which LED");
    display.setCursor(20, 35);
    display.print("is the first?");
  } else {
    display.setCursor(20, 16);
    display.print("Which blink");
    display.setCursor(26, 35);
    display.print("is longer?");
  }
  printDelayProgressBar();
  display.display();
}


void displayAnswer(bool correct) {
  display.clearDisplay();
  display.setFont(LARGE_FONT);
  if (correct) {
    display.setCursor(36, 26);
    display.print("Right"); 
  } else {
    display.setCursor(28, 26);
    display.print("Wrong");
  }
  printDelayProgressBar();
  display.display();
}


void displaySelectDelay(uint16_t d) {
  char text[10];
  display.clearDisplay();
  display.setCursor(19, 20);
  display.setFont(SMALL_FONT);
  display.print("Select time");
  display.setFont(LARGE_FONT);
  sprintf(text, "%d", d);
  display.setCursor(64 - getTextWidth(text) - 2, 54);
  display.print(text);
  display.setCursor(64 + 7, 54);
  display.print("ms");
  display.display();
} 


void displayDelayStats(void) {
  char text[10];
  uint8_t sum = 0;
  for (uint8_t i = 0; i < STATS_LENGTH; i++) {
    if (statsDelay[i] == 1) sum++;
  }
  sprintf(text, "%d/%d", sum, STATS_LENGTH);
  display.clearDisplay();
  display.setFont(LARGE_FONT);
  display.setCursor(64 - getTextWidth(text)/2, 16);
  display.print(text);
  display.setFont(SMALL_FONT);
  sprintf(text, "%d ms", durationList[durationPointer]);
  display.setCursor(64 - getTextWidth(text)/2, 40);
  display.print(text);
  printDelayProgressBar();
  display.display();
}


void displayReactionStats(void) {
  uint32_t sumTime = 0;
  uint16_t time, meanTime, minTime, maxTime;
  minTime = statsReaction[0];
  maxTime = 0;
  for (uint8_t i = 0; i < STATS_LENGTH; i++) {
    time = statsReaction[i];
    sumTime += time;
    if (time < minTime) 
      minTime = time;
    if (time > maxTime) 
      maxTime = time;
  }
  // Find median value instead of average:
  //meanTime = sumTime / STATS_LENGTH;
  qsort(statsReaction, STATS_LENGTH, sizeof(statsReaction[0]), compare_uint16);
  meanTime = (statsReaction[STATS_LENGTH/2] + statsReaction[STATS_LENGTH/2 - 1]) / 2;

  display.clearDisplay();
  display.setCursor(26, 20);
  display.setFont(LARGE_FONT);
  display.print(meanTime, DEC);
  display.print(" ms");
  display.setFont(SMALL_FONT);
  display.setCursor(30, 45);
  display.print("Min: ");
  display.print(minTime, DEC);
  display.setCursor(25, 63);
  display.print("Max: ");
  display.print(maxTime, DEC);
  display.display();
}


void displayReactionReady(void) {
  display.clearDisplay();
  display.setCursor(31, 36);
  display.setFont(LARGE_FONT);
  display.print("Ready");
  printReactionProgressBar();
  display.display();
}


void displayFalseStart(void) {
  display.clearDisplay();
  display.setCursor(6, 36);
  display.setFont(LARGE_FONT);
  display.print("False start!");
  printReactionProgressBar();
  display.display();
}


void displayReactionResult(uint16_t result) {
  display.clearDisplay();
  display.setFont(LARGE_FONT);
  if (result < REACTION_THRESHOLD) {
    display.setCursor(25, 36);
    display.print(result, DEC);
    display.print(" ms");
  } else {
    display.setCursor(14, 36);
    display.print("Too slow!");
  }
  printReactionProgressBar();
  display.display();
}


void displayMenu(uint8_t pnt) {  
  display.clearDisplay();
  display.setFont(SMALL_FONT);
  for (uint8_t i = 0; i < MENU_LENGTH; i++) {
    display.setCursor(14, i * 16 + 12);
    display.print(menu[i].name);
  }
  drawCursor(9, pnt * 16 + 6);
  display.display();
}


int compare_uint16(const void *cmp1, const void *cmp2) {
  int a = *((uint16_t *)cmp1);
  int b = *((uint16_t *)cmp2);
  return a > b ? -1 : (a < b ? 1 : 0);
}




