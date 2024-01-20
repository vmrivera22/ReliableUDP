int get_SN(char*, char*, int, int);
char* rm_SN(char*, char*, int, int);
int add_header(char*, char*, char*, char*, int);
int add_win(char*, int, int, int);
int get_num_pkts(char*, char*);
int get_mtu(char**);
char* z_buff(int, int);
int num_dig(int);
int port_v_check(char*);
int mtu_v_check(char*, int);
int winsz_v_check(char*);
bool dig_check(char*);
bool IP_v_check(char*);

// This function checks to see if the string input is composed of only numbers.
bool dig_check(char *a){
    int len_a = strlen(a);
    for(int i = 0; i < len_a; i+=1){
        if(isdigit(a[i]) == false){
            return false;
        }
    }
    return true;
}

// This function checks the validity of the winsz input.
int winsz_v_check(char *s_winsz){
    bool dig_win = dig_check(s_winsz);
    if(dig_win == false){
        cerr << "Window Size input is not valid.\n\n";
        exit(-1);
    }
    int num_winsz = atoi(s_winsz);
    return num_winsz;
}

// Function checks if a port is valid, if it is not then it exits.
int port_v_check(char *s_port){
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

// This function checks if a port is valid and returns a false if it is not and true if it is.
bool port_bool_check(char *s_port){
    if(s_port == NULL){
        cerr << "No port was entered.\n\n";
        return false;
    }
    bool dig_p = dig_check(s_port);
    if(dig_p == false){
        cerr << "Port input is not valid.\n\n";
        return false;
    }
    int port_in = atoi(s_port);
    if(port_in > 65536 || port_in < 1025){
        cerr << "Port out of acceptable range. Port has to be between 1025 and 65536.\n\n";
        return false;
    }
    return true;
}

// This function is used to check if the input MTU is valid.
int mtu_v_check(char *s_mtu, int head_len){
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

// This function is used to get the MTU from the header of a packet.
int get_mtu(char **buff_in){
    char *temp = strtok(*buff_in, ":");
    return atoi(temp);
}

// This function is used to get the number of packets from the header of a packet.
int get_num_pkts(char *temp, char *buff_in){
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
int get_SN(char *temp, char *buff_in, int offset, int num_read){
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
char* rm_SN(char *temp, char *buff_in, int offset, int num_read){
    try{
        memcpy(temp, buff_in + offset, num_read);
        return temp;
    }
    catch(...){
        exit(-1);
        return NULL;
    }
}

// This function is used to add the header of a packet including the SN.
int add_header(char *temp, char *total_pkts, char *s_SN, char *file_r, int fi_re){
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

// This functions is used to add the window size to the header of a packet
int add_win(char *buff, int in_size, int base_SN, int winsize){
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

// This function gets the number of digits in a number.
int num_dig(int n){
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
char* z_buff(int n, int amount){
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

// Checks the Validity of an IP address.
bool IP_v_check(char *IP){
    string temp = IP;
    int IP_len = strlen(IP);
    if(IP_len > 15){ // If an IP address is larger than 15 characters then it is invalid 255.255.255.255 is largest
        cerr << "Invalid IP\n";
        return false;
    }
    char *part = strtok(IP, ".");
    for(int i = 0; i < 4; i += 1){
        if(strlen(part) > 3){ // If any part (divided by the "."s) is larger than 3 then it is invalid (255 is largest)
            cerr << "Invalid IP\n";
            return false;
        }
        bool is_dig_all = dig_check(part);
        if(is_dig_all == false){ // If any part is not an digit (excluding the ".") then the IP is invalid.
            cerr << "Invalid IP\n";
            return false;
        }
        if(atoi(part) > 255 || atoi(part) < 0){ // If any part is not between 0 and 255 then it is not valid.
            cerr << "Invalid IP\n";
            return false;
        }
        if(i != 3){
            part = strtok(NULL, ".");
        }
    }
    return true;
}