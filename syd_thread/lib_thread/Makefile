CC = gcc
LD 	= gcc

LIB_SYD_THREAD_VER := 1.0.0
DLIB_FLAGS = -fPIC -shared
TAG := libsyd_thread.so
ELF := $(TAG).$(LIB_SYD_THREAD_VER)
SRC := $(shell find -name '*.c')
OBJ := $(SRC:%.c=%.o)


%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC $(BUILD_CFLAGS) $<

.PHONY: build  clean
all: clean build install

build: $(OBJ)
	$(CC) $(CFLAGS) $(BUILD_CFLAGS) $(DLIB_FLAGS) $^ -o $(ELF)

install:
	cp $(ELF) $(TAG)
	
clean:
	rm -rf  $(OBJ) $(ELF) $(TAG)
