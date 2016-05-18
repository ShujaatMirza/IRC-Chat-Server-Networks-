#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>
#include "debug.h"
#include "rtlib.h"
#include "sircd.h"
#include "irc_proto.h"


int curr_nodeID;
rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */

void init_node(char *nodeID, char *config_file);
void irc_server();
void closeconnection(int current_fd,int fdmax,fd_set* master);

void usage() {
    fprintf(stderr, "sircd [-h] [-D debug_lvl] <nodeID> <config file>\n");
    exit(-1);
}

int main( int argc, char *argv[] )
{
    extern char *optarg;
    extern int optind;
    int ch;

    while ((ch = getopt(argc, argv, "hD:")) != -1)
        switch (ch) {
    case 'D':
        if (set_debug(optarg)) {
            exit(0);
        }
        break;
        case 'h':
        default: /* FALLTHROUGH */
            usage();
        }
    argc -= optind;
    argv += optind;

    if (argc < 2) {
        usage();
    }

    init_node(argv[0], argv[1]);

    printf( "I am node %d and I listen on port %d for new users\n", curr_nodeID, curr_node_config_entry->irc_port );

    /* Initializing elements here */

    fd_set read_fds; // temp file descriptor list for select()   
    int port = curr_node_config_entry->irc_port;
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[512];    // buffer for client data
    int nbytes;
    struct sockaddr_in server;
    int i, j;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    //create a TCP socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener==-1)
    {
        perror("socket: ");
        exit(-1);
    }
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;   
    int len = sizeof(struct sockaddr_in);
     //Bind the socket
    if(bind(listener, (struct sockaddr *)&server, len) ==-1)
    {
        perror("bind");
        exit(-1);
    }
    //socket is converted to listening socket
    if(listen(listener,10)==-1)
    {
        perror("listen");
        exit(-1);
    }
    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    while(1){

    read_fds = master; // copy it
    
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("select error");
        exit(4);
    }
    bzero(buf,512);

     // loop through file descriptors
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { 
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept error");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from on socket %d\n",newfd);
                        
                    }
                } else {
                    // handle data from a client
                    char longbuf[1024];
                    bzero(longbuf,1024);
                    if ((nbytes = read(i, longbuf, sizeof longbuf)) <= 0) {
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            char hungup[512];
                            strcpy(hungup,"QUIT");
                            handle_line(hungup);
                            /* CLOSE CLIENT SOCKET AND REMOVE THE CLIENT FROM SERVER*/
                            close(i);
                        } else {
                    // got error
                            perror("recv error");
                        }
                        
                        FD_CLR(i, &master); // remove from master set
                    } 
                    else {

                        // client data 
                        current_fd =i; //assign the current file descriptor to Global Variable
                        
                        //Code to tackle if user sends data greater than 512 bytes    
                        if (strlen(longbuf)>512){
                            strncpy(buf,longbuf,510);
                            handle_line(buf);
                        }else{
                            handle_line(longbuf);
                        }

                        

                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                    
                                    /*if (write(j, buf, nbytes) == -1) {
                                       perror("send error");
                                    }*/
                                }
                            }
                        }
                    }
                } 
            } 
        } 

    }

    return 0;
}

/*
 * void init_node( int argc, char *argv[] )
 * Takes care of initializing a node for an IRC 
 * from the given command line arguments
 */
void init_node(char *nodeID, char *config_file) 
{
    int i;

    curr_nodeID = atol(nodeID);
    rt_parse_config_file("sircd", &curr_node_config_file, config_file );

    /* Get config file for this node */
    for( i = 0; i < curr_node_config_file.size; ++i ){
        if( curr_node_config_file.entries[i].nodeID == curr_nodeID )
             curr_node_config_entry = &curr_node_config_file.entries[i];
    }
    /* Check to see if nodeID is valid */
    if( !curr_node_config_entry )
    {
        printf( "Invalid NodeID\n" );
        exit(1);
    }
}

