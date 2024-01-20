// Client
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <array>
#include <sstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>

#include "myhost.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "packetfunc.h"

struct timeval times;

pthread_mutex_t m;

char **whole_file; // Buffer that will contain the contents of the input file.
int *r_err_buff;   // Buffer that will contain the number of bytes that are read in from the socket.

// Class that is sent in as an argument to the thread function.
class dest_struct{
    public:
        int socket = 0;
        char *IP = NULL;
        int port = 0;
        int window_size = 0;
        int mtu_size = 0;
        int total_pkts = 0;
        char *output_file = NULL;
        bool err_det = false;
};


// Thread handler
void* send_recive(void *arg){
    dest_struct serv_info = *(dest_struct*)arg;
    // Create a sockaddr_in with the information of the server.
    struct sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(serv_info.port);
    send_addr.sin_addr.s_addr = inet_addr(serv_info.IP);

    int acked = 0;
    int seq_num = 0;
    int e_seq_num = 0;
    int av_win = serv_info.window_size;
    char *read_buff = new char[serv_info.mtu_size+1]{'\0'}; // Alocate memory for a buffer to read
    bool alarm_exp = false;
    int *retransmit_buff = new int[serv_info.total_pkts+1]{0};
    int *ACK_recv_buff = new int[serv_info.total_pkts+1]{0}; // Alocate memory for a ack buffer


    // Make the transmission connected UDP
    int con_err = connect(serv_info.socket, (struct sockaddr *) &send_addr, sizeof(send_addr));
    if(con_err == -1){
        cerr << strerror(errno);
        cerr << "\nServer could not be reached.\nCannot detect server at IP: " << serv_info.IP << " Port: " << serv_info.port << "\n";
        serv_info.err_det = true;
        pthread_exit(NULL);
    }

    // Get the local port number.
    struct sockaddr_in local_addr;
    socklen_t len_local = sizeof(local_addr);
    int loc_info = getsockname(serv_info.socket, (struct sockaddr *) &local_addr, &len_local);
    if(loc_info == -1){
        cerr << "Could not get local Port\n";
        serv_info.err_det = true;
        pthread_exit(NULL);
    }
    int local_port = ntohs(local_addr.sin_port);

    while(acked != serv_info.total_pkts){
        // Retransmits packets if they are dropped. If they have been retransmitted 4 times already then move foward.
        if(alarm_exp == true){
            int to_s = (serv_info.window_size-1);
            if(retransmit_buff[e_seq_num] >= 4){
                if(e_seq_num == 0){
                    cerr << "Reached max re-transmission limit of first packet to IP: " << serv_info.IP << " Port: " << serv_info.port << ".\n";
                    cerr << "Server may be ureachable. Make sure the server's IP address is correct.\n\n";
                    serv_info.err_det = true;
                    pthread_exit(NULL);
                }
                // If we have reached the maximum number of retransmissions for a packet then exit the thread.
                cerr << "Packet: " << e_seq_num << " was lost.\n";
                cerr << "Reached max re-transmission limit\n";
                serv_info.err_det = true;
                pthread_exit(NULL);
                if(e_seq_num == serv_info.total_pkts){
                    break;
                }
                e_seq_num += 1;
                acked += 1;
                av_win += 1;
                to_s += 1;
            }
            else{
                cerr << "Packet loss detected\n";
                retransmit_buff[e_seq_num] += 1;
                char *time_buff = new char[200]{'\0'};

                // Print log of retransmitted packet.
                pthread_mutex_lock(&m); // Lock to avoid having different threads print at the same time.
                format_time(time_buff, TYPE_DATA, e_seq_num, e_seq_num, seq_num, e_seq_num + serv_info.window_size, local_port, serv_info.port, serv_info.IP, CLIENT_IN);
                pthread_mutex_unlock(&m);
                delete[] time_buff;
                
                // Retransmit the packet.
                int w_err_r = send(serv_info.socket, whole_file[e_seq_num], (r_err_buff[e_seq_num]), 0);
                if(w_err_r == -1){
                    cerr << "ERROR Retransmitting.\n\n";
                    serv_info.err_det = true;
                    pthread_exit(NULL);
                }
            }
            alarm_exp = false;
            av_win = 0;
        }
        while((av_win > 0 || alarm_exp == true) && seq_num != serv_info.total_pkts){
            // While we have not sent the maximum window size packets - send a packet
            int w_err = send(serv_info.socket, whole_file[seq_num], r_err_buff[seq_num], 0);
            if(w_err == -1){
                cerr << "\nERROR SENDING\nServer Unreachable.\nCannot detect server\n";
                serv_info.err_det = true;
                pthread_exit(NULL);
            }
            char *time_buff = new char[200]{'\0'};
            // Prints the date and time packet was sent.
            pthread_mutex_lock(&m);
            format_time(time_buff, TYPE_DATA, seq_num, e_seq_num, seq_num+1, e_seq_num + serv_info.window_size, local_port, serv_info.port, serv_info.IP, CLIENT_IN);//format_time(char *buff, int type, int SN, int base_SN, int nxt_SN, int bpw){
            pthread_mutex_unlock(&m);
            delete[] time_buff;
            av_win -= 1;
            seq_num += 1;
        }

        // Recive ACK Packets
        int occur = 0;
        while(occur != serv_info.window_size && av_win != serv_info.window_size){
            errno = 0;
            memset(read_buff, '\0', serv_info.mtu_size+1);

            // Read from the socket (check for ACKs).
            int r_err = recv(serv_info.socket, read_buff, serv_info.mtu_size, 0);
            if(r_err == -1){
                // If there is a socket read timeout then assume that the pakcet was lost and set flag to retransmit.
                if((errno == EWOULDBLOCK) || (errno == EAGAIN)){
                    alarm_exp = true;
                    errno = 0;
                    break;
                }
                if((errno == ECONNREFUSED)){
                    cerr << "Server Unreachable\nCannot detect server\n";
                    cerr << strerror(errno);
                    serv_info.err_det = true;
                    pthread_exit(NULL);
                }
                if((errno == ECONNRESET) || (errno == ECONNABORTED) || (errno == EHOSTUNREACH) || (errno == EHOSTDOWN)){
                    cerr << "Server terminated.\n\n";
                    cerr << strerror(errno);
                    serv_info.err_det = true;
                    pthread_exit(NULL);
                }
                cerr << "ERROR reading.\n\n";
                serv_info.err_det = true;
                pthread_exit(NULL);
            }
            if(r_err > 0){
                char *buf_seq = new char[12]{'\0'};
                int ack_num = get_SN(buf_seq, read_buff, 0, 10);
                ACK_recv_buff[ack_num] = 1;
                // Check to see if the ACK is greater than or equal to the ACK we were expecting.
                acked += 1;
                if(ack_num == e_seq_num){
                    int f_n = e_seq_num;
                    while(ACK_recv_buff[f_n] != 0){
                         f_n += 1;
                    }
                    av_win = (f_n-e_seq_num);
                    e_seq_num = f_n; // Increment the SN we expect since we got a sequential ACK
                }
                char *time_buff = new char[200]{'\0'};
                // Print out the date and time of when the ACK packet was recived.
                pthread_mutex_lock(&m);
                format_time(time_buff, TYPE_ACK, ack_num, e_seq_num, seq_num, e_seq_num + serv_info.window_size, local_port, serv_info.port, serv_info.IP, CLIENT_IN);
                pthread_mutex_unlock(&m);
                while((av_win > 0 || alarm_exp == true) && seq_num != serv_info.total_pkts){
                    // If there was an ACKed Packet then send another packet so we have winsz packets in transition at a time.
                    int w_err = send(serv_info.socket, whole_file[seq_num], r_err_buff[seq_num], 0);
                    if(w_err == -1){
                        cerr << "\nERROR SENDING\nServer Unreachable at port/IP combination.\nCannot detect server\n";
                        serv_info.err_det = true;
                        pthread_exit(NULL);
                    }
                    // Prints the date and time packet was sent.
                    pthread_mutex_lock(&m);
                    format_time(time_buff, TYPE_DATA, seq_num, e_seq_num, seq_num+1, e_seq_num + serv_info.window_size, local_port, serv_info.port, serv_info.IP, CLIENT_IN);//format_time(char *buff, int type, int SN, int base_SN, int nxt_SN, int bpw){
                    pthread_mutex_unlock(&m);
                    av_win -= 1;
                    seq_num += 1;
                }
                delete[] time_buff;
                delete[] buf_seq;
            }
            occur += 1;
        }
    }
    pthread_exit(NULL);
}



int main(int argc, char *argv[]){
    my_host my_client;
    errno = 0;

    // Check to see if all arguments were input.
    for(int b = 1; b <= 6; b += 1){
        if(argv[b] == NULL){
            cerr << "Not all arguments were input.\n\n";
            exit(-1);
        }
    }

    // Arguments
    char *servn;
    char *servaddr_conf;
    char *mtu;
    char *winsz;
    char *in_file_path;
    char *out_file;

    int num_servn;
    int num_mtu;
    int num_winsz;

    // Checks the validity of arguments.
    try{
        servn = argv[1];
        if(dig_check(servn) == false){ // Make sure servn is an int.
            cout << "Argument servn is expected to be an int type.\n";
            exit(-1);
        }
        num_servn = atoi(servn);
        servaddr_conf = argv[2];
        mtu = argv[3];
        num_mtu = my_client.mtu_v_check(mtu, 42); // Check the validity of the mtu input.
        winsz = argv[4];
        num_winsz = my_client.winsz_v_check(winsz); // Check the validity of the winsz input.
        in_file_path = argv[5];
        out_file = argv[6];
    }
    catch(...){
        cerr << "Not all arguments were input.\n\n";
        exit(-1);
    }

    // Make a socket arr.
    int *sock_arr = new int[num_servn]{-1};

    // Socket timeout variables.
    times.tv_sec = 1;
    times.tv_usec = 0;

    // Socket buffer size variables
    int set_read_size = 3000000;
    int set_write_size = 3000000;

    // Open the input file and check to see if it exists.
    FILE *input_file;
    try{
        input_file = fopen(in_file_path, "rb");
        if(input_file == NULL){ 
            cerr << "ERROR opening read file.\nMake sure the file and file path exist.\n\n";
            exit(-1);
        }
    }
    catch(...){
        cerr << "ERRROR opening read file.\nMake sure the file exists.\n\n";
        exit(-1);
    }

    // Initialize the header length and the sequence number.
    int seq_num = 0;
    int head_len = 10 + 10 + 2 + 20;

    // Get the total number of packets that will be made from the input file.
    int total_pkts = 0;
    int data_len = num_mtu - head_len;
    try{
        struct stat f_t;
        stat(in_file_path, &f_t);
        float fi_si = f_t.st_size;

        // Figure out how many bytes you have to allocate for hold_buff.
        float num_arr = fi_si/(data_len);
        total_pkts = ceil(num_arr) + 1;
    }
    catch(...){
        total_pkts = MAXSIZE_PKT;
    }
    char *s_total_pkts = my_client.z_buff(total_pkts, 10);

    // Allocate memory for the buffer that will contain the whole file and buffer that will read from the input file.
    char *file_r = new char[num_mtu-head_len+1]{'\0'};
    whole_file = new char*[total_pkts + 1]{NULL};

    r_err_buff = new int[total_pkts+1]{0};
    char *out_p_buff = new char[500]{'\0'};

    // Adds the output file path as a packet to send
    char *s_SN = my_client.z_buff(seq_num, 10);
    int len_b = my_client.add_header(out_p_buff, s_total_pkts, s_SN, out_file, strlen(out_file));
    whole_file[seq_num] = out_p_buff;
    r_err_buff[seq_num] = len_b; 
    seq_num += 1;

    // Break down the contents of the file into packets and place them into an array of strings.
    int fi_re = fread(file_r, 1, num_mtu-head_len, input_file);
    while(fi_re != 0){
        char *write_buff = new char[num_mtu+1]{'\0'};
        s_SN = my_client.z_buff(seq_num, 10);
        len_b = my_client.add_header(write_buff, s_total_pkts, s_SN, file_r, fi_re);
        whole_file[seq_num] = write_buff;
        r_err_buff[seq_num] = len_b; 
        seq_num += 1;
        memset(file_r, '\0', num_mtu-head_len);
        fi_re = fread(file_r, 1, num_mtu-head_len, input_file);
    }
    fclose(input_file);


    // Open server address config file and make sure that it exists  
    ifstream addr_file;
    try{
        addr_file.open(servaddr_conf, ios::binary);
        if(addr_file.is_open() == false){
            cerr << "ERROR opening address config file.\nMake sure the file and file path exist.\n\n";
            exit(-1);
        }
    }
    catch(...){
        cerr << "ERRROR opening address config file.\nMake sure the file exists.\n\n";
        exit(-1);
    }


    int *port_buff = new int[num_servn+1]{0}; // Buffer to contain the server ports from address config file.
    char **IP_buff = new char*[num_servn+1]{NULL}; // Buffer to contain the server IP addresses from the address config file.
    
    // Variables used to parse config IP/Port file.
    char *in_port;
    string line;
    int j = 0;
    int t_serv_n = 0;

    // Read and Parse config IP/Port file line by line.
    while(getline(addr_file, line) && (t_serv_n < num_servn)){
        char *c_line = strdup(line.c_str());
        char *in_IP = strtok(c_line, " \t");
        char *temp_IP = new char[strlen(in_IP) + 1];
        memcpy(temp_IP, in_IP, strlen(in_IP));
        in_port = strtok(NULL, " \t");
        bool IP_check = IP_v_check(in_IP); // Make sure the IP is valid
        bool port_check = port_bool_check(in_port); // Make sure the Port is valid
        if(IP_check == true && port_check == true){ // If both the port and IP are valid than add them to their respective buffers.
            IP_buff[j] = temp_IP;
            port_buff[j] = atoi(in_port);
        }
        else{ // If a port or IP is not valid then the buffer at index [j] is left NULL and 0.
            if(IP_check == false){
                cerr << "Incorrect IP (" << in_IP << ") entered in destination file\n";
            }
            if(port_check == false){
                cerr << "Incorrect Port (" << in_port << ") entered in destination file\n";
            }
        }
        t_serv_n += 1;
        j += 1;
    }
    addr_file.close();

    // If there servn is larger than the number of lines that were read then we got an invalid input. 
    if(num_servn > j){
        cerr << "Argument servn was larger than the number of IP/Port combination (lines) in servaddr_cofig file.\n";
        exit(-1);
    }

    // Thread creation  
    pthread_t thread_arr[num_servn];

    // Array consisting of structures that will be passed in as arguments to the thread handler function.
    dest_struct **thread_dest_arr = new dest_struct*[num_servn+1];
    dest_struct *thread_dest;
    pthread_mutex_init(&m, NULL);
    int i;
    int num_threads = num_servn;
    if(num_threads > 200){ // Make sure that not too many threads are created.
        num_threads = 200;
    }

    // Sets up and creates all threads.
    for(i = 0; i < num_threads; i += 1){
        if(port_buff[i] == 0 || IP_buff[i] == NULL){ // If the port and IP were invalid then skip current i.
            continue;
        }
        sock_arr[i] = socket(AF_INET, SOCK_DGRAM, 0); // Create a socket
        if(sock_arr[i] == -1){
            cerr << "Error making socket.\n\n";
        }
        try{ // Set a socket timer limit and set the socket buffer read and write size.
            setsockopt(sock_arr[i], SOL_SOCKET, SO_RCVTIMEO, &times, sizeof(times));
            setsockopt(sock_arr[i], SOL_SOCKET, SO_RCVBUF, &set_read_size, sizeof(set_read_size));
            setsockopt(sock_arr[i], SOL_SOCKET, SO_SNDBUF, &set_write_size, sizeof(set_write_size));
        }
        catch(...){
            cerr << "ERROR seting socket timer.\n\n";
        }

        // Fill in the information into the struct that will be sent to the thread handler function.
        thread_dest = new dest_struct[sizeof(dest_struct)];
        thread_dest->port = port_buff[i];
        thread_dest->IP = IP_buff[i];
        thread_dest->mtu_size = num_mtu;
        thread_dest->window_size = num_winsz;
        thread_dest->output_file = out_file;
        thread_dest->total_pkts = total_pkts;
        thread_dest->socket = sock_arr[i];
        thread_dest_arr[i] = thread_dest;

        int check_thr = pthread_create(&thread_arr[i], NULL, &send_recive, (void *) thread_dest);
        if(check_thr != 0){
            cerr << "Error creating threads.\n";
            exit(-1);
        }
    }

    // Function for joining threads.
    for(i = 0; i < num_threads; i += 1){
        if(port_buff[i] == 0 || IP_buff[i] == NULL){ // If the port or IP were wrong then the thread was never created so skip.
            continue;
        }
        int check_join = pthread_join(thread_arr[i], NULL);
        if(check_join != 0){
            cerr << "Error joining threads.\n";
            exit(-1);
        }
    }

    pthread_mutex_destroy(&m);
    bool err_in_thread = false;
    // Check to see if any threads exited with an error
    for(i = 0; i < num_threads; i+= 1){
        if(port_buff[i] == 0 || IP_buff[i] == NULL){ // If a thread was never created then it was an error with IP or Port input.
            cerr << "Error explanation above. Error transfering file to 1 or more destinations due to incorrect IP or Port.\n";
            err_in_thread = true;
            continue;
        }
        else{
            delete[] thread_dest_arr[i];
        }
        if(thread_dest_arr[i]->err_det == true){ // Output to cerr the IP and Port of the server connection thread that exited with error.
            cerr << "Error explanation above. Error transfering file to destination IP: " << thread_dest_arr[i]->IP << " Port: " << thread_dest_arr[i]->port << "\n";
            err_in_thread = true;
        }
    }
    delete[] port_buff;
    delete[] IP_buff;
    if(err_in_thread == true){ // If an error occured in any of the threads then exit with error since program was not executed as expected.
        exit(-1);
    }
    return 0;
}