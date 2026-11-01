#include "input/TEchoLiteKeyboard.h"
#include "modules/CannedMessageModule.h"
#include "modules/StoreForwardModule.h"
#include "PowerFSM.h"

#include "graphics/Screen.h"

#include "nimble/NimbleBluetooth.h"

#include "input/kbI2cBase.h"
using Key = TCA8418KeyboardBase::TCA8418Key;

// T-Echo Lite KeyShield key mapping
// LilyGO KeyShield layout (5 rows x 4 cols):
// Row 0: YES, *, 0, #
// Row 1: NO,  7, 8, 9
// Row 2: DOWN, 4, 5, 6
// Row 3: CENTER, 1, 2, 3
// Row 4: UP, ESC, HOME, MAIL

// Modifier keys
constexpr uint8_t modifierRightShiftKey = 18; // HOME key
constexpr uint8_t modifierRightShift = 0b0001;
constexpr uint8_t modifierSymKey = 19;        // MAIL key
constexpr uint8_t modifierSym = 0b0010;

// Key tap map: [key][modifier_flag % 3]
// Each key has 3 different functions based on modifierFlag
// Digit keys (5,6,7,9,10,11,13,14,15,18) are handled by multi-tap input,
// so their tap map entries are not used for character output.
static unsigned char TEchoLiteTapMap[_TECHO_LITE_NUM_KEYS][3] = {
    {Key::UP, Key::UP, Key::BL_TOGGLE},             // key 0: UP (single=UP, double=UP, triple=backlight)
    {Key::ESC, Key::ESC, Key::NOTIF_VIB_TOGGLE},           // key 1: ESC (triple=notification vibration toggle)
    {Key::LEFT, Key::BSP, Key::BL_TOGGLE},          // key 2: HOME (single=LEFT, double=BSP, triple=backlight)
    {Key::RIGHT, Key::BSP, Key::MUTE_TOGGLE},       // key 3: MAIL (single=RIGHT, double=BSP, triple=vibration)
    {Key::SELECT, Key::TAB, Key::BL_TOGGLE},        // key 4: CENTER (single=SELECT, double=TAB, triple=backlight)
    {0x00, 0x00, 0x00},                             // key 5: '1' (multi-tap: . , 1)
    {0x00, 0x00, 0x00},                             // key 6: '2' (multi-tap: a b c 2 @)
    {0x00, 0x00, 0x00},                             // key 7: '3' (multi-tap: d e f 3)
    {Key::DOWN, Key::DOWN, Key::BL_TOGGLE},         // key 8: DOWN
    {0x00, 0x00, 0x00},                             // key 9: '4' (multi-tap: g h i 4)
    {0x00, 0x00, 0x00},                             // key 10: '5' (multi-tap: j k l 5)
    {0x00, 0x00, 0x00},                             // key 11: '6' (multi-tap: m n o 6)
    {Key::ESC, Key::ESC, Key::BL_TOGGLE},           // key 12: NO
    {0x00, 0x00, 0x00},                             // key 13: '7' (multi-tap: p q r 7 s)
    {0x00, 0x00, 0x00},                             // key 14: '8' (multi-tap: t u v 8)
    {0x00, 0x00, 0x00},                             // key 15: '9' (multi-tap: w x y 9 z)
    {Key::SELECT, Key::TAB, Key::BL_TOGGLE},        // key 16: YES
    {0x00, 0x00, 0x00},                             // key 17: '*' (multi-tap: * backspace newline)
    {0x00, 0x00, 0x00},                             // key 18: '0' (multi-tap: space 0)
    {0x23, 0x23, 0x00}                              // key 19: '#' (shift-toggle, # for direct)
};

// Key tap mod: how many functions each key has
// Multi-tap keys have mod=1 (all taps go through multi-tap state machine)
static uint8_t TEchoLiteTapMod[_TECHO_LITE_NUM_KEYS] = {
    3, 3, 3, 3, 3, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1
    // ^ ESC key (index 1) changed from 1 to 3 to enable triple-tap
};

// ============================================================================
// Multi-tap character tables (Nokia-style phone keypad)
// Index: 0-based. 255 = end of list (no more characters for this key).
// Key indices: 5='1', 6='2', 7='3', 9='4', 10='5', 11='6', 13='7', 14='8', 15='9', 18='0'
// ============================================================================
// Unified character table: [key_index][tap_count]
// SHIFT OFF = lowercase/normal, SHIFT ON = uppercase/special
// 255 = null terminator
static const char multiTapChars[11][6] = {
    // key 5: '1' -> SHIFT OFF: . , 1 | SHIFT ON: + - !
    {'.', ',', '1', 255, 255, 255},
    // key 6: '2' -> SHIFT OFF: a b c 2 @ | SHIFT ON: A B C @ (4th tap)
    {'a', 'b', 'c', '2', '@', 255},
    // key 7: '3' -> SHIFT OFF: d e f 3 | SHIFT ON: D E F ~ (3rd tap)
    {'d', 'e', 'f', '3', 255, 255},
    // key 9: '4' -> SHIFT OFF: g h i 4 | SHIFT ON: G H I $ (3rd tap)
    {'g', 'h', 'i', '4', 255, 255},
    // key 10: '5' -> SHIFT OFF: j k l 5 | SHIFT ON: J K L % (3rd tap)
    {'j', 'k', 'l', '5', 255, 255},
    // key 11: '6' -> SHIFT OFF: m n o 6 | SHIFT ON: M N O ^ (3rd tap)
    {'m', 'n', 'o', '6', 255, 255},
    // key 13: '7' -> SHIFT OFF: p q r 7 s | SHIFT ON: P Q R & (3rd tap) S (4th tap)
    {'p', 'q', 'r', '7', 's', 255},
    // key 14: '8' -> SHIFT OFF: t u v 8 | SHIFT ON: T U V _ (3rd tap)
    {'t', 'u', 'v', '8', 255, 255},
    // key 15: '9' -> SHIFT OFF: w x y 9 z | SHIFT ON: W X Y / (3rd tap) Z (4th tap)
    {'w', 'x', 'y', '9', 'z', 255},
    // key 18: '0' -> SHIFT OFF: space 0 | SHIFT ON: space = (2nd tap)
    {' ', '0', 255, 255, 255, 255},
    // key 17: '*' -> * backspace (triple-tap removed: no visible effect in single-line input)
    {'*', 0x08, 255, 255, 255, 255}
};

// SHIFT ON special character overrides: [key_index][tap_index]
// Only defined for positions where SHIFT ON differs from SHIFT OFF
static const char shiftOnOverrides[11][6] = {
    // key 5: '1' -> idx0: +, idx1: -, idx2: !
    {'+', '-', '!', 255, 255, 255},
    // key 6: '2' -> idx3: @ (idx0-2 are uppercase A B C, handled by normal uppercase logic)
    {255, 255, 255, '@', 255, 255},
    // key 7: '3' -> idx3: ~
    {255, 255, 255, '~', 255, 255},
    // key 9: '4' -> idx3: $
    {255, 255, 255, '$', 255, 255},
    // key 10: '5' -> idx3: %
    {255, 255, 255, '%', 255, 255},
    // key 11: '6' -> idx3: ^
    {255, 255, 255, '^', 255, 255},
    // key 13: '7' -> idx3: &
    {255, 255, 255, '&', 255, 255},
    // key 14: '8' -> idx3: _
    {255, 255, 255, '_', 255, 255},
    // key 15: '9' -> idx3: /
    {255, 255, 255, '/', 255, 255},
    // key 18: '0' -> idx1: =
    {255, '=', 255, 255, 255, 255},
    // key 17: '*' -> no overrides (same for both shift states)
    {255, 255, 255, 255, 255, 255}
};

// Map key index -> multi-tap table index (0-10)
// Returns 255 if not a multi-tap key
static uint8_t multiTapIndexForKey(uint8_t key)
{
    switch (key) {
        case 5:  return 0;  // '1'
        case 6:  return 1;  // '2'
        case 7:  return 2;  // '3'
        case 9:  return 3;  // '4'
        case 10: return 4;  // '5'
        case 11: return 5;  // '6'
        case 13: return 6;  // '7'
        case 14: return 7;  // '8'
        case 15: return 8;  // '9'
        case 18: return 9;  // '0'
        case 17: return 10; // '*'
        default: return 255;
    }
}

bool TEchoLiteKeyboard::isMultiTapKey(uint8_t key)
{
    return multiTapIndexForKey(key) != 255;
}

char TEchoLiteKeyboard::getMultiTapChar(uint8_t key, uint8_t index)
{
    uint8_t tableIdx = multiTapIndexForKey(key);
    if (tableIdx == 255) return 255;
    // Clamp index to valid range
    uint8_t maxIdx = 0;
    while (multiTapChars[tableIdx][maxIdx] != 255 && maxIdx < 5) maxIdx++;
    if (index > maxIdx - 1) index = maxIdx - 1;
    return multiTapChars[tableIdx][index];
}

// Static member for global access
TEchoLiteKeyboard *TEchoLiteKeyboard::instance = nullptr;

TEchoLiteKeyboard *TEchoLiteKeyboard::getInstance()
{
    return instance;
}

TEchoLiteKeyboard::TEchoLiteKeyboard()
    : TCA8418KeyboardBase(_TECHO_LITE_ROWS, _TECHO_LITE_COLS), modifierFlag(0), last_modifier_time(0), last_key(UINT8_MAX),
      tap_interval(500)
{
    instance = this;
    LOG_INFO("T-Echo Lite KeyShield keyboard initialized (5 rows x 4 cols)");
    // Initialize I2C communication and reset the TCA8418
    begin();
    
    // Initialize AW21009 backlight driver
    initAW21009();

    // Initialize AW86224 vibration driver
    initAW86224();
};

void TEchoLiteKeyboard::trigger(void)
{
    // Check backlight auto-off: if backlight is on and no keyboard activity for 30s, turn off
    if (backlight_on && last_key_activity > 0) {
        uint32_t now = millis();
        uint32_t elapsed = now - last_key_activity;
        if (elapsed > BACKLIGHT_AUTO_OFF_MS) {
            backlight_on = false;
            backlight_auto_off = true;  // mark as auto-off so any key press can re-light
            brightness = 0;
            setAW21009Brightness(0);
            last_key_activity = now;
        }
    }

    // Multi-tap timeout (Nokia-style): after 750ms silence, emit the
    // selected character and reset the pending state.
    if (mt_pending) {
        uint32_t now = millis();
        if (now - mt_last_tap > MULTI_TAP_THRESHOLD) {
            char c = 0;
            
            // Unified character table: SHIFT OFF = base, SHIFT ON = overrides/uppercase
            uint8_t tableIdx = multiTapIndexForKey(mt_key);
            uint8_t maxIdx = 0;
            while (multiTapChars[tableIdx][maxIdx] != 255 && maxIdx < 5) maxIdx++;
            c = multiTapChars[tableIdx][mt_char_idx % maxIdx];
            
            // SHIFT ON: check for special character override
            if (shiftHeld && shiftOnOverrides[tableIdx][mt_char_idx % maxIdx] != 255) {
                c = shiftOnOverrides[tableIdx][mt_char_idx % maxIdx];
            } else if (shiftHeld && c >= 'a' && c <= 'z') {
                // SHIFT ON + letter → uppercase
                c = c - 'a' + 'A';
            }
            // Log removed for clean output
            
            queueEvent((uint16_t)(unsigned char)c);
            
            // Force screen refresh after commit (in trigger, not released)
            if (cannedMessageModule && cannedMessageModule->isFreeTextActive()) {
                screen->forceDisplay(true);
            }
            
            mt_pending = false;
            mt_char_idx = 0;
            mt_key = 255;
        }
    }

    // Call base class trigger() to handle key press/release events
    TCA8418KeyboardBase::trigger();
    
    // Flush any pending tap events that have timed out
    flushPendingTap();
};

void TEchoLiteKeyboard::reset(void)
{
    // Call the base class reset() to initialize the TCA8418
    TCA8418KeyboardBase::reset();
    
    // Then reset the local state
    last_key = UINT8_MAX;
    last_tap = millis();
    modifierFlag = 0;
};

void TEchoLiteKeyboard::setBacklight(bool on)
{
    if (on) {
        // Set backlight brightness
        if (brightness > 0) {
            // Convert 0-127 scale to 0-4095 (12-bit)
            uint16_t level = (brightness * 4095) / 127;
            setAW21009Brightness(level);
            backlight_on = true;
        }
    }
};

// TCA8418 key number to our key index mapping
// TCA8418 uses COLUMN-major key IDs: (col-1)*10 + row
// Col 1: keys 1-5 (YES/NO/DOWN/CENTER/UP), Col 2: 11-15, Col 3: 21-25, Col 4: 31-35
static uint8_t keyMap[46] = {
    0, 16, 17, 18, 19,   // IDs 0-4:   unused, YES, *, 0, #
    0, 0, 0, 0, 0,       // IDs 5-9:   unused
    0, 12, 13, 14, 15,   // IDs 10-14: unused, NO, 7, 8, 9
    0, 0, 0, 0, 0,       // IDs 15-19: unused
    0, 8, 9, 10, 11,     // IDs 20-24: unused, DOWN, 4, 5, 6
    0, 0, 0, 0, 0,       // IDs 25-29: unused
    0, 4, 5, 6, 7,       // IDs 30-34: unused, CENTER, 1, 2, 3
    0, 0, 0, 0, 0,       // IDs 35-39: unused
    0, 0, 1, 2, 3,       // IDs 40-44: unused, UP, ESC, HOME, MAIL
    0                   // ID 45:    unused
};
void TEchoLiteKeyboard::pressed(uint8_t key)
{
    // If backlight was auto-off, re-light it at the saved level on any key press
    if (!backlight_on && backlight_auto_off && backlight_level > 0) {
        backlight_on = true;
        backlight_auto_off = false;
        brightness = backlight_level;
        uint16_t level = (brightness * 4095) / 127;
        setAW21009Brightness(level);
        last_key_activity = millis();
    }
    
    // Reset the backlight auto-off timer on any key activity
    if (backlight_on) {
        last_key_activity = millis();
    }
    
    // Haptic feedback on key press (synchronous, ~1-2ms I2C)
    vibrateKey(30);

    // Map TCA8418 key number to our internal key index
    uint8_t mappedKey = keyMap[key];
    if (mappedKey >= _TECHO_LITE_NUM_KEYS) {
        return;
    }

    // Update last_key with the mapped key
    last_key = mappedKey;

    // Track tap count for double/triple tap detection
    uint32_t now = millis();
    
    // Multi-tap (Nokia-style): accumulate tap count within the 1250ms window
    if (isMultiTapKey(mappedKey)) {
        int32_t gap = (int32_t)(now - mt_last_tap);
        if (mt_pending && mappedKey == mt_key && gap < (int32_t)MULTI_TAP_THRESHOLD) {
            // Same key, within window → advance to next character
            mt_char_idx++;
            uint8_t tableIdx = multiTapIndexForKey(mappedKey);
            uint8_t maxIdx = 0;
            while (multiTapChars[tableIdx][maxIdx] != 255 && maxIdx < 5) maxIdx++;
            if (mt_char_idx >= maxIdx) {
                // Past end of table → no-op tap (e.g. * key triple-tap):
                // commit nothing, reset so the next tap starts fresh from idx 0
                mt_char_idx = 0;
                mt_pending = false;
                mt_key = 255;
            }
            mt_last_tap = now;  // always refresh even in NO-OP case
        } else if (mt_pending && mappedKey != mt_key) {
            // Different key pressed → commit the pending char immediately,
            // then start the new key's count from 0
            // Same logic as trigger(): unified character table
            uint8_t oldTableIdx = multiTapIndexForKey(mt_key);
            uint8_t oldMaxIdx = 0;
            while (multiTapChars[oldTableIdx][oldMaxIdx] != 255 && oldMaxIdx < 5) oldMaxIdx++;
            char c = multiTapChars[oldTableIdx][mt_char_idx % oldMaxIdx];
            
            // SHIFT ON: check for special character override
            if (shiftHeld && shiftOnOverrides[oldTableIdx][mt_char_idx % oldMaxIdx] != 255) {
                c = shiftOnOverrides[oldTableIdx][mt_char_idx % oldMaxIdx];
            } else if (shiftHeld && c >= 'a' && c <= 'z') {
                // SHIFT ON + letter → uppercase
                c = c - 'a' + 'A';
            }
            // Log removed for clean output
            queueEvent(c);
            // Start new key
            mt_key = mappedKey;
            mt_char_idx = 0;
            mt_pending = true;
        } else {
            // New key or gap exceeded → start fresh
            mt_key = mappedKey;
            mt_char_idx = 0;
            mt_pending = true;
        }
        if (mt_pending) {
            mt_last_tap = now;
        }
    }
    
    // Track tap count for double/triple tap detection (non-multi-tap keys)
    // Threshold: 400ms for most keys, but 1200ms for * key (17) due to TCA8418 latency
    uint32_t since_release = last_release_time > 0 ? now - last_release_time : 10000;
    uint32_t threshold = (mappedKey == 17) ? 1000 : 400;  // * key: 1000ms window for multi-tap detection
    if (tap_key == mappedKey && since_release < threshold) {
        tap_count++;
    } else {
        tap_count = 1;
        tap_key = mappedKey;
    }
    last_tap = now;
    
    // * key (17): now multi-tap, handled by the multi-tap logic above
    
    // Update modifier flag (use mapped key)
    if (isModifierKey(mappedKey)) {
        updateModifierFlag(mappedKey);
        last_modifier_time = now;
    }
    
    // Mark state as Held so that trigger() will call released() on key-up
    state = Held;
    
    // NOTE: Event is queued in released() method, not here
};

void TEchoLiteKeyboard::released(void)
{
    if (last_key >= _TECHO_LITE_NUM_KEYS) {
        last_key = UINT8_MAX;
        return;
    }
    
    uint32_t now = millis();
    
    // --- Multi-tap key handling (Nokia-style) ---
    // Key 19 (#) toggles shift; multi-tap keys (5,6,7,9,10,11,13,14,15,18,17) handled via mt_* state
    if (isMultiTapKey(last_key) || last_key == 19) {
        // SHIFT is toggled exclusively by the # key (19); other keys don't clear it
        
        // SHIFT key (#) — single tap = toggle shift, double tap = emit #
        if (last_key == 19) {
            if (tap_count >= 2) {
                // Double tap: emit # character
                // FIX: The first tap already toggled SHIFT, so toggle it back
                shiftHeld = !shiftHeld;
                
                queueEvent(0x23);
                // Trigger display update after # emit
                if (cannedMessageModule && cannedMessageModule->isFreeTextActive()) {
                    screen->forceDisplay(true);
                }
            } else {
                // Single tap: toggle shift
                shiftHeld = !shiftHeld;
            }
            last_release_time = now;  // FIX: update release time for next tap detection
            last_key = UINT8_MAX;
            return;
        }
        
        // * key (17): now multi-tap, handled by the multi-tap logic in trigger()
        
        // Other multi-tap keys: don't emit on release —
        // the character is committed by the 1250ms timeout in trigger().
        last_release_time = now;
        last_key = UINT8_MAX;
        return;
    }
    
    // --- Non-multi-tap key handling ---
    // tap_count: 1=single, 2=double, 3=triple (capped at 3)
    uint8_t tapIndex = (tap_count - 1) % TEchoLiteTapMod[last_key];
    
    // Update the pending tap (don't queue yet — wait for debounce timeout)
    updatePendingTap(last_key, tapIndex);
    
    // Reset modifier flag if this is not a modifier key
    if (isModifierKey(last_key) == false) {
        modifierFlag = 0;
    }
    
    last_release_time = millis();
    last_key = UINT8_MAX;
};

void TEchoLiteKeyboard::updatePendingTap(uint8_t key, uint8_t tapIndex)
{
    has_pending_tap = true;
    pending_tap_key = key;
    pending_tap_count = tapIndex;
    pending_tap_time = millis();
}

void TEchoLiteKeyboard::flushPendingTap()
{
    if (!has_pending_tap) {
        return;
    }
    
    uint32_t now = millis();
    if (now - pending_tap_time < TAP_DEBOUNCE_MS) {
        return; // Still waiting for debounce timeout
    }
    
    // Flush the pending tap event
    uint8_t key = pending_tap_key;
    uint8_t tapIndex = pending_tap_count;
    
    uint16_t finalKey = TEchoLiteTapMap[key][tapIndex];
    
    if (finalKey == Key::BL_TOGGLE) {
        toggleBacklight();
    } else if (finalKey == Key::MUTE_TOGGLE) {
        toggleVibration();
    } else if (finalKey == Key::NOTIF_VIB_TOGGLE) {
        toggleNotificationVibration();
    } else {
        queueEvent(finalKey);
    }
    
    has_pending_tap = false;
    pending_tap_key = 255;
    pending_tap_count = 0;
    
    // Reset tap tracking now that we've flushed the pending event
    tap_count = 0;
    tap_key = 255;
}

void TEchoLiteKeyboard::updateModifierFlag(uint8_t key)
{
    if (key == modifierRightShiftKey) {
        modifierFlag ^= modifierRightShift;
    } else if (key == modifierSymKey) {
        modifierFlag ^= modifierSym;
    }
};

bool TEchoLiteKeyboard::isModifierKey(uint8_t key)
{
    return (key == modifierRightShiftKey || key == modifierSymKey);
};

void TEchoLiteKeyboard::toggleBacklight(bool off)
{
    // Cycle: OFF → 30% → 100% → OFF → 30% → 100% → ...
    if (off) {
        backlight_on = false;
        backlight_auto_off = false;  // intentional off, not auto-off
        brightness = 0;
        setAW21009Brightness(0);
    } else if (!backlight_on) {
        // OFF → 30%
        backlight_on = true;
        backlight_auto_off = false;  // intentional on
        brightness = 40;
        backlight_level = 40;
        setAW21009Brightness(1289);  // 40/127 * 4095
        last_key_activity = millis();
    } else if (brightness < 127) {
        // 30% → 100%
        brightness = 127;
        backlight_level = 127;
        backlight_auto_off = false;  // intentional on
        setAW21009Brightness(4095);
        last_key_activity = millis();
    } else {
        // 100% → OFF
        backlight_on = false;
        brightness = 0;
        setAW21009Brightness(0);
    }
};

// ============================================================================
// AW21009 Backlight Control
// ============================================================================

void TEchoLiteKeyboard::initAW21009()
{
    // AW21009 register map (verified from AW21009QNR datasheet + Llgok/cpp_bus_driver):
    // 0x00: Chip Reset / Chip ID (read returns 0x00)
    // 0x20: GCR — Global Control (bit7 AutoPwrSave, bits5-4 ClockFreq, bit1 PwmRes, bit0 ChipEnable)
    // 0x21-0x2E: LED0-LED7 Brightness (12-bit each: Low byte, then High nibble)
    // 0x45: UPDATE register (write 0x00 to apply brightness/scaling changes)
    // 0x46-0x4E: LED0-LED7 SL Scaling (current limit, 8-bit each)
    // 0x58: Global Current Limit (8-bit)
    // 0x70: Software Reset (write 0x00)
    //
    // GCR value 0xB1 = AutoPwrSave(1) | ClockFreq 16MHz(010) | PwmRes 12bit+dither(1) | ChipEnable(1)

    const uint8_t AW21009_ADDR = 0x20;

    // Enable RT9080_EN pin (3.3V power for AW21009 and AW86224)
    // Use LilyGO's KeyShield power-on sequence: HIGH -> 100ms -> LOW -> 100ms -> HIGH -> 1000ms
    #ifndef RT9080_EN
    #define RT9080_EN 31
    #endif

    pinMode(RT9080_EN, OUTPUT);

    // Power cycle sequence for KeyShield (AW21009 + AW86224)
    digitalWrite(RT9080_EN, HIGH);
    delay(100);
    digitalWrite(RT9080_EN, LOW);
    delay(100);
    digitalWrite(RT9080_EN, HIGH);
    delay(1000);  // Wait for 3.3V rail to stabilize

    // 1. Verify chip presence by reading chip ID (register 0x00)
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x00); // Chip Reset / Chip ID register
    uint8_t tx_status = Wire.endTransmission(false);
    if (tx_status != 0) {
        LOG_WARN("AW21009 I2C comm failed at 0x20 (tx_status=%d)", tx_status);
        return;
    }
    Wire.requestFrom(AW21009_ADDR, (int)1);
    uint8_t chip_id = Wire.available() ? Wire.read() : 0xFF;

    if (chip_id != 0x00) {
        LOG_WARN("AW21009 chip ID mismatch: expected 0x00, got 0x%02X", chip_id);
    }

    // 2. Software reset (register 0x70, value 0x00)
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x70); // Software Reset
    Wire.write(0x00);
    Wire.endTransmission();
    delay(5);

    // 3. Set global control: AutoPowerSave=1, ClockFreq=16MHz(010), PwmRes=12bit+dither(1), ChipEnable=1
    // GCR = 0xB1, written to register 0x20
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x20); // Global Control
    Wire.write(0xB1);
    Wire.endTransmission();
    delay(1);

    // 4. Set global current limit to 100% (register 0x58)
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x58); // Global Current Control
    Wire.write(0xFF);
    Wire.endTransmission();

    // 5. Set LED current limit (SL scaling) to 100% for all 8 channels (registers 0x46-0x4E)
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x46); // Scaling Start
    for (int i = 0; i < 8; i++) {
        Wire.write(0xFF);
    }
    Wire.endTransmission();

    // 6. Set initial brightness to 0 for all 8 channels (registers 0x21-0x2E)
    // Format: Low byte, then High nibble (bits 8-11)
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x21); // Brightness Start
    for (int i = 0; i < 8; i++) {
        Wire.write(0x00); // Low byte
        Wire.write(0x00); // High nibble
    }
    Wire.endTransmission();

    // 7. Update register (0x45) to apply all changes
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x45); // Update
    Wire.write(0x00);
    Wire.endTransmission();

    LOG_INFO("AW21009 backlight initialized (chip ID: 0x%02X)", chip_id);
};

void TEchoLiteKeyboard::setAW21009Brightness(uint16_t level)
{
    // AW21009 uses 12-bit brightness (0-4095)
    if (level > 4095) {
        level = 4095;
    }

    const uint8_t AW21009_ADDR = 0x20;

    // Set brightness for all 8 LED channels (registers 0x21-0x2E)
    // Format: Low byte, then High nibble (bits 8-11)
    uint8_t low = level & 0xFF;
    uint8_t high = (level >> 8) & 0x0F;

    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x21); // Brightness Start
    for (int i = 0; i < 8; i++) {
        Wire.write(low);  // Low byte
        Wire.write(high); // High nibble
    }
    Wire.endTransmission();

    // Update register (0x45) to apply brightness changes
    Wire.beginTransmission(AW21009_ADDR);
    Wire.write(0x45); // Update
    Wire.write(0x00);
    Wire.endTransmission();

};

void TEchoLiteKeyboard::initAW86224()
{
    aw86224.begin();
};

void TEchoLiteKeyboard::vibrateKey(uint16_t durationMs)
{
    if (!key_vibrate_enabled) return;
    aw86224.vibrate(durationMs);
};

void TEchoLiteKeyboard::vibrateNotification(uint16_t durationMs)
{
    // Notification vibration - independent of key_vibrate_enabled
    aw86224.vibrate(durationMs);
};

void TEchoLiteKeyboard::toggleVibration()
{
    key_vibrate_enabled = !key_vibrate_enabled;
    LOG_INFO("[TEchoLite] Key vibration: %s", key_vibrate_enabled ? "ON" : "OFF");
}

void TEchoLiteKeyboard::toggleNotificationVibration()
{
    // Cycle notification modes: 3(VIB+LIGHT) -> 0(OFF) -> 1(VIB ONLY) -> 2(LIGHT ONLY) -> 3(VIB+LIGHT)
    uint8_t oldMode = notif_mode;
    notif_mode = (notif_mode + 1) % 4;
    
    const char *modeNames[] = {"OFF", "VIB ONLY", "LIGHT ONLY", "VIB+LIGHT"};
    LOG_INFO("[TEchoLite] Notification mode: %s -> %s (mode=%d -> %d)", 
             modeNames[oldMode], modeNames[notif_mode], oldMode, notif_mode);
}

bool TEchoLiteKeyboard::isNotificationVibrationEnabled() const
{
    return (notif_mode == 1 || notif_mode == 3);
}

bool TEchoLiteKeyboard::isNotificationLightEnabled() const
{
    return (notif_mode == 2 || notif_mode == 3);
}

void TEchoLiteKeyboard::flashBacklightNotification()
{
    // Flash backlight 3 times (extended duration for better visibility)
    uint16_t savedLevel = (brightness * 4095) / 127;
    
    // Flash sequence: ON(100ms) -> OFF(80ms) -> ON(100ms) -> OFF(80ms) -> ON(100ms)
    setAW21009Brightness(savedLevel > 0 ? savedLevel : 1289);  // First flash (100ms)
    delay(100);
    setAW21009Brightness(0);
    delay(80);
    setAW21009Brightness(savedLevel > 0 ? savedLevel : 1289);  // Second flash (100ms)
    delay(100);
    setAW21009Brightness(0);
    delay(80);
    setAW21009Brightness(savedLevel > 0 ? savedLevel : 1289);  // Third flash (100ms)
    delay(100);
    
    // Restore original state
    if (!backlight_on) {
        setAW21009Brightness(0);
    } else {
        setAW21009Brightness(savedLevel);
    }
}

