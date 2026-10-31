#include "AW86224Driver.h"
#include "configuration.h"

AW86224Driver aw86224;

void AW86224Driver::begin()
{
    // I2C probe
    wire->beginTransmission(AW86224_ADDR);
    uint8_t status = wire->endTransmission();
    if (status != 0) {
        LOG_INFO("[AW86224] Not found (I2C addr 0x%02X, status=%d)", AW86224_ADDR, status);
        present = false;
        return;
    }

    // Soft reset
    writeReg(REG_SRST, 0xAA);
    delay(10);

    present = true;

    // F0 detection: find the motor's resonant frequency
    writeReg(REG_F0_CTRL, 0x01); // start F0 detection
    delay(100); // wait for detection to complete

    uint8_t f0hi = readReg(REG_F0_HI);
    uint8_t f0lo = readReg(REG_F0_LO);
    f0 = ((uint32_t)f0hi << 8) | f0lo; // in 0.1Hz units

    LOG_INFO("[AW86224] LRA motor found, F0=%.1f Hz (raw=0x%04X)", f0 / 10.0, f0);

    // If F0 detection failed (0 or too low), use default 235Hz
    if (f0 < 1000 || f0 > 30000) {
        f0 = 2350; // 235.0 Hz
        LOG_INFO("[AW86224] F0 detection failed, using default 235.0 Hz");
    }

    // Load sine wave at F0 into RAM
    uint16_t freqHz = f0 / 10;
    loadSineWave(freqHz, 100, 2); // amplitude 100, 2 full periods

    // Configure RAM playback mode
    // PLAY_MODE = 0 (RAM mode) in register 0x08
    writeReg(REG_PLAYCFG3, 0x04); // PLAY_MODE=0, BRK_EN=1 (default)

    // Set playback queue: waveform 1
    writeReg(REG_WAVSEQ1, 0x01); // waveform ID 1
    writeReg(REG_WAVSEQ2, 0x00);
    writeReg(REG_WAVSEQ3, 0x00);
    writeReg(REG_WAVSEQ4, 0x00);

    // Set gain to max (0xF = 15/15 = 100%)
    writeReg(REG_PLAYCFG2, 0xF0);

    LOG_INFO("[AW86224] LRA vibration motor initialized (addr 0x%02X, F0=%.1f Hz)", AW86224_ADDR, f0 / 10.0);
}

void AW86224Driver::loadSineWave(uint16_t freqHz, uint8_t amplitude, uint8_t num_periods)
{
    // Generate a sine wave into SRAM at the motor's resonant frequency
    // AW86224 sample rate: 12kHz
    uint16_t samples_per_period = 12000 / freqHz;
    if (samples_per_period < 8) samples_per_period = 8;
    if (samples_per_period > 200) samples_per_period = 200;

    uint16_t total_samples = samples_per_period * num_periods;
    if (total_samples > 255) total_samples = 255;
    waveform_samples = total_samples;

    // SRAM base address: 0x3800 (from register 0x2D=0x38, 0x2E=0x00)
    uint16_t base_addr = 0x3800;

    // SRAM layout:
    // - 1 byte: waveform version
    // - 4 bytes: waveform header (start_hi, start_lo, end_hi, end_lo)
    // - N bytes: waveform data

    // Set RAM write address to base_addr
    writeReg(REG_RAMADDRH, (base_addr >> 8) & 0xFF);
    writeReg(REG_RAMADDRL, base_addr & 0xFF);

    // Enable SRAM initialization
    uint8_t sysctrl1 = readReg(REG_SYSCTRL1);
    writeReg(REG_SYSCTRL1, sysctrl1 | 0x02); // set EN_RAMINIT bit

    // Write waveform library version (1 byte)
    wire->beginTransmission(AW86224_ADDR);
    wire->write(REG_RAMDATA);
    wire->write(0x01); // version 1
    wire->endTransmission();

    // Write waveform header (4 bytes): start_addr, end_addr
    // Waveform data starts at base_addr + 5 (after version + header)
    uint16_t data_start = 5; // relative to base_addr
    uint16_t data_end = data_start + total_samples - 1;

    wire->beginTransmission(AW86224_ADDR);
    wire->write(REG_RAMDATA);
    wire->write((data_start >> 8) & 0xFF); // start_addr_hi
    wire->write(data_start & 0xFF);         // start_addr_lo
    wire->write((data_end >> 8) & 0xFF);    // end_addr_hi
    wire->write(data_end & 0xFF);           // end_addr_lo
    wire->endTransmission();

    // Generate and write sine wave samples (signed 8-bit, 2's complement)
    for (uint16_t i = 0; i < total_samples; i++) {
        float phase = 2.0 * PI * (i % samples_per_period) / samples_per_period;
        int16_t sample = (int16_t)(amplitude * sin(phase));
        int8_t val = (int8_t)sample; // 2's complement

        wire->beginTransmission(AW86224_ADDR);
        wire->write(REG_RAMDATA);
        wire->write(val);
        wire->endTransmission();
    }

    // Disable SRAM initialization
    writeReg(REG_SYSCTRL1, sysctrl1 & ~0x02);

    LOG_DEBUG("[AW86224] Loaded sine wave: freq=%dHz, amp=%d, samples=%d",
              freqHz, amplitude, total_samples);
}

void AW86224Driver::vibrate(uint16_t durationMs)
{
    if (!present) return;

    // Set gain to max (0xF = 100%)
    writeReg(REG_PLAYCFG2, 0xF0);

    // Trigger GO in register 0x09
    uint8_t cfg4 = readReg(REG_PLAYCFG4);
    writeReg(REG_PLAYCFG4, cfg4 | 0x01); // set GO bit
}

void AW86224Driver::stop()
{
    if (!present) return;

    // Set STOP bit in register 0x09
    uint8_t cfg4 = readReg(REG_PLAYCFG4);
    writeReg(REG_PLAYCFG4, cfg4 | 0x02); // set STOP bit
}

void AW86224Driver::writeReg(uint8_t reg, uint8_t val)
{
    wire->beginTransmission(AW86224_ADDR);
    wire->write(reg);
    wire->write(val);
    wire->endTransmission();
}

uint8_t AW86224Driver::readReg(uint8_t reg)
{
    wire->beginTransmission(AW86224_ADDR);
    wire->write(reg);
    wire->endTransmission();
    wire->requestFrom(AW86224_ADDR, (int)1);
    return wire->available() ? wire->read() : 0xFF;
}