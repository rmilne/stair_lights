#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 0
#define FSR 0
#define MOTION 2
#define ON 1
#define OFF 0
#define UP 2
#define DOWN 3

// Stairs settings
#define UPDATE_DELAY 50 // wait between updates of whole loop
#define FADE_RATE 30
#define NUM_STAIRS 16
#define BRIGHTNESS 254
#define DURATION 10000  // 10 seconds
#define STAIR_DELAY 600 // .6 seconds between stairs
#define FSR_THRESHOLD = 150 // TODO Needs calibration



Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_STAIRS, PIN, NEO_GRB + NEO_KHZ800);
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
//


class Step
{
    int index;
    int state;
    int brightness;
    int target_brightness;
    long duration;
    unsigned long prev_time;
    int fade;


    public:
    Step(int idx, int fade_rate)
    {
        index = idx;		// Which step
        state = OFF;		// Current state
        brightness = 0;		// Current brightness
        target_brightness = 0;	// Fade toward this value brightness
        duration = 0;		// How long to be ON
        fade = fade_rate;		// Fade speed
    }

    void set(int on, int target, long dur)
    {
        state = on;
        target_brightness = target;
        duration = dur;
        prev_time = millis();
    }

    void update(unsigned long curr_time)
    {
        if (state == ON)
        {
            if (brightness < target_brightness)
            {
                brightness = brightness + fade;
                if (brightness > 255) 
                {
                    brightness = 255;
                }
                strip.setPixelColor(index, brightness, brightness, brightness);
            }

            // set how much time is left
            duration = duration - (curr_time - prev_time);
            if (duration <= 0 || target_brightness <= 0)
            {
                state = OFF;
                target_brightness = 0;
            }
        } 
        if (state == OFF)
        {
            target_brightness = 0;
            if (brightness > target_brightness) 
            {
                brightness = target_brightness - fade;
                if (brightness <= 0)
                {
                    brightness = 0;

                }
                strip.setPixelColor(index, brightness, brightness, brightness);	
            }
        }
    }
};

class Stairs
{
    Step steps[NUM_STAIRS];
    int state;
    int last_stair_idx;
    unsigned long last_stair_on_time;
    unsigned long stair_delay;
    int brightness;
    long duration;

    public:
    Stairs(int bright, long dur, unsigned long strdelay)
    {
        int i = 0;

        // Variables for staggered stair steps
        state = OFF;
        last_stair_idx = -1;
        last_stair_on_time = 0;
        stair_delay = strdelay;
        brightness = bright;
        duration = dur;


        // create stairs
        for (i = 0; i < NUM_STAIRS; i++) {
            steps[i] = new Step(i, FADE_RATE);
        }
    }

    void update()
    {
        int i = 0;
        unsigned long curr_time = millis();
        long since_last_update = curr_time - last_stair_on_time;
        int update = 0;

        // check for stairs turning on in progress
        if (since_last_update >= stair_delay || since_last_update < 0) 
        {
            if (state == UP) 
            {
                last_stair_idx++;
                steps[last_stair_idx].set(ON, brightness, duration)
                    if (last_stair_idx == (NUM_STAIRS - 1))
                    {
                        state = OFF;
                    }
            }
            if (state == DOWN)
            {
                last_stair_idx--;
                steps[last_stair_idx].set(ON, brightness, duration)
                    if (last_stair_idx == 0)
                    {
                        state = OFF;
                    }
            }
        }

        // update each step
        for (i = 0; i < NUM_STAIRS; i++) {
            steps[i].update(curr_time);
        }
    }

    void going_up() 
    {
        state = UP;
        last_stair_idx = 0;
        last_stair_on_time = 0;
    }

    void going_down()
    {
        state = DOWN;
        last_stair_idx = NUM_STAIRS-1;
        last_stair_on_time = 0;
    }
};

long last_fsr;
long last_motion;
Stairs stairs = new Stairs(BRIGHTNESS, DURATION, STAIR_DELAY);

void setup() {
    pinMode(MOTION, INPUT);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    last_fsr = 0;
    last_motion = 0;
}

void loop() {
    // FSR is top of stairs
    // MOTION is bottom
    bool top;
    bool bottom;
    if (stairs.state == OFF)
    {
        top = fsr_detect();
        bottom = motion_detect();
        if (top)
        {
            stairs.going_down();
        }
        else if (bottom)
        {
            stairs.going_up();
        }
    }
    stairs.update();
    delay(UPDATE_DELAY);
}

bool fsr_detect() 
{
    // TODO - perhaps build moving window of values and take the current difference from the resting average
    //        this will work better if the resting stair pressure changes over time (house movement)
    uint16_t fsr = analogRead(FSR);
    if (fsr >= FSR_THRESHOLD)
    {
        return true;
    }
    return false;
}

void motion_detect() {
    int motion = digitalRead(MOTION);
    if (motion == HIGH)
    {
        return true;
    }
    return false;
}

