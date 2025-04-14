#include <xx3933.h>

#define PIN_WAKEUP 22
AS3933 LF(5);

volatile bool detected = false;
void IRAM_ATTR onWakeupDetected()
{
    detected = true;
}
void setup()
{
    Serial.begin(115200);
    pinMode(PIN_WAKEUP, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_WAKEUP), onWakeupDetected, RISING);
    delay(2000);
    LF.begin(125000, 0b1010001010110010, AS3933::WAKE_OUT::TOUT_350);
    LF.setAGC(AS3933::AGC_MOD::AGC_AUTO);
    LF.operationMode(AS3933::OPT_MOD::OPT_ONOF, AS3933::TIME_OFF::TOFF_1);
}

void loop()
{
    if (detected)
    {
        Serial.println("WAKE-PIN INTERRUPTIONS.");
        detected = false;
    }
}
