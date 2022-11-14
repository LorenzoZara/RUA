#include "../inc/func.h"


extern int socket_fd;
extern int num_client;

/*
int prob(int p);

    Description:    It's a probability function

    Arguments:      - p : percentage probability measure {0, 100}

    Return values:  Returns a boolean determined by p parameter
                    With p% of probability this function returns 0
                    With (1-p)% of probability this function returns 1
*/
int prob(int p){
    return random() % 100 < p;
}

/*
int create_socket(int s_port, struct sockaddr_in server_addr);

    Descriptio:     Creates a socket

    Arguments:      - s_port : port to create socket on
                    - server_addr : struct that contains server infos

    Return values:  On success returns the socket descriptor,
                    otherwise returns -1.
*/
int create_socket(int s_port, struct sockaddr_in server_addr){
    
    // socket creation
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == -1){
        printf("\nSocket %d creation failed.\n",s_port);
    }else {
        printf("\nSocket %d created succesfully.\n",s_port);
    }
    // struct flushing
    bzero(&server_addr, sizeof(server_addr));

    // IP address and socket port assignment
	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = inet_addr(ADDRESS); 
	server_addr.sin_port = htons(PORT);

    return socket_fd;
}

char* ispresent(char* file_name){
    char file[30]= DIRECTORY;
    char ok[50]="200 ";
    char sizestr[25];

    strcat(file,file_name);

    int fd=open(file,O_RDONLY, 0666);
    int size=lseek(fd,0,SEEK_END);
    
    bzero(sizestr,sizeof(sizestr));
    sprintf(sizestr,"%d",size);
    if(fd==-1){
        return "404";
    }
    else{
        return strcat(ok  , sizestr);
    }
    close(fd);
}

//display commands
void display(){
    printf("\nChoose an operation:\n\n"
			"- list         [Usage >> list]\n"
			"- download     [Usage >> get filename]\n"
			"- upload       [Usage >> put filename]\n"
            "- exit         [Usage >> exit]\n\n");
}

//display request message
void display_request_message(char *cmd, char *filename, char *file_len){

    printf("\nREQUEST MESSAGE:\n\n");
    
    printf("\n__________________________________________________ \n"
             "                   |                               \n"
             "   C O M M A N D   |   %s                          \n"
             " __________________|______________________________ \n"
             "                   |                               \n"
             "  F I L E  N A M E |   %s                          \n"
             " __________________|______________________________ \n"
             "                   |                               \n"
             "    L E N G T H    |   %s                          \n"
             " __________________|______________________________ \n\n",

        cmd, filename, file_len);

}

//display response message
void display_response_message(char *cmd, char *filename, char *file_len){

    printf("\nRESPONSE MESSAGE:\n\n");
    
    printf("\n__________________________________________________ \n"
             "                   |                               \n"
             "   STATUS  CODE    |   %s                          \n"
             " __________________|______________________________ \n"
             "                   |                               \n"
             "  F I L E  N A M E |   %s                          \n"
             " __________________|______________________________ \n"
             "                   |                               \n"
             "    L E N G T H    |   %s                          \n"
             " __________________|______________________________ \n\n",
    
        cmd, filename, file_len);

}