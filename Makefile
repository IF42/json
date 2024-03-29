CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Ofast
LIBS=


ifeq ($(OS),Windows_NT)
	LIBS += -l:libvector.so 
else
	LIBS += -lvector
endif


INCLUDE_PATH=/usr/include/
LIB_PATH=/usr/lib64/

TARGET=libjson.a
CACHE=.cache
OUTPUT=$(CACHE)/release


MODULES += json.o

TEST += test.o


OBJ=$(addprefix $(CACHE)/,$(MODULES))
T_OBJ=$(addprefix $(CACHE)/,$(TEST))


$(CACHE)/%.o:
	$(CC) $(CFLAGS) -c $< -o $@


all: env $(OBJ)
	ar -crs $(OUTPUT)/$(TARGET) $(OBJ)


-include dep.list


.PHONY: env dep clean install


dep:
	$(CC) -MM src/*.c test/*.c | sed 's|[a-zA-Z0-9_-]*\.o|$(CACHE)/&|' > dep.list


exec: env $(T_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(T_OBJ) $(OBJ) $(LIBS) -o $(OUTPUT)/test
	$(OUTPUT)/test


install:
	cp -v $(OUTPUT)/$(TARGET) $(LIB_PATH)/$(TARGET)
	cp -v src/json.h $(INCLUDE_PATH)/json.h


env:
	mkdir -pv $(CACHE)
	mkdir -pv $(OUTPUT)


clean: 
	rm -rvf $(CACHE)



