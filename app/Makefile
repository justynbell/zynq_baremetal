BSP_PATH := ../bsp
SRC_DIR := src
BUILD_DIR := build
RTOS_DIR := freertos

CC := arm-none-eabi-gcc
CFLAGS := -Wall -O0 -g3 -fmessage-length=0
CC_FLAGS := -MMD -MP -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
LN_FLAGS := --specs=Xilinx.spec -Wl,-build-id=none -Wl,--start-group -lxil -lgcc -lc -lm -Wl,--end-group

# RTOS Source Files #
RTOS_SOURCES := \
	$(RTOS_DIR)/tasks.c \
	$(RTOS_DIR)/queue.c \
	$(RTOS_DIR)/list.c \
	$(RTOS_DIR)/timers.c \
	$(RTOS_DIR)/event_groups.c \
	$(RTOS_DIR)/stream_buffer.c \
	$(RTOS_DIR)/portable/GCC/ARM_CA9/port.c \
	$(RTOS_DIR)/portable/MemMang/heap_4.c

RTOS_S_SOURCES := $(RTOS_DIR)/portable/GCC/ARM_CA9/portASM.S

# Application Source Files #
c_SOURCES := $(wildcard $(SRC_DIR)/*.c)
S_SOURCES := $(wildcard $(SRC_DIR)/*.S)

# Object Files #
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(c_SOURCES))
OBJS += $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(S_SOURCES))
OBJS += $(patsubst $(RTOS_DIR)/%.c, $(BUILD_DIR)/%.o, $(RTOS_SOURCES))
OBJS += $(patsubst $(RTOS_DIR)/%.S, $(BUILD_DIR)/%.o, $(RTOS_S_SOURCES))

# Includes
INCLUDEPATH := \
	-I$(BSP_PATH)/ps7_cortexa9_0/include -I$(SRC_DIR) \
	-I$(RTOS_DIR)/include \
	-I$(RTOS_DIR)/portable/GCC/ARM_CA9

# Libraries #
LIBPATH := -L$(BSP_PATH)/ps7_cortexa9_0/lib

# Linker Script #
LSCRIPT := -Tlscript.ld

DEPFILES := $(OBJS:.o=.d)

EXEC := app.elf

.PHONY: all clean size

all: $(EXEC) size

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CC_FLAGS) $(CFLAGS) $(LN_FLAGS) $(LIBPATH) $(LSCRIPT)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.o: $(RTOS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.o: $(RTOS_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

# Generate assembly output
#$(BUILD_DIR)/%.s: $(SRC_DIR)/%.c
#	@mkdir -p $(BUILD_DIR)
#	$(CC) $(CFLAGS) $(CC_FLAGS) -S $< -o $@ $(INCLUDEPATH)

size:
	@arm-none-eabi-size $(EXEC)

clean:
	$(RM) $(OBJS) $(DEPFILES) $(EXEC)

-include $(DEPFILES)