#!/usr/bin/python3
#coding:utf-8
import sys,os
import urllib
length = os.getenv('CONTENT_LENGTH')

if length:
    postdata = sys.stdin.read(int(length))
    print ("Content-type:text/html\n")
    print ('<html>') 
    print ('<head>') 
    print ('<title>POST</title>') 
    print ('<meta charset="utf-8">')
    print ('</head>') 
    print ('<body>') 
    print ('<h2> got post data </h2>')
    print ('<ul>')
    for data in postdata.split('&'):
    	print  ('<li>'+data+'</li>')
    print ('</ul>')
    print ('</body>')
    print ('</html>')
    
else:
    print ("Content-type:text/html\n")
    print ('no found')