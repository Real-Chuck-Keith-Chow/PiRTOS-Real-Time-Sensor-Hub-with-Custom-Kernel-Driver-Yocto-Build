// main.cpp  — C++ showcase for Raspberry Pi Pico / Pico 2
// Initializes hardware, starts scheduler/RTOS tasks, configures interrupts. Only high-level logic goes here no low-level register twiddling. 
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// ---------- Config ----------
constexpr uint LED_PIN    = PICO_DEFAULT_LED_PIN; // board LED
constexpr uint PIR_PIN    = 16;                   // PIR to GP16 (idle low)
constexpr uint BUTTON_PIN = 14;                   // pushbutton to GND (internal pull-up)
constexpr uint ADC_PIN    = 26;                   // GP26 = ADC0
constexpr uint BAUD       = 115200;

// ---------- Utilities ----------
static inline uint adc_channel_from_pin(uint pin) { return pin - 26u; }

// Simple moving-average filter
template<typename T, size_t N>
class MovingAvg {
public:
    void push(T v) {
        sum_ -= buf_[idx_];
        buf_[idx_] = v;
        sum_ += v;
        idx_ = (idx_ + 1) % N;
        if (count_ < N) count_++;
    }
    T average() const { return count_ ? static_cast<T>(sum_ / count_) : 0; }
private:
    T buf_[N]{}; size_t idx_{0}; size_t count_{0}; long long sum_{0};
};

// Non-blocking periodic timer helper
class Periodic {
public:
    explicit Periodic(uint32_t period_ms) : period_ms_(period_ms) {
        deadline_ = make_timeout_time_ms(period_ms_);
    }
    bool ready() {
        if (absolute_time_diff_us(get_absolute_time(), deadline_) <= 0) {
            deadline_ = make_timeout_time_ms(period_ms_);
            return true;
        }
        return false;
    }
    void reset(uint32_t period_ms) {
        period_ms_ = period_ms;
        deadline_ = make_timeout_time_ms(period_ms_);
    }
private:
    uint32_t period_ms_;
    absolute_time_t deadline_;
};

// Debounced button using GPIO IRQ dispatch
class DebouncedButton {
public:
    using cb_t = void(*)(bool pressed, void* user);

    DebouncedButton(uint pin, bool active_low, uint32_t debounce_ms,
                    cb_t cb, void* user)
        : pin_(pin), active_low_(active_low),
          debounce_us_(debounce_ms * 1000u), cb_(cb), user_(user) {

        gpio_init(pin_);
        gpio_set_dir(pin_, GPIO_IN);
        if (active_low_) gpio_pull_up(pin_);
        else             gpio_pull_down(pin_);

        registry_[pin_] = this;
        // One shared callback for all GPIO IRQs:
        gpio_set_irq_enabled_with_callback(pin_, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                           true, &DebouncedButton::trampoline);
    }

    bool pressed() const {
        bool level = gpio_get(pin_);
        return active_low_ ? !level : level}

// ---------- App state ----------
static volatile bool g_armed = false;

// Button callback: toggle arm/disarm
static void on_button(bool pressed, void* /*user*/) {
    if (!pressed) return;            // act on press (active-low)
    g_armed = !g_armed;
    printf("[button] %s\n", g_armed ? "ARMED" : "DISARMED");
    // quick visual acknowledgment
    for (int i=0;i<1;i++){ gpio_put(LED_PIN, 1); sleep_ms(60); gpio_put(LED_PIN, 0); sleep_ms(60); }
}

// ---------- Main ----------
int main() {
    stdio_init_all();
    // Give USB CDC a moment to enumerate (no hard block if not connected)
    for (int i=0;i<100 && !tud_cdc_connected(); ++i) sleep_ms(10);

    gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);

    // PIR
    gpio_init(PIR_PIN); gpio_set_dir(PIR_PIN, GPIO_IN); gpio_pull_down(PIR_PIN);

    // Button (active-low with internal pull-up)
    DebouncedButton btn(BUTTON_PIN, /*active_low=*/true, /*debounce_ms=*/30, &on_button, nullptr);

    // ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(adc_channel_from_pin(ADC_PIN));
    MovingAvg<uint32_t, 16> temp_avg;

    // Periodic “tasks”
    Periodic blink_idle(500);   // 2 Hz “heartbeat” when disarmed
    Periodic read_adc(100);     // 10 Hz ADC sampling
    Periodic print_telemetry(500); // 2 Hz print

    printf("C++ demo online. DISARMED. Press button to ARM. Baud=%u\n", BAUD);

    while (true) {
        // Heartbeat only when DISARMED
        if (!g_armed && blink_idle.ready()) {
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
        }

        // Sample ADC regularly (works armed or disarmed)
        if (read_adc.ready()) {
            uint32_t raw = adc_read();       // 12-bit (0..4095) on RP2040/RP2350
            temp_avg.push(raw);
        }

        if (g_armed) {
            // PIR-LED logic (simple and responsive)
            bool motion = gpio_get(PIR_PIN);  // PIR goes high on motion
            gpio_put(LED_PIN, motion);
        }

        // Telemetry print at a slower rate
        if (print_telemetry.ready()) {
            printf("armed=%d, pir=%d, adc_avg=%lu\n",
                   g_armed ? 1 : 0,
                   gpio_get(PIR_PIN) ? 1 : 0,
                   static_cast<unsigned long>(temp_avg.average()));
        }

        tight_loop_contents(); // hint for power/analysis tools
    }
}

