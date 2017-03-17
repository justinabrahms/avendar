CC      = g++
PROF    = -O0 -g
RELEASE = -O3
COVERAGE= -fprofile-arcs -ftest-coverage
C_FLAGS = -m32 -fpermissive -Wall -Wno-parentheses -Wno-char-subscripts -Wno-write-strings $(PROF) $(NOCRYPT) $(COVERAGE)
L_FLAGS = -m32 -lm -L/usr/lib/i386-linux-gnu -L/lib/i386-linux-gnu -lcrypt -L/usr/local/bin/ -lmysqlclient -lgcov $(PROF)

# TODO(jabrahms): Move this into things which are mud dependencies and things which rarely change (eg StringUtil)
O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o AirTitles.o \
          alchemy.o alias.o ban.o bit.o comm.o CommandTrie.o const.o db.o db2.o dictionary.o \
          Direction.o DisplayPanel.o Drakes.o EarthTitles.o EchoAffect.o \
	      effects.o Encumbrance.o ExperienceList.o extras.o Faction.o fight.o fight2.o FireTitles.o \
          flags.o Forge.o gold.o handler.o \
	      healer.o house_powers.o interp.o note.o languages.o LeyGroup.o lookup.o Luck.o \
          magic.o mem.o mob_commands.o mob_prog.o NameMaps.o Oaths.o olc.o olc_act.o olc_save.o \
          PhantasmInfo.o PhantasmTrait.o ProgConditionals.o PyreInfo.o \
          psionics.o recycle.o RomanNumerals.o RoomPath.o RuneInfo.o Runes.o \
          save.o scan.o Shades.o ShadeControl.o skills.o skills_chirurgeon.o \
          skills_rogue.o SomaticArtsInfo.o songs.o special.o spells_air.o spells_druid.o \
          spells_earth.o spells_fire.o spells_fire_void.o spells_fire_water.o \
          spells_fire_spirit.o spells_fire_earth.o spells_fire_air.o \
          spells_generic.o spells_shared.o spells_earth_void.o \
	      spells_spirit.o spells_spirit_water.o spells_spirit_void.o spells_spirit_earth.o spells_spirit_air.o \
          spells_air_earth.o spells_air_void.o spells_void.o spells_water.o spells_water_earth.o spells_water_void.o \
          spells_water_air.o SpiritTitles.o StatEmitter.o string.o StringUtil.o tables.o Titles.o \
	      update.o VoidTitles.o weather.o Weave.o cards.o mount.o puerto.o gameserv.o coloretto.o \
	      version.o WaterTitles.o

USER_DIR = .
GTEST_DIR = deps/googletest

TESTS = StringUtil_test

main:
	./update_version > version.h
	make rom

rom: $(O_FILES)
	$(CC) -o avendar $(O_FILES) $(L_FLAGS)

prof: $(O_FILES)
	$(CC) -o avendar -pg $(O_FILES) $(L_FLAGS)

clean:
	rm -f avendar *~ *.o *.a *.gcov *.gcda *.gcno $(TESTS)

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<

.cpp.o: merc.h
	$(CC) -c $(C_FLAGS) $<

test: $(TESTS)

# TESTS!!!
#
# The bulk of these were pulled from GoogleTests's Makefile samples.


# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -m32 -isystem $(GTEST_DIR)/include $(COVERAGE)

# Flags passed to the C++ compiler.
CXXFLAGS += -m32 -g -Wall -Wextra -pthread

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.


# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^



StringUtil_test.o : StringUtil_test.cpp $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(C_FLAGS) $(USER_DIR)/$*.cpp

StringUtil_test : StringUtil.o StringUtil_test.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@ 
