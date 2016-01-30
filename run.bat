rem run coordinator and pa
cd \MyWorks\school\Csci-NetworkSystem\progAssign_3

echo " run coordinator"
start coor\debug\coor.exe

cd pa_3\debug

rem pause Press to run AS0
rem start pa_3.exe 0 127.0.0.1 10275

pause Press to run AS1
start pa_3.exe 1 127.0.0.1 10275

pause Press to run AS2
start pa_3.exe 2 127.0.0.1 10275

pause Press to run AS4
start pa_3.exe 4 127.0.0.1 10275

pause Press to run AS5
start pa_3.exe 5 127.0.0.1 10275

echo done