import socket
import RPi.GPIO as GPIO

import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(18,GPIO.OUT)


UDP_IP = "10.242.12.80"
UDP_PORT = 5005


clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#UDP

clientSock.bind((UDP_IP, UDP_PORT))

GPIO.setwarnings(False)
GPIO.output(18,GPIO.LOW)

while True:
          data, addr = clientSock.recvfrom(1024)
          print"received message:" , data
          #data = int(data)
          if data == "key down":
              GPIO.output(18,GPIO.HIGH)
          if data == "key up":
              GPIO.output(18,GPIO.LOW)