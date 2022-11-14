# -------- Variables --------
TARGET=bin/main

## ADD HERE OBJECT FILES: e.g. bin/class.o
OBJECTS= $(TARGET).o bin/threads.o bin/cmd.o bin/func.o bin/interrupt.o

# -------- Compiler --------
CC=gcc

# -------- Compiling options --------

# -------- Compiling --------
bin/%.o : src/%.c
	$(CC) -c $< -o $@

# -------- Linking --------
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lpthread -lm

# -------- Running --------
run:
	./$(TARGET)

# -------- Cleaning --------
clean:
	rm bin/*.o $(TARGET)


###########################
# Documentation/Reminders #
###########################
# file name: either makefile or Makefile
# (not Make, nor Makefile.txt, nor Makefile.mk nor Makefile.mak etc...)
#
#
#Format of a rule:
#
#target: dependency(ies)
#[TAB]	command(s)
#
#(the second line has to start with a TABULATION)
#
#Warning about TAB:
#when copying/pasting from a PDF file e.g, re-check ALL the tabulations
#
#
#User-defined variables:
#definition: VAR_NAME=value
#use: $(VAR_NAME)
#
#
#Built-in variables:
#'$@' the target name
#'$<' the first dependency
#'$^' list of all the dependencies (including $<)
