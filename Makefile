# ----------------------------------------------------------------
.PHONY: default test only
# ---------------------------------------------------------------------
include ./Vars.mk
# ---------------------------------------------------------------------
INCS = -I./inc $(UTILS_INC)
LIBS = $(UTILS_LIB) $(API_LIB)
# ---------------------------------------------------------------------
ALL_SRC = $(wildcard ./src/*.cpp)

ALL_OBJ = $(subst ./src/,./obj/,$(ALL_SRC:.cpp=.o))
LIB_OBJ = $(filter-out ./obj/main.o,$(ALL_OBJ))

AIR_SRC = src/main.cpp
AIR_OBJ = obj/main.o

ALL_DEP = $(ALL_OBJ:.o=.d)
# ---------------------------------------------------------------------
LIB_TARGET = lib/libaspenapi.a
AIR_TARGET = bin/a2ir
# ---------------------------------------------------------------------
default: $(LIB_TARGET) $(AIR_TARGET) test

lib:  $(LIB_TARGET)
only: $(LIB_TARGET)

obj/%.o: src/%.cpp
	$(CPP) $(CPPFLAGS) $(INCS) -c -o $@ $<

$(LIB_TARGET): $(LIB_OBJ) 
	$(AR) rcs $@ $(LIB_OBJ)

$(AIR_TARGET): $(AIR_OBJ) $(LIB_TARGET)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIBS)

test: $(AIR_TARGET)
	$(AIR_TARGET)

-include $(ALL_DEP)

clean: 
	-rm -f core bin/* obj/* lib/* ./test/my-alt0.json
