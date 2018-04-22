========================================================================
       CONSOLE APPLICATION : AppClose
========================================================================

AppClose direct usage examples:
  AppClose.exe 1234
  AppClose.exe c:\path\app.pid

where 
  1234 - process pid
  c:\path\app.pid - contains 1234

usage sample with the Apache:
folder structure:
/bin/httpd.exe
/bin/...
/bin/AppClose.exe (this repo  executable)
/conf/httpd.conf  (contains: PidFile   "httpd.pid")
/stop_console.apache.bat

stop_console.apache.bat:
%CD%\bin\AppClose.exe %CD%\httpd.pid 
