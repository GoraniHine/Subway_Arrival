#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

#define SERVO 2

int servorControl()
{
    int i;
    int dir = 1;
    int pos = 5;
    softPwmCreate(SERVO, 0, 200);

    while(1)
    {
        pos += dir;
        if(pos < 5 || pos > 25) dir *= -1;
        softPwmWrite(SERVO, pos);
        delay(10);
    }

    return 0;
}

int main(void)
{
    if(wiringPiSetup() == -1)
    {
        return -1;
    }

    servoControl();

    return 0;
}