
CXX=cl.exe
CXXFLAGS=-nologo -W4 -Zi -DEBUG:FASTLINK -EHsc -std:c++latest -DUNICODE=1 -D_UNICODE=1 -MDd
LINK=link.exe
LINKFLAGS=-nologo -DEBUG
RC=rc.exe

threadexec_OBJS=main.obj threadexec.res vc???.pdb
clean_TAREGET=$(threadexec_OBJS) threadexec.exe threadexec.ilk threadexec.pdb

threadexec.exe: main.obj threadexec.res
	$(LINK) $(LINKFLAGS) /OUT:$@ $**

main.obj: main.cpp Resource.h
	$(CXX) -c $(CXXFLAGS) -Fo:$@ $*.cpp

threadexec.res: threadexec.rc
	$(RC) -nologo $*.rc

clean:
	@IF EXIST "*~" @FOR /F "delims=" %%i IN ( 'dir /b "*~" ' ) DO @DEL %%i
	@for %%I IN ( $(clean_TAREGET) ) do @IF EXIST "%%I"  @DEL %%I 
