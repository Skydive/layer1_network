Q = @
CC = g++
CXXVERSION = -std=c++20

CFLAGS = $(CXXVERSION)

SRC_DIR = src
OBJ_DIR = obj
OUT_DIR = bin

FLAGS_DEP := $(shell pkg-config --cflags --libs cppzmq) -I include
FLAGS_EXTRA = -mtune=native -O2
FLAGS_DEBUG = -Wextra -g

CFLAGS := $(CFLAGS) $(FLAGS_DEP) $(FLAGS_EXTRA) $(FLAGS_DEBUG)


SRCS = decode.c demod.c
TARGETS = zmq_analyse zmq_transmit

OBJS = $(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(basename $(SRCS))))

all: $(TARGETS)

zmq_analyse: $(OBJ_DIR)/zmq_analyse.o $(OBJS)
	$(Q)echo $(basename $(test))
	$(Q)echo " LD $(basename $@)"
	$(Q)mkdir -p $(OUT_DIR)
	$(Q)$(CC) $(CFLAGS) -o $(OUT_DIR)/$(basename $@) $(OBJ_DIR)/$@.o $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(Q)echo " CC $<"
	$(Q)mkdir -p $(OBJ_DIR)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	$(Q)$(RM) -rf $(OUT_DIR)
	$(Q)$(RM) -rf $(OBJ_DIR)
	$(Q)echo " RM $(OBJ_DIR)"
	$(Q)echo " RM $(OUT_DIR)"
