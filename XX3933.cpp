#include "xx3933.h"

XX3933_TRANSMIT::XX3933_TRANSMIT(uint8_t out) : _pinOut(out) {}

void XX3933_TRANSMIT::PWMon(uint32_t time)
{
    switch (_pinOut)
    {
    case 3:
        TCCR2A |= _BV(COM2B1); // Enable PWM on D3
        break;
    case 11:
        TCCR2A |= _BV(COM2A1); // Enable PWM on D11
        break;
    case 9:
        TCCR1A |= _BV(COM1A1); // Enable PWM on D9
        break;
    case 10:
        TCCR1A |= _BV(COM1B1); // Enable PWM on D10
        break;
    }
    if (time > 0)
    {
        delayMicroseconds(time);
    }
}

void XX3933_TRANSMIT::PWMoff(uint32_t time)
{
    switch (_pinOut)
    {
    case 3:
        TCCR2A &= ~_BV(COM2B1); // Disable PWM on D3
        break;
    case 11:
        TCCR2A &= ~_BV(COM2A1); // Disable PWM on D11
        break;
    case 9:
        TCCR1A &= ~_BV(COM1A1); // Disable PWM on D9
        break;
    case 10:
        TCCR1A &= ~_BV(COM1B1); // Disable PWM on D10
        break;
    }

    if (time > 0)
    {
        delayMicroseconds(time);
    }
}

bool XX3933_TRANSMIT::begin(uint32_t freq, bool async)
{
    _async = async;
    pinMode(_pinOut, OUTPUT);

    // Matikan semua timer terlebih dahulu (hanya timer yang relevan)
    if (_pinOut == 9 || _pinOut == 10)
    {
        // Timer1
        TCCR1A = 0;
        TCCR1B = 0;
        TCNT1 = 0;
    }
    else if (_pinOut == 3 || _pinOut == 11)
    {
        // Timer2
        TCCR2A = 0;
        TCCR2B = 0;
        TCNT2 = 0;
    }
    else
    {
        // Tidak valid
        return false;
    }

    // Konfigurasi prescaler dan TOP tergantung timer
    if (_pinOut == 9 || _pinOut == 10)
    {
        // === Timer1: 16-bit, untuk pin 9 & 10 ===
        struct PrescalerSetting
        {
            uint16_t bits;
            uint16_t value;
        };
        const PrescalerSetting options[] = {
            {_BV(CS10), 1},
            {_BV(CS11), 8},
            {_BV(CS11) | _BV(CS10), 64},
            {_BV(CS12), 256},
            {_BV(CS12) | _BV(CS10), 1024}};

        bool found = false;
        uint16_t top = 0;
        uint8_t bestBits = 0;

        for (auto &opt : options)
        {
            top = (F_CPU / (freq * opt.value)) - 1;
            if (top <= 65535)
            {
                bestBits = opt.bits;
                found = true;
                break;
            }
        }

        if (!found)
        {
            top = (F_CPU / 125000UL) - 1;
            bestBits = _BV(CS10);
        }

        // COM1A1 untuk D9, COM1B1 untuk D10
        // Aktifkan mode PWM output pada D10 (OC1B), non-inverting
        if (_pinOut == 9)
            TCCR1A = _BV(COM1A1) | _BV(WGM11);
        else
            TCCR1A = _BV(COM1B1) | _BV(WGM11);

        TCCR1B = _BV(WGM12) | _BV(WGM13) | bestBits;
        ICR1 = top;
        if (_pinOut == 9)
            OCR1A = top / 2;
        else
            OCR1B = top / 2;
    }
    else if (_pinOut == 3 || _pinOut == 11)
    {
        // === Timer2: 8-bit, untuk pin 3 & 11 ===
        uint8_t prescalerBits = 0;
        uint16_t prescalerValue = 0;
        uint16_t top = 0;

        if ((F_CPU / freq / 1) <= 255)
        {
            prescalerBits = _BV(CS20);
            prescalerValue = 1;
        }
        else if ((F_CPU / freq / 8) <= 255)
        {
            prescalerBits = _BV(CS21);
            prescalerValue = 8;
        }
        else if ((F_CPU / freq / 32) <= 255)
        {
            prescalerBits = _BV(CS21) | _BV(CS20);
            prescalerValue = 32;
        }
        else if ((F_CPU / freq / 64) <= 255)
        {
            prescalerBits = _BV(CS22);
            prescalerValue = 64;
        }
        else if ((F_CPU / freq / 128) <= 255)
        {
            prescalerBits = _BV(CS22) | _BV(CS20);
            prescalerValue = 128;
        }
        else if ((F_CPU / freq / 256) <= 255)
        {
            prescalerBits = _BV(CS22) | _BV(CS21);
            prescalerValue = 256;
        }
        else if ((F_CPU / freq / 1024) <= 255)
        {
            prescalerBits = _BV(CS22) | _BV(CS21) | _BV(CS20);
            prescalerValue = 1024;
        }
        else
        {
            prescalerBits = _BV(CS20);
            prescalerValue = 1;
        }

        top = (F_CPU / (freq * prescalerValue)) - 1;

        TCCR2A = _BV(WGM21) | _BV(WGM20);
        if (_pinOut == 3)
            TCCR2A |= _BV(COM2B1);
        else
            TCCR2A |= _BV(COM2A1);
        TCCR2B = _BV(WGM22) | prescalerBits;
        OCR2A = top; // digunakan sebagai TOP
        if (_pinOut == 3)
            OCR2B = top / 2;
        else
            OCR2A = top / 2;
    }
    else
    {
        return false;
    }

    return true;
}

void XX3933_TRANSMIT::burst(uint16_t durationUs)
{
    XX3933_TRANSMIT::PWMon(durationUs);
    XX3933_TRANSMIT::PWMoff(0);
}

void XX3933_TRANSMIT::sparationBit()
{
    XX3933_TRANSMIT::PWMoff(360);
}

void XX3933_TRANSMIT::preamble()
{
    // Kirimkan preamble (contoh: 10101010 Manchester encoded)
    for (int i = 0; i < (XX3933_TRANSMIT::preambleLength / 2); i++)
    {
        PWMon(348);
        PWMoff(360);
    }
}

void XX3933_TRANSMIT::sendPattern(uint16_t pattern)
{
    // Kirimkan pola wake-up 16-bit Manchester encoded
    for (int i = 15; i >= 0; i--)
    {
        bool bit = (pattern >> i) & 0x01; // Ambil bit dari pola
        XX3933_TRANSMIT::manchesterBit(bit);
    }
}

void XX3933_TRANSMIT::manchesterBit(bool bit)
{
    // Kirim bit Manchester (0: HIGH -> LOW, 1: LOW -> HIGH)
    if (bit)
    {
        PWMoff(348);
        PWMon(360);
    }
    else
    {
        PWMon(348);
        PWMoff(360);
    }
}

void XX3933_TRANSMIT::sendCarrier(uint16_t pattern)
{
    // Kirim Carrier Burst selama 2020 µs
    XX3933_TRANSMIT::burst(2020);
    XX3933_TRANSMIT::sparationBit();
    XX3933_TRANSMIT::preamble();
    XX3933_TRANSMIT::sendPattern(pattern);
}

void XX3933_TRANSMIT::sendData(uint16_t pattern16)
{
    if (XX3933_TRANSMIT::_async)
    {
        if (millis() - XX3933_TRANSMIT::_lastTimeSend >= 1500)
        {
            XX3933_TRANSMIT::sendCarrier(pattern16);
            XX3933_TRANSMIT::_lastTimeSend = millis();
        }
    }
    else
    {
        XX3933_TRANSMIT::sendCarrier(pattern16);
        // beri jeda pengiriman data
        delay(1500);
    }
}

void XX3933_TRANSMIT::end()
{
    if (_pinOut == 3 || _pinOut == 11)
    {
        TCCR2A = 0;
        TCCR2B = 0;
    }
    else if (_pinOut == 9 || _pinOut == 10)
    {
        TCCR1A = 0;
        TCCR1B = 0;
    }
    digitalWrite(_pinOut, LOW);
}

// ==================================
// ==================================
// ==================================
// ==================================
// ==================================
// ==================================

XX3933_RECEIVE::XX3933_RECEIVE(uint8_t CS_PIN, uint8_t IRQ_PIN) : _ss(CS_PIN),
                                                                  _err(true),
                                                                  _spi(&SPI),
                                                                  _spiSettings(SPISettings(2000000, MSBFIRST, SPI_MODE1)) {}

void XX3933_RECEIVE::begin(uint32_t freq, uint16_t pattern16, WAKE_OUT tOut)
{
    pinMode(_ss, OUTPUT);
    digitalWrite(_ss, LOW);
    _spi->begin();

    directCMD(0b11000100); // preset default
    setPattern(pattern16, tOut);

    directCMD(0b11000000); // clear wake up

    return true;
}

void XX3933_RECEIVE::setAGC(AGC_MOD agc)
{
    byte dt1 = read(REG1);
    switch (agc)
    {
    case AGC_AUTO:
        bitSet(dt1, 5); // AGC_UD
        break;
    case AGC_DOWN:
        bitClear(dt1, 5); // AGC_UD
        break;
    }

    write(REG1, dt1);
}

void XX3933_RECEIVE::operationMode(OPT_MOD operate, TIME_OFF autoOff = TOFF_1)
{
    byte dt0 = read(REG0);

    switch (operate)
    {
    case OPT_STD:
        setNrOfActiveAntennas(3);
        bitClear(dt0, 4);
        bitClear(dt0, 5);
        break;
    case OPT_SCAN:
        bitSet(dt0, 4);
        bitClear(dt0, 5);
        break;
    case OPT_ONOF:
        setNrOfActiveAntennas(3);
        bitSet(dt0, 5);
        // set time On/OFF mode
        byte dt4 = read(REG4);
        switch (autoOff)
        {
        case TOFF_1:
            bitClear(dt4, 6);
            bitClear(dt4, 7);
            break;
        case TOFF_2:
            bitSet(dt4, 6);
            bitClear(dt4, 7);
            break;
        case TOFF_4:
            bitClear(dt4, 6);
            bitSet(dt4, 7);
            break;
        case TOFF_8:
            bitSet(dt4, 6);
            bitSet(dt4, 7);
            break;
        }
        write(REG4, dt4);
        break;
    }
    write(REG0, dt0);
}

void XX3933_RECEIVE::printRSSI()
{
    uint8_t rssi1 = read(10);
    uint8_t rssi2 = read(11);
    uint8_t rssi3 = read(12);
    if (millis() - lastTimeRssi > 50)
    {
        Serial.print(" \tRSSI ");

        Serial.print(" | CH1: ");
        Serial.print(rssi1);
        Serial.print(" | CH2: ");
        Serial.print(rssi2);
        Serial.print(" | CH3: ");
        Serial.print(rssi3);
        Serial.println("");
        lastTimeRssi = millis();
    }
}

void XX3933_RECEIVE::directCMD(byte cmd)
{
    digitalWrite(_ss, HIGH);
    SPI.beginTransaction(_spiSettings);
    SPI.transfer(0xC0 | cmd);
    SPI.endTransaction();
    digitalWrite(_ss, LOW);
}

byte XX3933_RECEIVE::read(byte reg)
{
    byte retVal;
    digitalWrite(_ss, HIGH);
    SPI.beginTransaction(_spiSettings);
    SPI.transfer(reg | 0x40);
    retVal = SPI.transfer(0);
    SPI.endTransaction();
    digitalWrite(_ss, LOW);
    return retVal;
}

void XX3933_RECEIVE::write(byte reg, byte data)
{
    digitalWrite(_ss, HIGH);
    SPI.beginTransaction(_spiSettings);
    SPI.transfer(reg & 0x3F);
    SPI.transfer(data);
    SPI.endTransaction();
    digitalWrite(_ss, LOW);
}

uint16_t XX3933_RECEIVE::invert_bits(uint16_t x)
{
    return ~x & 0xFFFF;
}

uint16_t XX3933_RECEIVE::shiftCustom(uint16_t value)
{
    uint16_t result = value >> 1;
    if (value & 0x8000)
        result |= 0x0001;
    return result & 0x7FFF; // pastikan MSB = 0
}

void XX3933_RECEIVE::setPattern(uint16_t pattern16, WAKE_OUT tO)
{
    byte dt0 = read(REG0);
    bitSet(dt0, 7); // PAT32
    write(REG0, dt0);
    byte dt1 = read(REG1);
    bitSet(dt1, 1);   // EN_WPAT
    bitClear(dt1, 2); // EN_WPAT
    write(REG1, dt1);

    uint16_t hasil = shiftCustom(pattern16);
    uint16_t invr = invert_bits(hasil);
    uint8_t patt_r6 = (invr >> 8) & 0xFF; // ambil 8 bit atas
    uint8_t patt_r5 = invr & 0xFF;        // ambil 8 bit bawah
    write(0x05, patt_r5);
    write(0x06, patt_r6);

    byte dt7 = read(REG7);
    switch (tO)
    {
    case TOUT_NONE:
        bitClear(dt7, 5);
        bitClear(dt7, 6);
        bitClear(dt7, 7);
        break;
    case TOUT_50:
        bitSet(dt7, 5);
        bitClear(dt7, 6);
        bitClear(dt7, 7);
        break;
    case TOUT_100:
        bitClear(dt7, 5);
        bitSet(dt7, 6);
        bitClear(dt7, 7);
        break;
    case TOUT_150:
        bitClear(dt7, 5);
        bitSet(dt7, 6);
        bitSet(dt7, 7);
        break;
    case TOUT_200:
        bitSet(dt7, 5);
        bitClear(dt7, 6);
        bitClear(dt7, 7);
        break;
    case TOUT_250:
        bitSet(dt7, 5);
        bitClear(dt7, 6);
        bitSet(dt7, 7);
        break;
    case TOUT_300:
        bitSet(dt7, 5);
        bitSet(dt7, 6);
        bitClear(dt7, 7);
        break;
    case TOUT_350:
        bitSet(dt7, 5);
        bitSet(dt7, 6);
        bitSet(dt7, 7);
        break;
    }
    write(REG7, dt7);
}

void XX3933_RECEIVE::setNrOfActiveAntennas(byte number)
{
    byte dt0 = read(REG0);
    switch (number)
    {
    case 1:
        bitSet(dt0, 1);
        bitClear(dt0, 2);
        bitClear(dt0, 3);
        break;
    case 2:
        bitSet(dt0, 1);
        bitClear(dt0, 2);
        bitSet(dt0, 3);
        break;
    case 3:
        bitSet(dt0, 1);
        bitSet(dt0, 2);
        bitSet(dt0, 3);
        break;
    }
    write(REG0, dt0);
}
