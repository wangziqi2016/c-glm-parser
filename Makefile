#! /usr/bin/make

# Generic Makefile that should work with any program you're going to compile.
# Any complaints should be directed at honghual@sfu.ca
#
# To compile and link your program all you have to do is run 'make' in the
#    current directory.
# To clean up object files run 'make clean_object'.
# To delete any compiled files run 'make clean'.
# Originated in 2001 by Haris Teguh
# modified May-2012 by Honghua Li

# Including of non standard library files:
#   INCLUDEDIR is where the header files can be found
#   LIBDIR is where the library object files can be found
INCLUDEDIR=include/
LIBDIR=lib
GLUI_LIB=lib
# If you have more source files add them here 
SOURCE= data_pool.c logging.c weight_vector.c feature_generator.c

# The compiler we are using 
CC= g++

# The flags that will be used to compile the object file.
# If you want to debug your program,
# you can add '-g' on the following line
CFLAGS= -O3 -g -Wall -pedantic -std=gnu++11 -std=c++11

# The name of the final executable 
EXECUTABLE= c-glm-parser

# The basic library we are using add the other libraries you want to link
# to your program here 

# Linux (default)
LDFLAGS = -lXext -lX11 -lm

# If you have other library files in a different directory add them here 
INCLUDEFLAG= -I. -I$(INCLUDEDIR) -Iinclude/
LIBFLAG= -L$(LIBDIR) -L$(GLUI_LIB)

# Don't touch this one if you don't know what you're doing 
OBJECT= $(SOURCE:.c=.o)

# Don't touch any of these either if you don't know what you're doing 
all: $(OBJECT) depend
	$(CC) $(CFLAGS) $(INCLUDEFLAG) $(LIBFLAG) $(OBJECT) -o $(EXECUTABLE) $(LDFLAGS)

depend:
	$(CC) -M $(SOURCE) > depend

$(OBJECT):
	$(CC) $(CFLAGS) $(INCLUDEFLAG) -c -o $@ $(@:.o=.c)

clean_object:
	rm -f $(OBJECT)

clean:
	rm -f $(OBJECT) depend $(EXECUTABLE)

include depend