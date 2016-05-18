#include "irc_proto.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include "sircd.h"
#include <stdio.h>


#define MAX_COMMAND 16
int chmax=0;
void NickChangeMessage(char * old,char * new);
char *trimwhitespace(char *str);
int isspace(int c);
void makeMessage(char *makemessage);
void generateError(int errorCode,char *parameter);


#define CMD_ARGS char *prefix, char **params, int n_params
typedef void (*cmd_handler_t)(CMD_ARGS);
#define COMMAND(cmd_name) void cmd_name(CMD_ARGS)
    

struct dispatch {
    char cmd[MAX_COMMAND];
    int needreg; /* Must the user be registered to issue this cmd? */
    int minparams; /* send NEEDMOREPARAMS if < this many params */
    cmd_handler_t handler;
};


#define NELMS(array) (sizeof(array) / sizeof(array[0]))

//macro for comand handlers
COMMAND(cmd_nick);
COMMAND(cmd_user);
COMMAND(cmd_quit);
COMMAND(cmd_join);
COMMAND(cmd_list);
COMMAND(cmd_part);
COMMAND(cmd_privmsg);
COMMAND(cmd_who);

/* Dispatch table.  "reg" means "user must be registered in order
 * to call this function".  "#param" is the # of parameters that
 * the command requires.  It may take more optional parameters.
 */
struct dispatch cmds[] = {
    /* cmd,    reg  #parm  function */
    { "NICK",    0, 1, cmd_nick },
    { "USER",    0, 4, cmd_user },
    { "QUIT",    1, 0, cmd_quit },
    { "JOIN",    1, 1, cmd_join },
    { "LIST",    0, 0, cmd_list },
    { "PART",    1, 1, cmd_part },
    { "PRIVMSG", 1, 1, cmd_privmsg },
    { "WHO",     1, 1, cmd_who },
};

/* Handle a command line.  NOTE:  You will probably want to
 * modify the way this function is called to pass in a client
 * pointer or a table pointer or something of that nature
 * so you know who to dispatch on...
 * Mostly, this is here to do the parsing and dispatching
 * for you
 *
 * This function takes a single line of text.  You MUST have
 * ensured that it's a complete line (i.e., don't just pass
 * it the result of calling read()).  
 * Strip the trailing newline off before calling this function.
 */

void
handle_line(char *line)
{

    char *prefix = NULL, *command=NULL, *pstart, *params[MAX_MSG_TOKENS];
    int n_params = 0;
    char *trailing = NULL;

    DPRINTF(DEBUG_INPUT, "Handling line: %s\n", line);
    command = line;
    if (*line == ':') {
	prefix = ++line;
	command = strchr(prefix, ' ');
    }
    if (!command || *command == '\0') {
	/* Send an unknown command error! */
        //some_of_your_code_better_go_here();
	return;
    }
    
    while (*command == ' ') {
	*command++ = 0;
    }
    if (*command == '\0') {
        //and_more_of_your_code_should_go_here();
	/* Send an unknown command error! */
	return;
    }
    command=trimwhitespace(command);
    pstart = strchr(command, ' ');
    if (pstart) {
	while (*pstart == ' ') {
	    *pstart++ = '\0';
	}
	if (*pstart == ':') {
	    trailing = pstart;
	} else {
	    trailing = strstr(pstart, " :");
	}
	if (trailing) {
	    while (*trailing == ' ')
		*trailing++ = 0;
	    if (*trailing == ':')
		*trailing++ = 0;
	}
	
	do {
	    if (*pstart != '\0') {
		params[n_params++] = pstart;
	    } else {
		break;
	    }
	    pstart = strchr(pstart, ' ');
	    if (pstart) {
		while (*pstart == ' ') {
		    *pstart++ = '\0';
		}
	    }
	} while (pstart != NULL && n_params < MAX_MSG_TOKENS);
    }

    if (trailing && n_params < MAX_MSG_TOKENS) {
		params[n_params++] = trailing;
    }
    
    DPRINTF(DEBUG_INPUT, "Prefix:  %s\nCommand: %s\nParams (%d):\n",
	    prefix ? prefix : "<none>", command, n_params);
    int i;
    for (i = 0; i < n_params; i++) {
	DPRINTF(DEBUG_INPUT, "   %s\n", params[i]);
    }
    DPRINTF(DEBUG_INPUT, "\n");

    for (i = 0; i < NELMS(cmds); i++) {
	if (!strcasecmp(cmds[i].cmd, command)) {
		//check if command requires registration
	    if (cmds[i].needreg && client_array[current_fd].registered!=1){
				generateError(451,command);
		//check if client didn't specify enough parameters	
	    } else if (n_params < cmds[i].minparams) {
	    	generateError(461,command);
	    } else {
		/* Here's the call to the cmd_foo handler... modify
		 * to send it the right params per your program
		 * structure. */
			(*cmds[i].handler)(prefix, params, n_params);
	    }
	    break;
	}
    }
    if (i == NELMS(cmds)) {
	/* ERROR - unknown command! */
    	generateError(421,command);
        //yet_again_you_should_put_code_here();
    }
    return;
}

/* Command handlers */
void cmd_nick(char *prefix, char **params, int n_params)
{
	int k;
	int isFound=0;
	//check if nick already exists
	for(k=0;k<=fdmax;k++){
		if((strcmp(client_array[k].nick, params[0]))==0){ 
			generateError(433,params[0]);
			isFound=1;
		}
	}
	//if Nick doesn't match, then create a new nick
	if (isFound ==0){ 
		//if Client is changing nick, then inform other clients on the channel
		if (strlen(client_array[current_fd].nick)!=0)
			NickChangeMessage(client_array[current_fd].nick,params[0]);

		strcpy(client_array[current_fd].nick,params[0]);
		printf("New Nick is %s\n",client_array[current_fd].nick);
		fflush(stdout);
	}
	//Register the user
	client_array[current_fd].nickRegistered=1;
	if(client_array[current_fd].nickRegistered && client_array[current_fd].userRegistered)
		client_array[current_fd].registered=1;

	return;
}

void cmd_user(char *prefix, char **params, int n_params)
{

	strcpy(client_array[current_fd].user,params[0]);
	strcpy(client_array[current_fd].hostname,params[1]);
	strcpy(client_array[current_fd].servername, params[2]);
	strcpy(client_array[current_fd].realname, params[3]);
	//Register the user
	client_array[current_fd].userRegistered =1;
	if(client_array[current_fd].nickRegistered && client_array[current_fd].userRegistered)
		client_array[current_fd].registered=1;

	printf("New User connected with RealName: %s, HostName: %s, ServerName: %s, UserName: %s\n",
		client_array[current_fd].realname,client_array[current_fd].hostname,
		client_array[current_fd].servername,client_array[current_fd].user);
	fflush(stdout);

	return;
}

void cmd_quit(char *prefix, char **params, int n_params)
{
	//close the User's socket
	close(current_fd);
    FD_CLR(current_fd, &master);
    //quit the channel(s) joined by the user
    char *quitChannel[1];
	quitChannel[0]=client_array[current_fd].channel;
	cmd_part(prefix,quitChannel,1);
	//clear the record of the user from Client array
	bzero (&client_array[current_fd], sizeof(client_array[current_fd]));

	printf("Connection closed on %d\n",current_fd);
	fflush(stdout);

	return;
}

void cmd_join(char *prefix, char **params, int n_params)
{
	//verify if the protocol for Join has been followed
	if(params[0][0]=='#'){
		if (strcmp(client_array[current_fd].channel, "")!=0){
			//quit the already joined channel 
			char *oldChannel[1];
			oldChannel[0]=client_array[current_fd].channel;
			cmd_part(prefix,oldChannel,1);
		}
		int chexists=0; //CHECK IF THE CHANNEL EXISTS IN CHANNEL array
		int i;
		//Case 1: Requested Channel already exists
		for (i=0;i<chmax;i++){		
			if(!strcmp(channel_array[i].channelName,params[0])){
				char makemessage[MAX_MSG_LEN];
				makeMessage(makemessage);
				strcat(makemessage," JOIN : NEW USER!\n");
				int k;
				for(k=0;k<=fdmax;k++){
					if (strcmp(client_array[k].channel,params[0])==0){
						write(k,makemessage,strlen(makemessage));
					}
				}
				//increment the number of users in that channel
				channel_array[i].numOfUsers +=1;
				chexists = 1;
			}	
		}
		//Case 2: Channel doesnot exist
		if(chexists==0){
			//Then add the New Channel to the Channel array
			channel_array[chmax].numOfUsers=0;
			strcpy(channel_array[chmax].channelName,params[0]);
			channel_array[chmax].numOfUsers +=1;
			chmax+=1;
		}
		//Update the Client's channel status
		strcpy(client_array[current_fd].channel,params[0]);//add the user's channel to the requested
		client_array[current_fd].isChannel=1;

	} else{ //Generate Error if # omitted in commmand
		generateError(1,params[0]);
	}

	return;
}

void cmd_list(char *prefix, char **params, int n_params){
	int i;
	for (i=0;i<chmax;i++){
		char listMessage[MAX_MSG_LEN];
		strcpy(listMessage,"Channel Name: ");
		strcat(listMessage,channel_array[i].channelName);
		strcat(listMessage,", Number of Clients: ");
		
		//send the channel name to the calling user
		write(current_fd,listMessage,strlen(listMessage));
		
		sprintf(listMessage, "%d", channel_array[i].numOfUsers);
		strcat(listMessage,"\n");
		
		//send the No of clients on that channel to the calling user
		write(current_fd,listMessage,strlen(listMessage));
	}
	//In case, there exist no channels
	if(chmax==0){
		char listMessage[MAX_MSG_LEN];
		strcpy(listMessage,"There are no channels!\n");
		write(current_fd,listMessage,strlen(listMessage));
	}
	return;
}

void cmd_part(char *prefix, char **params, int n_params){
	int i,j;
	//check for multiple arguments
	for(j=0;j<n_params;j++){
		//check if user is part of that channel
		if(!strcmp(client_array[current_fd].channel,params[j])){
			for (i=0;i<chmax;i++){
				//locate the required channel
				if (!strcmp(channel_array[i].channelName,params[j])){
					//Decrement the no of users in that channel
					channel_array[i].numOfUsers -=1;
					char makemessage[MAX_MSG_LEN];
					makeMessage(makemessage);
					strcat(makemessage," QUIT : Connection Closed in Channel ");
					strcat(makemessage,channel_array[i].channelName);
					strcat(makemessage,".\n");
					int k;
					for(k=0;k<=fdmax;k++){
						//Send Quit message to all clients in the targeted channel
						if (strlen(client_array[current_fd].channel)!=0){
							if (strcmp(client_array[k].channel,params[j])==0){
								write(k,makemessage,strlen(makemessage));
							}
						}
					}
					//Delete the Channel if no clients connected to it
					if (channel_array[i].numOfUsers==0){
						int c;
						for (c = i; c < chmax; c++ )
				 			channel_array[c] = channel_array[c+1];
						chmax -=1;
					}
					//Clear the Client's Channel record
					bzero (&client_array[current_fd].channel, sizeof(client_array[current_fd].channel));
				}
			}

		}else{ //user is not part of the channel
			generateError(441,params[j]);
		}
	}
	return;
}

void cmd_privmsg(char *prefix, char **params, int n_params){
	int c,chExits=0;
	//Make the message for target client
	char makemessage[MAX_MSG_LEN];
	makeMessage(makemessage);
	strcat(makemessage," PRIVMSG :");
	//concatenate all the message params
	for(c=1;c<n_params;c++){
		strcat(makemessage,params[c]);
		strcat(makemessage," ");
	}
	strcat(makemessage,"\n");
	//check if message is for a channel
	if (params[0][0]=='#'){
		for(c=0;c<=chmax;c++){
			if(!strcmp(channel_array[c].channelName,params[0])){
				chExits=1;
			}
		}
		//check if the channnel exits
		if(chExits==0){
			generateError(403,params[0]);
			return;
		}
		//check if user is not on that channel
		if(strcmp(client_array[current_fd].channel,params[0])){
			generateError(441,params[0]);
			return;
		}
		if(n_params==1){
			generateError(412,"ERROR");
		}
		//write to all included in that channel
		for(c=0;c<=fdmax;c++){
			if (strlen(client_array[c].channel)!=0){ 
				if (c!=current_fd && strcmp(client_array[c].channel,params[0])==0){
					write(c,makemessage,strlen(makemessage));
				}
			}	
		}
	//the message is for a user, not channel
	}else{
		int j,nickExists=0;
		for(c=0;c<=fdmax;c++){
			//write to the target client
			if(!strcmp(client_array[c].nick,params[0])){
				j=c;//store the value of FD of targeted client
				nickExists =1;
			}
		}
		//check if the target client even exists
		if (nickExists != 1)
			generateError(401,params[0]);
		//write to target client is message has text
		if(n_params==1){
			generateError(412,"ERROR");
		}else{
			write(j,makemessage,strlen(makemessage));
		}
	}
	return;	
}

void cmd_who(char *prefix, char **params, int n_params){

	int c,isFound=0;
	char message[MAX_MSG_LEN];
	for(c=0;c<chmax;c++){
		if(!strcmp(channel_array[c].channelName,params[0]))
			isFound=1;
	}
	if(isFound!=1){
		generateError(403,params[0]);
		return;
	}
	strcpy(message,"The clients on this channel are: ");
	for(c=0;c<=fdmax;c++){
		if(!strcmp(client_array[c].channel,params[0])){
			strcat(message,client_array[c].nick);
			strcat(message,", ");
		}
	}
	strcat(message,"\n");
	write(current_fd,message,strlen(message));
		
	return;

}


void NickChangeMessage(char * old,char * new){
	char message[MAX_MSG_LEN];
	strcpy(message,":");
	strcat(message,trimwhitespace(old));
	strcat(message," NICK ");
	strcat(message,trimwhitespace(new));
	strcat(message,"\n");
	int i;
	for(i=0;i<=fdmax;i++){
		if (strlen(client_array[current_fd].channel)!=0){ //check if user is registered
			if (i!=current_fd && (strcmp(client_array[i].channel,client_array[current_fd].channel)==0)){
				write(i,message,strlen(message));
			}
		}	
	}
	return;
}
void makeMessage(char * makemessage){
	strcpy(makemessage,":");
	strcat(makemessage,trimwhitespace(client_array[current_fd].nick));
	strcat(makemessage,"!");
	strcat(makemessage,trimwhitespace(client_array[current_fd].user));
	strcat(makemessage,"@");
	strcat(makemessage,trimwhitespace(client_array[current_fd].hostname));
	return;
}

void generateError(int errorCode,char *parameter){
	char message[MAX_MSG_LEN];
	switch(errorCode){
		case 401 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :NO SUCH NICK\n");
		write(current_fd,message,strlen(message));
		break;
		case 403 :
		strcpy(message,"ERROR:");
		strcat(message,parameter);
		strcat(message," :NO SUCH CHANNEL\n");
		write(current_fd,message,strlen(message));
		break;
		case 433 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :NICK NAME IN USE\n");
		write(current_fd,message,strlen(message));
		break;
		case 441 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :NOT ON CHANNEL\n");
		write(current_fd,message,strlen(message));
		break;
		case 1 :
		strcpy(message,"");
		strcat(message,parameter);
		strcat(message," :ERR_INVALID\n");
		write(current_fd,message,strlen(message));
		break;
		case 451 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :NOT REGISTERED\n");
		write(current_fd,message,strlen(message));
		break;
		case 461 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :NEED MORE PARAMS\n");
		write(current_fd,message,strlen(message));
		break;
		case 421 :
		strcpy(message,"ERROR: ");
		strcat(message,parameter);
		strcat(message," :UNKNOWN COMMAND\n");
		write(current_fd,message,strlen(message));
		break;
		case 412 :
		strcpy(message,"");
		strcat(message,parameter);
		strcat(message," :NO TEXT TO SEND\n");
		write(current_fd,message,strlen(message));
		break;
	}
}

char *trimwhitespace(char *str)
{
  char *end;
  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/* And so on */

