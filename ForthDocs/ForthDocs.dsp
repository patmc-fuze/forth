# Microsoft Developer Studio Project File - Name="ForthDocs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=ForthDocs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ForthDocs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ForthDocs.mak" CFG="ForthDocs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ForthDocs - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "ForthDocs - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "ForthDocs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ForthDocs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ForthDocs - Win32 Release"
# Name "ForthDocs - Win32 Debug"
# Begin Source File

SOURCE=..\ForthLib\core.h
# End Source File
# Begin Source File

SOURCE=.\ForthClasses.txt
# End Source File
# Begin Source File

SOURCE=.\ForthCode.txt
# End Source File
# Begin Source File

SOURCE=.\ForthDocs.vpj
# End Source File
# Begin Source File

SOURCE=..\ForthLib\ForthNotes.txt
# End Source File
# Begin Source File

SOURCE=..\ForthLib\ForthOpList.txt
# End Source File
# Begin Source File

SOURCE=..\ForthLib\Debug\ForthOps.asm
# End Source File
# Begin Source File

SOURCE=.\ForthStructs.txt
# End Source File
# Begin Source File

SOURCE=..\ForthLib\ForthTbd.txt
# End Source File
# Begin Source File

SOURCE=..\Sandbox\forthtest.txt
# End Source File
# Begin Source File

SOURCE=.\ForthVocabularies.txt
# End Source File
# End Target
# End Project
