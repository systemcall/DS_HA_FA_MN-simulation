/* userver.c */
/* ref http://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c */
/* ref http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>           /* for gettimeofday() */
#include <time.h>            
#include <stdlib.h>
#include <strings.h>

#define TRUE 1

/* This is the server program. It opens a socket with the given port
 * and then begins an infinite loop. Each time through the loop it 
 * accepts a pkt and prints out the contents and the current time. 
 * Command line is `userver portnumber'.  
 */

main(int argc,char *argv[])
{
  int sock, length;
  struct sockaddr_in server, client, uha;
  struct hostent *hp, *gethostbyname();
  int msgsock;
  char buf[1024];
  char fmt[64], out[64];
  int rval, clilen, s_time;
  int i, seqno, reg, fa_port;
  char str[INET_ADDRSTRLEN];
  struct sockaddr_in* addr;
  struct timeval rcvtime;
  struct timezone zone;
  struct tm *tm;
  
  /* create socket */
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0) {
    perror("opening datagram socket");
    exit(1);
  }
  
  /* name socket using wildcards */
  
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1]));
  if(bind(sock, (struct sockaddr *)&server, sizeof(server))) {
    perror("binding socket name");
    exit(1);
  }
  
  /* get current relative time */
    if (gettimeofday(&rcvtime, &zone) < 0) {
      perror("getting time");
      exit(1);
    }
    s_time = rcvtime.tv_sec;
  
  /* find out assigned port number and print out */
  
  length = sizeof(server);
  if(getsockname(sock, (struct sockaddr *)&server, &length)) {
    perror("getting socket name");
    exit(1);
  }
  printf("Socket has port #%d\n",ntohs(server.sin_port));
  
  // we set the initial FA port here
  fa_port = atoi(argv[4]);
  
  i = 0;
  do {
    bzero(buf,sizeof(buf));
    clilen = sizeof(client);
    while ((rval = recvfrom(sock,buf,1024,0,(struct sockaddr *)&client,&clilen))<0){
      perror("receiver recvfrom");
    }
    // mobile node only cares about sequence number in payload,
    // and the FA who sent the payload. We can extract the FA
    // directly from the sockaddr structure. 
    sscanf(buf, "%d", &seqno );
    
    /* get current relative time */
    if (gettimeofday(&rcvtime, &zone) < 0) {
      perror("getting time");
      exit(1);
    }
    /* ref */
    if ( (tm = localtime(&rcvtime.tv_sec)) != NULL ){
      strftime(fmt, sizeof(fmt), "%H:%M:%S", tm);
      snprintf(out, sizeof(out), fmt, rcvtime.tv_usec);
    }
    
    /* ref */
    addr = (struct sockaddr_in*)&client;
    int ipAddr = addr->sin_addr.s_addr;
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    
    if ( (rcvtime.tv_sec - s_time) > 4 ){
      s_time = rcvtime.tv_sec;
      
      // Here we hard code the port swapping mechanism we could also swap the foreign agent ip we're expecting
      // if we are to run the foreign agents on separate machines.
      if ( ntohs(client.sin_port) == 6001 ) fa_port = 6002;
      else if ( ntohs(client.sin_port) == 6002 ) fa_port = 6001;
      
      // We wrtie the new payload details for the home agent. we set the registration request to true, and send the new port#
      // We don't really care about the seqno. 
      sprintf(buf, "%d %d %d\n", seqno, 1, fa_port);
      
      uha.sin_family = AF_INET;
      hp = gethostbyname(argv[2]);
      if(hp == 0) {
	printf("%s: unknown host\n", argv[2]);
	exit(2);
      }
      bcopy(hp->h_addr, &uha.sin_addr, hp->h_length);
      uha.sin_port = ntohs(atoi(argv[3]));
      
      if ((rval=sendto(sock,buf,sizeof(buf),0,(struct sockaddr *)&uha,sizeof(uha))) < 0) {
	perror("writing on datagram socket");
      }
      printf("Registration sent. Time = %s New care-of-address = %s/%d\n", out, str, fa_port);
      i = 0;
    }
    else {
      printf("Sequence Number = %2d Time = %s FA %s/%d. ", seqno, out, str, htons(server.sin_port) );
      if ( ntohs(client.sin_port) == fa_port ) printf("Accepted from %d\n", ntohs(client.sin_port));
      else printf("Rejected from %d\n", ntohs(client.sin_port));
    }
    i++;
    
  } while (rval != 0);
  close(sock);
  
}
