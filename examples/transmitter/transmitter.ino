#include <xx3933.h>

/**
   Pin PWM yang dapat digunakan pada libary ini:
   D3 Timer2
   D9 Timer1
   D10 Timer1
   D11 Timer2

   sistem ini telah diuji sukses pada Board Arduino UNO,
   untuk Board selainnya saya belum melakukan pengujian.
*/
#define PIN_TX 3
#define FREQ_LF 1250000
XX3933_TRANSMIT LF(PIN_TX);

const uint16_t pattern = 0b1010001010110010;

void setup()
{
  Serial.begin(9600);
  LF.begin(FREQ_LF);
}

void loop()
{
  LF.sendData(pattern);
  Serial.println("Kirim");
}
