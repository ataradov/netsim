##############################################################################
.PHONY: all directory clean size

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS += -W -Wall --std=gnu99 -Os
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -funsigned-char -funsigned-bitfields
CFLAGS += -mcpu=cortex-m0plus -mthumb
CFLAGS += -MD -MP -MT build/$(*F).o -MF build/$(@F).d

ifeq (build, Debug)
  CFLAGS += -g
endif

LDFLAGS += -mcpu=cortex-m0plus -mthumb
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--script=simulator.ld
LDFLAGS += --specs=nano.specs

INCLUDES += \
  -I.

SRCS += \
  demo.c \
  phy.c \
  nwk.c \
  startup.c \

DEFINES += \
  -DPHY_SIMULATOR \
  -DF_CPU=1000000 \

CFLAGS += $(INCLUDES) $(DEFINES)

OBJS = $(addprefix build/, $(notdir %/$(subst .c,.o, $(SRCS))))

all: directory build/TimeSync.elf build/TimeSync.hex build/TimeSync.bin size

build/TimeSync.elf: $(OBJS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

build/TimeSync.hex: build/TimeSync.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O ihex -R .eeprom $^ $@

build/TimeSync.bin: build/TimeSync.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary -R .eeprom $^ $@

%.o:
	@echo CC $@
	@$(CC) $(CFLAGS) $(filter %$(subst .o,.c,$(notdir $@)), $(SRCS)) -c -o $@

directory:
	@gmkdir -p build

size: build/TimeSync.elf
	@echo size:
	@$(SIZE) -t $^

clean:
	@echo clean
	@-rm -rf build

-include $(wildcard build/*.d)

