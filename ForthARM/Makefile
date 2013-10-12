#.PHONY:	all clean depends

CC = g++
GAS = as
CDEBUG = -g
#DEFINES = -DWIN32 -D_ASM_INNER_INTERPRETER
DEFINES = -DRASPI -DLINUX -DNO_ASM_INNER_INTERPRETER
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -save-temps=obj
WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable
CPPFLAGS = $(CFLAGS)

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
	
#ASMSRCS = InnerInterpDS.s
ASMSRCS = 

OBJDIR = obj
ASMOBJS := $(patsubst %.s,$(OBJDIR)/%.o,$(ASMSRCS))
OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS)) $(ASMOBJS)
DEPDIR = dep
DEPS := $(patsubst %.cpp,$(DEPDIR)/%.d,$(SRCS))

libforth.a:	$(OBJS)
	ar rcs libforth.a $(OBJS)
	
forth:	ForthMainARM.cpp
	gcc ForthMainARM.cpp -DRASPI -DLINUX -DNO_ASM_INTERPRETER -o forth -L. -lforth -lm -lc -lstdc++ -ldl -lpthread

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
smb://demense/H%24/prj/Forth/Junk/Makefile
	
$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -o $(OBJDIR)/$*.o $<
    
$(OBJDIR)/%.o: %.s
	$(GAS) -o $(OBJDIR)/$*.o $<
    
all:	ForthLib.a
	echo "Deps =" $(DEPS)

clean:
	-rm $(OBJDIR)/*.o
	-rm $(DEPDIR)/*.d
