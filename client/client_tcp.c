#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "../message.h"
#include <sys/time.h>

int main(int argc, char *argv[]){
    int port_number, sockfd;
    double total_time = 0.0;
    int total_bytes = 0;		
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct timeval start, end;	
	
    // Check if have enough arguments are given 
    if ( argc < 4 ){
    	printf( "Not enough argument.\n" );
	exit( 0 );
    }
    port_number = atoi( argv[2] );
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ){
        perror( "ERROR creating socket failed.\n");
	exit( 0 );
    }
    server = gethostbyname( argv[ 1 ] );
    
    // Check if servername input on the command line is legal 
    if ( server == NULL ){
        printf( "Server does not exist.\n" );
	    exit( 0 );
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = port_number;		
    
    // Connect with a server
    if ( connect( sockfd, (struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ){
        perror( "ERROR connecting fail.\n" );
	    exit( 0 );
    }

    // Make the first message to request the file 
    char *file_name = argv[3];
    struct msg_t initial_message;
    make_msg( &initial_message, 1, 0, 1, strlen( file_name ), file_name ); 
    send( sockfd, &initial_message, sizeof( initial_message ), 0 );

    // Start to receive messages from the server    	
    FILE *fp;
    fp = fopen( file_name, "w" );
  
    // indicator is used to terminate the while loop 
    int indicator = 1;
    while ( indicator ){
  	    // Receive message
	    struct msg_t recmsg; 
	    // Measuring each the time for receive a single message. 
 	    gettimeofday( &start, NULL );
	    recv( sockfd, &recmsg, sizeof( recmsg ), MSG_WAITALL  );
	    gettimeofday( &end, NULL );
	    total_time = total_time + ( ( end.tv_sec * 1000000 + end.tv_usec ) - ( start.tv_sec * 1000000 + start.tv_usec ) );
	    total_bytes = total_bytes + recmsg.payload_len;
	    printf("client: RX %s %d %d %d\n", str_map[ recmsg.msg_type ], recmsg.cur_seq, recmsg.max_seq, recmsg.payload_len );
	    switch( recmsg.msg_type ){
		case 3:
		    fwrite( recmsg.payload, sizeof(char), recmsg.payload_len, fp);
		    struct msg_t newmes;
		    make_msg( &newmes, 4, recmsg.cur_seq, recmsg.max_seq, sizeof(""), "" );
	 	    send( sockfd, &newmes, sizeof( newmes ), 0 );

		    // Check if file download finished. If so, send out a MSG_TYPE_FINISH message to the server	 	
		    if ( recmsg.payload_len < BUF_SZ ){		        
			make_msg( &newmes, 5, 0, 0, sizeof(""), "" );
	 	        send( sockfd, &newmes, sizeof( newmes ), 0 ); 
		        printf( "File download completed.\n" );
			fclose( fp );
			indicator = 0; 
			printf( "Total time taken: %f\n", total_time ); 
			printf( "Total bytes transferred: %d\n", total_bytes );
			printf( "Number of messages received: %d\n", recmsg.cur_seq );
			close( sockfd );
		    }	
		    break; 
		case 2:
		    printf( "Server obtain file failed.\n" );
	 	    exit( 0 );
		    break;		  	
	    } 
    }
    return 0;
}
 





