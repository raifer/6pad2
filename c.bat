@echo off
for %%i in (*.cpp) do isnewer %%i obj\%%~ni.o && g++ -c -g -std=gnu++11 %%i -w -o obj\%%~ni.o
isnewer res.rc obj\rc.o && windres res.rc -o obj\rc.o
g++ -g -w obj\*.o -luser32 -lkernel32 -lcomdlg32 -lcomctl32 -lgdi32 -lwinmm -lboost-regex -L. -lpython34 -mthreads -m32