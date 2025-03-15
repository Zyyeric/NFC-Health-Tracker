#include "pulsesensor_util.h"
#include "pulsesensor.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "app_timer.h"
#include "nrf_delay.h"

// Moving average filter settings
#define MOVING_AVG_WINDOW 10    // Use a larger window for better smoothing
static float ma_buffer[MOVING_AVG_WINDOW];  
static uint32_t ma_sample_count = 0;        
static float ma_sum = 0.0f;                 
static bool ma_filled = false;              

// Display update interval
#define DISPLAY_UPDATE_INTERVAL_MS 3000  
static uint32_t last_display_update = 0;

// Peak detection thresholds with hysteresis
#define PEAK_THRESHOLD        2350.0f   // Must exceed to count as a peak
#define LOWER_THRESHOLD       2000.0f   // Must fall below before detecting a new peak
#define MIN_PEAK_INTERVAL_MS  650       // Refractory period for valid peaks

// Sliding window for peak timestamps (only the most recent peaks are used)
#define SLIDING_WINDOW_SIZE 5
static uint32_t peak_timestamps[SLIDING_WINDOW_SIZE];
static uint8_t  peak_count = 0;
static uint32_t last_peak_time = 0;
static bool peak_detected = false;  

// Time threshold for resetting the sliding window if no valid peaks occur
#define NO_PEAK_TIMEOUT_MS 5000

// Timer for periodic sampling (every 2 ms)
APP_TIMER_DEF(m_sample_timer);
#define SAMPLE_INTERVAL_MS   2
#define APP_TIMER_TICKS_MS(x) APP_TIMER_TICKS(x)
static volatile uint32_t elapsed_time_ms = 0;
#define MEASUREMENT_WINDOW_MS  30000  // 30-second measurement window

// Stabilization period: ignore data for the first 5 seconds
#define STABILIZATION_TIME_MS  5000

// Start the sample timer.
void start_sample_timer(void)
{
    ret_code_t err_code;
    err_code = app_timer_create(&m_sample_timer,
                                APP_TIMER_MODE_REPEATED,
                                sample_timer_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(m_sample_timer, APP_TIMER_TICKS(SAMPLE_INTERVAL_MS), NULL);
    APP_ERROR_CHECK(err_code);
}

// Stop the sample timer.
void stop_sample_timer(void)
{
    ret_code_t err_code;
    err_code = app_timer_stop(m_sample_timer);
    APP_ERROR_CHECK(err_code);
}

// Compute BPM from the detected peaks using the sliding window.
// Returns 0 if there are not enough peaks.
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
    return (avg_interval_ms > 0) ? (60000 / avg_interval_ms) : 0;
}

// Called every SAMPLE_INTERVAL_MS (2 ms).
void sample_timer_callback(void * p_context)
{
    // Read a raw ADC sample.
    float raw_sample = adc_sample_blocking();  // ADC counts (0-4095)

    // Apply a moving-average filter.
    uint8_t index = (uint8_t)(ma_sample_count % MOVING_AVG_WINDOW);
    if (ma_sample_count < MOVING_AVG_WINDOW)
    {
        ma_sum += raw_sample;
        ma_buffer[index] = raw_sample;
        ma_sample_count++;
        if (ma_sample_count >= MOVING_AVG_WINDOW)
        {
            ma_filled = true;
        }
    }
    else
    {
        ma_sum = ma_sum - ma_buffer[index] + raw_sample;
        ma_buffer[index] = raw_sample;
        ma_sample_count++;
    }
    float filtered_sample = (ma_filled) ? (ma_sum / MOVING_AVG_WINDOW) : raw_sample;
    
    // Update elapsed time.
    elapsed_time_ms += SAMPLE_INTERVAL_MS;
    
    // Skip processing during the stabilization period.
    if (elapsed_time_ms < STABILIZATION_TIME_MS)
    {
        return;
    }
    
    // If no valid peak has been detected for a while, reset the sliding window.
    if ((elapsed_time_ms - last_peak_time) > NO_PEAK_TIMEOUT_MS)
    {
        peak_count = 0;  // Reset the window
    }
    
    // Peak Detection with hysteresis.
    if (!peak_detected &&
        (filtered_sample > PEAK_THRESHOLD) &&
        ((elapsed_time_ms - last_peak_time) >= MIN_PEAK_INTERVAL_MS))
    {
        // Rising edge detected.
        last_peak_time = elapsed_time_ms;
        if (peak_count < SLIDING_WINDOW_SIZE)
        {
            peak_timestamps[peak_count++] = elapsed_time_ms;
        }
        else
        {
            // Shift the sliding window.
            for (uint8_t i = 0; i < SLIDING_WINDOW_SIZE - 1; i++)
            {
                peak_timestamps[i] = peak_timestamps[i + 1];
            }
            peak_timestamps[SLIDING_WINDOW_SIZE - 1] = elapsed_time_ms;
        }
        peak_detected = true;
    }
    else if (peak_detected && (filtered_sample < LOWER_THRESHOLD))
    {
        // Allow detection of the next peak.
        peak_detected = false;
    }
    
    // Update display at fixed intervals.
    if ((elapsed_time_ms - last_display_update) >= DISPLAY_UPDATE_INTERVAL_MS)
    {
        last_display_update = elapsed_time_ms;
        uint32_t current_bpm = calculate_bpm();
        if (current_bpm != 0)
        {
            printf("Current BPM: %lu\n", current_bpm);
        }
        else
        {
            // Optionally indicate that no valid pulse is detected.
            printf("No valid pulse detected.\n");
        }
    }
    
    // End measurement after the set window.
    if (elapsed_time_ms >= MEASUREMENT_WINDOW_MS)
    {
        uint32_t final_bpm = calculate_bpm();
        printf("Last BPM Sample: %lu\n", final_bpm);
        stop_sample_timer();
    }
}
