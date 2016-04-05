#include "comm.h"
#include "Player.h"


void Player::send_message(const char *txt) {
  send_to_char(txt, this->ch);
}
