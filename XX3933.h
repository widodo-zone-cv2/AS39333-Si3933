#ifndef XX3933_LIB_H
#define XX3933_LIB_H
#include "Arduino.h"

/**
 * @warning
 * Library Pemancar gelombang Modulasi untuk AS3933/Si3933 ini
 * hanya berlaku untuk berjalan di arduino Uno dengan frequensi kerja 16MHz
 * menggunakan Timer1 untuk mengolah frequensi pemancarnya.
 * untuk mikrokotroller lainnya masih dalam tahap pengembangan.
 */
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
