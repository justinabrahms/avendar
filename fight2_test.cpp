// Import order seems to matter. Including merc.h defines T, which I suspect gtest also defines.
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Player.h"
#include "fight2.h"

// TODO(abrahms): Move to mock_player.cpp?
class MockPlayer : public IPlayer {
  MOCK_METHOD1(send_message, void(const char *txt));
  MOCK_METHOD0(get_ch, CHAR_DATA*());
};

TEST(Nock, DoesntHaveSkillErrors) {
  // MockPlayer player;
  // EXPECT_CALL(player, send_message(StartsWith("Huh?")));
  // EXPECT_CALL(player, send_message());

  
  // do_nock(mockPlayer, "")
  EXPECT_EQ(true, false);
}
