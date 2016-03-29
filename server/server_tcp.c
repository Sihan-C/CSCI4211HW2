#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "../message.h"
#include <sys/stat.h>

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, port_number, state;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     socklen_t clientlen = sizeof( cli_addr );
     char buffer[ BUF_SZ ];
     char str[ INET_ADDRSTRLEN ];	

     // Check if have enough arguments are given 
     if ( argc < 2 ){
         printf( "Not enough argument." );
	 exit( 0 );
     }
     port_number = atoi( argv[ 1 ] );
     
     // Creat socket		
     sockfd = socket( AF_INET, SOCK_STREAM, 0 );
     if (sockfd < 0){ 
         perror( "ERROR creating a socket" );
         exit( 0 ); 
     }
     printf( "Socket created..\n" );
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = port_number;

     // Bind socket	
     if ( bind( sockfd, (struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ){ 
         perror( "ERROR binding\n" );
	 exit( 0 );
     }
     printf( "Socket bound to port %d..\n", port_number );
     
     // Listen
     if ( listen( sockfd, 1 ) < 0 ){
	 perror( "ERROR listening\n" );
	 exit( 0 ); 
     }

     // Accept
     newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clientlen );
     if ( newsockfd < 0 ){
     	 perror( "ERROR accepting\n" ); 
         exit( 0 );
     }
     inet_ntop( AF_INET, &( cli_addr.sin_addr ), str, INET_ADDRSTRLEN ); 	
     printf( "Client %s connected..\n", str );	     
	 	 	  	
     // Receive the first message from the client 	
     struct msg_t message;
     state = recv( newsockfd, &message, sizeof( message ), MSG_WAITALL );
     if ( state < 0 ){ 
         perror( "ERROR reading from socket\n" );
	     exit( 0 );
     }
     char *file_name = message.payload;
        
     if ( message.msg_type == 1 ){
         FILE *fp;
         fp = fopen( file_name, "r" );
 	 // Check if file open succeed. If not, sent a MSG_TYPE_GET_ERR message back to the client
         if ( fp == NULL ){
             struct msg_t openfailmsg;
             make_msg( &openfailmsg, 2, 0, 0, 0, "" );
	     send( newsockfd, &openfailmsg, sizeof( openfailmsg ), 0 );
         }
	 int indicator = 1;
	 int seq = 1; 

  	 // Calculate the file size
	 struct stat buf;
	 int fd = fileno( fp );
	 fstat(fd, &buf);
	 int size = buf.st_size;
	 int max_seq = ( ( size - 1 ) / BUF_SZ ) + 1;

         while ( indicator ){	
             int x = fread( buffer, sizeof( char ), BUF_SZ, fp );     
             
	     // Start to send message
	     struct msg_t res;
             make_msg( &res, 3, seq, max_seq, x, buffer );
	     send( newsockfd, &res, sizeof( res ), 0 ); 
	     
	     // Receive message  
	     struct msg_t recvmsg;	
             int r = recv( newsockfd, &recvmsg, sizeof( recvmsg ), MSG_WAITALL );
             if (r < 0) {
                 perror( "ERROR receving" );
                 exit(1);
             } 		 
	     printf("client: RX %s %d %d %d\n", str_map[ recvmsg.msg_type ], recvmsg.cur_seq, recvmsg.max_seq, recvmsg.payload_len );

	     // Check if msg_type is MSG_TYPE_GET_ACK. if not then there must be some problems
             if ( recvmsg.msg_type != 4 ){
                 printf("Invalid message!");
		 exit( 0 );	 	 	
	     }

	     // Check if reaching EOF
	     if( max_seq == seq ){
 	     	 indicator = 0;	
             } 		
	     seq++;	
	 }
	 
	 // Revcive finish message sent from the client
	 struct msg_t finmsg;	
         recv( newsockfd, &finmsg, sizeof( finmsg ), MSG_WAITALL );
	 printf("client: RX %s %d %d %d\n", str_map[ finmsg.msg_type ], finmsg.cur_seq, finmsg.max_seq, finmsg.payload_len );
	 if ( finmsg.msg_type == 5 ){
	     close( sockfd );
         }
	 else{
  	     printf( "Invalid message type." );		
	     exit( 0 );	
	 }
     }
     else{ 
     	 printf( "Invalid message type." );
	 exit( 0 );	
     }
     return 0; 
}


