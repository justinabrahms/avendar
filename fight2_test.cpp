// Import order seems to matter. Including merc.h defines T, which I suspect gtest also defines.
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "fight2.h"
#include "Player.h"
using ::testing::StartsWith;

// TODO(abrahms): Move to mock_player.cpp?
class MockPlayer : public IPlayer {
public:
  MOCK_METHOD1(send_message, void(const char *txt));
  MOCK_METHOD0(get_ch, CHAR_DATA*());
};

TEST(Nock, DoesntHaveSkillErrors) {
  MockPlayer player;
  EXPECT_CALL(player, send_message(StartsWith("Huh?")));
  // EXPECT_CALL(player, send_message());

  
  do_nock(&player, "");
  // EXPECT_EQ(true, false);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
} 
