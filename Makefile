LDFLAGS  := -std=c++11 -pedantic -Wall -O0 -no-pie -DLLAMA_DEBUG
SRC_DIRS := src src/module src/parser src/vm
SOURCES  := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
OUTPUT   := $(patsubst src/%.cpp, bin/%.o, $(SOURCES))
TARGET   := main
INCLUDES := -Isrc -Iinclude `pkg-config -cflags fmt`
LIBS     := -lm `pkg-config -libs fmt`

.PHONY: all link run clean

all: $(OUTPUT) link run

bin/%.o: src/%.cpp
	@mkdir -p $(@D)
	@echo '[ Building $<... ]'
	g++ $(LDFLAGS) -c $^ -o $@ $(INCLUDES)

link:
	@echo '[ Linking... ]'
	g++ $(LDFLAGS) $(OUTPUT) -o $(TARGET) $(LIBS)

run:
	@echo '[ Running... ]'
	./$(TARGET)

clean:
	@echo '[ Cleaning... ]'
	rm -fr bin/*.o bin/*.d $(TARGET)