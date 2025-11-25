#define SERVO 2

#include <iostream>
#include <wiringPi.h>
#include <softPwm.h>

using namespace std;

class dist{
    private:
        float result, s, e;
    public:
        void measures(float i);
        void measuree(float j);
        float calc();
};

void dist::measures(float i)
{
    s = i;
}

void dist::measuree(float j)
{
    e = j;
}

float dist::calc()
{
    result = (e - s) / 58;
    cout << "distance(cm): " << result << "\n";
    return result;
}

int servorControlUp();
int servorControlDown();

int main(void)
{
    dist sonic;

    if(wiringPiSetup() == -1)
    {
        return -1;
    }

    softPwmCreate(SERVO, 0, 200);

    pinMode(0, OUTPUT);
    pinMode(1, INPUT);

    while(1)
    {
        digitalWrite(0, 0);
        delayMicroseconds(2);
        digitalWrite(0, 1);
        delayMicroseconds(10);
        digitalWrite(0, 0);   

        while(digitalRead(1) == 0)
        {
            sonic.measures(micros());
        }
        while(digitalRead(1) == 1)
        {
            sonic.measuree(micros());
        }
        
        if(50 < sonic.calc())
        {
            servorControlUp();
            delay(5000);
            servorControlDown();
        }
        
        delay(100);
    }
    
    return 0;
}

int servorControlUp()
{
    int i;
    int dir = 1;
    int pos = 5;
    
    while(1)
    {
        pos += dir;
        softPwmWrite(SERVO, pos);
        if(pos >= 25) break;
        delay(10);
    }

    return 0;
}

int servorControlDown()
{
    int i;
    int dir = -1;
    int pos = 25;

    while(1)
    {
        pos += dir;
        softPwmWrite(SERVO, pos);
        if(pos <= 5) break;
        delay(10);
    }

    return 0;
}