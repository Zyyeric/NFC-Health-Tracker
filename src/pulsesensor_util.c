#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "app_timer.h"
#include "nrf_delay.h"
#include "pulsesensor.h"

//----------------------------------------------------
// Filter & Timing Parameters
//----------------------------------------------------
#define SAMPLE_INTERVAL_MS       2      // Sampling every 2 ms (500 Hz)
#define SAMPLING_RATE            (1000 / SAMPLE_INTERVAL_MS)

#define MOVING_AVG_WINDOW        10     // Moving average window size for additional smoothing

// Dynamic threshold update period (ms)
#define THRESHOLD_UPDATE_PERIOD  1000   

// Minimum interval between peaks (ms)
#define MIN_PEAK_INTERVAL_MS     650    
// Timeout to reset peak history if no peaks are found (ms)
#define NO_PEAK_TIMEOUT_MS       5000   
// Stabilization time (ignore first few seconds)
#define STABILIZATION_TIME_MS    5000   

// Sliding window for BPM calculation (number of recent peaks)
#define SLIDING_WINDOW_SIZE      5

//----------------------------------------------------
// Global Variables
//----------------------------------------------------
static float ma_buffer[MOVING_AVG_WINDOW];
static uint32_t ma_sample_count = 0;
static float ma_sum = 0.0f;
static bool ma_filled = false;

static volatile uint32_t elapsed_time_ms = 0;

static uint32_t peak_timestamps[SLIDING_WINDOW_SIZE];
static uint8_t  peak_count = 0;
static uint32_t last_peak_time = 0;
static bool peak_detected = false;

// Dynamic threshold parameters – updated every THRESHOLD_UPDATE_PERIOD
static float dynamic_threshold = 0.0f;
static float recent_max = 0.0f;
static float recent_min = 4095.0f;  // Assuming a 12-bit ADC

// Timer definition
APP_TIMER_DEF(m_sample_timer);

void start_sample_timer(void);
void stop_sample_timer(void);
//----------------------------------------------------
// Filter Functions
//----------------------------------------------------

// A simple bandpass filter: a cascade of a high-pass filter (to remove DC) 
// and a low-pass filter (to remove high-frequency noise).
// (Note: This is a simplified IIR design. In a production system you may want a 
//  more sophisticated design based on your sensor's response.)
float bandpass_filter(float sample) {
    // High-pass filter to remove DC offset.
    static float prev_input = 0, prev_hp_output = 0;
    float hp_alpha = 0.95f;  // Adjust to control the high-pass cutoff
    float high_pass = sample - prev_input + hp_alpha * prev_hp_output;
    prev_input = sample;
    prev_hp_output = high_pass;
    
    // Low-pass filter on the high-pass output.
    static float prev_lp_output = 0;
    float lp_alpha = 0.1f;   // Adjust to control the low-pass cutoff
    float low_pass = lp_alpha * high_pass + (1.0f - lp_alpha) * prev_lp_output;
    prev_lp_output = low_pass;
    
    return low_pass;
}

// A simple moving average filter.
float moving_average_filter(float sample) {
    uint8_t index = (uint8_t)(ma_sample_count % MOVING_AVG_WINDOW);
    if (ma_sample_count < MOVING_AVG_WINDOW) {
        ma_sum += sample;
        ma_buffer[index] = sample;
        ma_sample_count++;
        if (ma_sample_count >= MOVING_AVG_WINDOW) {
            ma_filled = true;
        }
    } else {
        ma_sum = ma_sum - ma_buffer[index] + sample;
        ma_buffer[index] = sample;
        ma_sample_count++;
    }
    return (ma_filled) ? (ma_sum / MOVING_AVG_WINDOW) : sample;
}

//----------------------------------------------------
// BPM Calculation Function
//----------------------------------------------------
uint32_t calculate_bpm(void) {
    if (peak_count < 2) {
        return 0;  // Not enough peaks detected
    }
    
    uint32_t total_interval = 0;
    for (uint8_t i = 1; i < peak_count; i++) {
        total_interval += (peak_timestamps[i] - peak_timestamps[i - 1]);
    }
    
    uint32_t avg_interval_ms = total_interval / (peak_count - 1);
    return (avg_interval_ms > 0) ? (60000 / avg_interval_ms) : 0;
}

//----------------------------------------------------
// Sampling Timer Callback
//----------------------------------------------------
void sample_timer_callback(void * p_context) {
    // Obtain raw sample from ADC (function must be implemented elsewhere)
    float raw_sample = adc_sample_blocking();  // ADC counts (e.g., 0-4095)

    // Filter the raw sample
    float filtered_sample = moving_average_filter(bandpass_filter(raw_sample));
    
    elapsed_time_ms += SAMPLE_INTERVAL_MS;
    
    // Update dynamic range every THRESHOLD_UPDATE_PERIOD ms
    static uint32_t last_threshold_update = 0;
    if ((elapsed_time_ms - last_threshold_update) >= THRESHOLD_UPDATE_PERIOD) {
        // Set threshold to 60% between the min and max seen in the recent period
        dynamic_threshold = recent_min + (recent_max - recent_min) * 0.6f;
        // Reset the min and max for the next period
        recent_max = 0;
        recent_min = 4095;
        last_threshold_update = elapsed_time_ms;
    } else {
        // Continuously track signal range for dynamic thresholding.
        if (filtered_sample > recent_max) {
            recent_max = filtered_sample;
        }
        if (filtered_sample < recent_min) {
            recent_min = filtered_sample;
        }
    }
    
    // Skip peak detection during the stabilization period.
    if (elapsed_time_ms < STABILIZATION_TIME_MS) {
        return;
    }
    
    // Reset sliding window if no peaks for a long time.
    if ((elapsed_time_ms - last_peak_time) > NO_PEAK_TIMEOUT_MS) {
        peak_count = 0;
    }
    
    // Peak Detection using dynamic threshold and refractory period.
    if (!peak_detected &&
        (filtered_sample > dynamic_threshold) &&
        ((elapsed_time_ms - last_peak_time) >= MIN_PEAK_INTERVAL_MS)) {
        last_peak_time = elapsed_time_ms;
        if (peak_count < SLIDING_WINDOW_SIZE) {
            peak_timestamps[peak_count++] = elapsed_time_ms;
        } else {
            // Shift the window and add the new peak.
            for (uint8_t i = 0; i < SLIDING_WINDOW_SIZE - 1; i++) {
                peak_timestamps[i] = peak_timestamps[i + 1];
            }
            peak_timestamps[SLIDING_WINDOW_SIZE - 1] = elapsed_time_ms;
        }
        peak_detected = true;
    }
    // Reset peak_detected flag when signal falls below a fraction of the threshold.
    else if (peak_detected && (filtered_sample < (dynamic_threshold * 0.8f))) {
        peak_detected = false;
    }
    
    // Update output display every few seconds.
    static uint32_t last_display_update = 0;
    if ((elapsed_time_ms - last_display_update) >= 3000) {
        last_display_update = elapsed_time_ms;
        uint32_t current_bpm = calculate_bpm();
        if (current_bpm != 0) {
            printf("Current BPM: %lu\n", current_bpm);
        } else {
            printf("No valid pulse detected.\n");
        }
    }
    
    // End measurement after a predefined window (optional)
    if (elapsed_time_ms >= 30000) {
        uint32_t final_bpm = calculate_bpm();
        printf("Final BPM: %lu\n", final_bpm);
        // Stop the sample timer.
        stop_sample_timer();
    }
}

//----------------------------------------------------
// Timer Start/Stop Functions
//----------------------------------------------------
void start_sample_timer(void) {
    ret_code_t err_code;
    err_code = app_timer_create(&m_sample_timer,
                                APP_TIMER_MODE_REPEATED,
                                sample_timer_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(m_sample_timer, APP_TIMER_TICKS(SAMPLE_INTERVAL_MS), NULL);
    APP_ERROR_CHECK(err_code);
}

void stop_sample_timer(void) {
    ret_code_t err_code;
    err_code = app_timer_stop(m_sample_timer);
    APP_ERROR_CHECK(err_code);
}
