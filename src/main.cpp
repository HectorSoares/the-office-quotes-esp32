#include "the_office_bitmaps.h"
#include "the_office_quotes.h"
#include <Arduino.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_3C.h>
#include <SPI.h>

#define BUTTON_PIN 0

#define EPD_SCK 4
#define EPD_MOSI 6
#define EPD_CS 7
#define EPD_DC 8
#define EPD_RST 9
#define EPD_BUSY 10

GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT>
    display(GxEPD2_290_C90c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

const int TEXT_X = 138;
const int TEXT_Y = 32;
const int TEXT_W = 158;
const int TEXT_H = 94;
const int LINE_HEIGHT = 15;
const int VISIBLE_LINES = 6;
const int MAX_LINES = 20;

struct Quote {
  String character;
  String text;
};

Quote currentQuote;
String quoteLines[MAX_LINES];
int totalLines = 0;
int firstVisibleLine = 0;
bool lastButtonState = HIGH;

const uint8_t *getCharacterBitmap(const String &character) {
  if (character == "Michael Scott")
    return epd_bitmap_michael;
  if (character == "Jim Halpert")
    return epd_bitmap_jim;
  if (character == "Pam Beesly")
    return epd_bitmap_pam;
  if (character == "Dwight Schrute")
    return epd_bitmap_dwight;
  if (character == "Creed Bratton")
    return epd_bitmap_creed;
  if (character == "Kelly Kapoor")
    return epd_bitmap_kelly;
  if (character == "Stanley Hudson")
    return epd_bitmap_stanley;
  if (character == "Angela Martin")
    return epd_bitmap_angela;
  if (character == "Kevin Malone")
    return epd_bitmap_kelvin;
  if (character == "Andy Bernard")
    return epd_bitmap_andy;
  return nullptr;
}

String cleanText(String text) {
  text.replace("\xE2\x80\x99", "'");
  text.replace("\xE2\x80\x98", "'");
  text.replace("\xE2\x80\x9C", "\"");
  text.replace("\xE2\x80\x9D", "\"");
  text.replace("\xE2\x80\x93", "-");
  text.replace("\xE2\x80\x94", "-");
  text.replace("\xE2\x80\xA6", "...");

  String result;

  for (size_t i = 0; i < text.length(); i++) {
    uint8_t c = text[i];
    if (c >= 32 && c <= 126)
      result += (char)c;
  }
  return result;
}

Quote getRandomQuote() {
  Quote result;

  if (QUOTE_COUNT == 0)
    return result;

  size_t index = random(QUOTE_COUNT);

  result.character = quotes[index].character;
  result.text = cleanText(quotes[index].text);
  return result;
}

void prepareQuoteLines() {
  totalLines = 0;
  firstVisibleLine = 0;

  String textToWrap = "\"" + currentQuote.text + "\"";
  String currentLine = "";

  const int maxWidth = 296 - TEXT_X - 6;

  while (textToWrap.length() > 0 && totalLines < MAX_LINES) {
    int spaceIndex = textToWrap.indexOf(' ');

    if (spaceIndex == -1)
      spaceIndex = textToWrap.length();

    String word = textToWrap.substring(0, spaceIndex);

    String testLine;

    if (currentLine.length() == 0) {
      testLine = word;
    } else {
      testLine = currentLine + " " + word;
    }

    int16_t bx, by;
    uint16_t bw, bh;

    display.getTextBounds(testLine, TEXT_X, TEXT_Y, &bx, &by, &bw, &bh);

    if (bw > maxWidth && currentLine.length() > 0) {
      quoteLines[totalLines++] = currentLine;
      currentLine = word;
    } else {
      currentLine = testLine;
    }

    if (spaceIndex == textToWrap.length()) {
      textToWrap = "";
    } else {
      textToWrap = textToWrap.substring(spaceIndex + 1);
    }
  }

  if (currentLine.length() > 0 && totalLines < MAX_LINES) {
    quoteLines[totalLines++] = currentLine;
  }
}

void drawQuoteLines() {
  display.setFont(NULL);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);

  int cursorY = TEXT_Y + 8;

  for (int i = 0; i < VISIBLE_LINES; i++) {
    int lineIndex = firstVisibleLine + i;

    if (lineIndex >= totalLines) {
      break;
    }

    display.setCursor(TEXT_X, cursorY);
    display.print(quoteLines[lineIndex]);

    cursorY += LINE_HEIGHT;
  }
}

void drawInitialScreen() {
  const uint8_t *bitmap = getCharacterBitmap(currentQuote.character);

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    if (bitmap != nullptr) {
      display.drawInvertedBitmap(0, 0, bitmap, 128, 128, GxEPD_BLACK);
    }

    display.drawFastVLine(132, 0, 128, GxEPD_RED);

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(142, 18);
    display.print(currentQuote.character);

    drawQuoteLines();

  } while (display.nextPage());
}

void drawNextTextPage() {
  display.setPartialWindow(TEXT_X, TEXT_Y, TEXT_W, TEXT_H);
  display.firstPage();

  do {
    display.fillRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, GxEPD_WHITE);
    drawQuoteLines();
  } while (display.nextPage());
}

void showError(const char *message) {
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(10, 30);
    display.print("The Office");

    display.setFont(NULL);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 60);
    display.print(message);

  } while (display.nextPage());
}

void loadQuote() {
  currentQuote = getRandomQuote();

  if (currentQuote.character.length() == 0 || currentQuote.text.length() == 0) {
    showError("No quotes available");
    return;
  }

  Serial.println();
  Serial.print("Character: ");
  Serial.println(currentQuote.character);

  Serial.print("Quote: ");
  Serial.println(currentQuote.text);

  prepareQuoteLines();

  Serial.print("Total lines: ");
  Serial.println(totalLines);

  drawInitialScreen();
}

void nextQuotePage() {
  if (firstVisibleLine + VISIBLE_LINES < totalLines) {
    firstVisibleLine += VISIBLE_LINES;

    Serial.print("Showing lines ");
    Serial.print(firstVisibleLine + 1);
    Serial.print(" - ");

    int lastLine = firstVisibleLine + VISIBLE_LINES;

    if (lastLine > totalLines) {
      lastLine = totalLines;
    }

    Serial.println(lastLine);

    drawNextTextPage();
  } else {
    loadQuote();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200);
  randomSeed(esp_random());

  loadQuote();
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW) {
    delay(30);

    if (digitalRead(BUTTON_PIN) == LOW) {
      nextQuotePage();

      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }

  lastButtonState = buttonState;

  delay(10);
}