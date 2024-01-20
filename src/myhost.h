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

#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>

#define MAXSIZE 32000
#define MAXSIZE_PKT 10000
#define SERVER_IN 0
#define CLIENT_IN 1
#define TYPE_ACK 0
#define TYPE_DATA 1

using namespace std;

class my_host{
    public:
        const char *my_IP = "";
        const char *their_IP = "";
        int my_port = 0;
        int their_port = 0;

        int sockets = 0;
        int bound_s = 2;

        struct sockaddr_in myaddr;
        struct sockaddr_in theiraddr;

        int make_socket();
        int bind_socket();
        int write_message(char*, int);
        int read_message(char*&, int, int, int);
        int get_SN(char*, char*, int, int); //char *buff_in, int s_pos, int e_pos, int head
        char* rm_SN(char*, char*, int, int);
        int add_header(char*, char*, char*, char*, int);
        int add_win(char*, int, int, int);
        int get_num_pkts(char*, char*);
        int get_mtu(char**);
        char* z_buff(int, int);
        int num_dig(int);
        int find_error();
        int port_v_check(char*);
        int mtu_v_check(char*, int);
        int winsz_v_check(char*);
        bool dig_check(char*);
};

bool my_host :: dig_check(char *a){
    int len_a = strlen(a);
    for(int i = 0; i < len_a; i+=1){
        if(isdigit(a[i]) == false){
            return false;
        }
    }
    return true;
}

int my_host :: winsz_v_check(char *s_winsz){
    bool dig_win = dig_check(s_winsz);
    if(dig_win == false){
        cerr << "Window Size input is not valid.\n\n";
        exit(-1);
    }
    int num_winsz = atoi(s_winsz);
    return num_winsz;
}

int my_host :: port_v_check(char *s_port){
    if(s_port == NULL){
        cerr << "No port was entered.\n\n";
        exit(-1);
    }
    bool dig_p = dig_check(s_port);
    if(dig_p == false){
        cerr << "Port input is not valid.\n\n";
        exit(-1);
    }
    int port_in = atoi(s_port);
    if(port_in > 65536 || port_in < 1025){
        cerr << "Port out of acceptable range. Port has to be between 1025 and 65536.\n\n";
        exit(-1);
    }
    return port_in;
}

int my_host :: mtu_v_check(char *s_mtu, int head_len){
    bool dig_mtu = dig_check(s_mtu);
    if(dig_mtu == false){
        cerr << "MTU input is not valid.\n\n";
        exit(-1);
    }

    int num_mtu = atoi(s_mtu);
    if(num_mtu < (head_len + 2)){
        cerr << "Required minimum MTU is " << (head_len + 2) << ".\n\n";
        exit(-1);
    }
    else if(num_mtu > 32000){
        cerr << "The input MTU size was larger than the allowed 32000 for this program.\n\n";
        exit(-1);
    }
    return num_mtu;
}

int my_host :: find_error(){
    int error_type;
    socklen_t length_error_type = sizeof(error_type);
    int val = getsockopt(sockets, SOL_SOCKET, SO_ERROR, &error_type, &length_error_type);
    if(val == -1){
        return errno;
    }
    else{
        return error_type;
    }

}


// Function used to make a socket.
int my_host :: make_socket(){
    int socke = socket(AF_INET, SOCK_DGRAM, 0);
    sockets = socke;
    if(socke == -1){
        cerr << "Error making socket.\n\n";
        exit(-1);
        return -1;
    }
    return socke;
}

// Function used to bind a socket.
int my_host :: bind_socket(){
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(my_port);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bound_s = bind(sockets, (struct sockaddr*) &myaddr, sizeof(myaddr));
    if(bound_s == -1){
        cerr << "Error binding.\n\n";
        exit(-1);
        return -1;
    }
    return bound_s;
}

// Function used to write to the socket.
int my_host :: write_message(char *buff, int n_bytes){
    int msg = sendto(sockets, buff, n_bytes, 0, (struct sockaddr*) &theiraddr, sizeof(theiraddr));
    if(msg == -1){
        cerr << "ERROR WRITING\n";
        exit(-1);
        return -1;
    }
    return msg;
}

// Function used to read from the socket.
int my_host :: read_message(char *&buff, int n_bytes, int c, int flag){
    socklen_t leng = sizeof(myaddr);
    getsockname(sockets, (struct sockaddr *)&myaddr, &leng);
    socklen_t len = sizeof(theiraddr);

    int msg = 0;
    if(c == SERVER_IN){ // For the server.
        msg = recvfrom(sockets, buff, n_bytes, flag, (struct sockaddr*) &theiraddr, &len);
    }
    else if(c == CLIENT_IN){ // For the client.
        msg = recvfrom(sockets, buff, n_bytes, flag, NULL, NULL);
    }
    if(msg == -1){
        cerr << "ERROR READING\n\n";
        exit(-1);
        return -1;
    }
    return msg;
}

// Returns the mtu attached to the header.
int my_host :: get_mtu(char **buff_in){
    char *temp = strtok(*buff_in, ":");
    return atoi(temp);
}

int my_host :: get_num_pkts(char *temp, char *buff_in){
    try{
        memcpy(temp, buff_in, 10);
        int r = atoi(temp);
        if((r == 0) || (r == -1)){
            return MAXSIZE_PKT;
        }
        return atoi(temp);
    }
    catch(...){
        exit(-1);
    }
}

// Returns the sequence number of a packet that is attached the header.
int my_host :: get_SN(char *temp, char *buff_in, int offset, int num_read){
    try{
        memcpy(temp, buff_in + offset, num_read);
        return atoi(temp);
    }
    catch(...){
        exit(-1);
        return -1;
    }
}

// Removes the header from the packet and returns the char* without the header.
char* my_host :: rm_SN(char *temp, char *buff_in, int offset, int num_read){
    try{
        memcpy(temp, buff_in + offset, num_read);
        return temp;
    }
    catch(...){
        exit(-1);
        return NULL;
    }
}

int my_host :: add_header(char *temp, char *total_pkts, char *s_SN, char *file_r, int fi_re){
    int tot = 0;
    memcpy(temp, total_pkts, 10);
    tot += 10;
    temp[tot] = ':';
    tot += 1;
    memcpy(temp + (tot), s_SN, 10);
    tot += 10;
    temp[tot] = '\n';
    tot += 1;
    memcpy(temp + (tot), file_r, fi_re);
    tot += fi_re;
    return tot;
}

int my_host :: add_win(char *buff, int in_size, int base_SN, int winsize){
    char *temp = new char[in_size+1]{'\0'};
    memcpy(temp, buff, in_size);
    memset(buff, '\0', in_size+1);
    char *s_base_SN = z_buff(base_SN, 10);

    int off = 0;
    memcpy(buff+off, s_base_SN, 10);
    off += 10;
    char *s_winsize = z_buff(winsize, 10);
    memcpy(buff+off, s_winsize, 10);
    off += 10;
    memcpy(buff+off, temp, in_size);
    off += in_size;
    delete[] temp;
    return off;
}

int my_host :: num_dig(int n){
    try{
        if (n == 0){
            return 1;
        }
        int count = 0;
        while (n != 0) {
            n = n / 10;
            ++count;
        }
        return count;
    }
    catch(...){
        cerr << "ERROR getting the number of digites in SN.\n";
        exit(-1);
    }
}

// This functions pads integers with zeros and returns a char * that is 10 characters long.
char* my_host :: z_buff(int n, int amount){
    try{
        int r = num_dig(n);
        string a = to_string(n);
        while(r < amount){
            a = "0" + a;
            r += 1;
        }
        char *int_out = strdup(a.c_str());
        return int_out;
    }
    catch(...){
        cerr << "ERROR Padding header\n";
        exit(-1);
    }
}

// Function is used to print the log of packets for both the Server and the Client.
void format_time(char *buff, int type, int SN, int base_SN, int nxt_SN, int bpw, int lport, int rport, char *rip, int from){
    time_t curr_time;
    struct timeval time_mil;
    struct tm *gm_curr_time;
    gettimeofday(&time_mil, NULL);
    time(&curr_time);
    int millisec = lrint(time_mil.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        time_mil.tv_sec++;
    }

    gm_curr_time = gmtime(&curr_time);
    strftime(buff, 100, "%Y-%m-%dT%H:%M:%S", gm_curr_time);

    int len_time = strlen(buff);
    char *temp = new char[200]{'\0'};
    memcpy(temp, buff, len_time);

    string s_SN = to_string(SN);
    char *c_SN = strdup(s_SN.c_str());
    int size_SN = strlen(c_SN);

    string s_base_SN = to_string(base_SN);
    char *c_base_SN = strdup(s_base_SN.c_str());
    int size_base_SN = strlen(c_base_SN);

    string s_nxt_SN = to_string(nxt_SN);
    char *c_nxt_SN = strdup(s_nxt_SN.c_str());
    int size_nxt_SN = strlen(c_nxt_SN);

    string s_bpw = to_string(bpw);
    char *c_bpw = strdup(s_bpw.c_str());
    int size_bpw = strlen(c_bpw);
    memset(buff, '\0', 100);
    sprintf(buff, "%s.%03d", temp, millisec);
    delete[] temp;
    len_time = strlen(buff);

    const char* z = "Z, ";
    memcpy(buff+len_time, z, 3);
    len_time += 3;
    string s_lport = to_string(lport);
    char *c_str_lport = strdup(s_lport.c_str());
    memcpy(buff+len_time, c_str_lport, strlen(c_str_lport));
    len_time += strlen(c_str_lport);
    const char* com_space = ", ";
    memcpy(buff+len_time, com_space, 2);
    len_time += 2;
    memcpy(buff+len_time, rip, strlen(rip));
    len_time += strlen(rip);
    memcpy(buff+len_time, com_space, 2);
    len_time += 2;

    string s_rport = to_string(rport);
    char *c_str_rport = strdup(s_rport.c_str());
    memcpy(buff+len_time, c_str_rport, strlen(c_str_rport));
    len_time += strlen(c_str_rport);
    memcpy(buff+len_time, com_space, 2);
    len_time += 2;

    if(type == 0){
        const char *a ="ACK, ";
        memcpy(buff+len_time, a, 5);
        len_time += 5;
    }
    else{
        const char *a = "DATA, ";
        memcpy(buff+len_time, a, 6);
        len_time += 6;
    }
    memcpy(buff+len_time, c_SN, size_SN);
    len_time += size_SN;
    if(from == CLIENT_IN){
        buff[len_time] = ',';
        len_time += 1;
        buff[len_time] = ' ';
        len_time += 1;
        memcpy(buff+len_time, c_base_SN, size_base_SN);
        len_time += size_base_SN;
        buff[len_time] = ',';
        len_time += 1;
        buff[len_time] = ' ';
        len_time += 1;
        memcpy(buff+len_time, c_nxt_SN, size_nxt_SN);
        len_time += size_nxt_SN;
        buff[len_time] = ',';
        len_time += 1;
        buff[len_time] = ' ';
        len_time += 1;
        memcpy(buff+len_time, c_bpw, size_bpw);
        len_time += size_bpw;
    }
    cout << buff << "\n";
    

    return;
}