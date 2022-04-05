TARGET 		?= heist
SRC_DIRS 	?= src
BUILD_DIR	?= bin

CFLAGS		:= -ggdb -Wall -O0 -std=c11 -Wpedantic
LDFLAGS		:= -lpthread

SRCS 		:= $(shell find $(SRC_DIRS) -name *.c)
SRCS 		:= $(SRCS) $(shell find $(SRC_DIRS) -name *.cpp)
OBJS 		:= $(addsuffix .o,$(basename $(SRCS)))
DEPS 		:= $(OBJS:.o=.d)

INC_DIRS	:= $(shell find $(SRC_DIRS) -type d)
INC_FLAGS	:= $(addprefix -I,$(INC_DIRS))

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(BUILD_DIR)/$@ $(LOADLIBES) $(LDLIBS)

run:
	$(BUILD_DIR)/$(TARGET)

.PHONY: clean
clean:
	$(RM) -rf $(TARGET) $(OBJS) $(DEPS) $(BUILD_DIR)

all: $(TARGET)
-include $(DEPS)
