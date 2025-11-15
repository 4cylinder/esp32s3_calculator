// Calculator demo

//pin usage as follow:
//            CS  DC/RS  RESET  SDI/MOSI  SCK  SDO/MISO  LED    VCC     GND    
//ESP32-S3:   10    2      15      11      12      13     21     5V     GND 

/*********************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE 
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**********************************************************************************/
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "touch.h"  // 触摸相关头文件

TFT_eSPI myLcd = TFT_eSPI();
                             /*  r     g    b */
#define BLACK        0x0000  /*   0,   0,   0 */
#define BLUE         0x001F  /*   0,   0, 255 */
#define RED          0xF800  /* 255,   0,   0 */
#define GREEN        0x07E0  /*   0, 255,   0 */
#define CYAN         0x07FF  /*   0, 255, 255 */
#define MAGENTA      0xF81F  /* 255,   0, 255 */
#define YELLOW       0xFFE0  /* 255, 255,   0 */
#define WHITE        0xFFFF  /* 255, 255, 255 */
#define NAVY         0x000F  /*   0,   0, 128 */
#define DARKGREEN    0x03E0  /*   0, 128,   0 */
#define DARKCYAN     0x03EF  /*   0, 128, 128 */
#define MAROON       0x7800  /* 128,   0,   0 */
#define PURPLE       0x780F  /* 128,   0, 128 */
#define OLIVE        0x7BE0  /* 128, 128,   0 */
#define LIGHTGREY    0xC618  /* 192, 192, 192 */
#define DARKGREY     0x7BEF  /* 128, 128, 128 */
#define ORANGE       0xFD20  /* 255, 165,   0 */
#define GREENYELLOW  0xAFE5  /* 173, 255,  47 */
#define PINK         0xF81F  /* 255,   0, 255 */

/******************* UI details */
// each button is a circle
#define RADIUS 25
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 5
#define EDG_Y 5
#define EDG_X 10

#define BUTTON_COUNT 18
#define DIGIT_LIMIT 8

typedef struct _buttonProps {
  const char label[10];
  uint8_t textSize;
  uint16_t textColour;
  uint16_t buttonColour;
  int16_t xPos;
  int16_t yPos;     
} buttonProps;

int16_t columnX[4] = {
  EDG_X + RADIUS - 1,
  EDG_X + 3*RADIUS + BUTTON_SPACING_X - 1,
  EDG_X + 5*RADIUS + 2*BUTTON_SPACING_X - 1,
  EDG_X + 7*RADIUS + 3*BUTTON_SPACING_X - 1,
};

int16_t rowY[5] = {
  myLcd.height() - EDG_Y - 4*BUTTON_SPACING_Y - 9*RADIUS - 1,
  myLcd.height() - EDG_Y - 3*BUTTON_SPACING_Y - 7*RADIUS - 1,
  myLcd.height() - EDG_Y - 2*BUTTON_SPACING_Y - 5*RADIUS - 1,
  myLcd.height() - EDG_Y - 1*BUTTON_SPACING_Y - 3*RADIUS - 1,
  myLcd.height() - EDG_Y - RADIUS - 1,
};

buttonProps buttons[BUTTON_COUNT] = {
  // first row: 1,2,3,+
  {"1", 4, BLACK, CYAN, columnX[0], rowY[0]},
  {"2", 4, BLACK, CYAN, columnX[1], rowY[0]},
  {"3", 4, BLACK, CYAN, columnX[2], rowY[0]},
  {"+", 4, BLACK, YELLOW, columnX[3], rowY[0]},
  // second row: 4, 5, 6, -
  {"4", 4, BLACK, CYAN, columnX[0], rowY[1]}, 
  {"5", 4, BLACK, CYAN, columnX[1], rowY[1]},
  {"6", 4, BLACK, CYAN, columnX[2], rowY[1]},
  {"-", 4, BLACK, YELLOW, columnX[3], rowY[1]},
  // third row: 7, 8, 9, *
  {"7", 4, BLACK, CYAN, columnX[0], rowY[2]},
  {"8", 4, BLACK, CYAN, columnX[1], rowY[2]},
  {"9", 4, BLACK, CYAN, columnX[2], rowY[2]},
  {"*", 4, BLACK, YELLOW, columnX[3], rowY[2]},
  // fourth row: 0, decimal, ^, /
  {"0", 4, BLACK, CYAN, columnX[0], rowY[3]},
  {".", 4, BLACK, CYAN, columnX[1], rowY[3]},
  {"^", 4, BLACK, YELLOW, columnX[2], rowY[3]},
  {"/", 4, BLACK, YELLOW, columnX[3], rowY[3]},
  // fifth row: C, =
  {"C", 4, BLACK, LIGHTGREY, columnX[1], rowY[4]},
  {"=", 4, BLACK, GREEN, columnX[2], rowY[4]},
};

uint16_t px, py;
uint16_t text_x=10,text_y=5,text_x_add = 4*buttons[0].textSize+2,text_y_add = 6*buttons[0].textSize;
// number of digits rendered on-screen
uint16_t numRendered = 0;
char displayNum[DIGIT_LIMIT + 3] = "";
float num1 = 0;
float num2 = 0;
char op = '\0';
boolean decimalUsed = false;
boolean calculated = false;

boolean is_pressed(int16_t x1,int16_t y1,int16_t x2,int16_t y2,int16_t px,int16_t py) {
  if((px > x1 && px < x2) && (py > y1 && py < y2)) {
    return true;
  } else {
    return false;
  }
}

void displayCalculator(void) {
  uint16_t i;
  for(i = 0; i < BUTTON_COUNT; i++) {
    buttonProps button = buttons[i];
    myLcd.fillCircle(button.xPos, button.yPos, RADIUS,button.buttonColour);
    myLcd.setTextColor(button.textColour);
    myLcd.drawString(button.label, button.xPos-button.textSize*4/2+button.textSize/2+1, button.yPos-button.textSize*6/2+button.textSize/2+1, button.textSize);
  }

  // bottom line for calculation area
  myLcd.fillRect(1, 36, myLcd.width(), 2, BLACK);
}

void clearDisplay() {
  strcpy(displayNum, "");
  myLcd.fillRect(0, 1, myLcd.width(), 30, BLUE);
  decimalUsed = false;
}

void clearCalculator() {
  numRendered = 0;
  clearDisplay();
  num1 = 0;
  num2 = 0;
  calculated = false;
  decimalUsed = false;
}

void renderDigit(buttonProps button) {
  if (numRendered < DIGIT_LIMIT) {
    strcat(displayNum, button.label);
    myLcd.setTextColor(GREENYELLOW);
    myLcd.fillRect(0, 1, myLcd.width(), 30, BLUE);
    myLcd.drawString(displayNum, myLcd.width()-strlen(displayNum)*text_x_add, text_y, 4);
    numRendered++;
  }
}
// deprecated
void backspace() {
  if (numRendered > 0) {
    text_x -= (text_x_add - 1);
    myLcd.fillRect(text_x, text_y, text_x_add, text_y_add - 1, BLUE);
    numRendered--;
  }
}

boolean isOp(buttonProps button) {
  char c = button.label[0];
  return c=='+' || c=='-' || c=='*' || c=='/' || c=='^';
}

void performOp() {
  switch(op) {
    case '+':
      num1 += num2;
      break;
    case '-':
      num1 -= num2;
      break;
    case '*':
      num1 *= num2;
      break;
    case '/':
      num1 /= num2;
      break;
    case '^':
      num1 = pow(num1, num2);
      break;
    default:
      break;
  }
}

void showCalculation() {
  clearDisplay();
  performOp();
  
  myLcd.setTextColor(GREENYELLOW);
  int intVal = num1;
  if (num1 == intVal) {
    itoa(intVal, displayNum, 10);
  } else {
    sprintf(displayNum, "%.2f", num1);
  }
  myLcd.drawString(displayNum, myLcd.width()-strlen(displayNum)*text_x_add, text_y, 4);
  calculated = true;
}

void setup(void) {
  // Serial.begin(115200);
  myLcd.init();
  myLcd.setRotation(0);  
  touch_init(myLcd.width(), myLcd.height(), myLcd.getRotation());
  myLcd.fillScreen(BLUE); 
  displayCalculator();
}

void loop(void) {
  uint8_t i;
  if (touch_touched()) {
    px = touch_last_x;
    py = touch_last_y;
    for (i = 0; i < BUTTON_COUNT; i++) {
      buttonProps button = buttons[i];
      if (is_pressed(button.xPos - RADIUS, button.yPos - RADIUS, button.xPos + RADIUS, button.yPos + RADIUS, px, py)) {
        myLcd.fillCircle(button.xPos, button.yPos, RADIUS, DARKGREY);
        myLcd.setTextColor(WHITE);
        myLcd.drawString(button.label, button.xPos - strlen(button.label) * button.textSize * 4 / 2 + button.textSize / 2 + 1, button.yPos - button.textSize * 6 / 2 + button.textSize / 2 + 1, button.textSize);
        //delay(100);
        myLcd.fillCircle(button.xPos, button.yPos, RADIUS, button.buttonColour);
        myLcd.setTextColor(button.textColour);
        myLcd.drawString(button.label, button.xPos - strlen(button.label) * button.textSize * 4 / 2 + button.textSize / 2 + 1, button.yPos - button.textSize * 6 / 2 + button.textSize / 2 + 1, button.textSize);

        if (isdigit(button.label[0])) {
          if (calculated) {
            calculated = false;
            clearDisplay();
          }
          renderDigit(buttons[i]);
        } else if (button.label[0] == '.' && !decimalUsed) {
          decimalUsed = true;
          // If no number is entered yet, then prepend a 0, e.g. .5 becomes 0.5
          if (strlen(displayNum) == 0) {
            strcpy(displayNum, "0.");
          } else {
            // If the user forgets to add anything after the decimal, it'll just convert to an integer
            // E.g. "123." will register as 123 when the user presses an op or =
            strcat(displayNum, ".");
          }
        } else if (button.label[0] == 'C') {
          clearCalculator();
        } else if (isOp(button)) {
          // E.g. New session, user enters 5, then +, and then should enter the second number
          if (strlen(displayNum) > 0 && op == '\0') {
            op = button.label[0];
            num1 = atof(displayNum);
            clearDisplay();
          } else {
            // E.g. User has already entered 5 + 4, then hits + again. Save 9 into num1, then enter the third number (saved into num2)
            num2 = atof(displayNum);
            showCalculation();
            num2 = 0;
            op = button.label[0];
          }
        } else if (button.label[0] == '=' && op != '\0') {
          num2 = atof(displayNum);
          showCalculation();
          op = '\0';
          num2 = 0;
        }
        while (touch_touched()) {
          delay(10);
        }
      }
    }
  }
}
