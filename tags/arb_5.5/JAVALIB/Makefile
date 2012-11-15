.SUFFIXES: .java .class .h
cc = cc -g -Kpic -I/ultra1/coders/langm/jdk1.1/include -I/ultra1/coders/langm/jdk1.1/include/solaris

OBJECTS = Prompt.o
HEADERS = Prompt.h
CLASSES = Prompt.class 

help:
	@echo 'Options'

all:	source header lib

lib:	$(OBJECTS)
	ld -G -o libMyImpOfPrompt.so $(OBJECTS)

source:	$(CLASSES)

header: $(HEADERS)

clean:
	rm -f $(OBJECTS) $(HEADERS) $(CLASSES)

save:
	
.c.o:
	$(cc) -c $<

.class.h:
	javah -jni $(@F:.h=)
	

.java.class:
	javac $< 



