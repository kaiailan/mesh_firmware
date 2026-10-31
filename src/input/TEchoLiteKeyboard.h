#pragma once
#include "input/TCA8418KeyboardBase.h"
#include "AW86224Driver.h"

#define _TECHO_LITE_ROWS 5
#define _TECHO_LITE_COLS 4
#define _TECHO_LITE_NUM_KEYS 20

/**
 * T-Echo Lite KeyShield keyboard (LilyGO TCA8418)
 * 
 * LilyGO KeyShield layout (5 rows x 4 cols):
 * Row 0: YES, *, 0, #
 * Row 1: NO,  7, 8, 9
 * Row 2: DOWN, 4, 5, 6
 * Row 3: CENTER, 1, 2, 3
 * Row 4: UP, ESC, HOME, TAB
 */
class TEchoLiteKeyboard : public TCA8418KeyboardBase
{
  public:
    TEchoLiteKeyboard();
    void reset(void) override;
    void setBacklight(bool on) override;
    void trigger(void) override;
    void vibrateKey(uint16_t durationMs = 30);  // key press vibration (checks key_vibrate_enabled)
    void vibrateNotification(uint16_t durationMs = 30);  // notification vibration (ignores key_vibrate_enabled)
    bool isNotificationVibrationEnabled() const;  // public for ExternalNotificationModule
    void flashBacklightNotification();  // public for ExternalNotificationModule
    bool isNotificationLightEnabled() const;  // public for ExternalNotificationModule
    
    // Global access for ExternalNotificationModule
    static TEchoLiteKeyboard *instance;
    static TEchoLiteKeyboard *getInstance();

  protected:
    void pressed(uint8_t key) override;
    void released(void) override;

    void updateModifierFlag(uint8_t key);
    bool isModifierKey(uint8_t key);
    void toggleBacklight(bool off = false);

    uint8_t modifierFlag;
    uint32_t last_modifier_time;
    uint8_t last_key;
    uint32_t last_tap;
    int32_t tap_interval;
    uint8_t brightness = 0;
    uint8_t tap_count = 0;
    uint8_t tap_key = 255; // track which key is being tapped
    uint32_t last_release_time = 0; // time of last key release
    bool backlight_on = false;
    uint32_t last_key_activity = 0;  // timestamp of last keyboard activity
    uint8_t backlight_level = 0;     // last brightness level (40 or 127) before auto-off
    bool backlight_auto_off = false;  // true if backlight was auto-off (vs intentionally turned off)
    static const uint32_t BACKLIGHT_AUTO_OFF_MS = 30000;  // 30 second auto-off timer
    bool vibrate_pending = false;     // pending vibration (set in pressed(), executed in trigger())
    
    // Pending tap event (for double/triple tap debounce)
    bool has_pending_tap = false;
    uint8_t pending_tap_key = 255;
    uint8_t pending_tap_count = 0;
    uint32_t pending_tap_time = 0;
    static const uint32_t TAP_DEBOUNCE_MS = 500;
    
    // Dedup state for TCA8418 duplicate press events
    uint8_t last_dedup_key = 255;
    uint32_t last_dedup_time = 0;
    
    // Multi-tap input state (Nokia-style: N presses = Nth char, emit on timeout)
    uint8_t mt_char_idx = 0;        // character index being selected (0-based)
    uint8_t mt_key = 255;           // which key is being multi-tapped
    uint32_t mt_last_tap = 0;       // timestamp of last tap in the current burst
    bool mt_pending = false;        // true if a multi-tap selection is in progress
    static const uint32_t MULTI_TAP_THRESHOLD = 1250; // ms: window for consecutive taps
    bool shiftHeld = false;         // # key toggles uppercase
    bool cannedMessageActive = false; // True when CannedMessage input is active
    
    void flushPendingTap();
    void updatePendingTap(uint8_t key, uint8_t count);
    bool isMultiTapKey(uint8_t key);
    char getMultiTapChar(uint8_t key, uint8_t index);
    
    // AW21009 backlight control
    void initAW21009();
    void setAW21009Brightness(uint16_t level);

    // AW86224 vibration
    void initAW86224();
    void toggleVibration();  // toggles key vibration
    void toggleNotificationVibration();  // cycles notification mode: OFF -> VIB -> LIGHT -> VIB+LIGHT
    
    // Notification modes: 0=OFF, 1=VIB ONLY, 2=LIGHT ONLY, 3=VIB+LIGHT
    uint8_t notif_mode = 3;  // default: VIB+LIGHT
    
    bool key_vibrate_enabled = true;      // key press vibration (TAB triple-tap to toggle)
};
