Assignment 1
Suleyman Saib
sysaib@ucsc.edu
1674923

To run the server, first run 'make' then run './httpserver'

To do a GET, use:
curl http://localhost:80 --request-target filename

To do a PUT sending a local file to the server, use:
curl -T localfilename http://localhost:80 --request-target filename

To do a PUT sending some text into a file on the server
curl -X PUT -d sometexthere http://localhost:80 --request-target filename

LIMITATIONS:
- unable to set custom IP or port
- sometimes the errno is set and a false warning is generated
- when using the third command, the file is correctly created/edited but you are thrown into an infinite loop
- trying to write during a PUT will result in socket closing
- misusing the curl command will either result in a 400 response or throw the server into a loop/send another response not asked for in the assignment
- currently using int instead of uint_## 
