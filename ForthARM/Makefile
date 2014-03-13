#.PHONY:	all clean depends

CC = g++
GAS = as
CDEBUG = -g
DEFINES = -DRASPI -DLINUX -DASM_INNER_INTERPRETER
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -save-temps=obj
WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable
CPPFLAGS = $(CFLAGS)

GASFLAGS = --defsym ASM_INNER_INTERPRETER=1
#GASFLAGS =

LDFLAGS = $(CDEBUG)

SRCS = \
	ForthShell.cpp \
	ForthEngine.cpp \
	ForthInner.cpp \
	ForthInput.cpp \
	ForthVocabulary.cpp \
	ForthBuiltinClasses.cpp \
	ForthExtension.cpp \
	ForthStructs.cpp \
	ForthStructCodeGenerator.cpp \
	ForthOps.cpp \
	ForthServer.cpp \
	ForthClient.cpp \
	ForthPipe.cpp \
	ForthForgettable.cpp \
	ForthThread.cpp
	
ASMSRCS = InnerInterpARM.S

OBJDIR = obj
#ASMOBJS := $(patsubst %.s,$(OBJDIR)/%.o,$(ASMSRCS))
ASMOBJS := $(patsubst %.S,$(OBJDIR)/%.o,$(ASMSRCS))
OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS)) $(ASMOBJS)
DEPDIR = dep
DEPS := $(patsubst %.cpp,$(DEPDIR)/%.d,$(SRCS))

libforth.a:	$(OBJS)
	ar rcs libforth.a $(OBJS)
	
forth:	ForthMainARM.cpp libforth.a
	gcc ForthMainARM.cpp $(DEFINES) -o forth -L. -lforth -lm -lc -lstdc++ -ldl -lpthread

-include $(DEPS)

stuff:
	echo "DEPS=" $(DEPS)
	echo "OBJS=" $(OBJS)
	echo "SRCS=" $(SRCS)

#spoo: ForthOps.cpp
#	echo "at " $@ "   less" $< "   star" $* "   pat" $(patsubst %.cpp,%.d,$@)
	
$(DEPDIR)/%.d: %.cpp
	gcc -M $(CPPFLAGS) -o $(DEPDIR)/$*.d $<
	
#%.d: ../%.cpp
#	gcc -M $(CPPFLAGS) -o $*.d $<
	
#	echo "DEP at=" $@ "   less=" $< "   star=" $*

#%.o: ../%.cpp ../$(DEPDIR)/%.d
#%.o: ../%.cpp
#	$(CC) $(CFLAGS) -o $@ $<
#smb://demense/H%24/prj/Forth/Junk/Makefile
	
$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -o $(OBJDIR)/$*.o $<
    
$(OBJDIR)/%.o: %.S
	$(CC) $(CFLAGS) -o $(OBJDIR)/$*.o -c $<
    
#$(OBJDIR)/%.o: %.s
#	$(GAS) $(GASFLAGS) -o $(OBJDIR)/$*.o $<
    
all:	libforth.a forth

clean:
	-rm $(OBJDIR)/*.o
	-rm $(DEPDIR)/*.d
	-rm libforth.a
