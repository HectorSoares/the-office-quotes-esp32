#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "the_office_bitmaps.h"
#include "the_office_quotes.h"

#define BUTTON_PIN 0

#define EPD_SCK 4
#define EPD_MOSI 6
#define EPD_CS 7
#define EPD_DC 8
#define EPD_RST 9
#define EPD_BUSY 10

GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(
    GxEPD2_290_C90c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

struct Quote
{
  String character;
  String text;
};

bool lastButtonState = HIGH;

const uint8_t *getCharacterBitmap(const String &character)
{
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

String cleanText(String text)
{
  text.replace("\xE2\x80\x99", "'");
  text.replace("\xE2\x80\x98", "'");
  text.replace("\xE2\x80\x9C", "\"");
  text.replace("\xE2\x80\x9D", "\"");
  text.replace("\xE2\x80\x93", "-");
  text.replace("\xE2\x80\x94", "-");
  text.replace("\xE2\x80\xA6", "...");

  String result;

  for (size_t i = 0; i < text.length(); i++)
  {
    uint8_t c = text[i];

    if (c >= 32 && c <= 126)
    {
      result += (char)c;
    }
  }

  return result;
}

Quote getRandomQuote()
{
  Quote result;

  if (QUOTE_COUNT == 0)
  {
    return result;
  }

  size_t index = random(QUOTE_COUNT);

  result.character = quotes[index].character;
  result.text = cleanText(quotes[index].text);

  return result;
}

void showQuote(const Quote &quote)
{
  const uint8_t *bitmap = getCharacterBitmap(quote.character);

  display.setRotation(1);
  display.setPartialWindow(0, 0, 296, 128);
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);

    if (bitmap != nullptr)
    {
      display.drawInvertedBitmap(
          0,
          0,
          bitmap,
          128,
          128,
          GxEPD_BLACK);
    }

    display.drawFastVLine(132, 0, 128, GxEPD_RED);

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(142, 18);
    display.print(quote.character);

    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);

    String textToPrint = "\"" + quote.text + "\"";
    int cursorX = 142;
    int cursorY = 40;
    int maxWidth = 296 - 142 - 6;
    int lineHeight = 13;

    String currentLine = "";
    int spaceIndex = 0;

    while (textToPrint.length() > 0)
    {
      spaceIndex = textToPrint.indexOf(' ');

      if (spaceIndex == -1)
      {
        spaceIndex = textToPrint.length();
      }

      String word = textToPrint.substring(0, spaceIndex);
      String testLine = currentLine.length() == 0
                            ? word
                            : currentLine + " " + word;

      int16_t bx, by;
      uint16_t bw, bh;

      display.getTextBounds(
          testLine,
          cursorX,
          cursorY,
          &bx,
          &by,
          &bw,
          &bh);

      if (bw > maxWidth && currentLine.length() > 0)
      {
        display.setCursor(cursorX, cursorY);
        display.print(currentLine);

        cursorY += lineHeight;
        currentLine = word;

        if (cursorY > 120)
        {
          display.setCursor(cursorX, cursorY - 2);
          display.print("...");
          break;
        }
      }
      else
      {
        currentLine = testLine;
      }

      if (spaceIndex == textToPrint.length())
      {
        textToPrint = "";
      }
      else
      {
        textToPrint = textToPrint.substring(spaceIndex + 1);
      }
    }

    if (currentLine.length() > 0 && cursorY <= 120)
    {
      display.setCursor(cursorX, cursorY);
      display.print(currentLine);
    }

  } while (display.nextPage());
}

void showError(const char *message)
{
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(10, 30);
    display.print("The Office");

    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 60);
    display.print(message);

  } while (display.nextPage());
}

void loadQuote()
{
  Quote quote = getRandomQuote();

  Serial.println();
  Serial.print("Character: ");
  Serial.println(quote.character);

  Serial.print("Quote: ");
  Serial.println(quote.text);

  showQuote(quote);
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(
      EPD_SCK,
      -1,
      EPD_MOSI,
      EPD_CS);

  display.init(115200);

  randomSeed(esp_random());

  loadQuote();
}

void loop()
{
  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW)
  {
    delay(30);

    if (digitalRead(BUTTON_PIN) == LOW)
    {
      loadQuote();

      while (digitalRead(BUTTON_PIN) == LOW)
      {
        delay(10);
      }
    }
  }

  lastButtonState = buttonState;

  delay(10);
}