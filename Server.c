//Server.c 
#include <stdio.h>
#include <string.h>     //strlen
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>     //fork(), access(2)
#include <sys/wait.h>   //wait()

int OpenSocket(int port);
int ListenIncomingConnection(int sock_fd);
int AcceptConnections(int sock_fd);
char* RecieveData(int newSocket);
int SendData(int sock_fd, int newSocket, int data_recieved);
int parse_client_data(char* reply_buffer);

typedef struct sockaddr_in sockaddr_in;

int main(int argc , char *argv[])
{
  (void)argc;
  (void)argv;
  // create socket
	int port = 10732;
	printf( "creating socket on port %d\n", port );
    int sock_fd = OpenSocket(port); //bind
    if(sock_fd != -1){
       printf("Connected\n");
       //Accept the incoming connection
       AcceptConnections(sock_fd);
    }
     
    return 0;
}

int OpenSocket(int port)
{   
   struct sockaddr_in socketinfo; //https://msdn.microsoft.com/en-us/library/aa917469.aspx
   //create socket
   int sock_fd = socket( PF_INET, SOCK_STREAM, 0); 
   if(sock_fd == -1)
   {
   	  printf("Could not create socket.\n");
        return -1;
   }

   //socket binds to localhost
   socketinfo.sin_addr.s_addr = inet_addr("127.0.0.1"); 
   //in internet family
   socketinfo.sin_family = AF_INET;
   //connect socket to the port
   socketinfo.sin_port = htons(port); 

   //make pointer to socket info
   struct sockaddr_in* info_ptr = &socketinfo;
   //convert type
   struct sockaddr* address_info = (struct sockaddr*)info_ptr;

   //bind this remote server socket to port
   if (bind( sock_fd, address_info, sizeof(socketinfo) ) < 0 )
   {
      printf( "Failed to bind socket.\n" );
      return -1;
   }

   return sock_fd;
}

int ListenIncomingConnection(int sock_fd){
   //Listen for incoming connections. A max of 1 connection can happen.
   if(listen(sock_fd,1) < 0){
       printf("Error\n");
       return -1;
    }
    printf("Listening\n");
    return 0;
}

char* RecieveData(int newSocket){
    printf("about to recieve data\n");
    //get the incoming message from the client.
    char reply_buffer[256];
    //clear buffer before writing to it
    memset(reply_buffer, 0, sizeof(reply_buffer)); //<<<< We may need to do this in other places !
    // recv() will block until there is some data to read.
    if(recv(newSocket, reply_buffer, 256, 0) < 0)
    {
        printf("Failed to recieve message.\n");
        return "-1";
    }else{
      printf("Data recieved from client is: %s\n",reply_buffer);
      if(strcmp(reply_buffer, "quit\n") == 0){
           printf("EXITING NOW\n");
           close(newSocket);
           return "-1";
       }
       //malloc this shit
       char* returnMe = malloc(sizeof(char)*strlen(reply_buffer));
       strcpy(returnMe, reply_buffer);
       return returnMe; // 0 maps to other
    }
}

int parse_client_data(char* reply_buffer){
   char* command = malloc(sizeof(char)*100);
   char* name = malloc(sizeof(char)*100);
   char* length = malloc(sizeof(char)*100);
   char* size = malloc(sizeof(char)*100);
   char* key = malloc(sizeof(char)*100);
   char* value = malloc(sizeof(char)*100); 

   sscanf(reply_buffer, "<cmd>%[^<]</cmd><name>%[^<]</name><length>%[^<]</length><size>%[^<]</size><key>%[^<]</key><value>%[^<]</value>",
    command, name, length,size, key, value);
   printf("command=\"%s\"\nname=\"%s\"\nlength=\"%s\"\nsize=\"%s\"\nkey=\"%s\"\nvalue=\"%s\"\n", command, name, length, size, key, value);

   return 1;
}

int AcceptConnections(int sock_fd){
    while(1){
      //Listen for an incoming connection
      if(ListenIncomingConnection(sock_fd) == -1) printf("Error while listening\n");

      printf("about to fork 2 processes - child and parent. \n");
      struct sockaddr_in newclient; //accept creates a new socket
      socklen_t size = sizeof newclient;
      int newSocket = 0;
      //waiting to accept a connection
      newSocket = accept(sock_fd, (struct sockaddr *) &newclient, &size);
      int pid = fork();
      if(pid == 0) { //child process
         printf("in child!!!\n");
         // loop for an ongoing conversation with the client
         while(1){ 
            // 0 maps to other, 1 maps to no, 2 maps to yes
            int status = parse_client_data(RecieveData(newSocket)); //we can change the return value to a char* but then we would have to allocate memory
            SendData(sock_fd, newSocket, status);
         }
         return 0;
      }else{
         //wait(&pid); 
         printf("in parent!!!\n");
      }
    }
}

int SendData(int sock_fd, int newSocket, int data_recieved)
{
    (void)sock_fd;
    //Finally, a message can be sent!
    char buffer[256];
    if(data_recieved == 1){
       strcpy(buffer,"It must be a study day for you then!"); 
    }else if(data_recieved == 2){
       strcpy(buffer,"Today is BRIGHT."); 
    }else{
       strcpy(buffer,"Nooo you have to answer the question!!"); 
    }
    if(send(newSocket,buffer,sizeof(buffer),0) < 0){
    	printf("Error sending message\n");
    	return -1;
    }
    printf("Message Successfully Sent.\n");	
    return 0;
}












