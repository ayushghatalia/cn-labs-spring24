# Important Notes:
- This lab will be evaluated automatically. So, please ensure you follow instructions to the dot.
- Do not include unnecessary printf statements. Only print the strings mentioned in question. 
- Any strings which are required to be sent by the client or server must be **exactly as specified**. (This includes the number of bytes that are being sent)
- Certain functions such as those for connection are required for other test cases to run, if not implemented, the latter parts will not be evaluated.
- You'll receive a script containing sample test cases, and your submission will be evaluated against hidden test cases (not the ones you will use for testing) to determine the final score. So refrain from hard coding solutions.
- server_tester.py contains test cases to test the server code and the client_tester.py for the client code.
- Submit a zip file with the name `<id_no>_lab3.zip` containing your implementation of client and server. It should be a zip of this repo.

Welcome to the **Echo Server lab!** Before diving into the practical aspects, let's grasp the concept of an echo server. Imagine an echo server as a digital messenger that patiently waits for incoming messages on a specific TCP port. Once a connection is established between the echo server and a client, it acts like a friendly echo, repeating whatever message it receives.

In simpler terms, the echo server doesn't perform complex operations or alterations to the messages. Its primary job is to mirror back exactly what it hears from the client. So, if the client says "Hello" the echo server cheerfully responds with a "Hello".

In this lab, you'll have the opportunity to set up and interact with your own echo server. Get ready to explore the fascinating world of echo communication!

Let’s write a client-server communication program. There are two files you have been provided with **client.c** and **server.c**. These files currently consist of empty functions, it is your job to implement the functions according to the directions provided.

#Input: 
Both client.c and server.c file takes two command line arguments as input -- the ip_Address of the server, and the port number where the server is running. Please note that the ip_address and port number provided is that of the server.


# client.c:
`int main(int argc, char *argv[])`
  - main() has largely been implemented, all you have to do is process the command line args and assign them to the given variables, so that the command is accepted in the format `./a.out <ip_address> <port>`.

`int create_connection(char* addr, int port);` 
  - This function takes in the IP address and port of the server
  - It establishes a TCP connection with the server. On successful connection, return the socket descriptor. **(1 mark)**
  - If the client has started before the server and the client cannot connect, print “Could not find server” and exits. **(1 mark)**

`void send_data(int socket_id);` 
  - This function takes in the socket descriptor as a parameter.
  - It takes user input from stdin, and sends it as-is to the server. **(0.5 mark)**
  - The only condition where the client does not send the message to the server is when the user types "EXIT". If the user types `"EXIT"`. In that case the client should print `"Client exited successfully"` and the client program should terminate. **(0.5 mark)**


`void recv_data(int socket_id);`
  - This function takes in the socket descriptor as a parameter.
  - It receives data from the server.

# server.c:
`int main(int argc, char *argv[])`
  - main() has largely been implemented, all you have to do is process the command line args and assign them to the given variables, so that the command is accepted in the format `./a.out <ip_address> <port>`.

`int create_connection(char* addr, int port);` **(1 mark)**
  - This function takes in the IP address and port on which the server will run.
  - It will bind a socket to the given IP address and port.
  - It will set the socket to listen for new connections. (only one at a time)

`int client_connect(int socket_id);`
  - This function takes the socket on which the server is listening on as a parameter.
  - It will accept any incoming client connections and return the socket that the client connection was formed on.

`void echo_input(int socket_id);`
  - This function takes the socket which is connected to the client as input.
  - It will receive data sent from the client and it echoes this same data and sends it back to the client. **(1 mark)**
  - If the data sent by the client has a length<5, send back the string: `"Error: Message length must be more than 5 characters"` **(0.5 mark)**
  - After echoing back the data, the server can once again receive data from the client, this loop will repeat indefinitely. **(0.5 mark)**


## NOTE:
  - You are provided with scripts in the eval folder. Run those scripts to get your score. You can simply run the command `python <script_name>`.
  - Remember this will not be your final score. We will be running your solutions against hidden test cases.
