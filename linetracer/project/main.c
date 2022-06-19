#include "msp.h"
#include "..\inc\Clock.h"
#include "blinker.h"
#include "move.h"
#include "msp432p401r.h"

uint32_t tick;

enum RGBCOLOR
{
    RED = 0x01,
    GREEN = 0x02,
    BLUE = 0x04,
};

void Led_Init(void)
{
    // Setup Red LED
    P2->SEL0 &= ~0x01;
    P2->SEL1 &= ~0x01;
    P2->DIR |= 0x01;
    P2->OUT &= ~0x01;

    // Setup RGB LED
    P2->SEL0 &= ~0x07;
    P2->SEL1 &= ~0x07;
    P2->DIR |= 0x07;
    P2->OUT &= ~0x07;
}

void SysTick_Init(void)
{
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void SysTick_Wait(uint32_t delay)
{
    if (delay <= 1)
    {
        return;
    }
    SysTick->LOAD = (delay - 1);
    SysTick->VAL = 0;
    while ((SysTick->CTRL & 0x00010000) == 0)
    {
    };
}

void SysTick_Wait1us(int delay)
{
    SysTick_Wait(48 * delay);
}

void SysTick_Wait1ms(int delay)
{
    int i;
    for (i = 0; i < delay; ++i)
    {
        SysTick_Wait(48000);
    }
}

void SysTick_Wait1s()
{
    SysTick_Wait1ms(1000);
}


void TA2_0_IRQHandler()
{
    TIMER_A2->CCTL[0] &= ~0x0001;
    ++tick;
}

void TimerA2_Init(void)
{
    // 48Mhz / 6 / 4 = 2Mh == 2us
    TIMER_A2->CTL = 0x0280;
    TIMER_A2->CCTL[0] = 0x0010;
    TIMER_A2->CCR[0] = (500);
    TIMER_A2->EX0 = 0x0005;
    NVIC->IP[3] = (NVIC->IP[3]&0xFFFFFF00)|0x00000040;
    NVIC->ISER[0] = 0x00001000;
    TIMER_A2->CTL |= 0x0014; 
}

void InitBoard()
{
    Clock_Init48MHz();
    Tachometer_Init();
    Motor_Init();
    SysTick_Init();
    TimerA2_Init();
    Blinker_Init();
    // SysTick_Config(48000);
    Led_Init();
    // Setup Switch
    P1->SEL0 &= ~0x12;
    P1->SEL1 &= ~0x12;
    P1->DIR &= ~0x12;
    P1->REN |= 0x12;
    P1->OUT |= 0x12;

    // 0, 2, 4, 6
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;
    P5->DIR |= 0x08;
    P5->OUT &= ~0x08;
    // 1, 3, 5, 7
    P9->SEL0 &= ~0x04;
    P9->SEL1 &= ~0x04;
    P9->DIR |= 0x04;
    P9->OUT &= ~0x04;

    // 0~7 IR Sensor
    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;
    P7->DIR &= ~0xFF;
}

void TurnOff_Led()
{
    P2->OUT &= ~0x07;
}

void TurnOn_Led(int color)
{
    TurnOff_Led();
    P2->OUT |= color;
}

int Check_Switch(int num)
{
    static int lastLeft, lastRight;
    int left, right;
    switch (num)
    {
    case 1:
        left = !(P1->IN & 0x02);
        if (left != lastLeft)
        {
            lastLeft = left;
            return left;
        }
    case 2:
        right = !(P1->IN & 0x10);
        if (right != lastRight)
        {
            lastRight = right;
            return right;
        }
    default:
        return 0;
    }
}

int Check_Ir()
{
    P5->OUT |= 0x08;
    P9->OUT |= 0x04;

    P7->DIR = 0xFF;
    P7->OUT = 0xFF;
    SysTick_Wait1us(10);

    P7->DIR = 0x00;
    SysTick_Wait1us(1000);

    int sensor = P7->IN;

    P5->OUT &= ~0x08;
    P9->OUT &= ~0x04;

    return sensor;
}

int Ir_Is_On(int ir_result, int pos)
{
    return (ir_result & (1 << pos)) ? 1 : 0;
}

int Ir_Is_On_All(int ir_result, int pos_min, int pos_max)
{
    int i;
    int result = 1;
    for (i = pos_min; i <= pos_max; ++i)
    {
        result &= Ir_Is_On(ir_result, i);
    }
    return result;
}

int Ir_Is_On_Some(int ir_result, int pos_min, int pos_max)
{
    int i;
    int result = 0;
    for (i = pos_min; i <= pos_max; ++i)
    {
        result |= Ir_Is_On(ir_result, i);
    }
    return result;
}

int Debug_Ir(int ir_result, int *last_result)
{
    /*
    int i = 0;
    int result = 0;
    for (i = 0; i <= 7; ++i)
    {
        if (Ir_Is_On(ir_result, i))
        {
            result |= 1 << i;
        }
    }
    if (*last_result != result)
    {
        *last_result = result;
        for (i = 0; i <= 7; ++i)
        {
            printf("%d", (result & (1 << i)) ? 1 : 0);
        }
        printf("\n");
    }
    */
}

enum State
{
    BEFORE_START,
    GO,
    FINISH,
    ODOMETRY,
};

void Move_ZigZag(int isForward, int speed, int waitTick)
{
    int leftSpeed = speed;
    int rightSpeed = speed;

    if (isForward)
    {
        Forward();
    }
    else
    {
        Backward();
    }

    int firstTick = tick;
    while (1)
    {
        int irResult = Check_Ir();

        int blinker = 0;
        if (Ir_Is_On_All(irResult, 3, 4) || Ir_Is_On_All(irResult, 0, 1) || Ir_Is_On_All(irResult, 6, 7))
        {
            if (isForward)
            {
                Forward();
            }
            else
            {
                Backward();
            }
            leftSpeed = speed;
            rightSpeed = speed;
        }
        else
        {
            int isHardLeft = Ir_Is_On_Some(irResult, 0, 2);
            int isHardRight = Ir_Is_On_Some(irResult, 5, 7);

            if ((!isHardRight && !Ir_Is_On(irResult, 4)))
            {
                {
                    if (isForward)
                    {
                        Right();
                    }
                    else
                    {
                        Left();
                    }
                    leftSpeed = ZigZag_Speed(speed);
                    rightSpeed = ZigZag_Speed(speed);
                    blinker |= BK_RGHT;
                }
            }
            if ((!isHardLeft && !Ir_Is_On(irResult, 3)))
            {
                {
                    if (isForward)
                    {
                        Left();
                    }
                    else
                    {
                        Right();
                    }
                    leftSpeed = ZigZag_Speed(speed);
                    rightSpeed = ZigZag_Speed(speed);
                    blinker |= BK_LEFT;
                }
            }
        }
        Blinker_Output(blinker);

        Move(leftSpeed, rightSpeed);

        if (tick >= firstTick + waitTick)
        {
            break;
        }
        
        Wait();
    }
}

struct Map
{
    int leftTurnCount;
    int rightTurnCount;
    int isChecked;
    int x;
    int y;
    int id;
    struct Map* right;
    struct Map* left;
    struct Map* forward;
    int rightChecked;
    int leftChecked;
    int forwardChecked;
    struct Map* parent;
};

struct Map mapList[100];
struct Map* totalMap[50][50];
struct Map* targetMap;
int mapTop = -1;

enum Direction
{
    LEFT,
    RIGHT,
    DIR_FORWARD,
    BACKWARD
};

int robotX = 25, robotY = 25;
enum Direction robotDir = DIR_FORWARD;

enum Direction Get_Dir_Turn_Right()
{
    if (robotDir == LEFT)
    {
        return DIR_FORWARD;
    }
    else if (robotDir == DIR_FORWARD)
    {
        return RIGHT;
    }
    else if (robotDir == RIGHT)
    {
        return BACKWARD;
    }
    else if (robotDir == BACKWARD)
    {
        return LEFT;
    }
}

enum Direction Get_Dir_Turn_Left()
{
    if (robotDir == LEFT)
    {
        return BACKWARD;
    }
    else if (robotDir == DIR_FORWARD)
    {
        return LEFT;
    }
    else if (robotDir == RIGHT)
    {
        return DIR_FORWARD;
    }
    else if (robotDir == BACKWARD)
    {
        return RIGHT;
    }
}

// turnDIr. left: 0, right: 1, forward: 2
void Get_Map_Pos(int* x, int* y, int turnDir)
{
    int newDir;
    switch (turnDir)
    {
    case 0:
        newDir = Get_Dir_Turn_Left();
        break;
    case 1:
        newDir = Get_Dir_Turn_Right();
        break;
    case 2:
        newDir = robotDir;
        break;
    default:
        return;
    }

    if (newDir == LEFT)
    {
        *x = robotX - 1;
        *y = robotY;
    }
    else if (newDir == RIGHT)
    {
        *x = robotX + 1;
        *y = robotY;
    }
    else if (newDir == DIR_FORWARD)
    {
        *x = robotX;
        *y = robotY + 1;
    }
    else if (newDir == BACKWARD)
    {
        *x = robotX;
        *y = robotY - 1;
    }
}


struct Map* Init_Map(struct Map* map)
{
    map->leftTurnCount = 0;
    map->rightTurnCount = 0;
    map->isChecked = 0;
    map->x = 0;
    map->y = 0;
    map->right = 0;
    map->left = 0;
    map->forward = 0;
    map->parent = 0;
    map->id = 0;
    map->rightChecked = 0;
    map->leftChecked = 0;
    map->forwardChecked = 0;

    return map;
}

struct Map* Alloc_Map()
{
    struct Map* map = Init_Map(&mapList[++mapTop]);
    map->id = mapTop;
    return map;
}

void Wait()
{
    SysTick_Wait1ms(10);
}

void Countdown(int second)
{
    int i;
    for (i = 0; i < second; ++i)
    {
        TurnOn_Led(GREEN);
        SysTick_Wait1ms(200);
        TurnOff_Led();
        SysTick_Wait1ms(800);
    }
}


int Detect_Intersection(int irResult, int* leftExistReturn, int* rightExistReturn)
{
    int leftExist = Ir_Is_On_All(irResult, 4, 7);
    int rightExist = Ir_Is_On_All(irResult, 0, 3);
    int goodGoing = (Ir_Is_On(irResult, 3) && Ir_Is_On(irResult, 4)) || (Ir_Is_On(irResult, 2) && Ir_Is_On(irResult, 3)) || (Ir_Is_On(irResult, 4) && Ir_Is_On(irResult, 5));
    goodGoing = 1;
    if (!goodGoing)
    {
        return 0;
    }

    if (!leftExist && !rightExist)
    {
        return 0;
    }

    Forward();

    TurnOn_Led(BLUE);

    int detectStartTick = tick;
    Move(GLOBAL_SLOW_SPEED, GLOBAL_SLOW_SPEED);

    int inIntersection = Last_Straight_Length();
    while (1)
    {
        Wait();
        int irResult2 = Check_Ir();
        leftExist |= Ir_Is_On(irResult2, 6) && Ir_Is_On(irResult2, 7);
        rightExist |= Ir_Is_On(irResult2, 0) && Ir_Is_On(irResult2, 1);
        if (!Ir_Is_On(irResult2, 0) && !Ir_Is_On(irResult2, 1) && !Ir_Is_On(irResult2, 6) && !Ir_Is_On(irResult2, 7))
        {
            break;
        }
    }
    Stop();
    
    // detected time
    if (Last_Straight_Length() - inIntersection < DETECT_INTERSECTION_LENGTH)
    {
        return 0;
    }

    *leftExistReturn = leftExist;
    *rightExistReturn = rightExist;

    return 1;
}

int Detect_Intersection_Without_Slow(int irResult, int* leftExistReturn, int* rightExistReturn)
{
    int leftExist = Ir_Is_On_All(irResult, 4, 7);
    int rightExist = Ir_Is_On_All(irResult, 0, 3);
    int goodGoing = (Ir_Is_On(irResult, 3) && Ir_Is_On(irResult, 4)) || (Ir_Is_On(irResult, 2) && Ir_Is_On(irResult, 3)) || (Ir_Is_On(irResult, 4) && Ir_Is_On(irResult, 5));
    goodGoing = 1;
    if (!goodGoing)
    {
        return 0;
    }

    if (!leftExist && !rightExist)
    {
        return 0;
    }

    TurnOn_Led(BLUE);

    int inIntersection = Last_Straight_Length();
    while (1)
    {
        Wait();
        int irResult2 = Check_Ir();
        leftExist |= Ir_Is_On(irResult2, 6) && Ir_Is_On(irResult2, 7);
        rightExist |= Ir_Is_On(irResult2, 0) && Ir_Is_On(irResult2, 1);
        if (!Ir_Is_On(irResult2, 0) && !Ir_Is_On(irResult2, 1) && !Ir_Is_On(irResult2, 6) && !Ir_Is_On(irResult2, 7))
        {
            break;
        }
    }
    Stop();

    // detected time
    if (Last_Straight_Length() - inIntersection < DETECT_INTERSECTION_LENGTH)
    {
        return 0;
    }

    *leftExistReturn = leftExist;
    *rightExistReturn = rightExist;

    return 1;
}

#define THREETHREEMODE
#undef THREETHREEMODE

#ifdef THREETHREEMODE
// 3x3�쓣 �쐞�븿
void Detect_And_Turn_Intersection(int irResult, int speed)
{
    static int crossCount;
    int leftExist;
    int rightExist;

    if (!Detect_Intersection(irResult, &leftExist, &rightExist))
    {
        return;
    }

    int blinker = 0;

    if (leftExist)
    {
        blinker |= FR_LEFT;
    }
    if (rightExist)
    {
        blinker |= FR_RGHT;
    }

    Blinker_Output(blinker);

    Stop_With_Wait();

    Move_Exactly(1, 11000, SLOW_SPEED, speed);
    TurnOn_Led(BLUE | GREEN);

    Stop_With_Wait();

    switch (crossCount)
    {
        case 0:
            Turn_Smooth(0);
            break;
        case 1:
            break;
        case 2:
            Turn_Smooth(0);
            break;
    }
    
    Stop_With_Wait();

    Start_Smooth(1, 1000, speed);
    crossCount++;
}
#endif

// left: 0, right: 1, forward: 2
void Detect_And_Turn_Intersection_Force_Turn(int irResult, int speed, int dir)
{
    int blinker = 0;
    Blinker_Output(blinker);

    Stop_With_Wait();

    Move_Exactly(1, GO_BEFORE_TURN, SLOW_SPEED, speed);
    TurnOn_Led(BLUE | GREEN);

    Stop_With_Wait();

    switch (dir)
    {
        case 0:
            Turn_Fast(0);
            break;
        case 1:
            Turn_Fast(1);
            break;
        case 2:
            break;
    }

    Stop_With_Wait();

    Start_Smooth_Dist(1, SLOW_SPEED, speed, 50000);
}

#ifndef THREETHREEMODE
void Detect_And_Turn_Intersection(int irResult, int speed)
{
    int leftExist;
    int rightExist;

    if (!Detect_Intersection(irResult, &leftExist, &rightExist))
    {
        return;
    }

    int blinker = 0;

    if (leftExist)
    {
        blinker |= FR_LEFT;
    }
    if (rightExist)
    {
        blinker |= FR_RGHT;
    }

    Blinker_Output(blinker);

    Stop_With_Wait();

    Move_Exactly(1, GO_BEFORE_TURN, GLOBAL_SLOW_SPEED, GLOBAL_SLOW_SPEED);
    TurnOn_Led(BLUE | GREEN);

    Stop_With_Wait();

    irResult = Check_Ir();
    int forwardExist = Ir_Is_On(irResult, 3) || Ir_Is_On(irResult, 4);

    struct Map* map = targetMap;
    struct Map* totalCurrentMap = totalMap[robotX][robotY];
    if (totalCurrentMap && totalCurrentMap->isChecked)
    {
        if (map->parent->forward == map)
        {
            map->parent->forward = 0;
        }
        if (map->parent->left == map)
        {
            map->parent->left = 0;
        }
        if (map->parent->right == map)
        {
            map->parent->right = 0;
        }
        Find_Next_Load(speed);
        return;
    }
    map->isChecked = 1;

    printf("detect intersect (%d, %d)\n", robotX, robotY);

    int newMapX;
    int newMapY;
    if (leftExist)
    {
        Get_Map_Pos(&newMapX, &newMapY, 0);
        if (totalMap[newMapX][newMapY] == 0)
        {
            map->left = Alloc_Map();
            map->left->x = newMapX;
            map->left->y = newMapY;
            totalMap[newMapX][newMapY] = map->left;
            map->left->parent = map;
            printf("left Add %d. (%d, %d)\n", map->left->id, map->left->x, map->left->y);
        }
        else
        {
            map->left = totalMap[newMapX][newMapY];
            map->left->parent = map;
        }
    }
    if (rightExist)
    {
        Get_Map_Pos(&newMapX, &newMapY, 1);
        if (totalMap[newMapX][newMapY] == 0)
        {
            map->right = Alloc_Map();
            map->right->x = newMapX;
            map->right->y = newMapY;
            totalMap[newMapX][newMapY] = map->right;
            map->right->parent = map;
            printf("right Add %d. (%d, %d)\n", map->right->id, map->right->x, map->right->y);
        }
        else
        {
            map->right = totalMap[newMapX][newMapY];
            map->right->parent = map;
        }
    }
    if (forwardExist)
    {
        Get_Map_Pos(&newMapX, &newMapY, 2);
        if (totalMap[newMapX][newMapY] == 0)
        {
            map->forward = Alloc_Map();
            map->forward->x = newMapX;
            map->forward->y = newMapY;
            totalMap[newMapX][newMapY] = map->forward;
            map->forward->parent = map;
            printf("forward Add %d. (%d, %d)\n", map->forward->id, map->forward->x, map->forward->y);
        }
        else
        {
            map->forward = totalMap[newMapX][newMapY];
            map->forward->parent = map;
        }
    }

    if (map->forward)
    {
        targetMap = map->forward;
        map->forwardChecked = 1;
        Get_Map_Pos(&robotX, &robotY, 2);
    }
    else if (map->left)
    {
        Turn_Smooth(0);
        map->leftTurnCount = 1;
        map->leftChecked = 1;
        targetMap = map->left;
        Get_Map_Pos(&robotX, &robotY, 0);
        robotDir = Get_Dir_Turn_Left();
    } else  if (map->right)
    {
        Turn_Smooth(1);
        map->rightTurnCount = 1;
        map->rightChecked = 1;
        targetMap = map->right;
        Get_Map_Pos(&robotX, &robotY, 1);
        robotDir = Get_Dir_Turn_Right();
    }
    else

    {
        // Fault
        printf("map->id: %d\nmap->left: %d\nmap->right: %d\nmap->forward: %d\n", map->id, map->left ? map->left->id : 0, map->right ? map->right->id : 0, map->forward ? map->forward->id : 0);
        while (1)
        {
            int flag = 0;
            if (map->right)
            {
                flag |= (RED);
            }
            else if (totalMap[newMapX][newMapY] != 0)
            {
                flag |= (GREEN);
            }
            TurnOn_Led(flag);
            SysTick_Wait1ms(200);
            TurnOff_Led();
            SysTick_Wait1ms(200);
        }
    }

    printf("goto: (%d, %d). %p.\n", robotX, robotY, targetMap);

    Stop_With_Wait();
    Last_Turn();
    Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
}
#endif

void Detect_Wall(int irResult, int speed)
{
    int isWall = !Ir_Is_On_Some(irResult, 0, 7);

    if (!isWall)
    {
        return;
    }

    Move(GLOBAL_SLOW_SPEED, GLOBAL_SLOW_SPEED);

    int i;
    for (i = 0; i < 20; ++i)
    {
        Wait();
        irResult = Check_Ir();

        if (Ir_Is_On_Some(irResult, 0, 7))
        {
            return;
        }
    }

    Stop_With_Wait();
    Find_Next_Load(speed);
}

void Backward_To_Intersection(int speed)
{
    int leftExist;
    int rightExist;

    Turn_Smooth(1);
    Turn_Smooth(1);
    
    Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
    while (1)
    {
        int irResult = Check_Ir();

        if (Detect_Intersection(irResult, &leftExist, &rightExist))
        {
            Stop_With_Wait();
            
            Move_Exactly(1, 11000, GLOBAL_SLOW_SPEED, GLOBAL_SLOW_SPEED);
            Turn_Smooth(1);
            Turn_Smooth(1);
            return;
        }
        Move_ZigZag(1, speed, 0);
        
        Wait();
    }
}


void Find_Next_Load(int speed)
{
    struct Map* map = targetMap;
    Blinker_Output(0);
    
    while (1)
    {
        Backward_To_Intersection(speed);
        Stop_With_Wait();

        struct Map* currentMap = targetMap->parent;
        robotX = currentMap->x;
        robotY = currentMap->y;

        printf("Back finish. %d, (%d, %d) LTC: %d, RTC: %d\n", currentMap->id, currentMap->x, currentMap->y, currentMap->leftTurnCount, currentMap->rightTurnCount);

        if (currentMap->leftTurnCount)
        {
            Turn_Smooth(1);
            currentMap->leftTurnCount = 0;
            robotDir = Get_Dir_Turn_Right();
        }
        else if (currentMap->rightTurnCount)
        {
            Turn_Smooth(0);
            currentMap->rightTurnCount = 0;
            robotDir = Get_Dir_Turn_Left();
        }

        Stop_With_Wait();

        if (currentMap->forward && currentMap->forwardChecked == 0 && currentMap->forward->isChecked == 0)
        {
            currentMap->forwardChecked = 1;
            targetMap = currentMap->forward;

            Get_Map_Pos(&robotX, &robotY, 2);
            
            Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
            printf("FindNext Goto forward: (%d, %d). map: %d, (%d, %d)\n", robotX, robotY, targetMap->id, targetMap->x, targetMap->y);
            break;
        } else if (currentMap->left && currentMap->leftChecked == 0 && currentMap->left->isChecked == 0)
        {
            Turn_Smooth(0);
            Stop_With_Wait();
            currentMap->leftTurnCount = 1;
            currentMap->leftChecked = 1;
            targetMap = currentMap->left;

            Get_Map_Pos(&robotX, &robotY, 0);
            robotDir = Get_Dir_Turn_Left();

            Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
            printf("FindNext Goto left: (%d, %d). map: %d, (%d, %d)\n", robotX, robotY, targetMap->id, targetMap->x, targetMap->y);
            break;
        }
        else if (currentMap->right && currentMap->rightChecked == 0 && currentMap->right->isChecked == 0)
        {
            Turn_Smooth(1);
            Stop_With_Wait();
            currentMap->rightChecked = 1;
            currentMap->rightTurnCount = 1;
            targetMap = currentMap->right;

            Get_Map_Pos(&robotX, &robotY, 1);
            robotDir = Get_Dir_Turn_Right();

            Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
            printf("FindNext Goto right: (%d, %d). map: %d, (%d, %d)\n", robotX, robotY, targetMap->id, targetMap->x, targetMap->y);
            break;
        }
        else 
        {
            targetMap = currentMap;
            printf("FindNext Back: (%d, %d). map: %d, (%d, %d)\n", targetMap->parent->x, targetMap->parent->y, targetMap->parent->id, targetMap->parent->x, targetMap->parent->y);
        }

    }

    Last_Turn();
}

void Backtracking(struct Map* currentMap)
{
    if (!currentMap->parent)
    {
        return;
    }

    Backtracking(currentMap->parent);


}

void Optimal_Go(int speed)
{
    struct Map* stack[100];
    int stackSize = 0;
    struct Map* currentMap = targetMap;
    int leftExist, rightExist;

    while (currentMap->parent)
    {
        stack[stackSize++] = currentMap;
        currentMap = currentMap->parent;
    }
    stack[stackSize++] = currentMap;

    int i;
    for (i = 0; i < stackSize; ++i)
    {
        printf("%d. (%d, %d)\n", stack[i]->id, stack[i]->x, stack[i]->y);
    }

    int lastFinishDetected;

    while (stackSize)
    {
        TurnOn_Led(RED);

        int ir_result = Check_Ir();

        if (stackSize == 1 && (
              (Ir_Is_On_All(ir_result, 2, 5)
              && !Ir_Is_On(ir_result, 0)
              && !Ir_Is_On(ir_result, 1)
              && !Ir_Is_On(ir_result, 6)
              && !Ir_Is_On(ir_result, 7)) ||
              (Ir_Is_On_All(ir_result, 3, 6)
              && !Ir_Is_On(ir_result, 0)
              && !Ir_Is_On(ir_result, 1)
              && !Ir_Is_On(ir_result, 2)
              && !Ir_Is_On(ir_result, 7)) ||
              (Ir_Is_On_All(ir_result, 1, 4)
              && !Ir_Is_On(ir_result, 0)
              && !Ir_Is_On(ir_result, 5)
              && !Ir_Is_On(ir_result, 6)
              && !Ir_Is_On(ir_result, 7))
        )
        )
        {
            return;
        }
        else
        {
            lastFinishDetected = 0;
            if ((stackSize - 1) && Detect_Intersection(ir_result, &leftExist, &rightExist))
            {
                int turnDir = 0;
                struct Map* nextMap = stack[stackSize - 2];
                if (currentMap->left == nextMap)
                {
                    turnDir = 0;
                }
                else if (currentMap->right == nextMap)
                {
                    turnDir = 1;
                }
                else if (currentMap->forward == nextMap)
                {
                    turnDir = 2;
                }
                else
                {
                    while (1)
                    {
                        TurnOn_Led(RED | GREEN);
                        SysTick_Wait1ms(200);
                        TurnOff_Led();
                        SysTick_Wait1ms(200);
                    }
                }

                Detect_And_Turn_Intersection_Force_Turn(ir_result, speed, turnDir);
                --stackSize;
                currentMap = stack[stackSize - 1];
            }

//            Detect_Wall(ir_result, speed);

            Move_ZigZag(1, speed, 0);
        }

        Wait();
    }



    Stop();
    return;
}

int lastStraightLeftSteps;
int lastStraightRightSteps;

void Last_Turn()
{
    int32_t leftSteps, rightSteps;
    enum TachDirection leftDir, rightDir;
    uint16_t leftTach, rightTach;
    Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

    lastStraightLeftSteps = leftSteps;
    lastStraightRightSteps = rightSteps;
}

int Last_Straight_Length()
{
    int32_t leftSteps, rightSteps;
    enum TachDirection leftDir, rightDir;
    uint16_t leftTach, rightTach;
    Tachometer_Get(&leftTach, &leftDir, &leftSteps, &rightTach, &rightDir, &rightSteps);

    int32_t leftDiff = ABS(leftSteps - lastStraightLeftSteps);
    int32_t rightDiff = ABS(rightSteps - lastStraightRightSteps);

    int32_t ll = leftDiff * wheelCircumference / slotsPerRotation;
    int32_t rr = rightDiff * wheelCircumference / slotsPerRotation;

    return MIN(ll, rr);
}

int main(void)
{
    InitBoard();

    int speed = FAST_SPEED;
    int fastSpeed = SUPER_FAST_SPEED;
    int last_result = 0;
    enum State state = BEFORE_START;

    int tickViewToggle = 0;
    int tickLast = 0;

    targetMap = Alloc_Map();
    totalMap[25][25] = targetMap;
    targetMap->x = robotX;
    targetMap->y = robotY;
    int lastFinishDetected = 0;

    while (1)
    {
        // if (tickLast + 1000 <= tick)
        // {
        //     tickViewToggle ^= 1;
        //     if (tickViewToggle)
        //     {
        //         Blinker_Output(BK_LEFT);
        //     }
        //     else
        //     {
        //         Blinker_Output(0);
        //     }
        //     tickLast = tick;
        // }

        switch (state)
        {
        case BEFORE_START:
        {
            TurnOn_Led(RED | GREEN | BLUE);
            int ir_result = Check_Ir();
            Debug_Ir(ir_result, &last_result);

            if (Ir_Is_On_All(ir_result, 1, 6) &&
                !Ir_Is_On(ir_result, 0) &&
                !Ir_Is_On(ir_result, 7))
            {
                Countdown(3);
                // Move_ZigZag(1, speed, 1000);
                Last_Turn();
                Start_Smooth(1, GLOBAL_SLOW_SPEED, speed);
                state = GO;
            }

            if (Check_Switch(1))
            {
                state = ODOMETRY;
            }

            break;
        }
        case ODOMETRY:
        {
            Countdown(3);

            Move_Exactly(1, 150000, 1000, speed);
            Stop_With_Wait();

            Turn_Smooth(1);
            Stop_With_Wait();

            Move_Exactly(1, 150000, 1000, speed);
            Stop_With_Wait();

            Turn_Smooth(1);
            Stop_With_Wait();

            Turn_Smooth(1);
            Stop_With_Wait();

            Move_Exactly(1, 150000, 1000, speed);
            Stop_With_Wait();

            Turn_Smooth(0);
            Stop_With_Wait();

            Move_Exactly(1, 150000, 1000, speed);
            Stop_With_Wait();

            Turn_Smooth(0);
            Stop_With_Wait();

            Turn_Smooth(0);
            Stop_With_Wait();
            
            state = BEFORE_START;
            break;
        }
        case GO:
        {
            TurnOn_Led(RED);

            int ir_result = Check_Ir();
            Debug_Ir(ir_result, &last_result);

            if (
                  (Ir_Is_On_All(ir_result, 2, 5)
                  && !Ir_Is_On(ir_result, 0)
                  && !Ir_Is_On(ir_result, 1)
                  && !Ir_Is_On(ir_result, 6)
                  && !Ir_Is_On(ir_result, 7)) ||
                  (Ir_Is_On_All(ir_result, 3, 6)
                  && !Ir_Is_On(ir_result, 0)
                  && !Ir_Is_On(ir_result, 1)
                  && !Ir_Is_On(ir_result, 2)
                  && !Ir_Is_On(ir_result, 7)) ||
                  (Ir_Is_On_All(ir_result, 1, 4)
                  && !Ir_Is_On(ir_result, 0)
                  && !Ir_Is_On(ir_result, 5)
                  && !Ir_Is_On(ir_result, 6)
                  && !Ir_Is_On(ir_result, 7))
            )
            {
                if (lastFinishDetected == 0)
                {
                    Move(GLOBAL_SLOW_SPEED, GLOBAL_SLOW_SPEED);
                    lastFinishDetected = Last_Straight_Length();
                }
                else if (Last_Straight_Length() - lastFinishDetected >= FINISH_LINE_DETECTED_LENGTH)
                {
                    state = FINISH;
                }
            }
            else
            {
                lastFinishDetected = 0;
                Detect_And_Turn_Intersection(ir_result, speed);
                Detect_Wall(ir_result, speed);

                Move_ZigZag(1, speed, 0);

                if (Last_Straight_Length() >= 210000)
                {
                    TurnOn_Led(BLUE| RED|GREEN);
                    Get_Map_Pos(&robotX, &robotY, 2);
                    Last_Turn();
                    printf("(%d, %d)\n", robotX, robotY);
                }
            }

            Wait();

            break;
        }
        case FINISH:
        {
            Stop();

            while (1)
            {
                TurnOn_Led(BLUE);
                SysTick_Wait1ms(100);
                TurnOff_Led();
                SysTick_Wait1ms(100);
                int irResult = Check_Ir();

                if (Ir_Is_On_All(irResult, 1, 6) &&
                !Ir_Is_On(irResult, 0) &&
                !Ir_Is_On(irResult, 7))
                {
                    Countdown(3);
                    break;
                }
            }
            Stop();
            Optimal_Go(fastSpeed);

            break;
        }
        }
    }

    // WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;      // stop watchdog timer
}
