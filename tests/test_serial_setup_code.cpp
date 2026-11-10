#include "serial_setup_code.h"
#include <cassert>
#include <string>
int main(){
  SerialSetupCode parser;unsigned calls=0;
  auto send=[&](std::string s){bool ok=false;for(char c:s)ok=parser.feed(c,[&](const char* code){++calls;return std::string(code)=="123456789012";})||ok;return ok;};
  assert(send("SETUP 123456789012\r\n") && calls==1);
  assert(!send("SETUP 1234567890123\n"));assert(!send("SETUP 12345678901x\n"));
  assert(!send(std::string(10000,'A')+"\n"));assert(calls==1);
  send("SETUP 1234");parser.clear();assert(!send("56789012\n"));
  assert(send("SETUP 123456789012\n") && calls==2);
}
