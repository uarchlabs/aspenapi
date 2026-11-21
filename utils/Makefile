.PHONY: run clean default lib
# ------------------------------------------------------------------
include ../Vars.mk
# ------------------------------------------------------------------
# ------------------------------------------------------------------
MSG_SRC = $(wildcard ./src/msg.cpp)
MSG_OBJ = $(subst ./src,./obj,$(MSG_SRC:.cpp=.o))
MSG_DEP = $(subst ./src,./obj,$(MSG_SRC:.cpp=.d))

#STR_SRC = $(wildcard ./src/str.cpp)
#STR_OBJ = $(subst ./src,./obj,$(STR_SRC:.cpp=.o))
#STR_DEP = $(subst ./src,./obj,$(STR_SRC:.cpp=.d))

LIB = ./lib/libutils.a

TST_SRC = $(wildcard ./src/main.cpp)
TST_OBJ = $(subst ./src,./obj,$(TST_SRC:.cpp=.o))
TST_DEP = $(subst ./src,./obj,$(TST_SRC:.cpp=.d))

TST_OBJS   = $(TST_OBJ) $(STR_OBJ) $(MSG_OBJ)
TST_TARGET = ./bin/tst
# ------------------------------------------------------------------
LIBS   = $(LIB)
INCS = -I./inc
# ------------------------------------------------------------------
default: $(LIB) $(TST_TARGET)
only: $(LIB)
# ------------------------------------------------------------------
./obj/%.o: ./src/%.cpp
	$(CPP) -c $(CPPFLAGS) $(INCS) -o $@ $<
# ------------------------------------------------------------------
$(LIB): 	$(MSG_OBJ) $(STR_OBJ)
	$(AR) rcs $@ $^

# ------------------------------------------------------------------
$(TST_TARGET):	$(TST_OBJS)
	$(CPP) $(LDFLAGS) -o $@ $^ $(LIBS)
# ------------------------------------------------------------------
run:	$(TST_TARGET)
	$(TST_TARGET)
# ------------------------------------------------------------------
-include $(MSG_DEP) $(STR_DEP) $(TST_DEP)
# ------------------------------------------------------------------
clean:
	-rm -f bin/* obj/* lib/*
