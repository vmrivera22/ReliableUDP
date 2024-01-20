Victor Rivera
1701239 (vrivera7)

Sources Used:
    For this program I referenced Professor Parsa's lectures. I also referenced code I had previously written for other classes and the previous assignmnets. I also used both man pages and geeksforgeeks to figure out how some functions worked. Additionally I got help from the TA Yi Liu and the tutor Sammy. I used a Linked List that I had previously written for CSE 101 modified to work for the assignment. I used CodeVault https://www.youtube.com/watch?v=xoXzp4B8aQk&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2&index=5 video to make threads. 


Within the top directroy of this project there are three sub directories: bin, src, and doc.

Files: This project contains the following files:
At the top level: Makefile, README.md
In the src dir: myclient.cpp, myserver.cpp, myhost.h, ll.h, packetfunc.h
In the doc dir: tests.txt, protocol.txt, and graph.pdf
In the bin dir: {empty}


How to run the program:
    To run the program type make within the top directory. Start the server using the command:
    "./bin/myserver port droppc root_folder_path"

    port - the port of the server
    droppc - the percent chance of dropping a packet recived or sent.
    root_folder_path - the path where the output file will be replicated to (in addition to the path indicated by the client).

    Once the server is running the client can then send a file using: 
    "./bin/myclient servn servaddr.conf mtu winsz in-file-path out-file-path"

    servn - is the number of servers to replicate to.
    servaddr.conf - the file containing the IP addresses and Port numbers of the servers to replicate to.
    mtu - maximum transmission unit
    winsz - the window size
    in-file-path - the file to replicate (send to servers).
    out-file-path - the path and file of where the server should replicate the file to.

    Note that if the indicated outfile path does not exist then it will be created.

    The program exits with -1 if there was an error and exits with 0 if the file was successfully recived.


Makefile:
    The Makefile is used to create an executable of the program. To use it use the comand "make" within the same directory.
    This will create two executables, called "myserver" and the other called "myclient". To run the program start by running the
    server with the command "./bin/myserver port droppc root_file_path". After this use "./bin/myclient servn servaddr.conf mtu winsz in-file-path out-file-path"
    to send a file to the servers indicated in servaddr.conf. The servers will send acks in response to the arriving packets. Additionally a packet may be drop by the servers, in which case no ACK will be sent and the client will retransmit if some time passes without reciving the corresponding ACK. The servers will reorder and reconstruct the original file into the specified output file path concatinated to the root file path.

    The executables can be removed using the command "make clean".

README.md:
    This file is the README. It contains a description of the files within the program, descriptions of the program itself, and citations to code referenced.

myclient.cpp:
    This file is the file that contains the code for the client. The program takes 6 arguments and is ran using 
    "./myclient servn servaddr.conf mtu winsz in_file_path out_file_path". The purpose of this program is to send the contents of the in-file-path to multiple servers using threads, recive corresponding ACKs from each server, and retransmit packets if there is packet loss. 

myserver.cpp:
    This file is the file that contains the code for the server. The program takes 3 argument and is ran usint "./myserver port droppc root_file_path". The purpose of this program is to create a server that recives data packets from clients (can be multiple). It rearanges out of order packets and returns ACK packets to the clients as a response in order to ensure reliablity. The server can also drop packets at a rate indicated by the input argument "droppc". In the end, if no packet is completely lost the server will reconstruct the original file in the specified the clients argument output-file-path appended to the "root_file_path".

myhost.h:
    This header file contains a host class that are used to create server and host objects. Each object has variables for both their and the host they are sending data to, such as struct sockaddr_in. Within the file are functions that can be used by both the server and the client such as creating a socket, binding, removing headers, and getting the sequence number.

ll.h:
    This header file contains the linked list class and functions corresponding to the linked list. This is used in order for the sever to be able to handle multiple clients.

packetfunc.h:
    This header file contains functions that are helpful in manipulating packets using my implemented protocol. It includes functions such as adding a header, removing headers, getting information from headers, and checking if input arguments are valid.

tests.txt:
    This file contains 5 tests that I ran to test the functionality of the server and client.

protocol.txt:
    This file contains a description of the methods used to seperate, reorder packets, and retransmit. In other words the protocol implemented to ensure reliability.

graph.pdf:
    This file shows a graph of the DATA and ACK packets that are sent from a client to two servers using a 4mb file.

