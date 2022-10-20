CC = arm-himix410-linux-gcc
TARGET = ./bin/$(notdir $(CURDIR))
LIBS = -lpthread

INCDIR := . ../utilities/fifo ../utilities/hup ../utilities/hip ../utilities/linkedlist ../utilities/uart_dev ./client_mod ./client_app
Include := $(foreach n, $(INCDIR), -I$(n))
CFLAGS := $(Include)

SRC = $(foreach n, $(INCDIR), $(wildcard $(n)/*.c))

OBJS = $(patsubst %.c, %.o, $(SRC))

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

%.o : %.c
	@$(CC) -c $< -o $@ $(Include)

clean:
	$(RM) $(TARGET) $(OBJS)
.PHONY: clean