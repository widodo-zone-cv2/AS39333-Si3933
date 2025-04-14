#include <XX3933.h>

#define PIN_CS 5
#define PIN_WAKEUP 22

XX3933_RECEIVE LF(PIN_CS, PIN_WAKEUP);

volatile bool detected = false;
void IRAM_ATTR cllback()
{
    detected = true;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    LF.begin(125000, 0b1110001111010010, XX3933_RECEIVE::WAKE_OUT::TOUT_350);
    LF.attachInterrupt(cllback, RISING); // contoh trigger RISING
    LF.setAGC(XX3933_RECEIVE::AGC_MOD::AGC_AUTO);
    LF.operationMode(XX3933_RECEIVE::OPT_MOD::OPT_ONOF, XX3933_RECEIVE::TIME_OFF::TOFF_1);
}

void loop()
{
    if (detected)
    {
        Serial.println("WAKE-PIN INTERRUPTIONS.");
        detected = false;
    }
}
