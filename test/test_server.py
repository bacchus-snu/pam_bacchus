from http.server import HTTPServer, BaseHTTPRequestHandler
import base64
import json
import subprocess
import sys


class AuthentiationServer(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        body = self.rfile.read(content_length)
        body = body.decode()

        pubkey_b64 = self.headers['X-Bacchus-Id-Pubkey']
        timestamp = self.headers['X-Bacchus-Id-Timestamp']
        signature_b64 = self.headers['X-Bacchus-Id-Signature']

        message = (timestamp + body).encode('utf-8')
        pubkey = base64.b64decode(pubkey_b64, validate=True)
        signature = base64.b64decode(signature_b64, validate=True)

        check_input = pubkey + signature + message
        result = subprocess.run(['./test/check_signature'], input=check_input, capture_output=True)
        if result.returncode != 0:
            print(result.stderr.decode(), file=sys.stderr)
            self.send_response(401)
            self.end_headers()
            return

        try:
            post_body = json.loads(body)
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
