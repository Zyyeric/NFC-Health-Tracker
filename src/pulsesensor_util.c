#include "pulsesensor_util.h"
#include "pulsesensor.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

// Use a simple moving-average filter to smooth the PPG signal.
#define MOVING_AVG_WINDOW 5    // Number of samples for moving average

static float ma_buffer[MOVING_AVG_WINDOW];  // Circular buffer for moving average
static uint8_t ma_index = 0;                // Index into the buffer
static float ma_sum = 0.0f;                 // Sum of samples in the window
static bool ma_filled = false;              // Indicates if the window is fully populated

// Detect peaks when the filtered signal crosses above a set threshold (rising edge),
// enforcing a minimum interval between peaks to avoid false triggers.
#define PEAK_THRESHOLD        2300.0f   // Adjust based on sensor and environment (e.g., ambient light) 
#define MIN_PEAK_INTERVAL_MS  300       // Minimum interval (ms) between peaks (todo: this needs to be adjusted later)
 
#define MAX_PEAKS  10
static uint32_t peak_timestamps[MAX_PEAKS];
static uint8_t  peak_count = 0;
static uint32_t last_peak_time = 0;

// app_timer for periodic sampling (2 ms).
APP_TIMER_DEF(m_sample_timer);
#define SAMPLE_INTERVAL_MS   2
#define APP_TIMER_TICKS_MS(x) APP_TIMER_TICKS(x)

static volatile uint32_t elapsed_time_ms = 0;
#define MEASUREMENT_WINDOW_MS  30000  // 30-second measurement window (todo: Add button control to start and stop measurement)

// Define a stabilization period (e.g., first 5 seconds) before recording peaks.
// During this period, data is nor recorded or displayed. 
#define STABILIZATION_TIME_MS  5000

// Arrhythmia Detection 
#define BRADYCARDIA_THRESHOLD  60   // BPM less than 60 indicates bradycardia
#define TACHYCARDIA_THRESHOLD  100  // BPM greater than 100 indicates tachycardia

void start_sample_timer(void)
{
    ret_code_t err_code;
    err_code = app_timer_create(&m_sample_timer,
                                APP_TIMER_MODE_REPEATED,
                                sample_timer_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(m_sample_timer, APP_TIMER_TICKS(10), NULL);
    APP_ERROR_CHECK(err_code);
}

void stop_sample_timer(void)
{
    ret_code_t err_code;
    err_code = app_timer_stop(m_sample_timer);
    APP_ERROR_CHECK(err_code);
}

// Compute the BPM from the detected peaks.
// Returns 0 if there are not enough peaks (min 2) to calculate BPM. 
static uint32_t calculate_bpm(void)
{
    if (peak_count < 2) {
        return 0;  // Not enough peaks to calculate BPM.
    }
    
    uint32_t total_interval = 0;
    for (uint8_t i = 1; i < peak_count; i++)
    {
        total_interval += (peak_timestamps[i] - peak_timestamps[i - 1]);
    }
    
    uint32_t avg_interval_ms = total_interval / (peak_count - 1);
    return 60000 / avg_interval_ms;  // Convert average interval to BPM.
}

// Called every SAMPLE_INTERVAL_MS (2 ms).
void sample_timer_callback(void * p_context)
{
    // Read a raw ADC sample.
    float raw_sample = adc_sample_blocking();  // Returns raw ADC counts (0-4095)

    // Apply a moving-average filter.
    if (ma_index < MOVING_AVG_WINDOW)
    {
        ma_sum += raw_sample;
        ma_buffer[ma_index] = raw_sample;
        ma_index++;
        if (ma_index == MOVING_AVG_WINDOW)
        {
            ma_filled = true;
        }
    }
    else
    {
        uint8_t idx = ma_index % MOVING_AVG_WINDOW;
        ma_sum = ma_sum - ma_buffer[idx] + raw_sample;
        ma_buffer[idx] = raw_sample;
        ma_index++;
    }
    float filtered_sample = (ma_filled) ? (ma_sum / MOVING_AVG_WINDOW) : raw_sample;
    
    // Update elapsed time.
    elapsed_time_ms += SAMPLE_INTERVAL_MS;
    
    // Do not record or display data until the sensor has stabilized.
    // During the stabilization period, simply continue filtering.
    if (elapsed_time_ms < STABILIZATION_TIME_MS)
    {
        NRF_LOG_INFO("Stabilizing sensor... (%d ms)", elapsed_time_ms);
        return;
    }
    
    // Peak Detection
    // Use a static flag to detect rising edges.
    static bool was_above_threshold = false;
    bool is_above_threshold = (filtered_sample > PEAK_THRESHOLD);
    
    if (is_above_threshold && !was_above_threshold)
    {
        if ((elapsed_time_ms - last_peak_time) >= MIN_PEAK_INTERVAL_MS)
        {
            last_peak_time = elapsed_time_ms;
            if (peak_count < MAX_PEAKS)
            {
                peak_timestamps[peak_count++] = elapsed_time_ms;
            }
        }
    }
    was_above_threshold = is_above_threshold;
    
    // When the measurement window is complete, calculate BPM and perform arrhythmia checks.
    if (elapsed_time_ms >= MEASUREMENT_WINDOW_MS)
    {
        uint32_t final_bpm = calculate_bpm();
        
        // Simple arrhythmia detection based on BPM.
        const char *heart_condition = "Normal";
        if (final_bpm > 0)
        {
            if (final_bpm < BRADYCARDIA_THRESHOLD)
            {
                heart_condition = "Bradycardia";
            }
            else if (final_bpm > TACHYCARDIA_THRESHOLD)
            {
                heart_condition = "Tachycardia";
            }
        }

        // todo: Pass the bpm, filtered_sample, and condition to the display here. 

        NRF_LOG_INFO("Final BPM: %u, Condition: %s", final_bpm, heart_condition);
        
        // Stop the sampling timer.
        ret_code_t err_code = app_timer_stop(m_sample_timer);
        APP_ERROR_CHECK(err_code);
    }
}