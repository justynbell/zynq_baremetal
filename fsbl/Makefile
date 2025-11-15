BSP_PATH = ../bsp
CC := arm-none-eabi-gcc
CC_FLAGS := -MMD -MP -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard 
CFLAGS := -Wall -Wextra
BSP_FLAGS := -O2 -c
LN_FLAGS :=  -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -Wl,-build-id=none -specs=Xilinx.spec  -lrsa -Wl,--start-group,-lxil,-lgcc,-lc,--end-group -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,--end-group -Wl,--start-group,-lrsa,-lxil,-lgcc,-lc,--end-group -Wl,--gc-sections

c_SOURCES := $(wildcard *.c)
S_SOURCES := $(wildcard *.S)
s_SOURCES := $(wildcard *.s)
INCLUDES := $(wildcard *.h)
OBJS := $(patsubst %.c, %.o, $(c_SOURCES))
OBJS += $(patsubst %.S, %.o, $(S_SOURCES))
OBJS += $(patsubst %.s, %.o, $(s_SOURCES))
LSCRIPT := -Tlscript.ld

DEPFILES := $(patsubst %.o, %.d, $(OBJS))
EXEC := fsbl.elf

INCLUDEPATH := -I$(BSP_PATH)/ps7_cortexa9_0/include -I.
LIBPATH := -L$(BSP_PATH)/ps7_cortexa9_0/lib -L./

all: $(EXEC)

$(EXEC): $(OBJS) $(INCLUDES)
	$(CC) -o $@ $(sort $(OBJS)) $(CC_FLAGS) $(CFLAGS) $(LN_FLAGS) $(LIBPATH) $(LSCRIPT)

%.o:%.c
	$(CC) $(CFLAGS) $(BSP_FLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

%.o:%.S
	$(CC) $(CFLAGS) $(BSP_FLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

%.o:%.s
	$(CC) $(CFLAGS) $(BSP_FLAGS) $(CC_FLAGS) -c $< -o $@ $(INCLUDEPATH)

clean:
	rm -rf $(OBJS) $(EXEC) *.o *.d

-include $(DEPFILES)
