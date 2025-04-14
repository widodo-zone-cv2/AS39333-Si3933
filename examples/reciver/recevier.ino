#include <XX3933.h>

#define PIN_CS 5
#define PIN_WAKEUP GPIO_NUM_33

XX3933_RECEIVE LF(PIN_CS, PIN_WAKEUP);

bool detected = false;
void cllback()
{
    digitalWrite(2, HIGH);
}

void setup()
{
    Serial.begin(115200);
    delay(50);
    pinMode(2, OUTPUT);

    LF.begin(125000, 0b1110001111010010, XX3933_RECEIVE::WAKE_OUT::TOUT_350);
    LF.attachInterrupt(cllback); // contoh trigger RISING
    LF.setAGC(XX3933_RECEIVE::AGC_MOD::AGC_AUTO);
    LF.operationMode(XX3933_RECEIVE::OPT_MOD::OPT_ONOF, XX3933_RECEIVE::TIME_OFF::TOFF_1);

    delay(200); // beri waktu pesan terkirim
    digitalWrite(2, LOW);
    LF.goToSleep();
}

void loop()
{
}
