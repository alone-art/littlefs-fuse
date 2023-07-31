
PROC=main

LDFLAGS += -lpthread

SRC = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRC))

SRC += ../littlefs/lfs_util.c
SRC += ../littlefs/lfs.c
SRC += ../mbrblock.c
SRC += ../lfs_port.c

OBJS += ../littlefs/lfs_util.o
OBJS += ../littlefs/lfs.o
OBJS += ../mbrblock.o
OBJS += ../lfs_port.o

CFLAGS += -I../littlefs/ -I../

CFLAGS += $(foreach dir, $(ALL_SUB_DIRS), -I$(dir))

all : $(PROC)
	@echo "compile done"

$(PROC): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -fr $(PROC) $(OBJS)


