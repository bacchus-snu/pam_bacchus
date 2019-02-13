from http.server import HTTPServer, BaseHTTPRequestHandler
import json


class AuthentiationServer(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        body = self.rfile.read(content_length)
        try:
            post_body = json.loads(body.decode())
        except json.JSONDecodeError:
            self.send_response(400)
            self.end_headers()
            return
        username = post_body.get('username', None)
        password = post_body.get('password', None)

        if username == 'test' and password == 'doge':
            self.send_response(200)
            self.end_headers()
            return
        else:
            self.send_response(401)
            self.end_headers()
            return


server = HTTPServer(('localhost', 10101), AuthentiationServer)
server.serve_forever()
