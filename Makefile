CC      = g++
PROF    = -O0 -g
RELEASE = -O3
C_FLAGS =  -m32 -fpermissive -Wall -Wno-parentheses -Wno-char-subscripts -Wno-write-strings $(PROF) $(NOCRYPT) 
L_FLAGS =  -m32 -lm -L/usr/lib/i386-linux-gnu -L/lib/i386-linux-gnu -lcrypt -L/usr/local/bin/ -lmysqlclient $(PROF)

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

main:
	./update_version > version.h
	make rom

rom: $(O_FILES)
	$(CC) -o avendar $(O_FILES) $(L_FLAGS)

prof: $(O_FILES)
	$(CC) -o avendar -pg $(O_FILES) $(L_FLAGS)

clean:
	rm -f avendar *~ *.o

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<

.cpp.o: merc.h
	$(CC) -c $(C_FLAGS) $<
