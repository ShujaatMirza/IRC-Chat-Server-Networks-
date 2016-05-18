#ifndef _SIRCD_H_
#define _SIRCD_H_

#include <sys/types.h>
#include <netinet/in.h>

#define MAX_CLIENTS 512
#define MAX_MSG_TOKENS 10
#define MAX_MSG_LEN 512
#define MAX_USERNAME 32
#define MAX_NICKNAME 512
#define MAX_HOSTNAME 512
#define MAX_SERVERNAME 512
#define MAX_REALNAME 512
#define MAX_CHANNAME 512
#define MAX_CHANNELS 512

int current_fd; // the file descriptor under consideration
fd_set master;    // master file descriptor list
int fdmax;// maximum file descriptor number
int chmax;// number of channels in channel_array


typedef struct {
    int sock;
    struct sockaddr_in cliaddr;
    unsigned inbuf_size;
    int registered;
    int userRegistered;
    int nickRegistered;
    char hostname[MAX_HOSTNAME];
    char servername[MAX_SERVERNAME];
    char user[MAX_USERNAME];
    char nick[MAX_NICKNAME];
    char realname[MAX_REALNAME];
    char inbuf[MAX_MSG_LEN+1];
    char channel[MAX_CHANNAME];
    int isChannel;
} client;

client client_array[MAX_CLIENTS];

typedef struct{
    char channelName[MAX_CHANNAME];
    int numOfUsers;
} channel;

channel channel_array[MAX_CHANNELS];

#endif /* _SIRCD_H_ */
