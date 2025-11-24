#include <iostream>
#include <wiringPi.h>

using namespace std;

class dist{
    private:
        float result, s, e;
    public:
        void measures(float i);
        void measuree(float j);
        void calc();
};

void dist::measures(float i)
{
    s = i;
}

void dist::measuree(float j)
{
    e = j;
}

void dist::calc()
{
    result = (e - s) / 58;
    cout << "distance(cm): " << result << "\n";
}

int main(void)
{
    dist sonic;

    wiringPiSetup();

    pinMode(0, OUTPUT);
    pinMode(1, INPUT);

    while(1)
    {
        digitalWrite(0, 0);
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
        sonic.calc();
        delay(100);
    }
    
    return 0;
}
