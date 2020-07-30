#
#
#

CXX=cl.exe
CXXFLAGS=-nologo -W4 -WX -Zi -O1 -DEBUG:FASTLINK -EHsc -std:c++latest -DUNICODE=1 -D_UNICODE=1 -MDd 
LINK=link.exe
LINKFLAGS=-nologo -DEBUG /MANIFEST
RC=rc.exe

threadexec_OBJS=main.obj threadexec.res 
clean_TAREGET=$(threadexec_OBJS) threadexec.exe threadexec.ilk threadexec.pdb vc???.pdb

threadexec.exe: $(threadexec_OBJS) manifest.xml
	$(LINK) $(LINKFLAGS) /MANIFEST:EMBED /MANIFESTINPUT:manifest.xml /OUT:$@ $(threadexec_OBJS)

main.obj: main.cpp Resource.h traceer.h
	$(CXX) -c $(CXXFLAGS) -Fo:$@ $*.cpp

threadexec.res: threadexec.rc
	$(RC) -nologo $*.rc

clean:
	@IF EXIST "*~" @FOR /F "delims=" %%i IN ( 'dir /b "*~" ' ) DO @DEL %%i
	@for %%I IN ( $(clean_TAREGET) ) do @IF EXIST "%%I"  @DEL %%I 
