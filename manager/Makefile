QUIET=@
#
# Program name
#
PROGRAM=gpx-manager


#
# Compiler setup
#
#CC=gcc
CFLAGS+=-g -std=c99 
LIBS=


# 
# GTk Dependencies
#
GTK_CFLAGS=$(shell pkg-config --cflags gtk+-2.0 )
GTK_LIBS=$(shell pkg-config --libs gtk+-2.0 ) 

ifeq ($(GTK_CFLAGS),)
$(error Failed to find Gtk+-2.0) 
else
$(info Found Gtk+-2.0)
endif

CFLAGS+=$(GTK_CFLAGS)
LIBS+=$(GTK_LIBS)

CAIRO_CFLAGS=$(shell pkg-config --cflags cairo ) 
CAIRO_LIBS=$(shell pkg-config --libs cairo ) 

ifeq ($(CAIRO_CFLAGS),)
$(error Failed to find cairo) 
else
$(info Found cairo)
endif

CFLAGS+=$(CAIRO_CFLAGS)
LIBS+=$(CAIRO_LIBS)

LIBXML_CFLAGS=$(shell pkg-config --cflags libxml-2.0) 
LIBXML_LIBS=$(shell pkg-config --libs libxml-2.0) 

ifeq ($(LIBXML_CFLAGS),)
$(error Failed to find libxml-2.0) 
else
$(info Found libxml-2.0)
endif

CFLAGS+=$(LIBXML_CFLAGS)
LIBS+=$(LIBXML_LIBS)



##
# Check Vala
##

VALAC=$(shell which valac)
ifeq ($(VALAC),)
$(error Failed to find the vala compiler)
else
$(info Found valac: $(VALAC))
endif


VALA_SOURCES=\
    $(wildcard *.vala)

#
# C Sources to compile
#

C_SOURCES=\
	$(VALA_SOURCES:.vala=.c)

SOURCE_DIR=srctmp
C_SOURCES_DIR=\
	$(patsubst %,$(SOURCE_DIR)/%,$(C_SOURCES))

H_SOURCES=\
	$(wildcard *.h)
#
# Object directory
#
OBJ_DIR=\
	obj

#
# Object files
#
OBJ_DIR_FILES=\
	$(patsubst %,$(OBJ_DIR)/%,$(C_SOURCES:.c=.o))

#
# Extra build dependencies
#
EXTRA_SOURCES=\
	$(H_SOURCES)\
	Makefile

all: $(PROGRAM)

$(OBJ_DIR):
	$(info Create '$@' directory)
	$(QUIET)mkdir $@

$(SOURCE_DIR):
	$(info Create '$@' directory)
	$(QUIET)mkdir $@

$(SOURCE_DIR)/%.c: %.vala $(VALA_SOURCES)
	$(info Parsing '$^' int '$@')
	$(QUIET)$(VALAC) -C $^ --pkg=cairo --pkg=gio-2.0 --pkg=gtk+-2.0 --pkg=libxml-2.0 --vapidir=./vapi/ -b $(SOURCE_DIR)

$(PROGRAM): $(SOURCE_DIR) $(C_SOURCES_DIR) $(OBJ_DIR_FILES)
	$(info Linking '$^' into '$@')
	$(QUIET)$(CC) -o $@ $(OBJ_DIR_FILES) $(CFLAGS) $(LIBS)

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c  $(EXTRA_SOURCES) $(OBJ_DIR) 
	$(info Compiling '$@')
	$(QUIET)$(CC) -c -o $@  $< $(CFLAGS) 

.PHONY: view doc debug memcheck clean run all

clean:
	rm $(OBJ_DIR_FILES)
	rm $(C_SOURCES_DIR)
	rm $(PROGRAM)
	rm -r $(OBJ_DIR)
	rm -r $(SOURCE_DIR)

run: $(PROGRAM)
	./$(PROGRAM)


debug: $(PROGRAM)
	gdb ./$(PROGRAM)

memcheck: $(PROGRAM)
	valgrind --tool=memcheck --leak-check=full ./$(PROGRAM)
