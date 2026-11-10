// BALL'S EYE GOLF R13 — first-party startup for M5Stack StopWatch.
// The application owns its startup and drawing handoff.
// Hardware power/reset/pins are managed solely by M5Unified.
#include <Arduino.h>
#include <M5Unified.h>

namespace balls_eye_golf {
void begin();
void update();
}

namespace {
bool running = false;
}

void setup() {
    // Preserve the StopWatch init settings that made R12's display work.
    // In particular, board JSON MUST retain qio_opi for OPI PSRAM detection.
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;
    cfg.internal_imu = true;
    cfg.internal_rtc = true;
    cfg.internal_spk = true;
    cfg.internal_mic = true;

    M5.begin(cfg); // Exactly once. No manual GPIO46 or AMOLED power/reset.
    M5.Display.setBrightness(180);
    M5.Display.setRotation(0);

    // Never render through an uninitialized display; print the reason on USB.
    const int w = M5.Display.width();
    const int h = M5.Display.height();
    M5.Log.printf("\n=== BALL'S EYE GOLF / native boot ===\n");
    M5.Log.printf("board=%d display=%dx%d psram=%u flash=%u\n",
                  (int)M5.getBoard(), w, h,
                  (unsigned)ESP.getPsramSize(),
                  (unsigned)ESP.getFlashChipSize());
    if (M5.getBoard() != m5::board_t::board_M5StopWatch ||
        w < 400 || h < 400) {
        M5.Log.printf("STOP: StopWatch display was not initialized. No draw attempted.\n");
        return;
    }

    // Our own one-frame display handoff. Like the successful R12 path,
    // an actual text draw completes before the full golf renderer starts.
    // The title frame completes before the game renderer starts.
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.fillRect(0, 0, w, 92, 0x2AE5u);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setTextSize(1.0f);
    M5.Display.setTextColor(TFT_WHITE, 0x2AE5u);
    M5.Display.drawString("BALL'S EYE GOLF", w / 2, 66);

    balls_eye_golf::begin();
    running = true;
}

void loop() {
    if (!running) {
        delay(1000);
        return;
    }
    balls_eye_golf::update(); // Performs M5.update() once each iteration.
}
