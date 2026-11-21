# ------------------------------------------------------------------
BLD_ENV=ubuntu
MK=make
BOOST_INC=-I/usr/include
BOOST_LIB=-L/usr/lib/x86_64-linux-gnu
# ------------------------------------------------------------------
AR=ar
BISON = bison -d -r solved -y -v
YACC  = $(BISON)
LEX   = flex
FLEX  = flex
# ------------------------------------------------------------------
# ------------------------------------------------------------------
GPERF = /usr/bin/gperf
GPERFOPT = -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash -t
# ------------------------------------------------------------------
VG     = valgrind
VGOPTS = --tool=memcheck --leak-check=yes --show-reachable=yes \
         --num-callers=20 --track-fds=ye
# ------------------------------------------------------------------
#DEFINES = -D_GLIBCXX_USE_CXX11_ABI=0
CC       = gcc
CPP      = g++
DEP      = -MMD -MP 
WARN     = -Wall -Wno-deprecated-declarations
CPPFLAGS = -g2 -O2 -std=c++17 -fPIE $(WARN) $(DEP) $(DEFINES)
LDFLAGS  =

UTILS_INC = -I./utils/inc
UTILS_LIB = -L./utils/lib -lutils
API_LIB   = -L./lib -laspenapi

