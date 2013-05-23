#include "main.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <arm_math.h>
#include "stm32f4xx.h"

#include "adcdma/adcdma.h"


volatile	float xR[256];
volatile	float xL[256];
volatile	float xCR[512];
volatile	float xCL[512];
volatile	float xC[512];
volatile	float mag[512];
volatile uint16_t bufferR[512];
volatile uint16_t bufferL[512];

volatile uint8_t DmaStateRight = 0; 
volatile uint8_t DmaStateLeft = 0;


extern unsigned int cpuusage ;
int main(void)
{

	static float lastIndexMultInc = 1.05f;
	static float lastIndexMult = 1.0f;
	static uint32_t lastIndex = 0;
	static uint32_t unVoicedCnt = 0;
	arm_cfft_radix4_instance_f32 Audiofft; 
	arm_cfft_radix4_instance_f32 Audioifft; 
	
	//Init the PLL and FLASH prefecht buffers
	SystemInit();	

	//Init FFT LIB

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	// enable peripheral clock for USART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
	USART_Cmd(USART3, ENABLE); // enable USART2
	printf("RUNNING APP\r\n");
	

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12; // we want to configure all LED GPIO pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 		// we want the pins to be an output
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 	// this sets the GPIO modules clock speed
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 	// this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 	// this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOD, &GPIO_InitStructure); 	
	
	adcdma_Init(bufferR, bufferL, 512);

  
  
  
  
	arm_cfft_radix4_init_f32(&Audiofft, 256, 0, 1); 
	arm_cfft_radix4_init_f32(&Audioifft, 256, 1, 1); 
	
	
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	//GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
	while (1)
	{
		short update = 0;
		if(DmaStateRight == DMA_FIRST && DmaStateLeft == DMA_FIRST){
			DmaStateRight = 0;
			DmaStateLeft = 0;
			
			update = 1;
			short i = 0;
			for(i = 0; i<256;i++){
				//real
				xR[i] = (float)bufferR[i];
				xL[i] = (float)bufferL[i];
				

			}
		}
		
		if(DmaStateRight == DMA_LAST && DmaStateLeft == DMA_LAST){
			DmaStateRight = 0;
			DmaStateLeft = 0;
			update = 1;
			short i = 0;
			for(i = 0; i<256;i++){
				//real
				xR[i] = (float)bufferR[256+i];
				xL[i] = (float)bufferL[256+i];

			}
		}
		
		
		if(update == 1){
			update = 0;
			
					
			GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
			
			volatile float t = 1.0f;
			t = sqrtf(t);
			
			
			float rmsR = 0.0f;
			float rmsL = 0.0f;
			float mean = 0.0f;
			arm_mean_f32(xL, 256, &mean);
			arm_offset_f32(xL, -mean, xL, 256);
			
			arm_mean_f32(xR, 256, &mean);
			arm_offset_f32(xR, -mean, xR, 256);
			
			
			arm_rms_f32(xR, 256, &rmsR);
			arm_rms_f32(xL, 256, &rmsL);
				
			arm_scale_f32(xR, 1.0f/500.0f, xR, 256);	
			arm_scale_f32(xL, 1.0f/500.0f, xL, 256);
			
			//printf("%i\t%i\r\n" , (long)rmsR, (long)rmsL);
			
			//arm_correlate_f32(xL, 256, xR, 256, mag);
			uint32_t n = 0;
			for(n = 0; n <256; n++){
				xCR[2*n] = xR[n];
				xCR[2*n +1 ] = 0.0f;
				
				xCL[2*n] = xL[n];
				xCL[2*n +1 ] = 0.0f;
			}
			
		
	 		arm_cfft_radix4_f32(&Audiofft, xCR); 	
			arm_cfft_radix4_f32(&Audiofft, xCL);
			
			arm_cmplx_conj_f32(xCL, xCL, 256);
			arm_cmplx_mult_cmplx_f32(xCL, xCR, xC, 256);
			
			//phat
			arm_cmplx_mag_f32(xC, mag, 256);
			for(n = 0; n <256; n++){
				mag[n] = 1.0f/(mag[n] + 0.000001f);
			}			
			arm_cmplx_mult_real_f32(xC, mag, xC,256);
			
			//phat done
			
			arm_cfft_radix4_f32(&Audioifft, xC);
			
			arm_cmplx_mag_f32(xC, mag, 256);
			

			if(rmsL > 50){
	
				float maxCorr = 0.0f;
				uint32_t index = 1;
				uint32_t Rindex = 1;
				//arm_max_f32(mag, 256, &maxCorr, &index);

				
				
				for(n = 1; n <255; n++){
					if(mag[n] > maxCorr){
						maxCorr = mag[n];
						Rindex = n;
					}
				}
				
				float magRaw = mag[lastIndex];
				mag[lastIndex] = mag[lastIndex] * lastIndexMult;
				
				maxCorr = 0.0f;
				for(n = 1; n <255; n++){
					if(mag[n] > maxCorr){
						maxCorr = mag[n];
						index = n;
					}
				}
				
				if(lastIndex != Rindex){
					if(lastIndexMult > 1.0f){
						lastIndexMult /= lastIndexMultInc*lastIndexMultInc;
					}
				}
	
				if(lastIndex == index){
					if(lastIndexMult < 5000.0f){
						lastIndexMult *= lastIndexMultInc;
					}
				}else{
					lastIndex = index;
					lastIndexMult = 1.0f;
				}
			
	
			
				unVoicedCnt = 0;
				
				//2nd order approx
				float deltaT = ((mag[index-1])-(mag[index+1]))/((2*mag[index-1])+(2*magRaw)-(4*mag[index+1]));
				
				
				printf("%i,%i\r\n", (long)index, (long)(index*100.0f + deltaT*100.0f));
			}
			else
			{
				printf("%i,%i\r\n", 0,0);
				if(unVoicedCnt > 25)
				{
					lastIndex = 0;
					lastIndexMult = 1.0f;
				}
				else
				{
					unVoicedCnt++;
				}
			}
		/*	
			static short cnt = 0;
			if(cnt > 50){
				for(n = 0; n<15; n++){
					printf("%i,%i\r\n" , 100, 0);
				}
				for(n = 0; n<256; n++){
					printf("%i,%i\r\n" , 0, (long)(mag[n]));
					volatile short i = 10000;
					while(i--);
					
				}
				
				for(n = 0; n<15; n++){
					printf("%i,%i\r\n" , 100, 0);
				}
				cnt = 0;
			}else{
					cnt++;
			}
			*/
		}
		
	}
}
