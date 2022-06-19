/*
 * move.h
 *
 *  Created on: 2022. 6. 14.
 *      Author: hsk
 */

#ifndef MOVE_H_
#define MOVE_H_

#include "msp432p401r.h"

#include "../inc/odometry.h"
#include "../inc/Tachometer.h"
#include "../inc/TA3InputCapture.h"

/*
#define GLOBAL_SLOW_SPEED 2000
#define SLOW_SPEED 2000
#define FAST_SPEED 3000
#define TURN_MIN_SPEED 2000
#define TURN_MAX_SPEED 2500
#define TURN_FAST_MIN_SPEED 2500
#define TURN_FAST_MAX_SPEED 3500
*/

// #define GLOBAL_SLOW_SPEED 1700
// #define FAST_SPEED 2200
// #define SLOW_SPEED 2500
// #define SUPER_FAST_SPEED 3500
// #define TURN_MIN_SPEED 1700
// #define TURN_MAX_SPEED 2200
// #define TURN_FAST_MIN_SPEED 2500
// #define TURN_FAST_MAX_SPEED 3500

// 40초 배터리
// #define GLOBAL_SLOW_SPEED 2500
// #define BRAKE_SPEED 1000
// #define FAST_SPEED 3500
// #define SLOW_SPEED 3000
// #define SUPER_FAST_SPEED 5000
// #define TURN_MIN_SPEED 2500
// #define TURN_MAX_SPEED 3500
// #define TURN_FAST_MIN_SPEED 2500
// #define TURN_FAST_MAX_SPEED 3500
// #define ZigZag_Speed(runSpeed) (runSpeed)
// #define DETECT_INTERSECTION_LENGTH (5000)
// #define FINISH_LINE_DETECTED_LENGTH (5000)

#define GLOBAL_SLOW_SPEED 2500
#define BRAKE_SPEED 1000
#define FAST_SPEED 3500
#define SLOW_SPEED 2500
#define SUPER_FAST_SPEED 4000
#define TURN_MIN_SPEED 2500
#define TURN_MAX_SPEED 3500
#define TURN_FAST_MIN_SPEED 2500
#define TURN_FAST_MAX_SPEED 3500
#define ZigZag_Speed(runSpeed) (runSpeed)
#define DETECT_INTERSECTION_LENGTH (5000)
#define FINISH_LINE_DETECTED_LENGTH (5000)

// #define GO_BEFORE_TURN 5000
#define GO_BEFORE_TURN 11000

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))
#define smoothStartDist 70000

#define stopWithWaitTime 0
#define wheelDiameter 70000
#define distanceBetweenWheels 140000
#define wheelCircumference 219911
#define slotsPerRotation 360
#define eps 10000


void Move_Smooth(int dist, int minSpeed, int maxSpeed);
void Turn_Smooth(int isRight);
void Move_Exactly(int isForward, int dist, int minSpeed, int maxSpeed);
void Start_Smooth_Dist(int isForward, int minSpeed, int maxSpeed, int dist);

void PWM_Init34(uint16_t period, uint16_t duty3, uint16_t duty4);
void Motor_Init();
void PWM_Duty3(uint16_t duty3);
void PWM_Duty4(uint16_t duty4);
void Move(uint16_t leftDuty, uint16_t rightDuty);
void Forward();
void Backward();
void Left();
void Right();
void Stop_With_Wait();
void Stop();
void Start_Smooth(int, int, int);

#endif /* MOVE_H_ */
