========================================================================
       CONSOLE APPLICATION : AppClose
========================================================================

AppClose direct usage examples:
  AppClose.exe 1234 caption_string
  AppClose.exe c:\path\app.pid caption_string

where 
  1234 - process pid
  c:\path\app.pid - contains 1234
  caption_string - title string of the console window
*************************************************************************
Usage sample with the Apache.
Running Apache as a Console Application 
httpd.exe -k shutdown 
httpd.exe -k restart 
don't work and in this case you can use this small utility
to stop Apache process correctly.

folder structure:
/bin/httpd.exe
/bin/...
/bin/AppClose.exe (this repo  executable)
/conf/httpd.conf  (contains: PidFile   "httpd.pid")
/stop_console.apache.bat

stop_console.apache.bat:
%CD%\bin\AppClose.exe %CD%\httpd.pid  caption_string

