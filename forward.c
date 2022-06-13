#include "nsconn.h"

extern int record, output, netOut, format;

int forward(const char * stringToSend, int sock, FILE * run){


  int stlen;
  for (stlen = 0; stringToSend[stlen] != '\0'; ++stlen);
  
  // if networking enabled, send to socket
  if(netOut == true) {
    send(sock, stringToSend, stlen, 0);
  }
   
  // write to record file 
  if (record == true) fwrite(stringToSend,stlen,1,run);


            
  // if stdout enabled
  if(output == true) { 
    if(format == JSON){
      printf("%s\n", stringToSend); 
    } 
  }


}


