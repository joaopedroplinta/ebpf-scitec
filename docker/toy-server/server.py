#!/usr/bin/env python3
"""Servidor HTTP de brinquedo: gera tráfego TCP na porta 8080 para os
exemplos eBPF do minicurso observarem (ex: examples/04-tcp-monitor)."""
import http.server
import socketserver
import time

PORT = 8080


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = f"ok {time.time()}\n".encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass


if __name__ == "__main__":
    with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
        print(f"toy-server ouvindo na porta {PORT}")
        httpd.serve_forever()
