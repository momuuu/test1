#!/usr/bin/python3
#coding:utf-8
import os, sys
import urllib

query_string = os.getenv("QUERY_STRING")
print("Content-type:text/html\n");
print("<html><body><h3>"+ query_string+"</h3></body></html>")
 