#include "servo_warpper.h"
#include <Arduino.h>
namespace hardware
{
    bool Servo::servo_timer_configd = false;
    ledc_timer_t Servo::servo_timer = LEDC_TIMER_1;
    // d_start:0.5ms->frequency is 50hz
    // 0.5/20 -> 0.025, 2^13 = 8192, 0.025*(2^13) ~= 205
    // d_start:2.5ms->frequency is 50hz
    // 2.5/20 -> 0.125, 2^13 = 8192, 0.125*(2^13) ~= 1024
    mapper m1(0, 180, 205, 1024);
    Servo Servo_X(m1, 26, LEDC_CHANNEL_0);
    Servo Servo_Y(m1, 25, LEDC_CHANNEL_1);
    void Servo::write(int value) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, a2d.map(value));
        ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    }
    Servo::Servo(mapper a2d_, uint8_t pin_, ledc_channel_t channel_) : a2d(a2d_), channel(channel_)
    {
        ledc_channel_config_t ledc_channel = {
            .gpio_num = pin_,                  /*!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16 */
            .speed_mode = LEDC_LOW_SPEED_MODE, /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
            .channel = channel_,               /*!< LEDC channel (0 - 7) */
            .intr_type = LEDC_INTR_DISABLE,    /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
            .timer_sel = servo_timer,          /*!< Select the timer source of channel (0 - 3) */
            .duty = 0,                         /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
            .hpoint = 0                        /*!< LEDC channel hpoint value, the max value is 0xfffff */
        };
        ledc_channel_config(&ledc_channel);
        if (!servo_timer_configd)
        {
            servo_timer_configd = true;
            
            ledc_timer_config_t ledc_timer = {
                speed_mode:LEDC_LOW_SPEED_MODE,{                /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
                duty_resolution:LEDC_TIMER_13_BIT},  /*!< LEDC channel duty resolution */
                timer_num:servo_timer,               /*!< The timer source of channel (0 - 3) */
                freq_hz:50
            };
            ledc_timer_config(&ledc_timer);
            // ledc_fade_func_install(0);
        }
    }
    Servo::~Servo() {}

    mapper::mapper(int s_start_, int s_end_, int d_start_, int d_end_) : s_start(s_start_), s_end(s_end_), d_start(d_start_), d_end(d_end_) {
        interval = (d_end_ - d_start_ + 0.0) / (s_end_ - s_start_);
    }
    mapper::~mapper() {}
    uint32_t mapper::map(int s){
        //just in case
        if (s > s_end) s = s_end;
        if (s < s_start) s = s_start;
        return (d_start + (int)((s - s_start) * interval)) > d_end ? d_end : (d_start + (int)((s - s_start) * interval));
    }
}