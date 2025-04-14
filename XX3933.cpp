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
