PLATFORM ?= linux_x86_64

CC=gcc
CFLAGS += -Wall 
CFLAGS += -Wextra 
CFLAGS += -pedantic 
CFLAGS += -O2
CFLAGS += -Isrc

LIBS += -lalloc


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


-include config/$(PLATFORM).mk
-include dep.list


.PHONY: env dep clean install


dep:
	$(FIND) src test -name "*.c" | xargs $(CC) $(CFLAGS) -MM | sed 's|[a-zA-Z0-9_-]*\.o|$(CACHE)/&|' > dep.list


exec: env $(T_OBJ) 
	$(CC) $(CFLAGS) $(T_OBJ) $(OBJ) $(LIBS) -o $(OUTPUT)/test
	$(OUTPUT)/test


install:
	rm -rvf $(INDIR)/json
	cp -v $(OUTPUT)/$(TARGET) $(LIBDIR)/$(TARGET)
	cp -vr src/json $(INDIR)


env:
	mkdir -pv $(CACHE)
	mkdir -pv $(OUTPUT)


clean: 
	rm -rvf $(CACHE)



