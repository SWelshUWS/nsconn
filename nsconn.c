/*

  nsconn - a connector for the NeuroSKy Mindwave Mobile 2

  Stacey Welsh, April 2022


  TO DO:
    * redo error checking - what is considered best standards?
    * check that all memory allocations have been freed
    * add MAC arg to config file if field is empty
    * /usr folder is for read only files - change the install file!
    * socket needs keepalive option

*/

#include "nsconn.h"



  // global variables to control output methods 
  int output = false;  // print payload to stdout
  int netOut = false; // select socket type
  int record = false; //enable recording to file 
  int rawwavOff = false; // disable raw eeg wave output
  int format = JSON;

  SOCKET create_socket(const char* host, const char *port) {

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
          bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
      perror("socket_listen failed");
      exit(1);
    }

    if (bind(socket_listen,
        bind_address->ai_addr, bind_address->ai_addrlen)) {
      perror("bind() failed - check port is free or use another");
      exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return socket_listen;
}


#define MAX_REQUEST_SIZE 2047

struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    char address_buffer[128];
    SOCKET socket;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
};


struct client_info *get_client(struct client_info **client_list,
        SOCKET s) {
    struct client_info *ci = *client_list;

    while(ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }

    if (ci) return ci;
    struct client_info *n =
        (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = *client_list;
    *client_list = n;
    return n;
}


void drop_client(struct client_info **client_list,
        struct client_info *client) {
    CLOSESOCKET(client->socket);

    struct client_info **p = client_list;

    while(*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}


const char *get_client_address(struct client_info *ci) {
    getnameinfo((struct sockaddr*)&ci->address,
            ci->address_length,
            ci->address_buffer, sizeof(ci->address_buffer), 0, 0,
            NI_NUMERICHOST);
    return ci->address_buffer;
}


fd_set wait_on_clients(struct client_info **client_list, SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = *client_list;

    while(ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}



int main(int argc, char *argv[] ){

  // options to be parsed into struct
  int i;
  int option;

  int noNet = -1;
  int * noNetP = &noNet;
  unsigned char portOut[5] = {0};
  unsigned char dest[17]; // destination MAC for Bluetooth connection
  int optval = 1;
  socklen_t optlen = sizeof(optval);


  GKeyFile *keyfile;
  GKeyFileFlags flags;
  GError *error = NULL;
  gchar *defaultMAC, *defaultPort;
  const char * confpath = "/etc/nsconn.conf"; // change me before push to /etc/nsconn



  // text to display when -h specified
  const char * helpText = "\nBasic Usage:\n"
    "nsconn [options] [MAC address]\n\n"
    "\t -h: \tdisplay basic usage info and exit.\n"
    "\t -v: \tshow display version and exit\n"
    "\t -d: \tdisable raw wave output\n"
    "\t -o: \tenable stdout\n"
    "\t -n: \tenable output to network socket\n\n"
    "\t -f [json|binary]: \tspecify output format \n"
    "\t -p [port]: \t\tspecify which port to bind to\n\n"
    "configuration file: /etc/nsconn.conf\n\n";

  // splash on start
  printf("\n    _   _______ __________  _   ___   __ \n"
    "   / | / / ___// ____/ __ \\/ | / / | / / \n"
    "  /  |/ /\\__ \\/ /   / / / /  |/ /  |/ / \n"
    " / /|  /___/ / /___/ /_/ / /|  / /|  /  \n"
    "/_/ |_//____/\\____/\\____/_/ |_/_/ |_/   \n"
    "\nnsconn - a connector for the neurosky mindwave mobile 2\n\n");


  // Create a new GKeyFile object and a bitwise list of flags.
  keyfile = g_key_file_new ();
  flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

  // load configuration file
  if(!g_key_file_load_from_file (keyfile, confpath, flags, &error)){
    // handle non existant config
    fprintf(stderr, "could not load configuration file.\n check that /etc/nsconn.conf exists.\n");
    exit(0);
  }


  if(argc > 1){    // if arguments are provided
    // put ':' at the starting of the string so compiler can distinguish between '?' and ':'
    while((option = getopt(argc, argv, ":p:f:c:dovnh")) != -1){ //get option from the getopt() method

      switch(option){
        // options with no value
        case 'h':
          printf("%s", helpText);
          exit(0);
          break;

        case 'v':
          printf("%s\n", "nsconn v1.1 - March 2022\nbuilt using gcc version 9.3.0\n");
          exit(0);
          break;

        case 'o':
          // enable printing final payload to stdout
          printf("%s\n", "printing to stdout enabled...\n");
          output = true;
          break;

        case 'n':
          // netsocket out should only start when selected
            netOut = true;
            printf("%s\n", "network socket selected...");
          // could be expanded for more visualization types
          break;
        
        case 'd':      // override default config port
          rawwavOff = true;
          printf("%s\n", "raw EEG wave value disabled... ");
          break;

        case 'r':      // override default config port
          printf("%s\n", "Recording enabled... ");
          break;

          // options expecting values
        case 'p':      // override default config port
          // protect against buffer overflows with strcpy
          if (optarg == NULL) {
            exit(0);
          } else if (strlen(optarg) > 5) {
            // port should never be more than 5 characters
            printf("%s\n", "Port number too long.");
            exit(0);
          }
          else {
            // snprintf()
            strcpy(portOut, optarg);
          }

          printf("user defined port: %s\n", portOut);
          break;

        case 'f':      // change format of output
          if(strcmp(optarg, "json") == 0){
            format = JSON;
            printf("JSON output selected\n");
          } else if(strcmp(optarg, "binary") == 0){
            format = BIN;
            printf("binary output selected.\n");
          }
          break;

        
        case 'c':      // set the running config
          /*
            currently not used
            this would allow the user to set their own configuration file
          */
          printf("running config: %s\n", optarg);
          break;

          case ':':
            printf("option needs a value\n");
            break;

          case '?':      //used for some unknown options
            printf("invalid option selected: %c\n", optopt);
            exit(0);
            break;
        }
    }

    if (argv[optind]){    // if there are more arguments
      for(; optind < argc; optind++){ //when some extra arguments are passed
        if (!argv[optind+1]){   // if this is the last value
          if (strlen(argv[optind]) == 17){ // if the provided argument is the correct length
            // does this have to be memcpy?
            memcpy(dest, argv[optind], strlen(argv[optind])+1);
            // copy MAC from arg to dest
            // MAC should be checked against a pattern as well as for length
            if (strlen(dest) != 17){ // ensure MAC is correct length
              printf("invalid MAC.\n");
              exit(0);         
            } else {
              defaultMAC = g_key_file_get_string(keyfile,"bluetooth","DEFAULT_MAC",&error);
              if(strlen(defaultMAC) < 17){
                printf("no default MAC exists. add this MAC to config file? Y/n\n");
                char atc = 'y';
                scanf("%c", &atc);
                if ( tolower(atc) == 'y' ){
                  printf("saving MAC to config...\n");
                  g_key_file_set_string(keyfile,"bluetooth","DEFAULT_MAC",dest);                  
                  printf("default MAC: %s", defaultMAC);
                  if(g_key_file_save_to_file(keyfile, confpath, &error) == 0){
                    perror("could not write to file. \n");
                    exit(0);
                  } 
                }
              }
            }
          }
        } else {
          printf("%s\n", "too many arguments\n");
          exit(0);
        }
      }
    }
  }

  /*
    end gathering arguments -
    check which struct elements are still null
    and fill from the config file
  */

  if(strlen(dest) != 17){ // if MAC not set
    if(defaultMAC = g_key_file_get_string(keyfile,"bluetooth","DEFAULT_MAC",NULL)){
      if (strlen(defaultMAC) == 17){
        printf("%s\n", defaultMAC);
        // safe to use as length has been verified above
        strcpy(dest,defaultMAC);
      } else {
        perror("invalid or no MAC. please check the configuration file.\n");
        exit(0);
      }
    }
  } else {
    // check to see if the default value is empty
    // if yes, prompt to save
  }
  if(strlen(portOut) == 0){
    if((defaultPort = g_key_file_get_string(keyfile,"network","PORT",NULL)) < 0){
      fprintf(stderr, "could not get port from configuration file.\n");
      exit(1);
    }
    strcpy(portOut,defaultPort);

    // end start config

  }
  // start bluetooth socket

  struct sockaddr_rc addr = { 0 };
  int sIn, status, valread;
  int *sInP = &sIn;

  // create a TCP BT socket using RFCOMM (perror here)
  if(sIn = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)){
    printf("initializing bluetooth socket...\n");
  }else{
    printf("failed to create socket.\n");
    exit(0);
  }

  // set the connection parameters (who to connect to)
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_channel = (uint8_t) 1;

  // confirm destination MAC
  
  if(str2ba(dest, &addr.rc_bdaddr) != 0 ){
    printf("error: parsing MAC\n"); // replace with perror
    fprintf(stderr, "unable to parse MAC address.\n");
    exit(0);
  }

  // connect to headset (perror here)
  status = connect(sIn, (struct sockaddr *)&addr, sizeof(addr));

  // confirm connection
  if(status != 0 ) {
    fprintf(stderr, "failed to connect to headset. check bluetooth is enabled and headset paired.\n");
    exit(1);
  } else { printf("connected to device with MAC %s \n", dest); }


  // if no network socket
  if(netOut == false){
    if(output == true){
      puts("printing to stdout\n");
      parsePackets(sInP, -1); // start parsing packets to stdout
    } else {
      puts("no output method selected\n");
      exit(0);
    }
  }
  else{   // start network socket
    if(netOut == true){

      printf("initializing network socket... \t[127.0.0.1:%s]\n", portOut);

      // start here be dragons
      SOCKET server = create_socket(0, portOut);
      if(server < 0){
        perror("failed to create network socket.\n");
        close(server);
        exit(EXIT_FAILURE);
      }

      struct client_info *client_list = 0;
      if(setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()\n");
        close(server);
        exit(EXIT_FAILURE);
      }


      while(1) {

        fd_set reads;
        reads = wait_on_clients(&client_list, server);

        if (FD_ISSET(server, &reads)) {
          struct client_info *client = get_client(&client_list, -1);

          client->socket = accept(server,
            (struct sockaddr*) &(client->address),
            &(client->address_length));

          if (!ISVALIDSOCKET(client->socket)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
          }
          printf("New connection from %s.\n", get_client_address(client));
        }


        struct client_info *client = client_list;
        while(client) {

        int * outSockP = &client->socket;
        struct client_info *next = client->next;
        parsePackets(sInP, client->socket);

        client = next;
      }
    } //while(1)


    printf("\nClosing socket...\n");
    CLOSESOCKET(server);
    printf("Finished.\n");

    }
  }
}


