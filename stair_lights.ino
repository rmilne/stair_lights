#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define NEOPIXEL_PIN 7
#define FSR 0
#define MOTION 2
#define ON 1
#define OFF 0
#define UP 2
#define DOWN 3

// Stairs settings
#define UPDATE_DELAY 10 // (50 previously) wait between updates of whole loop
#define FADE_RATE 2
#define NUM_STAIRS 16
#define BRIGHTNESS 254
#define DURATION 10000  // 10 seconds
#define STAIR_DELAY 600 // .6 seconds between stairs
#define FSR_THRESHOLD 150 // TODO Needs calibration
#define DEBUG 0


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_STAIRS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
//

void p(char *str)
{
  if (DEBUG)
    Serial.println(str);
}

class Step
{
    public:
      int index;
      int state;
      int brightness;
      int target_brightness;
      long duration;
      unsigned long prev_time;
      int fade;

    public:
    Step() {}
    
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
            }
        } 
        if (state == OFF)
        {
            target_brightness = 0;
            if (brightness > target_brightness) 
            {
                brightness = brightness - fade;
                if (brightness <= 0)
                {
                    brightness = 0;
                }
                strip.setPixelColor(index, brightness, brightness, brightness);	
            }
        }
        prev_time = curr_time;
    }
};

class Stairs
{
    public:
      Step *steps[NUM_STAIRS];
      int state;
      int last_stair_idx;
      unsigned long last_stair_on_time;
      unsigned long last_print;
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
        last_print = 0;
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

        // check for stairs turning on in progress       
        if (since_last_update >= stair_delay || since_last_update < 0) 
        {
            if (state == UP) 
            {
                steps[last_stair_idx]->set(ON, brightness, duration);
                last_stair_on_time = curr_time;
                if (last_stair_idx == (NUM_STAIRS - 1))
                {
                    state = OFF; // reached the end
                    last_stair_idx = 0;
                }
                else
                {
                    last_stair_idx++;
                }
            }
            if (state == DOWN)
            {
                steps[last_stair_idx]->set(ON, brightness, duration);
                last_stair_on_time = curr_time;
                if (last_stair_idx == 0)
                {
                    state = OFF; // reached the end
                    last_stair_idx = 0;
                }
                else
                {
                    last_stair_idx--;
                }
            }
        }

        // update each step
        print_stairs(curr_time);
        for (i = 0; i < NUM_STAIRS; i++) {
            steps[i]->update(curr_time);
        }
    }

    void going_up() 
    {
        // TODO - cool down period, motion sensor stays high for a few seconds
        state = UP;
        last_stair_idx = 0;
        last_stair_on_time = 0;
    }

    void going_down()
    {
        // TODO - another cooldown period
        state = DOWN;
        last_stair_idx = NUM_STAIRS-1;
        last_stair_on_time = 0;
    }

    void print_stairs(unsigned long curr_time)
    {
        if (DEBUG != 1) return;
        if (curr_time - last_print > 300 && DEBUG) {
            for (int i = 0; i < NUM_STAIRS; i++) {
                Serial.print(steps[i]->state);
                Serial.print("\t");
            }
            Serial.println("<- state");
            for (int i = 0; i < NUM_STAIRS; i++) {
                Serial.print(steps[i]->brightness);
                Serial.print("\t");
            }
            Serial.println("<- brightness");
            for (int i = 0; i < NUM_STAIRS; i++) {
                Serial.print(steps[i]->duration);
                Serial.print("\t");
            }
            Serial.println("<- duration");
            /*
            for (int i = 0; i < NUM_STAIRS; i++) {
                Serial.print(strip.getPixelColor(i));
                Serial.print("\t");
            }
            Serial.println("<- pixel color");
            */
            last_print = curr_time;
        }
    }
};

long last_fsr;
long last_motion;
Stairs stairs(BRIGHTNESS, DURATION, STAIR_DELAY);


void setup() {
    pinMode(MOTION, INPUT);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    // All on to test LEDs working
    uint32_t magenta = strip.Color(255, 100, 255);
    strip.fill(magenta, 0, NUM_STAIRS);
    strip.show();
    delay(2000);
    magenta = strip.Color(0, 0, 0);
    strip.fill(magenta, 0, NUM_STAIRS);
    strip.show();
    
    last_fsr = 0;
    last_motion = 0;
    if (DEBUG)
    {
        Serial.begin(9600);
    }
    p("in setup");
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
            p("detected top weight, going_down()");
            stairs.going_down();
        }
        else if (bottom)
        {
            p("detected bottom motion, going_up()");
            stairs.going_up();
        }
    }
    stairs.update();
    strip.show();
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

bool motion_detect() {
    int motion = digitalRead(MOTION);
    if (motion == HIGH)
    {
        return true;
    }
    return false;
}
