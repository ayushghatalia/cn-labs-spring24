import socket
import subprocess
import time
import sys
import inspect
import os
import random

host = '127.0.0.1'
port = random.randint(10000, 20000)

def print_passed(msg):
    print("\033[92m" + msg + "\033[0m")


def print_failed(msg):
    print("\033[91m" + msg + "\033[0m")

def test_client_file():
    f1 = open("f1.txt", "w")
    input_data = "ECHO:TESTCASE1:4\n"
    process.stdin.write(input_data)
    process.stdin.flush()
    client_socket.settimeout(3)
    data = "OHCE:TEST"
    client_socket.sendall(data.encode())
    f1.write("CLIENT: " + input_data + "SERVER: " + data + "\n")
    time.sleep(1)
    input_data = "ECHO:TESTCASE2:-5\n"
    process.stdin.write(input_data)
    process.stdin.flush()
    client_socket.settimeout(3)
    data = "Error: Negative number of bytes"
    client_socket.sendall(data.encode())
    f1.write("CLIENT: " + input_data + "SERVER: " + data + "\n")
    time.sleep(1)
    input_data = "ECHO:TESTCASE3\n"
    process.stdin.write(input_data)
    process.stdin.flush()
    client_socket.settimeout(3)
    data = "OHCE:TESTC"
    client_socket.sendall(data.encode())
    f1.write("CLIENT: " + input_data + "SERVER: " + data + "\n")
    time.sleep(1)
    input_data = "ECHO:TESTCASE4:20\n"
    process.stdin.write(input_data)
    process.stdin.flush()
    client_socket.settimeout(3)
    data = "OHCE:TESTCASE4 (9 bytes sent)"
    client_socket.sendall(data.encode())
    f1.write("CLIENT: " + input_data + "SERVER: " + data + "\n")
    time.sleep(1)
    input_data = "TESTCASE5\n"
    process.stdin.write(input_data)
    process.stdin.flush()
    client_socket.settimeout(3)
    data = "Invalid command: Unable to echo!"
    client_socket.sendall(data.encode())
    f1.write("CLIENT: " + input_data + "SERVER: " + data + "\n")
    time.sleep(1)
    input_data = "EXIT\n"
    process.stdin.write(input_data)
    time.sleep(1)
    client_socket.close()
    f1.close()

def init_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.settimeout(1)
    server_socket.bind((host, port))
    server_socket.listen(5)
    global process
    process = subprocess.Popen(
        "gcc ../impl/client.c -o ../impl/client && ../impl/client {} {}".format(host, port),
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    time.sleep(2)

    global client_socket
    try:
        client_socket, client_address = server_socket.accept()
    except socket.timeout:
        return 0
    return 1


def eval():
    init_server()
    test_client_file()
    print_passed("File Created")

if __name__ == "__main__":
    try:
        eval()
    except:
        print_failed("Script crashed while Testing, Test Case Failed")