#ifndef __GATEWAY_H
#define __GATEWAY_H

#include "main.h"
#include "can.h"
#include <stdio.h>

void StartDefaultTask(void *argument);

extern volatile uint16_t friend_pot_value; // 송신측에서 보낸 포텐셔미터 값을 저장하는 변수
extern volatile uint32_t rx_cnt;

#endif