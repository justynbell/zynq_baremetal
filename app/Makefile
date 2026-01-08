BSP_PATH := ../bsp
SRC_DIR := src
BUILD_DIR := build

CC := arm-none-eabi-gcc
CFLAGS := -Wall -O0 -g3 -fmessage-length=0
CC_FLAGS := -MMD -MP -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
LN_FLAGS := --specs=Xilinx.spec -Wl,-build-id=none -Wl,--start-group -lxil -lgcc -lc -lm -Wl,--end-group

c_SOURCES := $(wildcard $(SRC_DIR)/*.c)
S_SOURCES := $(wildcard $(SRC_DIR)/*.S)
s_SOURCES := $(wildcard $(SRC_DIR)/*.s)
INCLUDES  := $(wildcard $(SRC_DIR)/*.h)

INCLUDEPATH := -I$(BSP_PATH)/ps7_cortexa9_0/include -I$(SRC_DIR)
LIBPATH := -L$(BSP_PATH)/ps7_cortexa9_0/lib

LSCRIPT := -Tlscript.ld

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(c_SOURCES))
OBJS += $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(S_SOURCES))
OBJS += $(patsubst $(SRC_DIR)/%.s, $(BUILD_DIR)/%.o, $(s_SOURCES))
DEPFILES := $(OBJS:.o=.d)

EXEC := app.elf

all: $(EXEC) size

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CC_FLAGS) $(CFLAGS) $(LN_FLAGS) $(LIBPATH) $(LSCRIPT)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

$(BUILD_DIR)/%.s: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(BSP_FLAGS) $(CC_FLAGS) -S $< -o $@ $(INCLUDEPATH)

size:
	@arm-none-eabi-size $(EXEC)

clean:
	$(RM) $(OBJS) $(DEPFILES) $(EXEC)

-include $(DEPFILES)