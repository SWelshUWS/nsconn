
#include "nsconn.h"

extern int format, output;

void parsePackets(int * btSock, int sock){

  
  // create datetime value here so it is maintained while parsing

  // get date for record name
  time_t rawtime;
  char name [255];
  time (&rawtime);

  // this should be changed for something prettier
  sprintf(name,"/usr/local/share/nsconn/record/%s",ctime(&rawtime));  // get current date/time as string
  strcat(name,".json");  // add json extension to filename

  char *p = name;
  // replace spaces with _
  for (; *p; ++p){
    if (*p == ' ')  *p = '_';
  }

  FILE * rec;

  // create record file and open for rw
  rec = fopen(name,"w+");


  /*
    TODO
      * change read() to recv() 
      * check for active client before sending more data
      * simplify the timestamp format (hh-mm_dd-mm-yy)
  
  */


  while(1){
        // char * stringToSend = (char *) malloc(250);
        unsigned int bufferIn[1] = {0};
        int pLength;
        unsigned char payload[256];

        read( *btSock, bufferIn, 1 );

        // check first byte for sync
        if( *bufferIn != SYNC ){
          printf("sync fail.\n");
          continue; // restart the process
        }
        // check sync byte 2 if byte 1 ok
        read( *btSock, bufferIn, 1 );
        if( *bufferIn != SYNC ){
          printf("sync fail 2.\n"); // error on fail (AA byte 2)
          continue;
        }else{  // if synchronisation is successful
          read(*btSock, bufferIn, 1 );
          pLength = *bufferIn;
          if( pLength >= 170 ) {
            printf("payload greater that 170\n");
            break;
          }

          for(int k = 0; k < pLength; k++){
            read(*btSock, bufferIn, 1); // next byte is payload length
            payload[k] = *bufferIn;
          }

          //  *buffer = 0;  // clear buffer
          read(*btSock, bufferIn, 1);
          int checksum = *bufferIn;

          // calculate checksum
          int checkPL = 0;
          for(int i=0; i<pLength; i++) checkPL += payload[i];
          checkPL &= 0xFF;
          checkPL = ~checkPL & 0xFF;

          // compare calculated checksum to checksum byte
          if( checksum != checkPL ) {
            printf("Payload != checksum\n");
            continue; // terminate and parse a new packet
          }

          // send for further processing if JSON in use
          if(format == JSON){
            jsonFormat(payload, pLength, sock, rec);
          } 
          // send straight to forward
          else if (format == BIN){
              // print to screen if enabled
              if (output = true){
                for (int i = 0; i < pLength; i++){
                  printf("%02X", payload[i]);
                }
            }
            printf("\n"); // create new line between each object
            const char * strToSend = payload;
            forward(strToSend, sock, rec);
            /* 
              check that client is still connected before sending
              another packet - if disconnected, send warning 
              message and disconnect 
            */

          }

        }

    //remove("run.json");  // delete running file
  }
}
