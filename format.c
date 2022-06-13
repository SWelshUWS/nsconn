#include "nsconn.h"

extern int rawwavOff;


// create unix timestamp for object
unsigned long getTime(){
  struct timeval tv;

  gettimeofday(&tv, NULL);

  unsigned long long currTime =
      (unsigned long long)(tv.tv_sec) * 1000 +
      (unsigned long long)(tv.tv_usec) / 1000;
   return currTime;  // return timestamp in milliseconds 
}

int jsonFormat(unsigned char * payload, int sz, int sock, FILE * run){


  unsigned char attention = 0;
  unsigned char meditation = 0;
  unsigned char poorsignal = 0;
  short raw = 0;
  unsigned long creationTime = 0;

       /*
          pretty sure this is all parsing correctly but need to double check
          to be certain
       */



  //printf("starting payload parse\n");
  for (int i = 0; i < sz; i++){

    if ((payload[i] == RAWWAV) && (payload[i+1] == 0x02)){  // pLength should always be 0x02

      if(rawwavOff == false){

        json_object *rawwavObj = json_object_new_object();
        i = (i+2); // skip next byte (we know it's 0x02)
        raw = (payload[i] << 8) | payload[i+1];

        // add value & time to object
        json_object_object_add(rawwavObj, "raweegpower", json_object_new_int(raw));
        json_object_object_add(rawwavObj, "time", json_object_new_int64(getTime()));
      i += 2;
      // send object string to forward()
      forward(json_object_to_json_string_ext(rawwavObj, JSON_C_TO_STRING_SPACED), sock, run);

      }else { i = i+4; }  // if rawwav disabled

    } else if (payload[i] == ASICEEGPOWER){

        if((payload[i+1]) == 24){  // check payload is correct length
          i = (i+2); // skip payload position to 1st value byte

          // asic object
          json_object *asicObj = json_object_new_object();


          // create array object to hold 8 signal bands
          json_object *jValsArray = json_object_new_array();


          // increments three bytes per loop until all values are parsed
          for(int j = 0; j < 24; j = (j+3)){
               
            unsigned char b1 = i;
            unsigned char b2 = (i+1);
            unsigned char b3 = (i+2);

            // concatenate 3 bytes to get bigendian eeg value
            uint32_t asicVal = (b1 << 16) | (b2 << 8) | b3;

            // create json array

            json_object *jVal = json_object_new_int(asicVal);
            json_object_array_add(jValsArray,jVal);


            i = i+2;  // increase number of bytes parsed in payload loop

            } // end band value collection
            // add collected values to root object

          json_object_object_add(asicObj,"asiceegpower", jValsArray);
          json_object_object_add(asicObj, "time", json_object_new_int64(getTime()));
          
          
          forward(json_object_to_json_string_ext(asicObj, JSON_C_TO_STRING_SPACED), sock, run);
        }

      }else if(payload[i] < RAWWAV){
        if (payload[i] == LOWPOWER){
          json_object *poorsigObj = json_object_new_object();
          poorsignal = payload[i+1];
      
          if((json_object_object_add(poorsigObj, "poorsignal", json_object_new_int(poorsignal))) && (json_object_object_add(poorsigObj, "time", json_object_new_int64(getTime()))) == 1 ){
            puts("poorsignal fail\n");
          }
          if(json_object_object_add(poorsigObj, "time", json_object_new_int64(getTime())) != 0){
              puts("time fail\n");
          }
          i++;  
          forward(json_object_to_json_string_ext(poorsigObj, JSON_C_TO_STRING_SPACED), sock, run);


        } else if (payload[i] == ATTENTION){
          json_object *attObj = json_object_new_object();
          attention = payload[i+1];
          if(json_object_object_add(attObj, "attention", json_object_new_int(attention)) != 0){
            puts("attention fail\n");
          }
          if(json_object_object_add(attObj, "time", json_object_new_int64(getTime())) != 0){
            puts("time fail\n");
          }
          i++;
          forward(json_object_to_json_string_ext(attObj, JSON_C_TO_STRING_SPACED), sock, run);

        } else if (payload[i] == MEDITATION){
          json_object *medObj = json_object_new_object();
          meditation = payload[i+1];
          if(json_object_object_add(medObj, "meditation", json_object_new_int(meditation)) != 0){
            puts("meditation fail\n");
          }
          if(json_object_object_add(medObj, "time", json_object_new_int64(getTime())) != 0){
            puts("time fail\n");
          }
          i++;
          forward(json_object_to_json_string_ext(medObj, JSON_C_TO_STRING_SPACED), sock, run);

        }
      }

  }

  return 0;

}
