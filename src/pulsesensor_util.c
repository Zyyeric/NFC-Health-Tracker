#include "pulsesensor_util.h"
#include "pulsesensor.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <display.h>
#include "max30102.h"
#include "app_timer.h"
#include "nrf_delay.h"

// Moving average filter settings
#define MOVING_AVG_WINDOW 10   
static float ma_buffer[MOVING_AVG_WINDOW];  
static uint32_t ma_sample_count = 0;        
static float ma_sum = 0.0f;                 
static bool ma_filled = false;        
static bool init_display = false;    

// Moving BPM average 
#define MOVING_AVG_BPM_WINDOW 5 
static float bpm_buffer[MOVING_AVG_BPM_WINDOW];
static uint32_t bpm_sample_count = 0;
static float bpm_sum = 0.0f;
static bool bpm_buffer_filled = false;

// Display update interval
#define DISPLAY_UPDATE_INTERVAL_MS 3000  
static uint32_t last_display_update = 0;

// Peak detection thresholds
#define PEAK_THRESHOLD        2350.0f   // Must exceed to count as a peak
#define LOWER_THRESHOLD       2000.0f   // Must fall below before detecting a new peak
#define MIN_PEAK_INTERVAL_MS  650       // Time between valid peaks

// Sliding window for recent peak timestamps
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
#define MEASUREMENT_WINDOW_MS  30000 

// Stabilization period where data is ignored for the first 5 seconds
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
    printf("end of start_sample_timer\n");
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
    else {
        if(!init_display) {
            uint16_t white = 0xFFFF;
            uint16_t black = 0x0000;

            // Write the BPM text
            write_text('B', white, black, 0, 0, 38, 23);
            write_text('P', white, black, 0, 25, 38, 48);
            write_text('M', white, black, 0, 50, 38, 73);
            write_text(':', white, black, 0, 75, 38, 98);
            write_text('-', white, black, 0, 100, 38, 123);
            write_text('-', white, black, 0, 125, 38, 148);

            // Write the temp text
            write_text('T', white, black, 48, 0, 86, 23);
            write_text('E', white, black, 48, 25, 86, 48);
            write_text('M', white, black, 48, 50, 86, 73);
            write_text('P', white, black, 48, 75, 86, 98);
            write_text(':', white, black, 48, 100, 86, 123);
            write_text('-', white, black, 48, 125, 86, 148);
            write_text('-', white, black, 48, 150, 86, 173);
            init_display = true;
        }
    }
    // If no valid peak has been detected for a while, reset the sliding window.
    if ((elapsed_time_ms - last_peak_time) > NO_PEAK_TIMEOUT_MS)
    {
        peak_count = 0;  // Reset the window
    }
    
    // Peak Detection 
    // Only detect peaks if the sample is above the threshold and the peak interval has passed.
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
        peak_detected = false; // Falling edge detected
    }
    
    // Update display at fixed intervals.
    if ((elapsed_time_ms - last_display_update) >= DISPLAY_UPDATE_INTERVAL_MS)
    {
        last_display_update = elapsed_time_ms;
        uint32_t current_bpm = calculate_bpm();
        if (current_bpm != 0)
        {   
            // Update the BPM moving average. 
            uint8_t bpm_index = bpm_sample_count % MOVING_AVG_BPM_WINDOW;
            if (bpm_sample_count < MOVING_AVG_BPM_WINDOW)
            {
                bpm_sum += current_bpm;
                bpm_buffer[bpm_index] = current_bpm; 
                bpm_sample_count ++; 
                if (bpm_sample_count >= MOVING_AVG_BPM_WINDOW) 
                {
                    bpm_buffer_filled = true; 
                }
            }
            else {
                bpm_sum = bpm_sum - bpm_buffer[bpm_index] + current_bpm; 
                bpm_buffer[bpm_index] = current_bpm; 
                bpm_sample_count ++; 
            }
            float filtered_bpm = (bpm_buffer_filled) ? (bpm_sum / MOVING_AVG_BPM_WINDOW) : (bpm_sum / bpm_sample_count); 
            uint32_t bpm_to_int = (uint32_t) filtered_bpm;
            printf("Current BPM: %lu\n", bpm_to_int);
            // Display the BPM 
            write_bpm(bpm_to_int);
            
            // Write the correct diagnosis to the display
            if (bpm_to_int < 60)
            {
                write_text('B', 0x00F8, 0x0000, 96, 0, 134, 23);
                write_text('R', 0x00F8, 0x0000, 96, 25, 134, 48);
                write_text('A', 0x00F8, 0x0000, 96, 50, 134, 73);
                write_text('D', 0x00F8, 0x0000, 96, 75, 134, 98);
                write_text('Y', 0x00F8, 0x0000, 96, 100, 134, 123);
                write_text('C', 0x00F8, 0x0000, 96, 125, 134, 148);
                write_text('A', 0x00F8, 0x0000, 96, 150, 134, 173);
                write_text('R', 0x00F8, 0x0000, 96, 175, 134, 198);
                write_text('D', 0x00F8, 0x0000, 96, 200, 134, 223);
                write_text('I', 0x00F8, 0x0000, 96, 225, 134, 248);
                write_text('A', 0x00F8, 0x0000, 96, 250, 134, 273);   
            }
           else if (bpm_to_int > 100) {
                write_text('T', 0x00F8, 0x0000, 96, 0, 134, 23);
                write_text('A', 0x00F8, 0x0000, 96, 25, 134, 48);
                write_text('C', 0x00F8, 0x0000, 96, 50, 134, 73);
                write_text('H', 0x00F8, 0x0000, 96, 75, 134, 98);
                write_text('Y', 0x00F8, 0x0000, 96, 100, 134, 123);
                write_text('C', 0x00F8, 0x0000, 96, 125, 134, 148);
                write_text('A', 0x00F8, 0x0000, 96, 150, 134, 173);
                write_text('R', 0x00F8, 0x0000, 96, 175, 134, 198);
                write_text('D', 0x00F8, 0x0000, 96, 200, 134, 223);
                write_text('I', 0x00F8, 0x0000, 96, 225, 134, 248);
                write_text('A', 0x00F8, 0x0000, 96, 250, 134, 273);   
            }
            else {
                write_text('R', 0x07E0, 0x0000, 96, 0, 134, 23);
                write_text('E', 0x07E0, 0x0000, 96, 25, 134, 48);
                write_text('G', 0x07E0, 0x0000, 96, 50, 134, 73);
                write_text('U', 0x07E0, 0x0000, 96, 75, 134, 98);
                write_text('L', 0x07E0, 0x0000, 96, 100, 134, 123);
                write_text('A', 0x07E0, 0x0000, 96, 125, 134, 148);
                write_text('R', 0x07E0, 0x0000, 96, 150, 134, 173);
                write_text(' ', 0x07E0, 0x0000, 96, 175, 134, 198);
                write_text('B', 0x07E0, 0x0000, 96, 200, 134, 223);
                write_text('P', 0x07E0, 0x0000, 96, 225, 134, 248);
                write_text('M', 0x07E0, 0x0000, 96, 250, 134, 273);  ;
            }
        }
        else
        {
            printf("No valid pulse detected.\n");
            write_text('-', 0xFFFF, 0x0000, 0, 100, 38, 123);
            write_text('-', 0xFFFF, 0x0000, 0, 125, 38, 148);
            write_text(' ', 0xFFFF, 0x0000, 96, 0, 134, 23);
            write_text(' ', 0xFFFF, 0x0000, 96, 25, 134, 48);
            write_text(' ', 0xFFFF, 0x0000, 96, 50, 134, 73);
            write_text(' ', 0xFFFF, 0x0000, 96, 75, 134, 98);
            write_text(' ', 0xFFFF, 0x0000, 96, 100, 134, 123);
            write_text(' ', 0xFFFF, 0x0000, 96, 125, 134, 148);
            write_text(' ', 0xFFFF, 0x0000, 96, 150, 134, 173);
            write_text(' ', 0xFFFF, 0x0000, 96, 175, 134, 198);
            write_text(' ', 0xFFFF, 0x0000, 96, 200, 134, 223);
            write_text(' ', 0xFFFF, 0x0000, 96, 225, 134, 248);
            write_text(' ', 0xFFFF, 0x0000, 96, 250, 134, 273);
        }

        // Read the temperature
        max30102_read_temp();
    }
}