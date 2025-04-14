#ifndef XX3933_LIB_H
#define XX3933_LIB_H
#include "Arduino.h"
#include <SPI.h>
/**
 * @warning
 * Library Pemancar gelombang Modulasi untuk AS3933/Si3933 ini
 * hanya berlaku untuk berjalan di arduino Uno dengan frequensi kerja 16MHz
 * menggunakan Timer1 untuk mengolah frequensi pemancarnya.
 * untuk mikrokotroller lainnya masih dalam tahap pengembangan.
 */
#ifdef defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
class XX3933_TRANSMIT
{
public:
    XX3933_TRANSMIT(uint8_t out);

    bool begin(uint32_t freq, bool async = false);

    void sendData(uint16_t pattern16);

    void end();

private:
    uint8_t _pinOut;
    bool _async;
    uint32_t _lastTimeSend;
    const uint8_t preambleLength = 8;

    void PWMon(uint32_t time);
    void PWMoff(uint32_t time);

    void burst(uint16_t durationUs);
    void sparationBit();
    void preamble();
    void sendPattern(uint16_t pattern);
    void manchesterBit(bool bit);

    void sendCarrier(uint16_t pattern);
};

#endif

class XX3933_RECEIVE
{
public:
    enum OPT_MOD // Mode Operasi Chip
    {
        OPT_STD = 0,
        OPT_SCAN = 1,
        OPT_ONOF = 2
    };
    enum TIME_OFF // On/Off Operate mode
    {
        TOFF_1, // 1 ms
        TOFF_2, // 2ms
        TOFF_4, // 4ms
        TOFF_8, // 8ms
    };
    enum AGC_MOD // mode AGC
    {
        AGC_AUTO,
        AGC_DOWN
    };
    enum WAKE_OUT // Time Out/clear Wake-up
    {
        TOUT_NONE = 0,
        TOUT_50,
        TOUT_100,
        TOUT_150,
        TOUT_200,
        TOUT_250,
        TOUT_300,
        TOUT_350,
    };

public:
    XX3933_RECEIVE(uint8_t CS_PIN, gpio_num_t IRQ_PIN);
    bool begin(uint32_t freq, uint16_t pattern16, WAKE_OUT tOut = TOUT_350);

    void attachInterrupt(void (*isr)()) __attribute__((always_inline))
    {
// Cek apakah bangun dari deep sleep
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
        pinMode(_irqPin, INPUT_PULLUP);
        ::attachInterrupt(digitalPinToInterrupt(_irqPin), isr, FALLING)
#elif defined(ESP32)
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
        {
            Serial.println("WAKE-UP MCU MODE");
            isr();
        }
        else
        {
            ::attachInterrupt(_irqPin, isr, RISING); // ESP32 pakai pin langsung
            Serial.println("Normal Boot MCU");
        }
#endif
    }

    void goToSleep()
    {
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_mode(); // 💤 Masuk sleep, akan bangun oleh ISR
        sleep_disable();
#elif defined(ESP32)
        esp_sleep_enable_ext0_wakeup(_irqPin, 1); // 1 = HIGH
        esp_deep_sleep_start();                   // Tidur di sini
#endif
    }

    void setAGC(AGC_MOD agc);
    void operationMode(OPT_MOD operate, TIME_OFF autoOff = TOFF_1);
    void printRSSI();

private:
    SPIClass *_spi;
    SPISettings _spiSettings;
    gpio_num_t _irqPin;
    int8_t _ss;
    boolean _err;
    uint32_t lastTimeRssi = 0;
    //
    enum REGISTER
    {
        REG0 = 0x00,
        REG1 = 0x01,
        REG2 = 0x02,
        REG3 = 0x03,
        REG4 = 0x04,
        REG5 = 0x05,
        REG6 = 0x06,
        REG7 = 0x07,
        REG8 = 0x08,
        REG21 = 0x15,
    };
    //
    void directCMD(byte cmd);
    byte read(byte reg);
    void write(byte reg, byte data);
    uint16_t invert_bits(uint16_t x);
    uint16_t shiftCustom(uint16_t value);
    void setPattern(uint16_t pattern16, WAKE_OUT tO);
    void setNrOfActiveAntennas(byte number);
};

#endif
