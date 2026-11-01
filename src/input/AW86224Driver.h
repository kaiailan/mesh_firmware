#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * AW86224 LRA (Linear Resonant Actuator) vibration motor driver.
 *
 * Minimal driver for the T-Echo Lite KeyShield.
 * I2C address: 0x58
 *
 * Usage:
 *   aw86224.begin();           // init + F0 detection + load waveform
 *   aw86224.vibrate(100);      // vibrate for 100ms (RAM mode)
 *   aw86224.stop();            // stop vibration
 */
class AW86224Driver {
public:
    void begin();
    void vibrate(uint16_t durationMs);
    void stop();
    bool isPresent() const { return present; }
    uint32_t getF0() const { return f0; }

private:
    static constexpr uint8_t AW86224_ADDR = 0x58;

    // AW86224 register map (from datasheet)
    static constexpr uint8_t REG_SRST      = 0x00; // soft reset (write 0xAA)
    static constexpr uint8_t REG_PLAYCFG2  = 0x07; // play config 2: GAIN (bits 7:4)
    static constexpr uint8_t REG_PLAYCFG3  = 0x08; // play config 3: PLAY_MODE (bits 1:0)
    static constexpr uint8_t REG_PLAYCFG4  = 0x09; // play config 4: STOP(bit1) GO(bit0)
    static constexpr uint8_t REG_WAVSEQ1   = 0x0A; // waveform sequence 1
    static constexpr uint8_t REG_WAVSEQ2   = 0x0B; // waveform sequence 2
    static constexpr uint8_t REG_WAVSEQ3   = 0x0C; // waveform sequence 3
    static constexpr uint8_t REG_WAVSEQ4   = 0x0D; // waveform sequence 4
    static constexpr uint8_t REG_F0_CTRL   = 0x20; // F0 detection control
    static constexpr uint8_t REG_F0_HI     = 0x21; // F0 result high byte
    static constexpr uint8_t REG_F0_LO     = 0x22; // F0 result low byte
    static constexpr uint8_t REG_RTPCFG1   = 0x2D; // RTP base address high
    static constexpr uint8_t REG_RTPCFG2   = 0x2E; // RTP base address low
    static constexpr uint8_t REG_RAMADDRH  = 0x40; // RAM write address high
    static constexpr uint8_t REG_RAMADDRL  = 0x41; // RAM write address low
    static constexpr uint8_t REG_RAMDATA   = 0x42; // RAM write data
    static constexpr uint8_t REG_SYSCTRL0  = 0x04; // system control 0 (STANDBY bit 0)
    static constexpr uint8_t REG_SYSCTRL1  = 0x43; // system control 1 (EN_RAMINIT bit)

    bool present = false;
    uint32_t f0 = 0; // detected resonant frequency in 0.1Hz units
    uint16_t waveform_samples = 0; // length of loaded waveform
    TwoWire *wire = &Wire;

    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
    void loadSineWave(uint16_t freqHz, uint8_t amplitude, uint8_t num_periods);
};

extern AW86224Driver aw86224;