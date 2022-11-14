
#include "../inc/func.h"

extern int socket_fd;
extern int num_client;




int create_socket(int s_port){
    struct sockaddr_in server_addr;
    // socket creation
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == -1){
        printf("\nSocket %d creation failed.\n",s_port);
    }else {
        printf("\nSocket %d created succesfully.\n",s_port);
    }

    bzero(&server_addr, sizeof(server_addr));

    // IP address and port number assignment
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(s_port);

    // binding
    if (bind(socket_fd, (SA *) (&server_addr), sizeof(server_addr)) == -1){
        printf("Binding failed x\n");
    }else{
        printf("Binding success √\n");
    }
    return socket_fd;
}

char* dirfile(){
    char buffer[BUFFER_SIZE];
    bzero(buffer,sizeof(buffer));
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (DIRECTORY)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(strcmp(ent->d_name,".") != 0 + strcmp(ent->d_name,"..") != 0 == 0){
                strcat(buffer,ent->d_name);
                strcat(buffer,"\n");
            }
        }
        return strcat(buffer,"");
        closedir (dir);
        }   
    else {
    /* could not open directory */
        return strcat(buffer,"406");
    }
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

int prob(int p){ 
    return random()%100 < p;
}


