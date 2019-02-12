#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define MESSAGE_MAGIC 0xBEEF
#define STRIP_COUNT 3
#define SAVE_CONFIG_EVERY_SECS 60

Adafruit_NeoPixel strips[STRIP_COUNT] = {
    Adafruit_NeoPixel(16, 6, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(4, 8, NEO_GRB + NEO_KHZ800)
};

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
    uint16_t Zone;
    uint16_t Parameter0;
    uint16_t Parameter1;
    Color_t Color;
} msgIn, msgOut;



struct StoreStruct
{
    uint32_t StaticColor;
    bool StaticActivate;

    uint32_t TheaterChaseColor;
    uint16_t TheaterChaseWait;
    bool TheaterChaseRainbow;
    bool TheaterChaseActivate;

    uint16_t RainbowWait;
    bool RainbowCycle;
    bool RainbowActivate;

    uint32_t WipeColor;
    uint16_t WipeWait;
    bool WipeActivate;
};
StoreStruct stripConfig[STRIP_COUNT];

void setup()
{
    msgOut.Magic = MESSAGE_MAGIC;
    Serial.begin(9600);
    Serial.println("Hello");
    uint16_t eeAddress = 0;

    for (uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++)
    {
        EEPROM.get(eeAddress, stripConfig[stripIndex]);
        eeAddress += sizeof(StoreStruct);

        // Serial.print("Strip: ");
        // Serial.println(stripIndex);
        // Serial.print("StaticColor: ");
        // Serial.println(stripConfig[stripIndex].StaticColor, HEX);
        // Serial.println("--------------------");
    }

    for(uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++){
        strips[stripIndex].begin();
        strips[stripIndex].show();
    }
}

bool configChanged;
unsigned long saveConfigMillis;

unsigned long theaterChaseMillis[STRIP_COUNT];
unsigned long colorWipeMillis[STRIP_COUNT];
unsigned long rainbowMillis[STRIP_COUNT];
unsigned long wipeMillis[STRIP_COUNT];

uint16_t theaterChaseCounter[STRIP_COUNT];
uint16_t theaterChaseRainbowCycleCounter[STRIP_COUNT];
uint16_t rainbowCounter[STRIP_COUNT];
uint16_t wipeCounter[STRIP_COUNT];

void loop()
{
    if (MessageReady())
    {
        ReadMessage();
        if (msgIn.Magic == MESSAGE_MAGIC)
        {
            switch (msgIn.Command)
            {
            case Command_t::Ping:
                msgOut.Command = Command_t::Pong;
                break;
            case Command_t::SetColor:

                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    strips[msgIn.Zone - 1].setPixelColor(msgIn.Parameter0, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[msgIn.Zone - 1].show();
                }
                msgOut.Command = Command_t::SetColor;
                break;
            case Command_t::SetColorAll:
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    for (uint16_t i = 0; i < strips[msgIn.Zone - 1].numPixels(); i++)
                        strips[msgIn.Zone - 1].setPixelColor(i, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[msgIn.Zone - 1].show();
                }
                msgOut.Command = Command_t::SetColor;
                break;
            case Command_t::SetMode:
                uint8_t mode = (msgIn.Parameter1 >> 8) & 0xff;
                uint8_t param1 = msgIn.Parameter1 & 0xff;
                switch(mode){
                    case 1:
                        if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                        {

                            stripConfig[msgIn.Zone - 1].StaticActivate = true;
                            stripConfig[msgIn.Zone - 1].TheaterChaseActivate = false;
                            stripConfig[msgIn.Zone - 1].RainbowActivate = false;
                            stripConfig[msgIn.Zone - 1].WipeActivate = false;

                            stripConfig[msgIn.Zone - 1].StaticColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);

                            for (uint16_t i = 0; i < strips[msgIn.Zone - 1].numPixels(); i++){
                                strips[msgIn.Zone - 1].setPixelColor(i, stripConfig[msgIn.Zone - 1].StaticColor);
                            }
                            strips[msgIn.Zone - 1].show();
                            configChanged = true;
                        }
                        break;
                    case 2: //Theater chase
                        if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                        {
                            stripConfig[msgIn.Zone - 1].StaticActivate = false;
                            stripConfig[msgIn.Zone - 1].TheaterChaseActivate = true;
                            stripConfig[msgIn.Zone - 1].RainbowActivate = false;
                            stripConfig[msgIn.Zone - 1].WipeActivate = false;

                            stripConfig[msgIn.Zone - 1].TheaterChaseColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                            stripConfig[msgIn.Zone - 1].TheaterChaseWait = msgIn.Parameter0;
                            stripConfig[msgIn.Zone - 1].TheaterChaseRainbow = ((param1 & 0x1) == 0x1);
                            configChanged = true;
                        }
                        break;
                    case 3: //rainbow
                        if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                        {
                            stripConfig[msgIn.Zone - 1].StaticActivate = false;
                            stripConfig[msgIn.Zone - 1].TheaterChaseActivate = false;
                            stripConfig[msgIn.Zone - 1].RainbowActivate = true;
                            stripConfig[msgIn.Zone - 1].WipeActivate = false;

                            stripConfig[msgIn.Zone - 1].RainbowWait = msgIn.Parameter0;
                            stripConfig[msgIn.Zone - 1].RainbowCycle = ((param1 & 0x1) == 0x1);
                            configChanged = true;
                        }
                        break;
                    case 4:
                        if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                        {
                            stripConfig[msgIn.Zone - 1].StaticActivate = false;
                            stripConfig[msgIn.Zone - 1].TheaterChaseActivate = false;
                            stripConfig[msgIn.Zone - 1].RainbowActivate = false;
                            stripConfig[msgIn.Zone - 1].WipeActivate = true;

                            stripConfig[msgIn.Zone - 1].WipeColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                            stripConfig[msgIn.Zone - 1].WipeWait = msgIn.Parameter0;
                            configChanged = true;
                        }
                        break;
                }
                break;
            case Command_t::ClearColor:
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    stripConfig[msgIn.Zone - 1].StaticActivate = false;
                    stripConfig[msgIn.Zone - 1].TheaterChaseActivate = false;
                    stripConfig[msgIn.Zone - 1].RainbowActivate = false;
                    stripConfig[msgIn.Zone - 1].WipeActivate = false;
                    setAllStrips(0, 0, 0);
                    configChanged = true;
                }
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
        if (stripConfig[stripIndex].TheaterChaseActivate && millis() - theaterChaseMillis[stripIndex] >= stripConfig[stripIndex].TheaterChaseWait)
        {
            theaterChaseMillis[stripIndex] = millis();
            theaterChase(strips[stripIndex], theaterChaseCounter[stripIndex], theaterChaseRainbowCycleCounter[stripIndex], stripConfig[stripIndex].TheaterChaseColor, stripConfig[stripIndex].TheaterChaseRainbow);
        }
        else if (stripConfig[stripIndex].RainbowActivate && millis() - rainbowMillis[stripIndex] >= stripConfig[stripIndex].RainbowWait)
        {
            rainbowMillis[stripIndex] = millis();
            rainbow(strips[stripIndex], rainbowCounter[stripIndex], stripConfig[stripIndex].RainbowCycle);
        }
        else if (stripConfig[stripIndex].WipeActivate && millis() - wipeMillis[stripIndex] >= stripConfig[stripIndex].WipeWait)
        {
            wipeMillis[stripIndex] = millis();
            colorWipe(strips[stripIndex], wipeCounter[stripIndex], stripConfig[stripIndex].WipeColor);
        }
    }

    if (configChanged && millis() - saveConfigMillis >= SAVE_CONFIG_EVERY_SECS){
        saveConfigMillis = millis();
        configChanged = false;

        uint16_t eeAddress = 0;
        for (uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++)
        {
            EEPROM.put(eeAddress, stripConfig[stripIndex]);
            eeAddress += sizeof(StoreStruct);
        }
    }
}

void theaterChase(Adafruit_NeoPixel &_strip, uint16_t &_theaterChaseCounter, uint16_t &_theaterChaseRainbowCycleCounter, uint32_t color, bool rainbow)
{
    if (rainbow){
        for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
        {
            _strip.setPixelColor(i + _theaterChaseCounter, Wheel((i + _theaterChaseRainbowCycleCounter) % 255)); //turn every third pixel on
        }
    } else {
        for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
        {
            _strip.setPixelColor(i + _theaterChaseCounter, color);
        }
    }
    
    _strip.show();
    for (uint16_t i = 0; i < _strip.numPixels(); i = i + 3)
    {
        _strip.setPixelColor(i + _theaterChaseCounter, 0);
    }

    _theaterChaseCounter++;
    _theaterChaseRainbowCycleCounter++;
    if (_theaterChaseCounter >= 3)
        _theaterChaseCounter = 0;
    if (_theaterChaseRainbowCycleCounter >= 256)
        _theaterChaseRainbowCycleCounter = 0;
}


void rainbow(Adafruit_NeoPixel &_strip, uint16_t &_rainbowCounter, bool cycle)
{
    if (cycle){
        for (uint16_t i = 0; i < _strip.numPixels(); i++)
        {
            _strip.setPixelColor(i, Wheel(((i * 256 / _strip.numPixels()) + _rainbowCounter) & 255));
        }
    } else {
        for (uint16_t i = 0; i < _strip.numPixels(); i++)
        {
            _strip.setPixelColor(i, Wheel((i + _rainbowCounter) & 255));
        }
    }
    
    _strip.show();

    _rainbowCounter++;
    if (cycle){
        if (_rainbowCounter >= 256 * 5)
            _rainbowCounter = 0;
    } else {
        if (_rainbowCounter >= 256)
            _rainbowCounter = 0;
    }
}

void colorWipe(Adafruit_NeoPixel &_strip, uint16_t &colowWipeCounter, uint32_t color)
{
    _strip.setPixelColor(colowWipeCounter, color);
    _strip.show();
    _strip.setPixelColor(colowWipeCounter, 0);
    colowWipeCounter++;
    if (colowWipeCounter == _strip.numPixels())
        colowWipeCounter = 0;
}

void setAllStrips(byte r, byte g, byte b)
{
    for (uint8_t stripIndex = 0; stripIndex < sizeof(strips); stripIndex++)
    {
        for (uint16_t i = 0; i < strips[stripIndex].numPixels(); i++)
        {
            strips[stripIndex].setPixelColor(i, r, g, b);
        }
        strips[stripIndex].show();
    }
}

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

