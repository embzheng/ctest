GCC = gcc
LD 	= gcc

CFLAGS	+= -g	
CFLAGS  += -rdynamic 
CFLAGS  += -fexceptions 

LIBDIR = -L ./lib_thread
INCLUDES = -I ./lib_thread

CFLAGS 	+= $(INCLUDES)
LDFLAGS = $(INCLUDES) $(LIBDIR) 
LDFLAGS += -lpthread -lrt -lsyd_thread

TARGET 	= threadtest
SRCS	= $(wildcard *.c)
OBJS	= $(SRCS:.c=.o) 

all: $(TARGET)
# make all *.c to *.o
%.o: %.c
	$(GCC) -c -o $@ $(CFLAGS) $<

$(TARGET): $(OBJS) 
	$(GCC) -o $(TARGET) $^ $(LDFLAGS) 	

clean:
	rm -f *.o
	rm -f $(TARGET)
    

