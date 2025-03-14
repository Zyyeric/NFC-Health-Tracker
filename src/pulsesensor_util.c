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
static uint32_t ma_sample_count = 0;        // Numbers of samples processed 
static float ma_sum = 0.0f;                 // Sum of samples in the window
static bool ma_filled = false;              // Indicates if the window is fully populated

#define DISPLAY_UPDATE_INTERVAL_MS 3000  // Update display every 3 seconds
static uint32_t last_display_update = 0;

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

// Encapsulate the logic for starting the sample timer. 
// Therefore, sample timer instance can be reused across modules. 
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

// Encapsulate the logic for ending the sample timer. 
// Therefore, sample timer instance can be reused across modules. 
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
    
    // Do not record or display data until the sensor has stabilized.
    // During the stabilization period, simply continue filtering.
    if (elapsed_time_ms < STABILIZATION_TIME_MS)
    {
        static uint32_t last_stab_log = 0;
        if ((elapsed_time_ms - last_stab_log) >= 500) // log every 500 ms during stabilization
        {
            last_stab_log = elapsed_time_ms;
            NRF_LOG_INFO("Stabilizing sensor... (%d ms)", elapsed_time_ms);
            // todo: tell the display that we are still stabilizing.
        }
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

    // Update display at fix intervals.
    if ((elapsed_time_ms - last_display_update) >= DISPLAY_UPDATE_INTERVAL_MS)
    {
        last_display_update = elapsed_time_ms;
        uint32_t current_bpm = calculate_bpm();

        // todo: Here we shall pass the current BPM and filtered sample to the display.
        // In addition, we pass a flag (final = 0) to indicate that sampling is still ongoing.
        // Something like -> update_display(current_bpm, filtered_sample, 0);
        NRF_LOG_INFO("Current BPM: %u", current_bpm);
    }
    
    // When the measurement window is complete, send a final update.
    if (elapsed_time_ms >= MEASUREMENT_WINDOW_MS)
    {
        uint32_t final_bpm = calculate_bpm();

        // todo: Pass the final BPM and the latest filtered sample.
        // The display module will compute the average BPM over the window and determine the health condition.
        // ->>>>>> update_display(final_bpm, filtered_sample, 1);
        NRF_LOG_INFO("Final BPM: %u", final_bpm);
        
        // Stop the sampling timer.
        stop_sample_timer();
    }
}