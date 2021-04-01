#include <stdint.h>
#include <stdbool.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "system_TM4C1294.h" 



#define BUFFER_SIZE 8
osThreadId_t produtor_id, consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;
uint8_t buffer[BUFFER_SIZE];
void onButtonDown(void);
void onButtonUp(void);
void handler (void);

uint8_t count=0;

void onButtonDown(void) {
	if (GPIOIntStatus(GPIO_PORTJ_BASE, false) & GPIO_PIN_0) {
		// PF4 was interrupt cause
		GPIOIntRegister(GPIO_PORTJ_BASE, onButtonUp);	// Register our handler function for port F
		GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0,
			GPIO_RISING_EDGE);			// Configure PF4 for rising edge trigger
		GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);	// Clear interrupt flag
	}
}

void onButtonUp(void) {
	if (GPIOIntStatus(GPIO_PORTJ_BASE, false) & GPIO_PIN_0) {
		// PF4 was interrupt cause
		GPIOIntRegister(GPIO_PORTJ_BASE, onButtonDown);	// Register our handler function for port F
		GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0,
			GPIO_FALLING_EDGE);			// Configure PF4 for falling edge trigger
		GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);	// Clear interrupt flag
                count++;
	}
}

void produtor(void *arg){
  uint8_t index_i = 0;
  
  while(1){
    osSemaphoreAcquire(vazio_id, osWaitForever); // há espaço disponível?
    buffer[index_i] = count; // coloca no buffer
    osSemaphoreRelease(cheio_id); // sinaliza um espaço a menos
    
    index_i++; // incrementa índice de colocação no buffer
    if(index_i >= BUFFER_SIZE)
      index_i = 0;
    count &= 0x0F; // produz nova informação
    osDelay(500);
  } // while
} // produtor

void consumidor(void *arg){
  uint8_t index_o = 0, state;
  
  while(1){
    osSemaphoreAcquire(cheio_id, osWaitForever); // há dado disponível?
    state = buffer[index_o]; // retira do buffer
    osSemaphoreRelease(vazio_id); // sinaliza um espaço a mais
    
    index_o++;
    if(index_o >= BUFFER_SIZE) // incrementa índice de retirada do buffer
      index_o = 0;
    
    LEDWrite(LED4 | LED3 | LED2 | LED1, state); // apresenta informação consumida
    osDelay(500);
  } // while
} // consumidor

void main(void){


  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOIntDisable(GPIO_PORTJ_BASE, GPIO_PIN_0);		
  GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);		
  GPIOIntRegister(GPIO_PORTJ_BASE, onButtonDown);		
  GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0,GPIO_FALLING_EDGE);				
  GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_0);		
  SystemInit();
  LEDInit(LED4 | LED3 | LED2 | LED1);

  osKernelInitialize();

  produtor_id = osThreadNew(produtor, NULL, NULL);
  consumidor_id = osThreadNew(consumidor, NULL, NULL);

  vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL); // espaços disponíveis = BUFFER_SIZE
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
