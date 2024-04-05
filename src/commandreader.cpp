#include "commandreader.hpp"

TCommandReader::TCommandReader()
{
    fcount=-1;
    fparam=0;
    favailable=false;
}

boolean TCommandReader::Available(String *command, String *param)
{
    boolean result=false;
    while (Serial.available()>0) {
        char b=Serial.read();
        if (b=='\r') {
            if (fcount>0) {
                if (fparam>0) {
                  *command=String(&fbuffer[0],fparam);
                  *param=String(&fbuffer[fparam],fcount-fparam+1);
                } else {
                    *command=String(&fbuffer[0],fcount+1);
                    *param="";
                }
                result=true;
            }
            fcount=-1;
            fparam=0;
            Serial.println();
            if (result) {
                return result;
            }
        } else {
            if (b<' ') {
                continue;
            }
            if (fcount<MAX_COMMAND_LEN) {
                if (b==' ') {
                    if (fparam==0 && fcount>0) {
                        fparam=fcount+1;
                        Serial.print(b);
                    }
                } else  {
                  Serial.print(b);
                  fcount++;
                  fbuffer[fcount]=b;
                }
            };
        }
    }
    return result;
}
