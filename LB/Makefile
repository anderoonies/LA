CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS  := --std=c++11 -I./src -I../lib/PEGTL -I../lib -g3
LD_FLAGS  := 
CC        := g++

all: dirs LB

dirs: obj bin

obj:
	mkdir -p $@

bin:
	mkdir -p $@

LB: $(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o ./bin/$@ $^

obj/%.o: src/%.cpp
	$(CC) $(CC_FLAGS) -c -o $@ $<

oracle: LB
	../scripts/generateOutput.sh b LBc

rm_tests_without_oracle:
	../scripts/rm_tests_without_oracle.sh b

test: dirs LB
	./scripts/test.sh

clean:
	rm -fr bin obj *.out *.a *.o *.S core.* tests/liveness/*.tmp tests/*.tmp
