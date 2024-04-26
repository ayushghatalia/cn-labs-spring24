import socket
import subprocess
import time
import sys
import inspect
import os
import math
import fnmatch

# host = '127.0.0.1'
# port = 8080

processes_a = []
processes_p = []
client_sockets_a = []
client_sockets_p = []
client_addresses_a = []
client_addresses_p = []
names_a = []
names_p = []
names = ["a", "b", "c", "d", "e"]
modes = ["active", "passive", "active", "passive", "active"]

def print_passed(msg):
    print("\033[92m" + msg + "\033[0m")


def print_failed(msg):
    print("\033[91m" + msg + "\033[0m")


def client_setup(host, port, num):
    global server_socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print(server_socket)
    server_socket.settimeout(2)
    server_socket.bind((host, port))
    server_socket.listen(3)
    print(f"Server listening on port {host}:{port}")

    global client_sockets_a
    global client_addresses_a
    global client_addresses_p
    global client_sockets_p 
    global processes_a
    global processes_p
    global names
    global names_a
    global names_p

    for i in range(0, num):
        process = subprocess.Popen(
        f"gcc ../impl/client.c -o client.out && ./client.out {host} {port} {names[i]} {modes[i]}",
        shell=True,
        stdin=subprocess.PIPE,
        # stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        )
        try:
            client_socket, client_address = server_socket.accept()
            data="NAME\n"
            client_socket.sendall(data.encode())
            name = client_socket.recv(1024).decode().strip("\x00")
            data="MODE\n"
            client_socket.sendall(data.encode())
            mode = client_socket.recv(1024).decode().strip("\x00")
    
            if name != names[i] + "\n" or mode != modes[i] + "\n":
                print_failed(f"client{i+1}_status: FAILED")
                raise ValueError()
            
            if(mode == "active\n"):
                client_addresses_a.append(client_address)
                names_a.append(name[:-1])
                client_sockets_a.append(client_socket)
                processes_a.append(process)
            elif mode == "passive\n":
                client_addresses_p.append(client_address)
                names_p.append(name[:-1])
                client_sockets_p.append(client_socket)
                processes_p.append(process)
            else:
                exit(1)

            print_passed(f"client{i+1}_status: SUCCESS")

        except (TimeoutError, ValueError) as e:
            print_failed(f"client_setup - 0.0/1.0")
            print_failed("client_mode_check - 0.0/1.0")
            return 0
    print_passed(f"client_setup - 1.0/1.0")
    for i in range(len(names)):
        if names[i] in names_a and modes[i] == "passive":
            print_failed("client_mode_check - 0.0/1.0")
            return 0
        elif names[i] in names_p and modes[i] == "active":
            print_failed("client_mode_check - 0.0/1.0")
            return 0
    print_passed("client_mode_check - 1.0/1.0")
    return 1

def test_cmds(num):
    global client_sockets_a
    global client_addresses_a
    global client_addresses_p
    global client_sockets_p 
    global processes_a
    global processes_p
    global names
    global names_a
    global names_p

    total_marks = 2.
   
    connected_clients = num
    loc = 0
 

    list_cmd = ["NOOP", "LIST", "MSGC:a|hello", "MSGC:b|crispy", "MESG: CREATE FILE", "CALL", "EXIT", "MSGC:c|sambar", "LIST", "NOOP",
            "MSGC:rava_idli|chutney", "MSGC:a|crispy", "EXIT", "CALL", "MESG: DELETE FILE", "LIST", "EXIT"]
    noop_l = [0, 9]
    list_l = [1, 8, 15]
    exit_l = [6, 12, 16]
    mesg_l = [4, 14]
    invalid_l = [5, 13]
    msgc_l = [2, 3, 7, 10, 11]
    exit_ans =  ['INITIALIZING......\nENTER CMD: MSGC:a:hello|active\na:hello: active\n\nENTER CMD: ENTER CMD: 1. a\n2. c\n3. b\n4. d\nENTER CMD: MSGC:c:crispy|active\nc:crispy: active\n\nENTER CMD: CLIENT TERMINATED: EXITING......', 
                'INITIALIZING......\nENTER CMD: 1. a\n2. c\n3. e\n4. b\n5. d\nENTER CMD: ENTER CMD: MSGC:c:sambar|active\nc:sambar: active\n\nENTER CMD: ENTER CMD: ENTER CMD: ENTER CMD: ENTER CMD: 1. c\n2. b\n3. d\nENTER CMD: CLIENT TERMINATED: EXITING......', 
                'INITIALIZING......\nENTER CMD: ENTER CMD: ENTER CMD: CLIENT TERMINATED: EXITING......']
    
    list_sol = [[], ['1. a\\n2. c\\n3. e\\n4. b\\n5. d\\n', '1. c\\n2. b\\n3. d\\n'], ['1. c\\n2. e\\n3. b\\n4. d\\n']]
    mscg_sol_a = [['e:01:hello\\n'], ['c:03:sambar\\n'], []]
    mscg_sol_p = [['a:02:crispy'], []]
    score = 0.000
    server_str = ""
    count = 0
    token = 0
    try:
        while loc < len(list_cmd):
            data = "POLL\n"
            client_sockets_a[token].sendall(data.encode())
            processes_a[token].stdin.write(list_cmd[loc] + '\n')
            # print(token)
            # print(processes_a[token])
            
            # print(names_a[token], token, list_cmd[loc])
            processes_a[token].stdin.flush()
            out = client_sockets_a[token].recv(1024)
            out = out.decode().strip("\x00")
            server_str += out
            if out == "NOOP\n":
                pass
            elif out == "EXIT\n":
                try:
                    output, _ = processes_a[token].communicate(timeout=2)
                    data = repr(output.strip())
                    x = data.split("ENTER CMD: ")[1:-1]
                    y = fnmatch.filter(x, '*:??:*\\n')
                    # print(names_a[token])
                    # print(y)
                    x = [a for a in x if "1. " in a]
                    # print(token)
                    # print(x)
                    if x != list_sol[token]:
                        print(x, list_sol[token], list_sol)
                        raise ValueError()
                    # print(repr(data))
                except (subprocess.TimeoutExpired, ValueError) as e:
                    processes_a[token].kill()
                    print_failed(f"list_check - 0.0/1.0")
                    return score
                # print(repr(data))
                # if data != exit_ans[token]:
                #     print("kk")
                #     raise ValueError()
                # print(y)
                if y != mscg_sol_a[token]:
                    raise ValueError()
                
                client_sockets_a[token].close()
                del(client_sockets_a[token])
                del(names_a[token])
                del(client_addresses_a[token])
                processes_a[token].terminate()
                del(processes_a[token])
                del(exit_ans[token])
                del[list_sol[token]]
                del[mscg_sol_a[token]]

                # print(client_addresses)
                # print(client_sockets)
            
                connected_clients -= 1
                token -= 1
                # print(connected_clients)
                if loc in exit_l:
                    # score += 0.333333
                    pass
        
            elif out == "LIST\n":
                ans = ""
                for j in range(0, len(names_a) - 1):
                    ans += names_a[j] + ":"
                ans += names_a[len(names_a) - 1]
                if len(names_p) == 0:
                    ans += "\n"
                else:
                    ans += ":"
                
                for j in range(0, len(names_p) - 1):
                    ans += names_p[j] + ":"
                if len(names_p):
                    ans += names_p[len(names_p) - 1] + "\n"

                client_sockets_a[token].sendall(ans.encode())

                if loc in list_l:
                    # score += .1111111
                    pass

            elif out[:5] == "MESG:":
                pass
            elif out[:5] == "MSGC:":
                flag = False
                count += 1
                val_list = out[5:].split("|")
                recipient = val_list[0]
                hex_val = hex(count)[2:].zfill(2)
                if recipient in names_a:
                    flag = True
                    ind = names_a.index(recipient)
                    output_str = "MSGC:" + names_a[token] + "|" + val_list[1][:-1] + "|" + hex_val + "\n"
                    # print(output_str)
                    client_sockets_a[ind].sendall(output_str.encode())

                if not flag:
                    if recipient in names_p:
                        flag = True
                        ind = names_p.index(recipient)
                        output_str = "MSGC:" + names_a[token] + "|" + val_list[1][:-1] + "|" + hex_val + "\n"
                        # print(output_str)
                        client_sockets_p[ind].sendall(output_str.encode())
                    
                if not flag:
                    count -= 1
                if loc in msgc_l:
                    score += 1
            else:
                if loc in invalid_l:
                    pass
            loc += 1
            if len(names_a):
                token = (token + 1)%len(names_a)
                # token = len(names_a) - 1 - masala_dosa
            # print("token:", token)
            # print(score)
        print_passed("list_check - 1.0/1.0")
        for s in client_sockets_p:
            s.close()
        try:
            for i in range(len(processes_p)):
                output, _ = processes_p[i].communicate(timeout=2)
                # print(output)
                data = output.strip()
                x = data.split("\n")[:]
                y = fnmatch.filter(x, '*:??:*')
                # print(y)
                if y != mscg_sol_p[i]:
                    raise ValueError()
        except (subprocess.TimeoutExpired, ValueError) as e:
            print("passive clients not terminated")
            print_failed(f"msgc_check - 3.0/4.0")
            return score
        # del(processes_p)
        server_socket.close()
        print_passed(f"msgc_check - 4.0/4.0")
            
        return score
    except Exception as e:
        print_failed(f"msgc_check - 0.0/3.0")
        return score
    

if __name__ == "__main__":
    ip = sys.argv[1]
    port = int(sys.argv[2])
    num = 5
    score1 = client_setup(ip, port, num)
    score2 = test_cmds(num)
    






