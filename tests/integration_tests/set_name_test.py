import socket

sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
sock.connect(("127.0.0.1",8080))

sock.sendall(b"SET name Damir\n")
response=sock.recv(1024)

assert response==b"OK!\n"

sock.close()

print("test passed")