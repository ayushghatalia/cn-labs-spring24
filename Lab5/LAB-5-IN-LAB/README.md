# **LAB5_MULTITHREADING_23-24_IN-LAB**

## Instructions

- This lab will be evaluated automatically.

- Any strings which are required to be sent by the client or server must
  be exactly as specified. (This includes the number of bytes that are
  being sent)

- You’ll receive a binary containing sample test cases, and your
  submission will be evaluated against hidden test cases (not the ones
  you will use for testing) to determine the final score. So refrain
  from hard coding solutions.

- Submit a zip file with the name \<id_no\>\_lab5.zip containing your
  implementation of client and server. It should be a zip of this repo.
  Ensure the impl folder is present and contains server.c.


- **To evaluate:** run ./server_final and ensure you are in the ./eval
  directory.

- You may also run ./server_final \<wait_time\> in order to increase the
  sleep time after connections are initiated. (default of 3)
  
- You can print anything you want, after running the eval file all the print statements will be written to log.txt

- **Run `python3 zipper.py <id_no>_lab5.zip`** from ./helper to make your zip for submission.

Please note the binary is meant to verify or evaluate your code, **not
for debugging it**. If you wish to use it to debug, do so at your own
risk.

The purpose of this lab is to implement a client and server program that
uses multi-threading to communicate. Which can eliminate the drawbacks
of blocking calls such as recv() and allows communication to occur in
parallel.

## **In Lab**

### **Client.c**

We now introduce two types of clients – root clients and normal clients
(idea is similar to active and passive clients that you had in the
polling server assignment). As the name suggests, the root client will
have more privileges as compared to the normal client. Here is the
detailed working of the client.

- The client will take in an IP address, port through CLI arguments. In
  the format: `./a.out <ip_address> <port>` and connect to the server.

  - The \<username\> and \<authority\> will be sent to the server as the
    <u>first message</u> after connecting. Send both in one message in
    the format: `<username>|<authority>\n`

  - Assume that this is ALWAYS the first communication that any client
    would compulsorily do to make your life easier :)

  - The \<username\> will be a single word without spaces or special
    characters.

  - The \<authority\> specifies whether it's a root user identified by
    sending "r" or a normal user identified by sending "n".

  - Example of the connection till here:

> \[cli\] ./a.out 127.0.0.1 4444
>
> \[cli\] alice|r\n


- If a root user connects, the server will expect the very next message
  to be the password.

- The client will send the password in the format: \<password\>. (\n is removed)



After this, normal functioning will follow:

- The client will take input through stdin and send it to the server.

- Commands that the client can send to the server are mentioned below

- The client can send and receive data from the server.

- **Note** that taking input from stdin and receiving data are both
    blocking calls. Ensure these 2 can run in parallel using
    multithreading.

- **Note:** The client will not be evaluated, however you need to
  implement it to test your server.

### **server.c**

The server that you develop should be capable of handling both root and
normal clients.

The server will listen to and can accept a max of 1024 client
connections. To start off,

- The server will take in IP address, port and password through CLI
  arguments in the format: `./a.out <ip_address> <port> <password>`.

  - The \<password\> is used by root users to login (see client.c for
    details).

  - E.g., `[cli] ./a.out 127.0.0.1 4444 user123\n`

- If a root user (client with ‘r’ as authorization) connects, the client
  must provide the \<password\> in the next message (see client.c for
  the details). **(3 marks)**

  - If the password validates, the user is connected. Send "PASSWORD
    ACCEPTED" back to the root client.

  - If the password is incorrect send "PASSWORD REJECTED" and disconnect
    the client.

Example what the client sees when server uses user123 as password: (Note: client cli behaviour is upto you)

> \[cli\] ./a.out 127.0.0.1 4444
>
> \[cli\] alice|r\n
>
> \[cli\] user123\n
>
> \[msg from server\] PASSWORD ACCEPTED

Failure example:

> \[cli\] ./a.out 127.0.0.1 4444
>
> \[cli\] alice|r\n
>
> \[cli\] xxxx123\n
>
> \[msg from server\] PASSWORD REJECTED

- If a normal user connects, then the client is automatically connected
  to the server without any password.

- The server must be able to receive data from all clients in parallel
  using multithreading.

- Based on the data received from the client, the server will perform
  different functions:

### **Additional Commands and Modifications to be implemented (built on take-home)**

#### **CAST:\<message\>\n (4 mark)**

- When root users issue this command. The server will send back the
  \<message\>\n to all the connected root users excluding itself. **(2 marks)**

- When normal users issue this command. The server will send back the
  \<message\>\n to all the connected normal users excluding itself. **(2 marks)**

**LIST\n: (3 mark)**

- When the server receives this command it sends a string containing the
  names of all the clients that are currently connected to in a new
  format:
  `LIST-<name1>|<authority1>:<name2>|<authority2>:<name3>|<authority3>\n`

- The server will broadcast this list to all connected clients when a
  client EXITs (this list will not include the exited client).
