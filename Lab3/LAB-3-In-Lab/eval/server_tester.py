import socket
import subprocess
import time
import os
import inspect
import random

host = '127.0.0.1'
port = random.randint(10000, 20000)
client_socket = None

marks = 0

def print_passed(msg):
    print("\033[92m" + msg + "\033[0m")


def print_failed(msg):
    print("\033[91m" + msg + "\033[0m")

def init_client():
    global client_socket

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.settimeout(1)

    subprocess.Popen("gcc ../impl/server.c -o ../impl/server && ../impl/server {} {}".format(host, port), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(3)
    try:
        client_socket.connect((host, port))
    except:
        print_failed("init_client - TEST CASE FAILED")
        return 0
    print_passed("init_client - TEST CASE PASSED")
    return 1


def test_case_1():
    message = "ECHO:TESTCASE1:4"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024).decode()
    expected = "OHCE:TEST"
    if data != expected:
        print_failed("test_case_1 - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_case_1 - TEST CASE PASSED")
        return 2

def test_case_2():
    message = "ECHO:TESTCASE2:-5"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024).decode()
    expected = "Error: Negative number of bytes"
    if data != expected:
        print_failed("test_case_2 - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_case_2 - TEST CASE PASSED")
        return 1

def test_case_3():
    message = "ECHO:TESTCASE3"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024).decode()
    expected = "OHCE:TESTC"
    if data != expected:
        print_failed("test_case_3 - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_case_3 - TEST CASE PASSED")
        return 1

def test_case_4():
    message = "ECHO:TESTCASE4:20"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024).decode()
    expected = "OHCE:TESTCASE4 (9 bytes sent)"
    if data != expected:
        print_failed("test_case_4 - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_case_4 - TEST CASE PASSED")
        return 2


def test_case_5():
    message = "TESTCASE5"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024).decode()
    expected = "Invalid command: Unable to echo!"
    if data != expected:
        print(data, expected, sep=" != ")
        print(len(data), len(expected), sep=" != ")
        print(data[0])
        print_failed("test_case_5 - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_case_5 - TEST CASE PASSED")
        return 1

def eval():
    global marks
    init_client()
    marks += test_case_1()
    marks += test_case_2()
    marks += test_case_3()
    marks += test_case_4()
    marks += test_case_5()

    print("Marks: ", marks)
    return marks

if __name__ == "__main__":
    try:
        eval()
    except:
        print_failed("Script crashed while Testing, Test Case Failed")
        print("Marks: ", marks)
