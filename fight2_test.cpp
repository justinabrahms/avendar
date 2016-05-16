// Import order seems to matter. Including merc.h defines T, which I suspect gtest also defines.
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "fight2.h"
#include "Player.h"
using ::testing::StartsWith;
using ::testing::Return;

// TODO(abrahms): Move to mock_player.cpp?
class MockPlayer : public IPlayer {
public:
  MOCK_METHOD1(send_message, void(const char *txt));
  MOCK_METHOD1(get_skill, int(int skill_number));
  MOCK_METHOD0(get_ch, CHAR_DATA*());
};

TEST(Nock, DoesntHaveSkillErrors) {
  testing::NiceMock<MockPlayer> player;
  EXPECT_CALL(player, get_skill(0))
    .WillOnce(Return(0));
  EXPECT_CALL(player, send_message(StartsWith("Huh?")));

  do_nock(&player, "");
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
} 
