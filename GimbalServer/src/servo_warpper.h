#ifndef SERVO_WARPPER_H
#define SERVO_WARPPER_H
#include "driver/ledc.h"

namespace hardware
{

    class mapper
    {
    private:
        int s_start;
        int s_end;
        int d_start;
        int d_end;
        float interval;
        /* data */
    public:
        mapper(int s_start_, int s_end_, int d_start_, int d_end_);
        ~mapper();
        uint32_t map(int s);
    };

    class Servo
    {
    private:
        static ledc_timer_t servo_timer;
        static bool servo_timer_configd;
        mapper a2d;
        ledc_channel_t channel;
        /* data */
    public:
        Servo(mapper a2d_, uint8_t pin_, ledc_channel_t channel);
        void write(int value);
        ~Servo();
    };

    extern Servo Servo_X;
    extern Servo Servo_Y;
    void servo_init();
}
#endif