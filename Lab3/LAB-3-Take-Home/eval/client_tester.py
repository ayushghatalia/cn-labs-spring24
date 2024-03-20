import socket
import subprocess
import time
import sys
import inspect
import os

host = '127.0.0.1'
port = 2021


def print_passed(msg):
    print("\033[92m" + msg + "\033[0m")


def print_failed(msg):
    print("\033[91m" + msg + "\033[0m")


def init_client_before_server():
    process = subprocess.Popen(
        "gcc ../impl/client.c -o ../impl/client && ../impl/client {} {}".format(host, port),
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    time.sleep(3)
    try:
        output, _ = process.communicate(timeout=2)
        data = output.strip()
    except subprocess.TimeoutExpired:
        print_failed("init_client_before_server - TEST CASE FAILED")
        return 0
    
    expected = "Could not find server"
    if data == expected:
        print_passed("init_client_before_server - TEST CASE PASSED")
        return 1
    else:
        print_failed("init_client_before_server - TEST CASE FAILED")
        return 0


def init_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.settimeout(1)
    server_socket.bind((host, port))
    server_socket.listen(5)
    global process
    process = subprocess.Popen(
        "gcc ../impl/client.c -o ../impl/client && ../impl/client 127.0.0.1 2021",
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
        print_failed("init_server - TEST CASE FAILED")
        return 0

    print_passed("init_server - TEST CASE PASSED")
    return 1


def test_client_data():
    expected = "data\n"
    input_data = "data"
    process.stdin.write(input_data + "\n")
    process.stdin.flush()

    client_socket.settimeout(3)
    data = client_socket.recv(1024).decode()
    client_socket.sendall(data.encode())

    if data != expected:
        print_failed("test_client_data - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_client_data - TEST CASE PASSED")
        return 0.5

def test_client_exit():
    expected = "Client exited successfully"
    input_data = "EXIT"
    process.stdin.write(input_data + "\n")
    process.stdin.flush()

    try:
        output, _ = process.communicate(timeout=2)
        data = output.strip()
    except subprocess.TimeoutExpired:
        print_failed("test_client_exit - TEST CASE FAILED")
        return 0

    if expected not in data:
        print_failed("test_client_exit - TEST CASE FAILED")
        return 0
    else:
        print_passed("test_client_exit - TEST CASE PASSED")
        return 0.5


def eval():
    marks = 0
    marks += init_client_before_server()
    marks += init_server()
    marks += test_client_data()
    marks += test_client_exit()

    print("\nTotal Marks: " + str(marks))


if __name__ == "__main__":
    current_script_path = inspect.getfile(inspect.currentframe())
    directory = os.path.dirname(current_script_path)
    os.chdir(directory)
    try:
        eval()
    except:
        print_failed("Script crashed while Testing, Test Case Failed")
