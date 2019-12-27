import socket
import time
import numpy as np
host_int = '192.168.108.25'
port_int = 8888
sock_int = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("wait for int")
sock_int.connect((host_int, port_int))  # 连接40服务器的客户端
print("connect int success")
Flag = '194.1 0.3'
sock_int.send(Flag.encode())
data =sock_int.recv(1024)
#ldata = np.fromstring(data, dtype ='float64')
print (data)
print(type(data))
Flag = '0'
sock_int.send(Flag.encode())
ldata = np.fromstring(data, dtype ='float64')
print('recv data:', ldata)
time.sleep(10)
#Flag = '0'
#sock_int.send(Flag.encode())
Flag = '194.1 0.3'
sock_int.send(Flag.encode())
time.sleep(3)
data =sock_int.recv(1024)
ldata = np.fromstring(data, dtype ='float64')
print (data)
time.sleep(10000)