
RM=rm -f

VERSION:=$(shell date "+%F_%T")
COMILE_OPT=
MACROS=-D__BUILD_VER__="\"V1.0 $(VERSION)\""

ifdef rel
CC=g++ -s -O3 -W -Wall $(MACROS) $(COMILE_OPT)
libs=-ldl -lrt -pthread
else
CC=g++ -g -O0 -W -Wall  $(MACROS) $(COMILE_OPT)
libs=-ldl -lrt -pthread
endif

prog:=bin_prog
lib_name:=sockapi
lib_path:=libs
inc_path:=extern

inc_dir=-I. -I$(inc_path)
lib_dir=-Wl,-rpath=$(lib_path) -L$(lib_path) -l$(lib_name)

all_srcs=$(wildcard test/*.cpp) $(wildcard ./*.cpp) 
all_objs=$(all_srcs:.cpp=.o)

default:$(prog)
.PHONY:default

$(prog):$(all_objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(all_objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)

clean:
	-@$(RM) $(all_objs) $(prog)
.PHONY:clean

run:$(prog)
	export LD_LIBRARY_PATH=$(lib_path) && ./$(prog)
.PHONY:run


