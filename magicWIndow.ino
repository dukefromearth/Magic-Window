/*
 * By Stephen Duke and Qiu Yi Wu
 * For public use
 */

#include <PololuLedStrip.h>
#include <TimeLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Math.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <SoftwareSerial.h>

const int txPin = 19;
const int rxPin = 18;

float prevMin;
float currMin;
int loopCount;
int maxLoops;
short buttonPress = 0;

SoftwareSerial BTSerial(rxPin, txPin); // RX, TX

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Create an ledStrip object and specify the pin it will use.
PololuLedStrip<12> ledStrip;

// Create a buffer for holding the colors (3 bytes per color).
#define LED_COUNT 60
rgb_color colors[LED_COUNT];




void setup()  {
  pinMode(9, INPUT);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  lcd.init();
  lcd.backlight();

  Serial.begin(38400);
  BTSerial.begin(38400);

  while (!Serial) ; // wait until Arduino Serial Monitor opens
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
}

void loop() {
  if (digitalRead(9) == HIGH) {
    delay(500);
    buttonPress++;
  }
  Serial.println(buttonPress);
  if (buttonPress == 0) {
    digitalClockDisplay();
  }
  else if (buttonPress == 1) {
    brightTwinkle(0, 1, 0);
    ledStrip.write(colors, LED_COUNT);
    delay(70);
    loopCount++;
  }
  else if (buttonPress == 2) {

    gradient();
  }
  else if (buttonPress == 3) {
    ledClear();
  }
  else if (buttonPress == 4) {
    for (uint16_t i = 0; i < LED_COUNT; i+=2)
    {
      colors[i] = rgb_color(255, 172, 68 );
    }
    ledStrip.write(colors, LED_COUNT);
  }
  else if (buttonPress == 5) {
    zip();
    ledClear();
    buttonPress = 0;
  }
}

void zip() {

  int r = 238;
  int g = 0;
  int b = 238;

  for (uint16_t i = 0; i < LED_COUNT; i++)
  {
    for (uint16_t j = 0; j < 5; j++) {
      colors[i] = rgb_color(r, g, b);
      if ((i - j) >= 0) {
        colors[i - j] = rgb_color(r / j, g / j, b / j);
      }
      ledStrip.write(colors, LED_COUNT);
      delay(5);
    }
  }
}
void ledClear() {
  for (uint16_t i = 0; i < LED_COUNT; i++)
  {
    colors[i] = rgb_color(0, 0, 0);
  }
  ledStrip.write(colors, LED_COUNT);
}

void gradient() {
  // Update the colors.
  byte time = millis() >> 2;
  for (uint16_t i = 0; i < LED_COUNT; i+=2)
  {
    byte x = time - 8 * i;
    colors[i] = rgb_color(x, 255 - x, x);
  }

  // Write the colors to the LED strip.
  ledStrip.write(colors, LED_COUNT);

  delay(20);
}

struct sunTime {
  float sunrise;
  float sunset;
};

struct sunTime calculateSunrise() {
  struct sunTime curSuntime;
  float summerSolstice = 171;   // happens on the 171st day
  float winterSolstice = 364;   // using dec31 as winter solstice instead of dec21
  float sumSolRise = 325;       // 325 minutes from midnight : 5:25
  float sumSolSet = 1230;       // 1230 minutes from midnight : 20:30
  float winSolRise = 440;       // 440 minutes from midnight : 7:20
  float winSolSet = 999;        // 999 minutes from midnight : 16:39

  float riseDiff = abs(sumSolRise - winSolRise);
  float setDiff = abs(sumSolSet - winSolSet);

  //calculate days since jan1
  float currentDay = day();
  for (int i = 1; i < month(); i = i + 1) {
    if (i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10 || i == 12) {
      currentDay = currentDay + 31;
    }
    else if (i == 4 || i == 6 || i == 9 || i == 11) {
      currentDay = currentDay + 30;
    }
    else if (i == 2) {
      currentDay = currentDay + 28;
    }
  }
  //subtract minutes from sunrise && add minutes to sunset
  if (currentDay < summerSolstice) {  //first half of year
    curSuntime.sunrise = winSolRise - abs(winSolRise - sumSolRise) * (currentDay / summerSolstice) - 40;
    curSuntime.sunset = winSolSet + abs(sumSolSet - winSolSet) * (currentDay / summerSolstice) + 40;
  }
  else {                              //second half of year
    curSuntime.sunrise = sumSolRise + abs(winSolRise - sumSolRise) * (currentDay / winterSolstice) - 50;
    curSuntime.sunset = sumSolSet - abs(sumSolSet - winSolSet) * (currentDay / winterSolstice) + 90;
  }
  return curSuntime;
}

//helper function to convert hours into minutes
float currentMinute() {
  int minutes = hour() * 60 + minute();
  return minutes;
}

void digitalClockDisplay() {
  //lcd.clear();
  rgb_color color;
  float ratio;
  currMin = currentMinute();

  struct sunTime todaySun = calculateSunrise();
  float midSun = ((todaySun.sunset - todaySun.sunrise) / 2) + 342;
  float timeData[7] = {ratio,
                       currMin,
                       todaySun.sunrise,
                       todaySun.sunset,
                       midSun
                      };

  if (currMin < todaySun.sunrise || currMin > todaySun.sunset) {
    brightTwinkle(0, 1, 0);
    ledStrip.write(colors, LED_COUNT);
    delay(20);
    loopCount++;
  }
  else if (prevMin != currMin) {
    if (currMin > todaySun.sunrise && currMin < todaySun.sunset) {
      //get brighter
      if (currMin < midSun) {
        ratio = 1 - ((currMin - todaySun.sunrise) / (midSun - todaySun.sunrise));
        color.red = 252 * ratio;
        color.green = 212 * ratio;
        color.blue = 64 * ratio;

      }
      //get dimmer
      else {
        ratio = 1 - (((currMin - midSun)) / (todaySun.sunset - midSun));
        color.red = 255 * ratio;
        color.green = 255 * ratio;
        color.blue = 255 * ratio;
      }
      updateColorBuffer(color);
    }
  }

  prevMin = currMin;

  Serial.println(currMin);
  Serial.println(todaySun.sunset);

  Serial.println(todaySun.sunrise);
  Serial.println(todaySun.sunset);
  serialDisplay();
  lcdDisplay(timeData);

}

rgb_color updateColor(int r, int g, int b) {
  rgb_color color;
  color.red = r;
  color.green = g;
  color.blue = b;
  return color;
}


void updateColorBuffer(rgb_color color) {
  for (uint16_t i = 0; i < LED_COUNT; i+=2)
  {
    colors[i] = color;
  }

  ledStrip.write(colors, LED_COUNT);
}

void twinkle() {
  maxLoops = 1200;
  if (loopCount < 400)
  {
    brightTwinkle(0, 1, 0);  // only white for first 400 loopCounts
  }
  else if (loopCount < 650)
  {
    brightTwinkle(0, 2, 0);  // white and red for next 250 counts
  }
  else if (loopCount < 900)
  {
    brightTwinkle(1, 2, 0);  // red, and green for next 250 counts
  }
  else
  {
    // red, green, blue, cyan, magenta, yellow for the rest of the time
    brightTwinkle(1, 6, loopCount > maxLoops - 100);
  }
}

void fade(unsigned char *val, unsigned char fadeTime)
{
  if (*val != 0)
  {
    unsigned char subAmt = *val >> fadeTime;  // val * 2^-fadeTime
    if (subAmt < 1)
      subAmt = 1;  // make sure we always decrease by at least 1
    *val -= subAmt;  // decrease value of byte pointed to by val
  }
}

void brightTwinkleColorAdjust(unsigned char *color)
{
  if (*color > 35)
  {
    // if reached max brightness, set to an even value to start fade
    *color = 34;
  }
  else if (*color % 2)
  {
    // if odd, approximately double the brightness
    // you should only use odd values that are of the form 2^n-1,
    // which then gets a new value of 2^(n+1)-1
    // using other odd values will break things
    *color = *color + 2;
  }
  else if (*color > 0)
  {
    fade(color, 4);
    if (*color % 2)
    {
      (*color)--;  // if faded color is odd, subtract one to keep it even
    }
  }
}

void brightTwinkle(unsigned char minColor, unsigned char numColors, unsigned char noNewBursts)
{
  // Note: the colors themselves are used to encode additional state
  // information.  If the color is one less than a power of two
  // (but not 255), the color will get approximately twice as bright.
  // If the color is even, it will fade.  The sequence goes as follows:
  // * Randomly pick an LED.
  // * Set the color(s) you want to flash to 1.
  // * It will automatically grow through 3, 7, 15, 31, 63, 127, 255.
  // * When it reaches 255, it gets set to 254, which starts the fade
  //   (the fade process always keeps the color even).
  if (loopCount > 2000) loopCount = 0;
  for (int i = 0; i < LED_COUNT; i++)
  {
    brightTwinkleColorAdjust(&colors[i].red);
    brightTwinkleColorAdjust(&colors[i].green);
    brightTwinkleColorAdjust(&colors[i].blue);
  }

  if (!noNewBursts && loopCount % 2)
  {
    // if we are generating new twinkles, randomly pick four new LEDs
    // to light up
    for (int i = 0; i < 1; i++)
    {
      int j = random(LED_COUNT);
      if (colors[j].red == 0 && colors[j].green == 0 && colors[j].blue == 0)
      {
        // if the LED we picked is not already lit, pick a random
        // color for it and seed it so that it will start getting
        // brighter in that color
        switch (random(numColors) + minColor)
        {
          case 0:
            colors[j] = rgb_color(1, 1, 1);  // white
            break;
          case 1:
            colors[j] = rgb_color(1, 0, 0);  // red
            break;
          case 2:
            colors[j] = rgb_color(0, 1, 0);  // green
            break;
          case 3:
            colors[j] = rgb_color(0, 0, 1);  // blue
            break;
          case 4:
            colors[j] = rgb_color(1, 1, 0);  // yellow
            break;
          case 5:
            colors[j] = rgb_color(0, 1, 1);  // cyan
            break;
          case 6:
            colors[j] = rgb_color(1, 0, 1);  // magenta
            break;
          default:
            colors[j] = rgb_color(1, 1, 1);  // white
        }
      }
    }
  }
}

void serialDisplay() {
  //print time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());

  //print day month year;
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

void lcdDisplay(float data[7]) {
  // digital clock display of the time
  lcd.setCursor(1, 0);
  lcd.print("Time: ");
  if (hour() < 10)
    lcd.print("0");
  lcd.print(hour());
  lcdTimePrint(minute());
  lcdTimePrint(second());

  //print sunrise time
  lcd.setCursor(0, 1);
  lcd.print((int)data[2] / 60);
  lcd.print(":");
  lcd.print((int)data[2] % 60);

  //print sunset time
  lcd.setCursor(11, 1);
  lcd.print((int)data[3] / 60);
  lcd.print(":");
  lcd.print((int)data[3] % 60);

}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void lcdTimePrint(int digits) {
  // utility function for lcd clock display: prints preceding colon and leading 0
  lcd.print(":");
  if (digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

