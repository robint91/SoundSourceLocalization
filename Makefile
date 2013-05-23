TARGET=ssl.bin
EXECUTABLE=ssl.elf

CC=arm-none-eabi-gcc
#LD=arm-none-eabi-ld 
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump

BIN=$(CP) -O binary

DEFS = -DUSE_STDPERIPH_DRIVER -DSTM32F4XX  -DHSE_VALUE=8000000 -D__FPU_USED -DSTM32F407VG -DARM_MATH_CM4

MCU = cortex-m4
MCFLAGS = -mcpu=$(MCU) -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard 

STM32_INCLUDES = -Ilib/cmsis \
				 -Ilib/cmsis_boot \
				 -Ilib/STM32F4xx_StdPeriph_Driver/inc 

OPTIMIZE       = -O1

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE) -fno-non-call-exceptions -Wall -ffunction-sections  -Wl,--gc-sections -Wl,-cref "-Wl,-Map=memory.map"  $(DEFS) -I./ -I./ $(STM32_INCLUDES)  -Wl,-T,stm32_flash.ld
AFLAGS	= $(MCFLAGS) 

SRC = 	src/main.c \
		src/adcdma/*.c \
		src/stm32f4xx_it.c \
		lib/newlib_stubs/stubs.c \
		lib/cmsis_boot/system_stm32f4xx.c \
		lib/STM32F4xx_StdPeriph_Driver/src/*.c \
		lib/cmsis_boot/startup/startup_stm32f4xx.c 
	 
OBJDIR = build
OBJ = $(SRC:%.c=$(OBJDIR)/%.o) 



all: $(TARGET) load

$(TARGET): $(EXECUTABLE)
	$(CP) -O binary $^ $@

$(EXECUTABLE): $(SRC) 
	$(CC) $(CFLAGS) $^ -lm -lg -lc -lnosys -Llib/cmsis -larm_cortexM4lf_math -o $@


	
clean:
	rm -f Startup.lst  $(TARGET)  $(EXECUTABLE) *.lst $(OBJ) $(AUTOGEN)  *.out *.map \
	 *.dmp
	 
	 
load:
	'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' -P $(TARGET) 0x08000000 -Run