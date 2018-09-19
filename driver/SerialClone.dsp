# Microsoft Developer Studio Project File - Name="SerialClone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SerialClone - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SerialClone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SerialClone.mak" CFG="SerialClone - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SerialClone - Win32 Checked" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SerialClone - Win32 Free" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SerialClone - Win32 Checked"

# PROP Use_MFC 0
# PROP Output_Dir ".\objchk\i386"
# PROP Intermediate_Dir ".\objchk\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gz /W3 /Zi /Od /Gy /I "$(TARGET_INC_PATH)" /I "$(CRT_INC_PATH)" /I "$(DDK_INC_PATH)" /I "$(WDM_INC_PATH)" /FI"warning.h" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D DBG=1 /D "DEPRECATE_DDK_FUNCTIONS" /D _WIN32_WINNT=$(_WIN32_WINNT) /D WINVER=$(WINVER) /D _WIN32_IE=$(_WIN32_IE) /D NTDDI_VERSION=$(NTDDI_VERSION) /FR /Fd".\objchk\i386\SerialClone.pdb" /Zel -cbstring /GF /c
# ADD BASE RSC /l 0x409 /i "$(CRT_INC_PATH)" /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(CRT_INC_PATH)" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 ntoskrnl.lib hal.lib ntstrsafe.lib /nologo /base:"0x10000" /version:5.0 /stack:0x40000,0x1000 /entry:"DriverEntry" /map:"SerialClone.map.map" /debug /machine:IX86 /nodefaultlib /out:".\objchk\i386\SerialClone.sys" /libpath:"$(TARGET_LIB_PATH)" /driver /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /OPT:REF /OPT:ICF /align:0x80 /osversion:5.00 /subsystem:native,1.10 /ignore:4010,4037,4039,4065,4070,4078,4087,4089,4099,4221,4210
# Begin Custom Build - ---------------------------Build SoftICE Symbols----------------------------
OutDir=.\objchk\i386
TargetName=SerialClone
InputPath=.\objchk\i386\SerialClone.sys
SOURCE="$(InputPath)"

"$(OutDir)\$(TargetName).nms" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(DRIVERWORKS)\bin\nmsym /trans:source,package,always $(OutDir)\$(TargetName).sys

# End Custom Build

!ELSEIF  "$(CFG)" == "SerialClone - Win32 Free"

# PROP Use_MFC 0
# PROP Output_Dir ".\objfre\i386"
# PROP Intermediate_Dir ".\objfre\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gz /W3 /Oy /Gy /I "$(TARGET_INC_PATH)" /I "$(CRT_INC_PATH)" /I "$(DDK_INC_PATH)" /I "$(WDM_INC_PATH)" /FI"warning.h" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _WIN32_WINNT=$(_WIN32_WINNT) /D WINVER=$(WINVER) /D _WIN32_IE=$(_WIN32_IE) /D NTDDI_VERSION=$(NTDDI_VERSION) /Zel -cbstring /GF /Oxs /c
# ADD BASE RSC /l 0x409 /i "$(CRT_INC_PATH)" /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(CRT_INC_PATH)" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 ntoskrnl.lib hal.lib ntstrsafe.lib /nologo /base:"0x10000" /version:5.0 /stack:0x40000,0x1000 /entry:"DriverEntry" /map /machine:IX86 /out:"objfre\i386\SerialClone.sys" /libpath:"$(TARGET_LIB_PATH)" /driver /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /OPT:REF /OPT:ICF /align:0x80 /osversion:5.00 /subsystem:native,1.10 /debug:MINIMAL /ignore:4010,4037,4039,4065,4070,4078,4087,4089,4099,4221,4210
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build - ---------------------------Build SoftICE Symbols----------------------------
OutDir=.\objfre\i386
TargetName=SerialClone
InputPath=.\objfre\i386\SerialClone.sys
SOURCE="$(InputPath)"

"$(OutDir)\$(TargetName).nms" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(DRIVERWORKS)\bin\nmsym /trans:source,package,always $(OutDir)\$(TargetName).sys

# End Custom Build

!ENDIF 

# Begin Target

# Name "SerialClone - Win32 Checked"
# Name "SerialClone - Win32 Free"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\clone.c
DEP_CPP_CLONE=\
	"..\..\..\WINDDK\2600~1.110\inc\crt\basetsd.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wdm.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wmilib.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\mce.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\ntstrsafe.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\bugcodes.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\evntrace.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\guiddef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ia64reg.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntdef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntiologc.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntstatus.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\wmistr.h"\
	".\pch.h"\
	".\SerialClone.h"\
	
NODEP_CPP_CLONE=\
	".\SerialClone.tmh"\
	
# End Source File
# Begin Source File

SOURCE=.\debug.c
DEP_CPP_DEBUG=\
	"..\..\..\WINDDK\2600~1.110\inc\crt\basetsd.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wdm.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wmilib.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\mce.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\ntstrsafe.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\bugcodes.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\evntrace.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\guiddef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ia64reg.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntdef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntiologc.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntstatus.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\wmistr.h"\
	".\pch.h"\
	".\SerialClone.h"\
	
NODEP_CPP_DEBUG=\
	".\debug.tmh"\
	
# End Source File
# Begin Source File

SOURCE=.\Filter.c
DEP_CPP_FILTE=\
	"..\..\..\WINDDK\2600~1.110\inc\crt\basetsd.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wdm.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wmilib.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\mce.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\ntstrsafe.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\bugcodes.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\evntrace.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\guiddef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ia64reg.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntdef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntiologc.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntstatus.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\wmistr.h"\
	".\pch.h"\
	".\SerialClone.h"\
	
NODEP_CPP_FILTE=\
	".\SerialClone.tmh"\
	
# End Source File
# Begin Source File

SOURCE=.\registry.c
DEP_CPP_REGIS=\
	"..\..\..\WINDDK\2600~1.110\inc\crt\basetsd.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wdm.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wmilib.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\mce.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\ntstrsafe.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\bugcodes.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\evntrace.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\guiddef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ia64reg.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntdef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntiologc.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntstatus.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\wmistr.h"\
	".\pch.h"\
	".\SerialClone.h"\
	
NODEP_CPP_REGIS=\
	".\Registry.tmh"\
	
# End Source File
# Begin Source File

SOURCE=.\SerialClone.c
DEP_CPP_SERIA=\
	"..\..\..\WINDDK\2600~1.110\inc\crt\basetsd.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wdm.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wdm\wxp\wmilib.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\mce.h"\
	"..\..\..\WINDDK\2600~1.110\inc\ddk\wxp\ntstrsafe.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\bugcodes.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\evntrace.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\guiddef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ia64reg.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntdef.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntiologc.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\ntstatus.h"\
	"..\..\..\WINDDK\2600~1.110\inc\wxp\wmistr.h"\
	".\pch.h"\
	".\SerialClone.h"\
	
NODEP_CPP_SERIA=\
	".\SerialClone.tmh"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\..\intrface.h
# End Source File
# Begin Source File

SOURCE=.\pch.h
# End Source File
# Begin Source File

SOURCE=.\SerialClone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rc;mc;mof"
# Begin Source File

SOURCE=.\SerialClone.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\SerialClone.inf.txt
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
