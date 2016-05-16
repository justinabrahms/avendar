#ifndef PLAYER_H
#define PLAYER_H

#include "merc.h"

class IPlayer {
public:
  // Q: when do things need a virtual destructor?
  // virtual ~IPlayer() {}
  virtual void send_message(const char *txt) = 0;
  virtual int get_skill(int skill_number) = 0;
  virtual CHAR_DATA* get_ch() = 0;  // bail out of superclass allowed for migration period.
};

class Player : public IPlayer {
public:
  Player(CHAR_DATA *ch) : ch(ch) {}

  virtual void send_message(const char *txt);
  virtual int get_skill(int skill_number);
  CHAR_DATA* get_ch() {
    return ch;
  }
private:
  CHAR_DATA* ch;
};

#endif
