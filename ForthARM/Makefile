#.PHONY:	all clean depends

CC = g++
GAS = as
CDEBUG = -g
DEFINES = -DRASPI -DLINUX
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -save-temps=obj -marm
#WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable -Wsign-compare -Wunused-but-set-variable
WARNING_FLAGS = -Wno-format -Wno-reorder -Wno-unused-variable
CPPFLAGS = $(CFLAGS) -I ../ForthLib

LDFLAGS = $(CDEBUG)
LD_LIBRARIES = -L ../ForthLib -lforth -lm -lc -lstdc++ -ldl -lpthread -lreadline
OBJDIR = obj

#DEPDIR = dep
#DEPS := $(patsubst %.cpp,$(DEPDIR)/%.d,$(SRCS))

$(OBJDIR)/ForthMainARM.o:	ForthMainARM.cpp
	echo "* " $*
	echo "< " $<
	echo "@ " $@
	$(CC) $(CPPFLAGS) -o $(OBJDIR)/ForthMainARM.o ForthMainARM.cpp

forth:	$(OBJDIR)/ForthMainARM.o ../ForthLib/libforth.a
	gcc $(OBJDIR)/ForthMainARM.o $(DEFINES) -o forth -I ../ForthLib $(LD_LIBRARIES)

logger: ForthLoggerARM.cpp
	gcc ForthLoggerARM.cpp -o logger

#-include $(DEPS)

#$(DEPDIR)/%.d: %.cpp
#	gcc -M $(CPPFLAGS) -o $(DEPDIR)/$*.d $<
	
clean:
	-rm -f libforth.a
	-rm -f $(OBJDIR)/*.o
#	-rm $(DEPDIR)/*.d

all:	libforth.a forth

