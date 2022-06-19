/*
 * move.c
 *
 *  Created on: 2022. 6. 14.
 *      Author: hsk
 */

#include "move.h"

extern int32_t tick;

void Move_Smooth(int dist, int minSpeed, int maxSpeed)
{
    int32_t leftSteps, rightSteps;
    enum TachDirection leftDir, rightDir;
    uint16_t leftTach, rightTach;

    Stop();
    Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

    int32_t firstLeftSteps = leftSteps;
    int32_t firstRightSteps = rightSteps;

    int32_t leftSpeed = minSpeed;
    int32_t rightSpeed = minSpeed;

    int leftStop = 0;
    int rightStop = 0;

    Move(leftSpeed, rightSpeed);

    while (1)
    {
        Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

        int32_t leftDiff = ABS(leftSteps - firstLeftSteps);
        int32_t rightDiff = ABS(rightSteps - firstRightSteps);

        int32_t ll = leftDiff * wheelCircumference / slotsPerRotation;
        int32_t rr = rightDiff * wheelCircumference / slotsPerRotation;

        
        if (ll < dist / 2)
        {
            // linearly increase speed [moved, dist/2] -> [minSpeed, maxSpeed]
            leftSpeed = minSpeed + (maxSpeed - minSpeed) * ll / (dist / 2);
        }
        else
        {
            // linearly decrease speed [moved, dist/2] -> [minSpeed, maxSpeed]
            leftSpeed = maxSpeed + (maxSpeed - BRAKE_SPEED) * (dist / 2 - ll) / (dist / 2);
        }

        if (rr < dist / 2)
        {
            // linearly increase speed [moved, dist/2] -> [minSpeed, maxSpeed]
            rightSpeed = minSpeed + (maxSpeed - minSpeed) * rr / (dist / 2);
        }
        else
        {
            // linearly decrease speed [moved, dist/2] -> [minSpeed, maxSpeed]
            rightSpeed = maxSpeed + (maxSpeed - BRAKE_SPEED) * (dist / 2 - rr) / (dist / 2);
        }
        

        leftSpeed = CLAMP(leftSpeed, BRAKE_SPEED, maxSpeed);
        rightSpeed = CLAMP(rightSpeed, BRAKE_SPEED, maxSpeed);

        if (ll >= dist)
        {
            leftStop = 1;
        }
        if (rr >= dist)
        {
            rightStop = 1;
        }

        if (leftStop)
        {
            leftSpeed = 0;
        }
        if (rightStop)
        {
            rightSpeed = 0;
        }

        if (leftStop && rightStop)
        {
            break;
        }

        if (tick % 10 == 0)
        {
            if (leftSpeed && rightSpeed)
            {
                leftSpeed = MAX(leftSpeed, rightSpeed);
                rightSpeed = leftSpeed;
            }
            Move(leftSpeed, rightSpeed);
        }
    }
    
    Stop();
}

void Start_Smooth_Dist(int isForward, int minSpeed, int maxSpeed, int dist)
{
    minSpeed = SLOW_SPEED;
    int32_t leftSteps, rightSteps;
    enum TachDirection leftDir, rightDir;
    uint16_t leftTach, rightTach;

    Stop();

    if (isForward)
    {
        Forward();
    }
    else
    {
        Backward();
    }

    Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

    int32_t firstLeftSteps = leftSteps;
    int32_t firstRightSteps = rightSteps;

    int32_t leftSpeed = minSpeed;
    int32_t rightSpeed = minSpeed;

    int leftStop = 0;
    int rightStop = 0;

    Move(leftSpeed, rightSpeed);
    
    while (1)
    {
        Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

        int32_t leftDiff = ABS(leftSteps - firstLeftSteps);
        int32_t rightDiff = ABS(rightSteps - firstRightSteps);

        int32_t ll = leftDiff * wheelCircumference / slotsPerRotation;
        int32_t rr = rightDiff * wheelCircumference / slotsPerRotation;
        
            // linearly increase speed [moved, dist/2] -> [minSpeed, maxSpeed]
        leftSpeed = minSpeed + (maxSpeed - minSpeed) * ll / (dist);
        rightSpeed = minSpeed + (maxSpeed - minSpeed) * rr / (dist);

        leftSpeed = CLAMP(leftSpeed, minSpeed, maxSpeed);
        rightSpeed = CLAMP(rightSpeed, minSpeed, maxSpeed);

        if (ll >= dist)
        {
            leftStop = 1;
        }
        if (rr >= dist)
        {
            rightStop = 1;
        }

        if (leftStop)
        {
            leftSpeed = 0;
        }
        if (rightStop)
        {
            rightSpeed = 0;
        }

        if (leftStop && rightStop)
        {
            break;
        }

        if (tick % 10 == 0)
        {
            if (leftSpeed && rightSpeed)
            {
                leftSpeed = MAX(leftSpeed, rightSpeed);
                rightSpeed = leftSpeed;
            }
            Move(leftSpeed, rightSpeed);
        }
    }
}

void Start_Smooth(int isForward, int minSpeed, int maxSpeed)
{
    Start_Smooth_Dist(isForward, minSpeed, maxSpeed, smoothStartDist);
}

int Check_Ir();
int Ir_Is_On(int ir_result, int pos);

void Turn_Smooth(int isRight)
{
    if (isRight)
    {
        Right();
    }
    else
    {
        Left();
    }

    int32_t dist = 109955;

    Move_Smooth(dist, TURN_MIN_SPEED, TURN_MAX_SPEED);
    /*
    int irResult = Check_Ir();
    while (1)
    {
        if (Ir_Is_On(irResult, 3) && Ir_Is_On(irResult, 4))
        {
            Stop();
            return;
        }

        if (Ir_Is_On(irResult, 2) || Ir_Is_On(irResult, 1) || Ir_Is_On(irResult, 0))
        {
            if (isRight)
            {
                Left();
            }
            else
            {
                Right();
            }
            Move(TURN_MIN_SPEED, TURN_MIN_SPEED);
        }
        else if (Ir_Is_On(irResult, 5) || Ir_Is_On(irResult, 6) || Ir_Is_On(irResult, 7))
        {
            if (isRight)
            {
                Right();
            }
            else
            {
                Left();
            }
            Move(TURN_MIN_SPEED, TURN_MIN_SPEED);
        }

        Wait();
    }
    */
}

void Turn_Fast(int isRight)
{
    if (isRight)
    {
        Right();
    }
    else
    {
        Left();
    }

    int32_t dist = 109955;

    Move_Smooth(dist, TURN_FAST_MIN_SPEED, TURN_FAST_MAX_SPEED);
}

void Move_Exactly(int isForward, int dist, int minSpeed, int maxSpeed)
{
    if (isForward)
    {
        Forward();
    }
    else
    {
        Backward();
    }

    Move_Smooth(dist, minSpeed, maxSpeed);
}

void PWM_Init34(uint16_t period, uint16_t duty3, uint16_t duty4)
{
    P2->DIR |= 0xC0;
    P2->SEL0 |= 0xC0;
    P2->SEL1 &= ~0xC0;

    TIMER_A0->CCTL[0] = 0x800;
    TIMER_A0->CCR[0] = period;

    TIMER_A0->EX0 = 0x0000;

    TIMER_A0->CCTL[3] = 0x0040;
    TIMER_A0->CCR[3] = duty3;
    TIMER_A0->CCTL[4] = 0x0040;
    TIMER_A0->CCR[4] = duty4;

    TIMER_A0->CTL = 0x02F0;
}

void Motor_Init()
{
    P3->SEL0 &= ~0xC0;
    P3->SEL1 &= ~0xC0;
    P3->DIR |= 0xC0;
    P3->OUT &= ~0xC0;

    P5->SEL0 &= ~0x30;
    P5->SEL1 &= ~0x30;
    P5->DIR |= 0x30;
    P5->OUT &= ~0x30;

    P2->SEL0 &= ~0xC0;
    P2->SEL1 &= ~0xC0;
    P2->DIR |= 0xC0;
    P2->OUT &= ~0xC0;

    PWM_Init34(15000, 0, 0);
}

void PWM_Duty3(uint16_t duty3)
{
    if (TIMER_A0->CCR[0] <= duty3)
    {
        return;
    }
    TIMER_A0->CCR[3] = duty3;
}

void PWM_Duty4(uint16_t duty4)
{
    if (TIMER_A0->CCR[0] <= duty4)
    {
        return;
    }
    TIMER_A0->CCR[4] = duty4;
}

void Move(uint16_t leftDuty, uint16_t rightDuty)
{
    P3->OUT |= 0xC0;
    PWM_Duty3(rightDuty);
    PWM_Duty4(leftDuty);
}

extern void SysTick_Wait1ms(uint32_t);
void Stop_With_Wait()
{
    P3->OUT &= ~0xC0;
    PWM_Duty3(0);
    PWM_Duty4(0);
    SysTick_Wait1ms(stopWithWaitTime);
}

void Stop()
{
    P3->OUT &= ~0xC0;
    PWM_Duty3(0);
    PWM_Duty4(0);
}

void Forward()
{
    P5->OUT &= ~0x10;
    P5->OUT &= ~0x20;
}

void Backward()
{
    P5->OUT |= 0x10;
    P5->OUT |= 0x20;
}

void Left()
{
    P5->OUT |= 0x10;
    P5->OUT &= ~0x20;
}

void Right()
{
    P5->OUT &= ~0x10;
    P5->OUT |= 0x20;
}
