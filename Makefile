CROSS   = riscv64-unknown-elf-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
ARCH    = $(CROSS)ar

BUILD_DIR       = build
RTOS_SOURCE_DIR = $(abspath ./kernel)

CPPFLAGS = \
	-DportasmHANDLE_INTERRUPT=handle_trap \
	-I . -I ../Common/include \
	-I $(RTOS_SOURCE_DIR)/include \
	-I $(RTOS_SOURCE_DIR)/portable/GCC/RISC-V \
	-I $(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RISCV_no_extensions
CFLAGS  = -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany \
	-Wall \
	-fmessage-length=0 \
	-ffunction-sections \
	-fdata-sections \
	-fno-builtin-printf
LDFLAGS = -nostartfiles -Tvm-guest.ld \
	-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany \
	-Xlinker --gc-sections \
	-Xlinker --defsym=__stack_size=300 \
	-Xlinker -Map=$(BUILD_DIR)/xvisor-guest.map

ifeq ($(DEBUG), 1)
    CFLAGS += -Og -ggdb3
else
    CFLAGS += -O2
endif

SRCS = main.c goldfish_rtc.c isolation_bench.c riscv-virt.c ns16550.c \
	$(RTOS_SOURCE_DIR)/event_groups.c \
	$(RTOS_SOURCE_DIR)/list.c \
	$(RTOS_SOURCE_DIR)/queue.c \
	$(RTOS_SOURCE_DIR)/stream_buffer.c \
	$(RTOS_SOURCE_DIR)/tasks.c \
	$(RTOS_SOURCE_DIR)/timers.c \
	$(RTOS_SOURCE_DIR)/portable/MemMang/heap_4.c \
	$(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/port.c

ASMS = start.S vector.S\
	$(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/portASM.S

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o) $(ASMS:%.S=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d) $(ASMS:%.S=$(BUILD_DIR)/%.d)

all: $(BUILD_DIR)/xvisor-guest.elf $(BUILD_DIR)/xvisor-guest.dump $(BUILD_DIR)/xvisor-guest.bin

$(BUILD_DIR)/xvisor-guest.elf: $(OBJS) vm-guest.ld Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.dump: $(BUILD_DIR)/%.elf
	$(OBJDUMP) -d $< > $@	

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
