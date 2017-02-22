GCC = gcc
LD 	= gcc

CFLAGS	+= -g	
CFLAGS  += -rdynamic 
CFLAGS  += -fexceptions 


CFLAGS 	+= $(INCLUDES)
LDFLAGS = $(INCLUDES) $(LIBDIR) 
LDFLAGS += -lpthread

TARGET 	= wakeuponlan
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
    

