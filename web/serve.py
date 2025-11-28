#!/usr/bin/env python3
"""
Simple HTTP server for MADOLA web app
Serves files from the current directory on port 8000
"""

import http.server
import socketserver
import os
import sys

# Change to the web directory
web_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(web_dir)

PORT = 8000

Handler = http.server.SimpleHTTPRequestHandler

print(f"Serving MADOLA web app from: {web_dir}")
print(f"Open your browser to: http://localhost:{PORT}")
print("Press Ctrl+C to stop the server")

try:
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        httpd.serve_forever()
except KeyboardInterrupt:
    print("\nServer stopped.")
    sys.exit(0)