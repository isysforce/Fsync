#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define MESSAGE_MAGIC 0xBEEF
#define STRIP_COUNT 3
#define SAVE_CONFIG_EVERY_SECS 10000
#define audioPin A1

Adafruit_NeoPixel strips[STRIP_COUNT] = {
    Adafruit_NeoPixel(16, 6, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(4, 8, NEO_GRB + NEO_KHZ800)
};

enum Command_t : uint8_t
{
    InvalidMessage = 0,
    InvalidRequest,
    InvalidParameter,
    Ping,
    Pong,
    SetColor,
    SetColorAll,
    SetMode,
    SetNumLed,
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
    uint16_t NumLeds;

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

    uint32_t FlashColor;
    uint16_t FlashWait;
    bool FlashActivate;

    bool AudioVisActivate;
    bool AudioRainbow;
    uint16_t AudioDivide;
};
StoreStruct stripConfig[STRIP_COUNT];

int audioCounters[STRIP_COUNT];

void setup()
{
    pinMode(audioPin, INPUT);
    msgOut.Magic = MESSAGE_MAGIC;
    Serial.begin(9600);
    // Serial.println("Hello");
    uint16_t eeAddress = 0;

    for (uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++)
    {
        // EEPROM.put(eeAddress, stripConfig[stripIndex]);
        EEPROM.get(eeAddress, stripConfig[stripIndex]);
        eeAddress += sizeof(StoreStruct);

        strips[stripIndex].updateLength(stripConfig[stripIndex].NumLeds);
    }

    for(uint8_t stripIndex = 0; stripIndex < STRIP_COUNT; stripIndex++){
        strips[stripIndex].begin();
        strips[stripIndex].show();
        audioCounters[stripIndex] = 255;
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

unsigned long audioMillis;
unsigned long keyboardMillis;
uint16_t keyboardLed;

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
                msgOut.Command = Command_t::SetColor;
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    strips[msgIn.Zone - 1].setPixelColor(msgIn.Parameter0, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[msgIn.Zone - 1].show();
                }
                break;
            case Command_t::SetColorAll:
                msgOut.Command = Command_t::SetColorAll;
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    for (uint16_t i = 0; i < strips[msgIn.Zone - 1].numPixels(); i++)
                        strips[msgIn.Zone - 1].setPixelColor(i, msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                    strips[msgIn.Zone - 1].show();
                }
                
                break;
            case Command_t::ClearColor:
                msgOut.Command = Command_t::ClearColor;
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    stripConfig[msgIn.Zone - 1].StaticActivate = false;
                    stripConfig[msgIn.Zone - 1].TheaterChaseActivate = false;
                    stripConfig[msgIn.Zone - 1].RainbowActivate = false;
                    stripConfig[msgIn.Zone - 1].WipeActivate = false;
                    stripConfig[msgIn.Zone - 1].AudioVisActivate = false;
                    setAllStrips(0, 0, 0);
                    configChanged = true;
                }
                break;
            case Command_t::SetNumLed:
                msgOut.Command = Command_t::ClearColor;
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT)
                {
                    stripConfig[msgIn.Zone - 1].NumLeds = msgIn.Parameter0;
                    strips[msgIn.Zone - 1].updateLength(stripConfig[msgIn.Zone - 1].NumLeds);
                    configChanged = true;
                }
                break;
            case Command_t::SetMode:
                msgOut.Command = Command_t::SetMode;
                uint8_t mode = (msgIn.Parameter1 >> 8) & 0xff;
                uint8_t param1 = msgIn.Parameter1 & 0xff;
                if (msgIn.Zone > 0 && msgIn.Zone <= STRIP_COUNT){
                    StoreStruct &stripZone = stripConfig[msgIn.Zone - 1];
                    switch(mode)
                    {
                    case 1:
                        stripZone.StaticActivate = true;
                        stripZone.TheaterChaseActivate = false;
                        stripZone.RainbowActivate = false;
                        stripZone.WipeActivate = false;
                        stripZone.AudioVisActivate = false;

                        stripZone.StaticColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);

                        for (uint16_t i = 0; i < strips[msgIn.Zone - 1].numPixels(); i++){
                            strips[msgIn.Zone - 1].setPixelColor(i, stripZone.StaticColor);
                        }
                        strips[msgIn.Zone - 1].show();
                        configChanged = true;
                        
                        break;
                    case 2: //Theater chase
                        stripZone.StaticActivate = false;
                        stripZone.TheaterChaseActivate = true;
                        stripZone.RainbowActivate = false;
                        stripZone.WipeActivate = false;
                        stripZone.AudioVisActivate = false;

                        stripZone.TheaterChaseColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                        stripZone.TheaterChaseWait = msgIn.Parameter0;
                        stripZone.TheaterChaseRainbow = ((param1 & 0x1) == 0x1);
                        configChanged = true;
                        break;
                    case 3: //rainbow
                        stripZone.StaticActivate = false;
                        stripZone.TheaterChaseActivate = false;
                        stripZone.RainbowActivate = true;
                        stripZone.WipeActivate = false;
                        stripZone.AudioVisActivate = false;

                        stripZone.RainbowWait = msgIn.Parameter0;
                        stripZone.RainbowCycle = ((param1 & 0x1) == 0x1);
                        configChanged = true;
                        break;
                    case 4:
                        stripZone.StaticActivate = false;
                        stripZone.TheaterChaseActivate = false;
                        stripZone.RainbowActivate = false;
                        stripZone.WipeActivate = true;
                        stripZone.AudioVisActivate = false;

                        stripZone.WipeColor = Color(msgIn.Color.R, msgIn.Color.G, msgIn.Color.B);
                        stripZone.WipeWait = msgIn.Parameter0;
                        configChanged = true;
                        break;
                    case 5: //Flash
                        break;
                    case 6: //Audio
                        stripZone.StaticActivate = false;
                        stripZone.TheaterChaseActivate = false;
                        stripZone.RainbowActivate = false;
                        stripZone.WipeActivate = false;
                        stripZone.AudioVisActivate = true;

                        stripZone.AudioDivide = msgIn.Parameter0;
                        stripZone.AudioRainbow = ((param1 & 0x1) == 0x1);

                        configChanged = true;
                        break;
                    default:
                        msgOut.Command = Command_t::InvalidParameter;
                        break;
                    }
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
        else if (stripConfig[stripIndex].AudioVisActivate)
        {
            audioVisualizer(strips[stripIndex], audioCounters[stripIndex], stripConfig[stripIndex].AudioDivide, stripConfig[stripIndex].AudioRainbow);
        }
        
    }

    // if (millis() - keyboardMillis >= 1000){
    //     keyboardMillis = millis();

    //     keyboardLed++;
    //     if (keyboardLed >= keyboardStrip.numPixels()){
    //         keyboardLed = 0;
    //     }
    // }

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


int decay = 0;
int decay_check = 0;
long pre_react = 0;
long react = 0;
// long react_divide = 255L;
long react_multiply = 0;

void audioVisualizer(Adafruit_NeoPixel &_strip, int &counter, long react_divide, bool rainbow)
{
    int audio_input = analogRead(audioPin);
    if (audio_input > 0){
        audioMillis = millis();
        pre_react = (_strip.numPixels() * (long)audio_input) / react_divide;

        if (pre_react > react)
        {
            react = pre_react;
        }

        audioRainbow(_strip, counter, 1);

        decay_check++;
        if (decay_check > decay)
        {
            decay_check = 0;
            if (react > 0)
                react--;
        }
    }
}

void audioRainbow(Adafruit_NeoPixel &_strip, int &counter, int speed){
    for(uint16_t i = _strip.numPixels() ; i > 0; i--){
        if (i < react)
        {
            // _strip.setPixelColor(i, Color(0, 0, 255));
            // _strip.setPixelColor(_strip.numPixels() - i, Color(0, 0, 255));
            _strip.setPixelColor(i, Wheel(((i * 256 / _strip.numPixels()) + counter) % 256));
            _strip.setPixelColor(_strip.numPixels() - i, Wheel(((i * 256 / _strip.numPixels()) + counter) % 256));
        }
        else 
        {
            _strip.setPixelColor(i, 0);
        }
    }
    _strip.show();

    counter = counter - speed; // SPEED OF COLOR WHEEL
    if (counter < 0)           // RESET COLOR WHEEL
        counter = 255;
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

