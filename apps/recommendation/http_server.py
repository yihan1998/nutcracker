import http.server
import socketserver

PORT = 8000

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Respond with a simple HTML page
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"<html><body><h1>HTTP Server</h1><p>This is a simple HTTP server.</p></body></html>")

    def do_POST(self):
        # Handle POST request
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        response = f"<html><body><h1>POST Data</h1><p>{post_data.decode('utf-8')}</p></body></html>"
        self.wfile.write(response.encode('utf-8'))

def entrypoint():
    # Create the server
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving at port {PORT}")
        httpd.serve_forever()
