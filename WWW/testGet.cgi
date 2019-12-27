#!/usr/bin/python3
#coding:utf-8
import os, sys
import urllib

query_string = os.getenv("QUERY_STRING")
print("Content-type:text/html\n")
print("<html><body><ul>")
for data in query_string.split('&'):
    print('<li>'+data+'</li>')
print("</ul></body></html>")
 