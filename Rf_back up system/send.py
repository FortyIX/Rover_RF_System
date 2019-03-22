import socket

UDP_IP = "Change To Pi's IP"
UDP_PORT=5005
MESSAGE = "Hello, World!"
print "UDP TARGET IP:", UDP_IP

print "UDP target port",UDP_PORT

print "message:", MESSAGE

sock = socket.socket(socket.AF_INET, # Internet
socket.SOCK_DGRAM) # UDP

sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))



