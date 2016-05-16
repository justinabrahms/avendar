#include "comm.h"
#include "Player.h"


void Player::send_message(const char *txt) {
  send_to_char(txt, this->ch);
}

int Player::get_skill(int skill_number) {
  return get_skill(skill_number);
}
