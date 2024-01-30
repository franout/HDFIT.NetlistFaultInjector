CC = gcc
CXX = g++
RM = rm -f
MKDIR = mkdir 

DEFINES ?= 

INCLUDES = -I./src/common -I./src/rtl -I./src/gate

CPPFLAGS = -pedantic \
		-Wall \
		-W \
		-Wno-sign-compare \
		-Wno-unused-parameter \
		-Werror \
		$(INCLUDES) \
		$(DEFINES) \
		-march=native \
		-std=c++17

LDFLAGS= -fopenmp

EXE = netlistFaultInjector

SRCS := ./src/main.cpp ./src/rtl/RtlFile.cpp ./src/gate/GateLFile.cpp ./src/common/common.cpp ./src/gate/DontTouchSignals.cpp
OBJS := $(patsubst ./src/%.cpp, ./build/%.o, $(SRCS))

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX)  $(CPPFLAGS) $(LDFLAGS)  -o $(EXE) $(OBJS) 

depend: .depend

build/%.o: ./src/%.cpp
	@if [ ! -d "$(dir $@)" ]; then $(MKDIR) $(dir $@); fi
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -c $< -o $@

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS)  -MM $^ >> ./.depend;

clean:
	$(RM) -r  ./build
	$(RM) $(EXE) $(OBJS)

# Include the generated dependency files if they exist
-include .depend
