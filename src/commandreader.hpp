#pragma once
#include <Arduino.h>
#define MAX_COMMAND_LEN 20
class TCommandReader {
  private:
   char fbuffer[MAX_COMMAND_LEN+1];
   int fcount; 
   int fparam;
   boolean favailable;
  public:
    TCommandReader();
    boolean Available(String *command, String *param);

};