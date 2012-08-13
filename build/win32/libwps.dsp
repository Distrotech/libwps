# Microsoft Developer Studio Project File - Name="libwps" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libwps - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libwps.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libwps.mak" CFG="libwps - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libwps - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libwps - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libwps - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /I "libwpd-0.9" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD " /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "libwpd-0.9" /D "NDEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\lib\libwps-0.1.lib"

!ELSEIF  "$(CFG)" == "libwps - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "libwpd-0.9" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GR /GX /ZI /Od /I "libwpd-0.9" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\lib\libwps-0.1.lib"

!ENDIF 

# Begin Target

# Name "libwps - Win32 Release"
# Name "libwps - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\lib\libwps_internal.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libwps_tools_win.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4Graph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4Text.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Graph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Struct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Table.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Text.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8TextStyle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSCell.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSContentListener.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSDebug.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSOLEParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSOLEStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSPageSpan.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSParagraph.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSSubDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSTable.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSTextParser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\lib\libwps.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libwps_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libwps_tools_win.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4Graph.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS4Text.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Graph.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Struct.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Table.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8Text.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPS8TextStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSCell.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSContentListener.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSDocument.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSEntry.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSOLEParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSOLEStream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSPageSpan.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSParagraph.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSPosition.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSSubDocument.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSTable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\WPSTextParser.h
# End Source File
# End Group
# End Target
# End Project
