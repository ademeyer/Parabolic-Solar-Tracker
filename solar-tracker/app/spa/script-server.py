#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
import subprocess

class Handler(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

    def do_GET(self):
        if self.path == '/':
            self._set_headers()
            result = subprocess.run(['./fetch_pos.sh'], capture_output=True, text=True)
            self.wfile.write(result.stdout.encode())

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 1900), Handler)
    print("Server started on port 1900")
    server.serve_forever()
