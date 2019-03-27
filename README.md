# Simple Web Server in C++

*Currently supports only `html|gzip|zip|gif|jpg` files*


## Usage 
1. `gcc main.cpp <yourfilename>.out`
2. `./<yourfilename>.out <port> <server-dir>`
3. Put files with supported formats inside your created <server-dir> and you can request them in browser on localhost:<port>/<filename>.<supported-filetype>