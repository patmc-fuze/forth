# Microsoft Developer Studio Project File - Name="ForthLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ForthLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ForthLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ForthLib.mak" CFG="ForthLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ForthLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ForthLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ForthLib - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "ForthLib - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FAs /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "ForthLib - Win32 Release"
# Name "ForthLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ForthClass.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthForgettable.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthInner.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthInput.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthOps.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthShell.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthThread.cpp
# End Source File
# Begin Source File

SOURCE=.\ForthVocabulary.cpp
# End Source File
# Begin Source File

SOURCE=.\InnerInterp.asm

!IF  "$(CFG)" == "ForthLib - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\InnerInterp.asm
InputName=InnerInterp

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff $(InputPath) 
	move $(InputName).obj $(OutDir) 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ForthLib - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\InnerInterp.asm
InputName=InnerInterp

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff $(InputPath) 
	move $(InputName).obj $(OutDir) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\core.inc
# End Source File
# Begin Source File

SOURCE=.\Forth.h
# End Source File
# Begin Source File

SOURCE=.\ForthClass.h
# End Source File
# Begin Source File

SOURCE=.\ForthCore.h
# End Source File
# Begin Source File

SOURCE=.\ForthEngine.h
# End Source File
# Begin Source File

SOURCE=.\ForthForgettable.h
# End Source File
# Begin Source File

SOURCE=.\ForthInner.h
# End Source File
# Begin Source File

SOURCE=.\ForthInput.h
# End Source File
# Begin Source File

SOURCE=.\ForthShell.h
# End Source File
# Begin Source File

SOURCE=.\ForthThread.h
# End Source File
# Begin Source File

SOURCE=.\ForthVocabulary.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
