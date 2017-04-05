#.PHONY:	all clean depends

CC = g++
GAS = as
CDEBUG = -g
DEFINES = -DLINUX
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -save-temps=obj -std=c++11 -m32
#WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable -Wsign-compare -Wunused-but-set-variable
WARNING_FLAGS = -Wno-format -Wno-reorder -Wno-unused-variable
CPPFLAGS = $(CFLAGS) -I ../ForthLib

LDFLAGS = $(CDEBUG)
LD_LIBRARIES = -L ../ForthLib -lforth -lm -lc -lstdc++ -ldl -lpthread -lreadline
OBJDIR = obj

#DEPDIR = dep
#DEPS := $(patsubst %.cpp,$(DEPDIR)/%.d,$(SRCS))

$(OBJDIR)/ForthMain.o:	ForthMain.cpp
	echo "* " $*
	echo "< " $<
	echo "@ " $@
	$(CC) $(CPPFLAGS) -o $(OBJDIR)/ForthMain.o ForthMain.cpp

../ForthLib/libforth.a:
	make -C ../ForthLib -f Makefile.ubu86 libforth.a

forth:	$(OBJDIR)/ForthMain.o ../ForthLib/libforth.a
	gcc $(OBJDIR)/ForthMain.o $(DEFINES) -o forth -I ../ForthLib $(LD_LIBRARIES)
	cp forth ../Sandbox

logger: ForthLogger.cpp
	gcc ForthLogger.cpp -o logger

#-include $(DEPS)

#$(DEPDIR)/%.d: %.cpp
#	gcc -M $(CPPFLAGS) -o $(DEPDIR)/$*.d $<
	
clean:
	make -C ../ForthLib -f Makefile.ubu86 clean
	-rm -f $(OBJDIR)/*.o
	-rm ./forth
#	-rm $(DEPDIR)/*.d

all:	../ForthLib/libforth.a forth

