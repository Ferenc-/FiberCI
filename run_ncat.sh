#!/usr/bin/env bash

VAR=$(
echo -ne "HTTP/1.1 200 OK\r\n"
echo -ne "Date: Mon, 23 May 2005 22:38:34 GMT\r\n"
echo -ne "Content-Type: text/html; charset=UTF-8\r\n"
echo -ne "Content-Encoding: UTF-8\r\n"
echo -ne "Content-Length: 138\r\n"
echo -ne "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\n"
echo -ne "Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\r\n"
echo -ne "ETag: \"3f80f-1b6-3e1cb03b\"\r\n"
echo -ne "Accept-Ranges: bytes\r\n"
echo -ne "Connection: close\r\n"
echo -ne "\r\n"
echo -ne "<html>\r\n"
echo -ne "<head>\r\n"
echo -ne "  <title>An Example Page</title>\r\n"
echo -ne "</head>\r\n"
echo -ne "<body>\r\n"
echo -ne "  Hello world, this very simple HTML document.\r\n"
echo -ne "</body>\r\n"
echo -ne "</html>\r\n"
)

ncat -lkp 9999 --sh-exec "echo -ne '$VAR'"
#ncat -lkp 9999 --sh-exec 'echo -ne "HTTP/1.0 200 OK\r\n\r\n Hello from $(uname -a)\n The date is $(date). PWD is"; echo "$PWD";'
