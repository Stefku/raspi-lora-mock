#!/usr/bin/python

import SimpleHTTPServer
import SocketServer
import sys
import os
import os.path
from os import path

PORT = 8080
BASE_DIR = '.'

if len(sys.argv) == 2:
    BASE_DIR = sys.argv[1]

FILE_COMMAND = result = os.path.join(BASE_DIR, "command.txt")

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_POST(self):
        if self.path=="/command":
            content_len = int(self.headers.getheader('content-length', 0))
            post_body = self.rfile.read(content_len)
            text_file = open(FILE_COMMAND, "w")
            text_file.write(post_body)
            text_file.close()
            self.send_response(200)
            return
        self.send_response(404)
        self.wfile.write("path not found")

    def do_DELETE(self):
        if self.path=="/command":
            if path.exists(FILE_COMMAND):
                os.remove(FILE_COMMAND)
                self.send_response(200)
                return
            self.send_response(204)
            self.send_header('Content-type','text/html')
            self.end_headers()
            # Send the html message
            self.wfile.write("command.txt does not exist")
        self.send_response(404)
        self.wfile.write("path not found")

    def do_GET(self):
        if self.path=="/command":
            if path.exists(FILE_COMMAND):
                with open(FILE_COMMAND, 'r') as file:
                    data = file.read()
                    self.send_response(200)
                    self.send_header('Content-type','text/html')
                    self.end_headers()
                    # Send the html message
                    self.wfile.write(data)
                    return
            self.send_response(204)
            self.send_header('Content-type','text/html')
            self.end_headers()
            # Send the html message
            self.wfile.write("command.txt does not exist")
            return
        self.send_response(404)
        self.send_header('Content-type','text/html')
        self.end_headers()
        self.wfile.write("path not found\navailable:\nGET /command\nPOST /command\nDELETE /command\n")

Handler = ServerHandler

try:
    httpd = SocketServer.TCPServer(("", PORT), Handler)
    print "serving at port", PORT
    print "looking at directory", BASE_DIR
    print "looking at command file", FILE_COMMAND
    httpd.serve_forever()
except KeyboardInterrupt:
	print '^C received, shutting down the web server'
	httpd.socket.close()