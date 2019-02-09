#include <Adafruit_NeoPixel.h>


#define MESSAGE_MAGIC 0xBEEF
#define STRIP_COUNT 3

Adafruit_NeoPixel strips[STRIP_COUNT] = {
    Adafruit_NeoPixel(16, 6, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(4, 8, NEO_GRB + NEO_KHZ800)
};


void setAllStrips(byte r, byte g, byte b){
    for (uint8_t stripIndex=0; stripIndex < sizeof(strips); stripIndex++){
        for (uint16_t i = 0; i < strips[stripIndex].numPixels(); i++)
        {
            strips[stripIndex].setPixelColor(i, r, g, b);
        }
        strips[stripIndex].show();
    }
}

enum Command_t : uint8_t
{
    InvalidMessage = 0,
    InvalidRequest,
    Ping,
    Pong,
    SetColor,
    SetColorAll,
    SetMode,
    ClearColor
};

struct Color_t
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
};

struct Message_t
{
    uint16_t Magic;
    Command_t Command;
    uint16_t Parameter0;
    uint16_t Parameter1;
    Color_t Color;
} msgIn, msgOut;

bool MessageReady()
{
    //Are there enough packets available to craft a message from?
    return Serial.available() >= sizeof(Message_t);
}

void ReadMessage()
{
    uint8_t *pdata = (uint8_t *)&msgIn;

    //Read message from serial one byte at a time
    for (int i = 0; i < sizeof(Message_t); i++)
        pdata[i] = (uint8_t)Serial.read();
}

void SendMessage()
{
    uint8_t *pdata = (uint8_t *)&msgOut;
    for (int i = 0; i < sizeof(Message_t); i++)
        Serial.write(pdata[i]);
}

void ClearSerial()
{
    while (Serial.available())
        Serial.read();
}

void setup()
{
    msgOut.Magic = MESSAGE_MAGIC;
    Serial.begin(9600);
    
    for(uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++){
        strips[stripIndex].begin();
        strips[stripIndex].show();
    }

}

unsigned long theaterChaseMillis[STRIP_COUNT];
unsigned long theaterChaseRainbowMillis[STRIP_COUNT];
unsigned long colorWipeMillis[STRIP_COUNT];
unsigned long rainbowMillis[STRIP_COUNT];
unsigned long rainbowCycleMillis[STRIP_COUNT];
unsigned long wipeMillis[STRIP_COUNT];
// Adafruit_NeoPixel theaterChaseStrips[STRIP_COUNT] = {strip, strip1};
uint32_t theaterChaseColor[STRIP_COUNT];
bool theaterChaseActivated[STRIP_COUNT];
uint16_t theaterChaseWait[STRIP_COUNT];
uint16_t theaterChaseCounter[STRIP_COUNT];

bool theaterChaseRainbowActivated[STRIP_COUNT];
uint16_t theaterChaseRainbowWait[STRIP_COUNT];
uint16_t theaterChaseRainbowCounter[STRIP_COUNT];
uint16_t theaterChaseRainbowCycleCounter[STRIP_COUNT];

bool rainbowActivated[STRIP_COUNT];
uint16_t rainbowWait[STRIP_COUNT];
uint16_t rainbowCounter[STRIP_COUNT];

bool rainbowCycleActivated[STRIP_COUNT];
uint16_t rainbowCycleWait[STRIP_COUNT];
uint16_t rainbowCycleCounter[STRIP_COUNT];

uint32_t wipeColor[STRIP_COUNT];
bool wipeActivated[STRIP_COUNT];
uint16_t wipeWait[STRIP_COUNT];
uint16_t wipeCounter[STRIP_COUNT];

void loop()
{
    if (MessageReady())
    {
        ReadMessage();
        if (msgIn.Magic == MESSAGE_MAGIC)
        {
            uint16_t param0 = msgIn.Parameter0; //65535 max
            uint8_t zone = (msgIn.Parameter1 >> 8) & 0xff; //255 max
            uint8_t mode = msgIn.Parameter1 & 0xff;        //255 max
            switch (msgIn.Command)
            {
            case Command_t::Ping:
                msgOut.Command = Command_t::Pong;
                break;
            case Command_t::SetColor:

                if (zone > 0 && zone <= STRIP_COUNT)
                {
                    strips[zone - 1].setPixelColor(param0, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[zone - 1].show();
                }
                msgOut.Command = Command_t::SetColor;
                break;
            case Command_t::SetColorAll:
                if (zone > 0 && zone <= STRIP_COUNT)
                {
                    for (uint16_t i = 0; i < strips[zone - 1].numPixels(); i++)
                        strips[zone - 1].setPixelColor(i, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[zone - 1].show();
                }
                msgOut.Command = Command_t::SetColor;
                break;
            case Command_t::SetMode:
                switch(mode){
                    case 1:
                        if (zone > 0 && zone <= STRIP_COUNT)
                        {
                            theaterChaseActivated[zone - 1] = false;
                            theaterChaseRainbowActivated[zone - 1] = false;
                            rainbowActivated[zone - 1] = false;
                            rainbowCycleActivated[zone - 1] = false;
                            wipeActivated[zone - 1] = false;

                            for (uint16_t i = 0; i < strips[zone - 1].numPixels(); i++){
                                strips[zone - 1].setPixelColor(i, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                            }
                            strips[zone - 1].show();
                        }
                        break;
                    case 2: //Theater chase
                        if (zone > 0 && zone <= STRIP_COUNT)
                        {
                            theaterChaseActivated[zone - 1] = true;
                            theaterChaseRainbowActivated[zone - 1] = false;
                            rainbowActivated[zone - 1] = false;
                            rainbowCycleActivated[zone - 1] = false;
                            wipeActivated[zone - 1] = false;

                            theaterChaseWait[zone - 1] = param0;
                            theaterChaseColor[zone - 1] = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B); //((uint32_t)msgIn.Color.R << 16 | (uint32_t)msgIn.Color.G << 8 | msgIn.Color.B);
                        }
                        break;
                    case 3: //Theater chase rainbow
                        if (zone > 0 && zone <= STRIP_COUNT)
                        {
                            theaterChaseActivated[zone - 1] = false;
                            theaterChaseRainbowActivated[zone - 1] = true;
                            rainbowActivated[zone - 1] = false;
                            rainbowCycleActivated[zone - 1] = false;
                            wipeActivated[zone - 1] = false;

                            theaterChaseRainbowWait[zone - 1] = param0;
                        }
                        break;
                    case 4: //rainbow
                        if (zone > 0 && zone <= STRIP_COUNT)
                        {
                            theaterChaseActivated[zone - 1] = false;
                            theaterChaseRainbowActivated[zone - 1] = false;
                            rainbowActivated[zone - 1] = true;
                            rainbowCycleActivated[zone - 1] = false;
                            wipeActivated[zone - 1] = false;

                            rainbowWait[zone - 1] = param0;

                        }
                        break;
                    case 5:
                        theaterChaseActivated[zone - 1] = false;
                        theaterChaseRainbowActivated[zone - 1] = false;
                        rainbowActivated[zone - 1] = false;
                        rainbowCycleActivated[zone - 1] = true;
                        wipeActivated[zone - 1] = false;

                        rainbowCycleWait[zone - 1] = param0;
                        break;
                    case 6:
                        theaterChaseActivated[zone - 1] = false;
                        theaterChaseRainbowActivated[zone - 1] = false;
                        rainbowActivated[zone - 1] = false;
                        rainbowCycleActivated[zone - 1] = false;
                        wipeActivated[zone - 1] = true;

                        wipeWait[zone - 1] = param0;
                        wipeColor[zone - 1] = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                        break;
                }
                break;
            case Command_t::ClearColor:
                setAllStrips(0, 0, 0);
                break;
            default:
                msgOut.Command = Command_t::InvalidRequest;
                break;
            }
        }
        else
        {
            ClearSerial();
            msgOut.Command = Command_t::InvalidMessage;
        }
        SendMessage();
    }

    for (uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++)
    {
        if (theaterChaseActivated[stripIndex] && millis() - theaterChaseMillis[stripIndex] >= theaterChaseWait[stripIndex])
        {
            theaterChaseMillis[stripIndex] = millis();

            theaterChase(theaterChaseColor[stripIndex], strips[stripIndex], theaterChaseCounter[stripIndex]);
        }

        if (theaterChaseRainbowActivated[stripIndex] && millis() - theaterChaseRainbowMillis[stripIndex] >= theaterChaseRainbowWait[stripIndex])
        {
            theaterChaseRainbowMillis[stripIndex] = millis();

            theaterChaseRainbow(strips[stripIndex], theaterChaseRainbowCounter[stripIndex], theaterChaseRainbowCycleCounter[stripIndex]);
        }

        if (rainbowActivated[stripIndex] && millis() - rainbowMillis[stripIndex] >= rainbowWait[stripIndex])
        {
            rainbowMillis[stripIndex] = millis();

            rainbow(strips[stripIndex], rainbowCounter[stripIndex]);
        }

        if (rainbowCycleActivated[stripIndex] && millis() - rainbowCycleMillis[stripIndex] >= rainbowCycleWait[stripIndex])
        {
            rainbowCycleMillis[stripIndex] = millis();

            rainbowCycle(strips[stripIndex], rainbowCycleCounter[stripIndex]);
        }

        if (wipeActivated[stripIndex] && millis() - wipeMillis[stripIndex] >= wipeWait[stripIndex])
        {
            wipeMillis[stripIndex] = millis();

            colorWipe(wipeColor[stripIndex], strips[stripIndex], wipeCounter[stripIndex]);
        }
    }
}


void theaterChase(uint32_t &color, Adafruit_NeoPixel &_strip, uint16_t &_theaterChaseCounter)
{
    for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
    {
        _strip.setPixelColor(i + _theaterChaseCounter, color);
    }
    _strip.show();
    for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
    {
        _strip.setPixelColor(i + _theaterChaseCounter, 0);
    }

    _theaterChaseCounter++;
    if (_theaterChaseCounter >= 3)
        _theaterChaseCounter = 0;
}



void theaterChaseRainbow(Adafruit_NeoPixel &_strip, uint16_t &_theaterChaseRainbowCounter, uint16_t &_theaterChaseRainbowCycleCounter)
{
    for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
    {
        _strip.setPixelColor(i + _theaterChaseRainbowCounter, Wheel((i + _theaterChaseRainbowCycleCounter) % 255)); //turn every third pixel on
    }

    _strip.show();
    for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
    {
        _strip.setPixelColor(i + _theaterChaseRainbowCounter, 0); //turn every third pixel off
    }
    _theaterChaseRainbowCounter++;
    _theaterChaseRainbowCycleCounter++;
    if (_theaterChaseRainbowCounter >= 3)
        _theaterChaseRainbowCounter = 0;
    if (_theaterChaseRainbowCycleCounter >= 256)
        _theaterChaseRainbowCycleCounter = 0;

}


void colorWipe(uint32_t &color, Adafruit_NeoPixel &_strip, uint16_t &colowWipeCounter)
{
    _strip.setPixelColor(colowWipeCounter, color);
    _strip.show();
     _strip.setPixelColor(colowWipeCounter, 0);
    colowWipeCounter++;
    if (colowWipeCounter == _strip.numPixels())
        colowWipeCounter = 0;
}



void rainbow(Adafruit_NeoPixel &_strip, uint16_t &rainbowCounter)
{
    for (uint16_t i = 0; i < _strip.numPixels(); i++)
    {
        _strip.setPixelColor(i, Wheel((i + rainbowCounter) & 255));
    }
    _strip.show();

    rainbowCounter++;
    if (rainbowCounter >= 256)
        rainbowCounter = 0;
}


void rainbowCycle(Adafruit_NeoPixel &_strip, uint16_t &_rainbowCycleCounter)
{
    for (uint16_t i = 0; i < _strip.numPixels(); i++)
    {
        _strip.setPixelColor(i, Wheel(((i * 256 / _strip.numPixels()) + _rainbowCycleCounter) & 255));
    }
    _strip.show();

    _rainbowCycleCounter++;
    if (_rainbowCycleCounter >= 256 * 5)
        _rainbowCycleCounter = 0;

}

uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t Wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}