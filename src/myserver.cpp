//Server
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <random>
#include <errno.h>
#include "myhost.h"
#include "ll.h"
#include "packetfunc.h"


using namespace std;

struct timeval times;

// Class that is used to handle more than 1 client.
class new_client{
    public:
        int port;
        int IP;
        struct sockaddr_in cli_addr;
};

// Function used to get the output path - removing the actual file name.
char* find_file(char *&out_file){
    try{
        string ret = out_file;
        size_t place = ret.find_last_of("/");
        if (place == string::npos){
            return NULL;
        }
        string path_temp = ret.substr(0, place);
        char *c_path_temp = strdup(path_temp.c_str());

        path_temp = ret.substr(place+1);
        out_file = strdup(path_temp.c_str());

        return c_path_temp;
    }
    catch(...){
        cout << "ERROR getting the file path.\n\n";
        exit(-1);
    }
}

// Function that is used to determine if a packet is dropped or not.
bool drop_a(int perc){
    static std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0,1.0); // Use uniform distribution so that the percent of dropped pkts is closser to dropcc
    double rand = distribution(generator);
    rand = rand * 100;
    if(rand <= perc){
        return true;
    }
    return false;
}

// This function makes the needed directories based on the input.
void mk_all_dir(string in_path){
    try{
        char *fi_path = strdup(in_path.c_str());
        string h = "";
        string sla = "/";
        char *c_sla = strdup(sla.c_str());
        char *all = strdup(h.c_str());
        char *temp = strtok(fi_path, "/");
        all = strcat(all, temp);
        mkdir(all, 777);
        while(temp != NULL){
            temp = strtok(NULL, "/");
            if(temp == NULL){
                break;
            }
            all = strcat(all, c_sla);
            all = strcat(all, temp);
            mkdir(all, 777);   
        }
        return;
    }
    catch(...){
        cout << "ERROR making output directories.\n\n";
        exit(-1);
    }
}


int main(int argc, char *argv[]){
    my_host my_server;
    LinkedList serv_list;
    
    // Check to make sure that all arguments were input.
    for(int b = 1; b <= 3; b += 1){
        if(argv[b] == NULL){
            cout << "Not all arguments were input.\n\n";
            exit(-1);
        }
    }

    // Variables to hold arguments.
    char *port;
    int n_port;
    char *droppc;
    string root_folder;
    try{
        port = argv[1];
        my_server.port_v_check(port); // Check the validity of the port input.
        my_server.my_port = atoi(port);
        droppc = argv[2];
        bool drop_v_check = dig_check(droppc);
        if((drop_v_check == false) || (atoi(droppc) > 100) || (atoi(droppc) < 0)){ // Make sure the droppc is between 0 and 100.
            cerr << "Invalid droppc input.\n";
            exit(-1);
        }
        root_folder = argv[3];
        if(root_folder == ""){
            cout << "Not all arguments were input.\n\n";
            exit(-1);
        }
    }
    catch(...){
        cout << "Error with input arguments.\n\n";
        exit(-1);
    }

    int p_drop = atoi(droppc);
    n_port = atoi(port);

    int socke = my_server.make_socket(); // Creates the socket.

    // Set the socket read and write buffers to 3000000.
    int set_read_size = 3000000;
    int set_write_size = 3000000;

    setsockopt(my_server.sockets, SOL_SOCKET, SO_RCVBUF, &set_read_size, sizeof(set_read_size));
    setsockopt(my_server.sockets, SOL_SOCKET, SO_SNDBUF, &set_write_size, sizeof(set_write_size));

    // Zeros out struct and fills it with servers info.
    bzero(&my_server.myaddr, sizeof(my_server.myaddr));
    my_server.myaddr.sin_family = AF_INET;
    my_server.myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_server.myaddr.sin_port = htons(n_port);

    int b_check = bind(my_server.sockets, (struct sockaddr*) &my_server.myaddr, sizeof(my_server.myaddr)); 
    if(b_check == -1){
        cout << "erno: " << errno << "\n\n";
        cout << "\n\n" << strerror(errno) << "\n\n";
        exit(-1);
    }

    char *buff = new char[MAXSIZE+1]{'\0'}; // Buffer to read and write from socket.
    bool lost = false;

    char *time_buff = new char[200]{'\0'}; // Keeps track of the number of times a packet was dropped.
    socklen_t len = sizeof(my_server.theiraddr);

    int ex_head = 22; // Expected size of the header for a packet.
    for(;;){
        memset(buff, '\0', MAXSIZE); // Clear buffer
        len = sizeof(my_server.theiraddr);
        int r_err = recvfrom(my_server.sockets, buff, MAXSIZE, MSG_WAITALL, (struct sockaddr*) &my_server.theiraddr, &len);
        if(r_err == -1){
            cout << "ERROR READING\n";
            exit(-1);
            return -1;
        }
        // Get the port and IP from where the read in packet came from.
        int from_port = ntohs(my_server.theiraddr.sin_port);
        char *from_IP = inet_ntoa(my_server.theiraddr.sin_addr);
        Node *clie_node = serv_list.find(from_port, from_IP); // Look to see if we have already recived a packet from this address.
        if(clie_node == NULL){ // If it is the first time getting a packet from this address then create a new node with its info in linked list.
            char *temp = new char[50]{'\0'};
            clie_node = serv_list.insert(from_port, from_IP);
            int g = my_server.get_num_pkts(temp, buff); // The number of packets that are expected from this IP/Port combination.
            clie_node->total_pkts = g;
            memset(temp, '\0', 49);
            clie_node->hold_buff = new char*[clie_node->total_pkts+1]{NULL}; // Allocate memory to hold incoming file.
            clie_node->r_err_buff = new int[clie_node->total_pkts+1]{0};
            clie_node->droped_pkts = new int[clie_node->total_pkts+1]{0};
            delete[] temp;
        }

        char *h= new char[(1+r_err)-ex_head]{'\0'};
        int seq_num = my_server.get_SN(h, buff, (11), 10); // Get the SN of the packet.
        delete[] h;
        bool d = drop_a(p_drop); // Check to see if the packet should be dropped.
        int u = seq_num;
        while(clie_node->hold_buff[u+1] != NULL){
            u += 1; // Check to see if we already have sequential packets stored in buff.
        }
        if((d == true) & (clie_node->pkt_count != 0)){ // Drop the packet if needed to.
            char *base_temp = new char[50]{'\0'};
            clie_node->droped_pkts[seq_num] += 1;
            if(clie_node->droped_pkts[seq_num] >= 5){ // Output message if the packet has been dropped more than 4 times (debug)
                cout << "Packet " << seq_num << " dropped more than 4 times.\n\n";
                clie_node->hold_buff[seq_num] = NULL;
                clie_node->pkt_count += 1;
            }
            delete[] base_temp;
            // Print out info of the packet that was dropped.
            format_time(time_buff, TYPE_DATA, seq_num, 0, u+1, 0+0, n_port, clie_node->port, clie_node->IP, SERVER_IN);
            memset(time_buff, '\0', 200);
            if((clie_node->total_pkts-1 >= clie_node->pkt_count) && clie_node->droped_pkts[clie_node->total_pkts-1] < 5){
                continue;
            }

        }
        if(clie_node->pkt_count == seq_num){ // If sequence num was the expected SN
            char *l = new char[(1+r_err)-ex_head]{'\0'};
            clie_node->r_err_buff[seq_num] = r_err;
            memset(l, '\0', (1+r_err)-ex_head);
            my_server.rm_SN(l, buff, ex_head, (r_err - (ex_head))); // Get rid of the header

            clie_node->hold_buff[seq_num] = l;
            d = drop_a(p_drop); // Check if ACK should be dropped.
            int ACK_t_SN = clie_node->pkt_count;
            while(clie_node->hold_buff[ACK_t_SN+1] != NULL){ // Check to see if sequential data is already stored
                ACK_t_SN += 1;
            }
            if((d == true) & (clie_node->pkt_count != 0)){ // Drop ACK if needed
                char *base_temp = new char[50]{'\0'};
                clie_node->droped_pkts[seq_num] += 1;
                if(clie_node->droped_pkts[seq_num] >= 5){ // If the packet was dropped more than 4 times output it.
                    cout << "Packet " << seq_num << " dropped more than 4 times.\n\n";
                    clie_node->hold_buff[seq_num] = NULL;
                    clie_node->pkt_count += 1;
                }
                delete[] base_temp;
                // Print out info of the ACK that was dropped.
                format_time(time_buff, TYPE_ACK, ACK_t_SN, 0, ACK_t_SN + 1, 0+0, n_port, clie_node->port, clie_node->IP, SERVER_IN);
                memset(time_buff, '\0', 200);
                continue;
            }
            char *ack = my_server.z_buff(clie_node->pkt_count, 10); // Create an ack and send it.
            int w_err = sendto(my_server.sockets, ack, 10, 0, (struct sockaddr*) &my_server.theiraddr, sizeof(my_server.theiraddr));
            if(w_err == -1){
                cout << "errno: " << errno << "\n" << strerror(errno) << "\n\n";   
                cout << "ERROR SENDING\n";
                exit(-1);
                return -1;
            }
            clie_node->pkt_count = ACK_t_SN;
            clie_node->pkt_count += 1;
        }
        else{ // If the SN was not the SN that was expected.
            char *l= new char[(1+r_err)-ex_head]{'\0'};
            clie_node->r_err_buff[seq_num] = r_err;
            d = drop_a(p_drop); // Check to see if the ACK should be dropped.
            int ACK_t_SN = clie_node->pkt_count;
            while(clie_node->hold_buff[ACK_t_SN+1] != NULL){
                ACK_t_SN += 1;
            }
            if((d == true) & (clie_node->pkt_count != 0)){ // If the ACK needs to be dropped then drop it.
                char *base_temp = new char[50]{'\0'};
                clie_node->droped_pkts[seq_num] += 1;
                if(clie_node->droped_pkts[seq_num] >= 5){
                    cout << "Packet " << seq_num << " dropped more than 4 times.\n\n";
                    clie_node->hold_buff[seq_num] = NULL;
                    clie_node->pkt_count += 1;
                }
                delete[] base_temp;
                // Print info regarding the dropped ACK.
                format_time(time_buff, TYPE_ACK, ACK_t_SN, 0, ACK_t_SN + 1, 0+0, n_port, clie_node->port, clie_node->IP, SERVER_IN);
                memset(time_buff, '\0', 200);
                continue;
            }

            char *ack = my_server.z_buff(seq_num, 10); // Create and send an ACK
            int w_err = sendto(my_server.sockets, ack, 10, 0, (struct sockaddr*) &my_server.theiraddr, sizeof(my_server.theiraddr));
            if(w_err == -1){
                cout << "ERROR sending to server.\n\n";
                exit(-1);
            }
            memset(l, '\0', (1+r_err)-ex_head);
            my_server.rm_SN(l, buff, ex_head, (r_err - (ex_head))); // Remove header from read packet.
            clie_node->hold_buff[seq_num] = l; // Store the input data after removing the header.
        }
        if(clie_node->pkt_count >= clie_node->total_pkts){ // If we have successfully recived the whole file.
            if(clie_node->hold_buff[0] == NULL){ // If the first packet was lost then there is no way to know the output path.
                cout << "Could not open outputfile. Packet containing outfile path lost or outfile path is unacceptable.\n\n";
                exit(-1);
            }
            string s_buff_path;
            string final_path;
            char *out_file = clie_node->hold_buff[0];
            char *fi_path = find_file(out_file);
            if(root_folder[root_folder.length()-1] != '/'){ // Add a "/" to the end of the root folder
                s_buff_path = root_folder + "/";
                final_path = root_folder + "/";
            }
            else{
                s_buff_path = root_folder;
                final_path = root_folder;
            }

            if(fi_path != NULL){ // If the file path exists (from client) then append it to s_buff_path
                string temp_fi_path = fi_path;
                s_buff_path = s_buff_path + temp_fi_path;
            }
            
            // Make directories based on the client path appended to the root path.
            mk_all_dir(s_buff_path);
            string t_out_file = clie_node->hold_buff[0];
            final_path = final_path + t_out_file; // append the client path to the root path to get the full file path
            FILE *output_file;
            output_file = fopen(final_path.c_str(), "wb");
            int i = 1;
            while(i < clie_node->total_pkts){ // Output the stored file to the output file
                if(clie_node->hold_buff[i] != NULL){
                    fwrite(clie_node->hold_buff[i], 1, clie_node->r_err_buff[i]-ex_head, output_file); // If the packets are in order write to file.
                    delete[] clie_node->hold_buff[i];
                }
                else{
                    cout << "Paket " << i << " was lost.\n\n";
                    lost = true;
                }
                i+=1;
            }
            fclose(output_file);
            serv_list.delete_Node(clie_node->port, clie_node->IP); // Remove the node from the linked list.
        }

    }
    delete[] buff;
    delete[] time_buff;
    close(socke);
    close(my_server.sockets);
    exit(0);
    if(lost == true){
        exit(-1);
    }
    return 0;
}