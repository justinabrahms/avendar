 /***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/


/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN(	spell_chill_touch);
DECLARE_SPELL_FUN(	spell_seedofgamaloth	);
DECLARE_SPELL_FUN(	spell_moonbornspeed	);
DECLARE_SPELL_FUN(	spell_beastform		);
DECLARE_SPELL_FUN(	spell_tendrilgrowth	);
DECLARE_SPELL_FUN(	spell_infectiousaura	);
DECLARE_SPELL_FUN(	spell_snakebite		);
DECLARE_SPELL_FUN(	spell_wither		);
DECLARE_SPELL_FUN(	spell_nettles		);
DECLARE_SPELL_FUN(	spell_shurangaze	);
DECLARE_SPELL_FUN(	spell_eyesoftheforest	);
DECLARE_SPELL_FUN(	spell_invocationofconveru);
DECLARE_SPELL_FUN(	spell_remove_hex	);
DECLARE_SPELL_FUN(	spell_lesserspiritshield);
DECLARE_SPELL_FUN(	spell_seraphic_wings	);
DECLARE_SPELL_FUN(	spell_glowing_gaze	);
DECLARE_SPELL_FUN(	spell_douse		);
DECLARE_SPELL_FUN(	spell_runeofembers	);
DECLARE_SPELL_FUN(	spell_blazingspear	);
DECLARE_SPELL_FUN(	spell_dim		);
DECLARE_SPELL_FUN(	spell_null		);
DECLARE_SPELL_FUN(	spell_form		);
DECLARE_SPELL_FUN(	spell_focus		);
DECLARE_SPELL_FUN(	spell_lang		);
DECLARE_SPELL_FUN(	spell_song		);
DECLARE_SPELL_FUN(	spell_aegisoflaw	);
DECLARE_SPELL_FUN(	spell_runeoflife	);
DECLARE_SPELL_FUN(	spell_wallofstone	);
DECLARE_SPELL_FUN(	spell_fleshtostone	);
DECLARE_SPELL_FUN(	spell_stoneshell	);
DECLARE_SPELL_FUN(	spell_encase		);
DECLARE_SPELL_FUN(	spell_holywater  	);
DECLARE_SPELL_FUN(	spell_meldwithwater  	);
DECLARE_SPELL_FUN(	spell_alterself  	);
DECLARE_SPELL_FUN(	spell_heatsink   	);
DECLARE_SPELL_FUN(	spell_createfoci   	);
DECLARE_SPELL_FUN(      spell_astralform        );
DECLARE_SPELL_FUN(	spell_astralprojection 	);
DECLARE_SPELL_FUN(	spell_affinity   	);
DECLARE_SPELL_FUN(	spell_masspeace   	);
DECLARE_SPELL_FUN(	spell_focusmind   	);
DECLARE_SPELL_FUN(	spell_avatar     	);
DECLARE_SPELL_FUN(	spell_spiritbond   	);
DECLARE_SPELL_FUN(	spell_spiritblock   	);
DECLARE_SPELL_FUN(	spell_aura       	);
DECLARE_SPELL_FUN(	spell_spiritstone   	);
DECLARE_SPELL_FUN(      spell_truesight         );
DECLARE_SPELL_FUN(	spell_manaconduit   	);
DECLARE_SPELL_FUN(      spell_exorcism          );
DECLARE_SPELL_FUN(	spell_lifebolt   	);
DECLARE_SPELL_FUN(      spell_lightsword        );
DECLARE_SPELL_FUN(	spell_lightspear   	);
DECLARE_SPELL_FUN(	spell_clarity   	);
DECLARE_SPELL_FUN(      spell_desecrateweapon   );
DECLARE_SPELL_FUN(      spell_consecrate        );
DECLARE_SPELL_FUN(	spell_dreamspeak   	);
DECLARE_SPELL_FUN(	spell_unshackle   	);
DECLARE_SPELL_FUN(      spell_poschan           );
DECLARE_SPELL_FUN(      spell_radiance          );
DECLARE_SPELL_FUN(      spell_raylight          );
DECLARE_SPELL_FUN(      spell_ritenathli        );
DECLARE_SPELL_FUN(	spell_runeofspirit   	);
DECLARE_SPELL_FUN(      spell_unseenservant     );
DECLARE_SPELL_FUN(	spell_zeal       	);
DECLARE_SPELL_FUN(	spell_spiritshield   	);
DECLARE_SPELL_FUN(	spell_improveddetectevil);
DECLARE_SPELL_FUN(	spell_thanatopsis   	);
DECLARE_SPELL_FUN(      spell_wrathkyana        );
DECLARE_SPELL_FUN(	spell_avengingseraph   	);
DECLARE_SPELL_FUN(	spell_speakwiththedead	);
DECLARE_SPELL_FUN(	spell_implosion   	);
DECLARE_SPELL_FUN(	spell_delayedblastfireball);
DECLARE_SPELL_FUN(	spell_wingsofflame   	);
DECLARE_SPELL_FUN(	spell_smolder   	);
DECLARE_SPELL_FUN(	spell_phoenixfire   	);
DECLARE_SPELL_FUN(	spell_flametongue   	);
DECLARE_SPELL_FUN(	spell_fireblast   	);
DECLARE_SPELL_FUN(	spell_blazinginferno   	);
DECLARE_SPELL_FUN(	spell_consume   	);
DECLARE_SPELL_FUN(      spell_barbs		);
DECLARE_SPELL_FUN(	spell_obscurealign	);
DECLARE_SPELL_FUN(	spell_ignite		);
DECLARE_SPELL_FUN(	spell_rainoffire   	);
DECLARE_SPELL_FUN(	spell_incineration   	);
DECLARE_SPELL_FUN(	spell_flare		);
DECLARE_SPELL_FUN(	spell_livingflame   	);
DECLARE_SPELL_FUN(	spell_dispersion   	);
DECLARE_SPELL_FUN(	spell_walloffire   	);
DECLARE_SPELL_FUN(	spell_baptismoffire   	);
DECLARE_SPELL_FUN(	spell_runeoffire   	);
DECLARE_SPELL_FUN(	spell_flameshield   	);
DECLARE_SPELL_FUN(	spell_naturalarmor  	);
DECLARE_SPELL_FUN(	spell_runeofearth   	);
DECLARE_SPELL_FUN(	spell_slip      	);
DECLARE_SPELL_FUN(	spell_meteorstrike   	);
DECLARE_SPELL_FUN(	spell_brittleform   	);
DECLARE_SPELL_FUN(	spell_strengthen   	);
DECLARE_SPELL_FUN(	spell_meldwithstone   	);
DECLARE_SPELL_FUN(	spell_shapearmor   	);
DECLARE_SPELL_FUN(	spell_shapeweapon   	);
DECLARE_SPELL_FUN(	spell_metaltostone   	);
DECLARE_SPELL_FUN(	spell_gravitywell   	);
DECLARE_SPELL_FUN(	spell_density   	);
DECLARE_SPELL_FUN(	spell_smoothterrain   	);
DECLARE_SPELL_FUN(	spell_voiceoftheearth   );
DECLARE_SPELL_FUN(	spell_stonetomud   	);
DECLARE_SPELL_FUN(	spell_calluponearth   	);
DECLARE_SPELL_FUN(	spell_stabilize   	);
DECLARE_SPELL_FUN(	spell_dispelillusions  	);
DECLARE_SPELL_FUN(	spell_knock      	);
DECLARE_SPELL_FUN(	spell_diamondskin   	);
DECLARE_SPELL_FUN(	spell_phantasmalcreation);
DECLARE_SPELL_FUN(	spell_diversions   	);
DECLARE_SPELL_FUN(	spell_dancingsword   	);
DECLARE_SPELL_FUN(	spell_protectionfromlightning);
DECLARE_SPELL_FUN(	spell_illusionaryobject );
DECLARE_SPELL_FUN(	spell_gaseousform   	);
DECLARE_SPELL_FUN(	spell_airrune   	);
DECLARE_SPELL_FUN(	spell_conjureairefreet	);
DECLARE_SPELL_FUN(	spell_thunderclap   	);
DECLARE_SPELL_FUN(	spell_groupteleport   	);
DECLARE_SPELL_FUN(	spell_windwall   	);
DECLARE_SPELL_FUN(	spell_airbubble   	);
DECLARE_SPELL_FUN(	spell_frostbite  	);
DECLARE_SPELL_FUN(	spell_icestorm   	);
DECLARE_SPELL_FUN(	spell_waterwalk  	);
DECLARE_SPELL_FUN(	spell_scry      	);
DECLARE_SPELL_FUN(	spell_spectralfist     	);
DECLARE_SPELL_FUN(	spell_obfuscation      	);
DECLARE_SPELL_FUN(	spell_windwalk      	);
DECLARE_SPELL_FUN(	spell_greaterinvis     	);
DECLARE_SPELL_FUN(	spell_suction      	);
DECLARE_SPELL_FUN(	spell_gust      	);
DECLARE_SPELL_FUN(	spell_delusions		);
DECLARE_SPELL_FUN(	spell_greaterventriloquate);
DECLARE_SPELL_FUN(	spell_resurrection  	);
DECLARE_SPELL_FUN(	spell_voidwalk  	);
DECLARE_SPELL_FUN(	spell_deathwalk  	);
DECLARE_SPELL_FUN(      spell_demonichowl       );
DECLARE_SPELL_FUN(      spell_demonpos          ); 
DECLARE_SPELL_FUN(	spell_waterelementalsummon);
DECLARE_SPELL_FUN(	spell_earthelementalsummon);
DECLARE_SPELL_FUN(	spell_protectionfromfire);
DECLARE_SPELL_FUN(	spell_currents		);
DECLARE_SPELL_FUN(	spell_cleansefood	);
DECLARE_SPELL_FUN(	spell_protectionfrompoison);
DECLARE_SPELL_FUN(	spell_protectionfromcold);
DECLARE_SPELL_FUN(	spell_resist_poison	);
DECLARE_SPELL_FUN(	spell_beltoflooters  	);
DECLARE_SPELL_FUN(	spell_birdofprey  	);
DECLARE_SPELL_FUN(	spell_pillage   	);
DECLARE_SPELL_FUN(	spell_agony	  	);
DECLARE_SPELL_FUN(	spell_mendwounds  	);
DECLARE_SPELL_FUN(	spell_dedication  	);
DECLARE_SPELL_FUN(	spell_matrix	  	);
DECLARE_SPELL_FUN(	spell_division	  	);
DECLARE_SPELL_FUN(	spell_command_weather	);
DECLARE_SPELL_FUN(	spell_commandword  	);
DECLARE_SPELL_FUN(	spell_findfamiliar  	);
DECLARE_SPELL_FUN(	spell_banish		);
DECLARE_SPELL_FUN(	spell_lesserdemonsummon	);
DECLARE_SPELL_FUN(	spell_greaterdemonsummon);
DECLARE_SPELL_FUN(	spell_suppress  	);
DECLARE_SPELL_FUN(	spell_tentacles  	);
DECLARE_SPELL_FUN(	spell_cloakofthevoid  	);
DECLARE_SPELL_FUN(	spell_possession  	);
DECLARE_SPELL_FUN(	spell_callguards	);
DECLARE_SPELL_FUN(	spell_locatecriminal	);
DECLARE_SPELL_FUN(	spell_moonray		);
DECLARE_SPELL_FUN(	spell_holyavenger	);
DECLARE_SPELL_FUN(	spell_reveal		);
DECLARE_SPELL_FUN(	spell_brotherhood	);
DECLARE_SPELL_FUN(	spell_scourgeofdarkness );
DECLARE_SPELL_FUN(	spell_soulblade		);
DECLARE_SPELL_FUN(      spell_soulflare         );
DECLARE_SPELL_FUN(	spell_mantleoffear	);
DECLARE_SPELL_FUN(	spell_enslave		);
DECLARE_SPELL_FUN(	spell_coverofdarkness	);
DECLARE_SPELL_FUN(	spell_consumption	);
DECLARE_SPELL_FUN(	spell_runeofeyes	);
DECLARE_SPELL_FUN(	spell_soulreaver	);
DECLARE_SPELL_FUN(	spell_coven		);
DECLARE_SPELL_FUN(	spell_demonsummon	);
DECLARE_SPELL_FUN(	spell_calmanimals	);
DECLARE_SPELL_FUN(	spell_naturegate	);
DECLARE_SPELL_FUN(	spell_hawkform		);
DECLARE_SPELL_FUN(	spell_wolfform		);
DECLARE_SPELL_FUN(	spell_bearform		);
DECLARE_SPELL_FUN(	spell_skycall    	);
DECLARE_SPELL_FUN(	spell_stampede		);
DECLARE_SPELL_FUN(	spell_animalswarm	);
DECLARE_SPELL_FUN(	spell_animaleyes	);
DECLARE_SPELL_FUN(	spell_speakwithplants 	);
DECLARE_SPELL_FUN(	spell_plantentangle	);
DECLARE_SPELL_FUN(	spell_warpwood		);
DECLARE_SPELL_FUN(	spell_giantgrowth	);
DECLARE_SPELL_FUN(	spell_shrink		);
DECLARE_SPELL_FUN(	spell_wallofvines	);
DECLARE_SPELL_FUN(	spell_forestwalk	);
DECLARE_SPELL_FUN(	spell_thornspray	);
DECLARE_SPELL_FUN(	spell_creepingcurse 	);
DECLARE_SPELL_FUN(	spell_riteofthesun	);
DECLARE_SPELL_FUN(	spell_circle_of_stones  );
DECLARE_SPELL_FUN(	spell_barkskin		);
DECLARE_SPELL_FUN(	spell_elementalprotection);
DECLARE_SPELL_FUN(	spell_abominablerune	);
DECLARE_SPELL_FUN(	spell_absorbelectricity	);
DECLARE_SPELL_FUN(	spell_acid_blast	);
DECLARE_SPELL_FUN(	spell_aquamove		);
DECLARE_SPELL_FUN(	spell_armor		);
DECLARE_SPELL_FUN(	spell_arrowgeas		);
DECLARE_SPELL_FUN(	spell_awaken		);
DECLARE_SPELL_FUN(	spell_chameleon		);
DECLARE_SPELL_FUN(	spell_starglamour	);
DECLARE_SPELL_FUN(	spell_commune_nature	);
DECLARE_SPELL_FUN(	spell_tangle_trail	);
DECLARE_SPELL_FUN(	spell_backfire		);
DECLARE_SPELL_FUN(	spell_disintegration	);
DECLARE_SPELL_FUN(	spell_ball_lightning	);
DECLARE_SPELL_FUN(	spell_beam_of_fire	);
DECLARE_SPELL_FUN(	spell_bindweapon	);
DECLARE_SPELL_FUN(	spell_bladebarrier	);
DECLARE_SPELL_FUN(	spell_bladeofvershak	);
DECLARE_SPELL_FUN(	spell_mireofoame	);
DECLARE_SPELL_FUN(	spell_blast		);
DECLARE_SPELL_FUN(	spell_blaze		);
DECLARE_SPELL_FUN(	spell_bless		);
DECLARE_SPELL_FUN(	spell_blindness		);
DECLARE_SPELL_FUN(      spell_blight            );
DECLARE_SPELL_FUN(	spell_blink		);
DECLARE_SPELL_FUN(	spell_blur		);
DECLARE_SPELL_FUN(	spell_brainwash		);
DECLARE_SPELL_FUN(	spell_burning_hands	);
DECLARE_SPELL_FUN(	spell_burnout		);
DECLARE_SPELL_FUN(	spell_call_lightning	);
DECLARE_SPELL_FUN(      spell_calm		);
DECLARE_SPELL_FUN(      spell_cancellation	);
DECLARE_SPELL_FUN(	spell_caressofpricina	);
DECLARE_SPELL_FUN(	spell_cause_critical	);
DECLARE_SPELL_FUN(	spell_cause_light	);
DECLARE_SPELL_FUN(	spell_cause_serious	);
DECLARE_SPELL_FUN(	spell_change_sex	);
DECLARE_SPELL_FUN(      spell_chain_lightning   );
DECLARE_SPELL_FUN(	spell_screamingarrow	);
DECLARE_SPELL_FUN(	spell_charm_person	);
DECLARE_SPELL_FUN(	spell_enervatingray	);
DECLARE_SPELL_FUN(	spell_coughing_dust	);
DECLARE_SPELL_FUN(	spell_cloudkill		);
DECLARE_SPELL_FUN(	spell_clumsiness	);
DECLARE_SPELL_FUN(	spell_colour_spray	);
DECLARE_SPELL_FUN(	spell_combustion	);
DECLARE_SPELL_FUN(	spell_communion		);
DECLARE_SPELL_FUN(	spell_coneofcold	);
DECLARE_SPELL_FUN(	spell_consuming_rage	);
DECLARE_SPELL_FUN(	spell_continual_light	);
DECLARE_SPELL_FUN(	spell_control_weather	);
DECLARE_SPELL_FUN(	spell_create_food	);
DECLARE_SPELL_FUN(	spell_create_rose	);
DECLARE_SPELL_FUN(	spell_create_spring	);
DECLARE_SPELL_FUN(	spell_create_water	);
DECLARE_SPELL_FUN(	spell_crystalizemagic	);
DECLARE_SPELL_FUN(	spell_cure_blindness	);
DECLARE_SPELL_FUN(	spell_cure_critical	);
DECLARE_SPELL_FUN(      spell_cure_disease	);
DECLARE_SPELL_FUN(	spell_cure_light	);
DECLARE_SPELL_FUN(	spell_cure_poison	);
DECLARE_SPELL_FUN(	spell_cure_serious	);
DECLARE_SPELL_FUN(	spell_cure_all_serious	);
DECLARE_SPELL_FUN(	spell_curse		);
DECLARE_SPELL_FUN(      spell_cursekijjasku     );
DECLARE_SPELL_FUN(	spell_dalliance		);
DECLARE_SPELL_FUN(	spell_defilementoflogor	);
DECLARE_SPELL_FUN(	spell_demoncall		);
DECLARE_SPELL_FUN(      spell_demonfire		);
DECLARE_SPELL_FUN(	spell_demoniccontrol	);
DECLARE_SPELL_FUN(	spell_desecration	);
DECLARE_SPELL_FUN(	spell_detect_evil	);
DECLARE_SPELL_FUN(	spell_detect_good	);
DECLARE_SPELL_FUN(	spell_detect_invis	);
DECLARE_SPELL_FUN(	spell_detect_magic	);
DECLARE_SPELL_FUN(	spell_detect_poison	);
DECLARE_SPELL_FUN(	spell_devotion		);
DECLARE_SPELL_FUN(	spell_disjunction	);
DECLARE_SPELL_FUN(	spell_dispel_evil	);
DECLARE_SPELL_FUN(      spell_dispel_good       );
DECLARE_SPELL_FUN(	spell_dispel_magic	);
DECLARE_SPELL_FUN(	spell_drain		);
DECLARE_SPELL_FUN(	spell_earthbind		);
DECLARE_SPELL_FUN(	spell_earthmaw		);
DECLARE_SPELL_FUN(	spell_earthquake	);
DECLARE_SPELL_FUN(	spell_enchant_armor	);
DECLARE_SPELL_FUN(	spell_enchant_weapon	);
DECLARE_SPELL_FUN(	spell_energy_drain	);
DECLARE_SPELL_FUN(      spell_embraceisetaton   );
DECLARE_SPELL_FUN(	spell_enfeeblement	);
DECLARE_SPELL_FUN(	spell_coronal_glow	);
DECLARE_SPELL_FUN(	spell_faerie_fog	);
DECLARE_SPELL_FUN(	spell_etherealblaze	);
DECLARE_SPELL_FUN(	spell_eyes_hunter	);
DECLARE_SPELL_FUN(	spell_farsight		);
DECLARE_SPELL_FUN(	spell_fever		);
DECLARE_SPELL_FUN(	spell_fireball		);
DECLARE_SPELL_FUN(	spell_firebolt		);
DECLARE_SPELL_FUN(	spell_firebrand		);
DECLARE_SPELL_FUN(	spell_fireproof		);
DECLARE_SPELL_FUN(	spell_firestorm		);
DECLARE_SPELL_FUN(	spell_flamestrike	);
DECLARE_SPELL_FUN(	spell_floating_disc	);
DECLARE_SPELL_FUN(	spell_flood		);
DECLARE_SPELL_FUN(	spell_fly		);
DECLARE_SPELL_FUN(	spell_fortify		);
DECLARE_SPELL_FUN(	spell_freeze		);
DECLARE_SPELL_FUN(      spell_frenzy		);
DECLARE_SPELL_FUN(	spell_frostbrand	);
DECLARE_SPELL_FUN(	spell_fury		);
DECLARE_SPELL_FUN(	spell_gate		);
DECLARE_SPELL_FUN(      spell_globedarkness     );
DECLARE_SPELL_FUN(	spell_giant_strength	);
DECLARE_SPELL_FUN(	spell_gust		);
DECLARE_SPELL_FUN(	spell_harm		);
DECLARE_SPELL_FUN(      spell_haste		);
DECLARE_SPELL_FUN(	spell_heal		);
DECLARE_SPELL_FUN(	spell_hungerofgrmlarloth);
DECLARE_SPELL_FUN(	spell_whirlpool		);
DECLARE_SPELL_FUN(	spell_revitalize	);
DECLARE_SPELL_FUN(	spell_massheal		);
DECLARE_SPELL_FUN(	spell_heat_metal	);
DECLARE_SPELL_FUN(      spell_holy_word		);
DECLARE_SPELL_FUN(	spell_drawblood		);
DECLARE_SPELL_FUN(	spell_iceblast		);
DECLARE_SPELL_FUN(	spell_icebolt		);
DECLARE_SPELL_FUN(	spell_oaklance		);
DECLARE_SPELL_FUN(	spell_iceshard		);
DECLARE_SPELL_FUN(	spell_icyprison		);
DECLARE_SPELL_FUN(	spell_icyshield		);
DECLARE_SPELL_FUN(	spell_identify		);
DECLARE_SPELL_FUN(	spell_mushroomcircle	);
DECLARE_SPELL_FUN(	spell_insectswarm	);
DECLARE_SPELL_FUN(	spell_animaltongues	);
DECLARE_SPELL_FUN(	spell_immolation	);
DECLARE_SPELL_FUN(	spell_infravision	);
DECLARE_SPELL_FUN(	spell_inferno		);
DECLARE_SPELL_FUN(	spell_inspire		);
DECLARE_SPELL_FUN(	spell_invis		);
DECLARE_SPELL_FUN(	spell_jawsofidcizon	);
DECLARE_SPELL_FUN(	spell_know_alignment	);
DECLARE_SPELL_FUN(	spell_leechrune		);
DECLARE_SPELL_FUN(	spell_lifeshield	);
DECLARE_SPELL_FUN(	spell_lightning_bolt	);
DECLARE_SPELL_FUN(	spell_lightningbrand	);
DECLARE_SPELL_FUN(	spell_lightning_storm	);
DECLARE_SPELL_FUN(	spell_locate_object	);
DECLARE_SPELL_FUN(	spell_moonsight		);
DECLARE_SPELL_FUN(	spell_magneticgrasp	);
DECLARE_SPELL_FUN(	spell_manabarbs		);
DECLARE_SPELL_FUN(	spell_mass_flying	);
DECLARE_SPELL_FUN(      spell_mass_healing	);
DECLARE_SPELL_FUN(	spell_mass_invis	);
DECLARE_SPELL_FUN(	spell_melt_metal	);
DECLARE_SPELL_FUN(	spell_mindshell		);
DECLARE_SPELL_FUN(	spell_mirrorimage	);
DECLARE_SPELL_FUN(	spell_missileattraction	);
DECLARE_SPELL_FUN(	spell_reflectiveaura	);
DECLARE_SPELL_FUN(	spell_mysticanchor	);
DECLARE_SPELL_FUN(	spell_nexus		);
DECLARE_SPELL_FUN(	spell_nightfears	);
DECLARE_SPELL_FUN(	spell_normaldemonsummon	);
DECLARE_SPELL_FUN(	spell_nova		);
DECLARE_SPELL_FUN(	spell_numb		);
DECLARE_SPELL_FUN(	spell_paradise		);
DECLARE_SPELL_FUN(	spell_parch		);
DECLARE_SPELL_FUN(	spell_pass_door		);
DECLARE_SPELL_FUN(	spell_petrify		);
DECLARE_SPELL_FUN(      spell_phasedoor         );
DECLARE_SPELL_FUN(      spell_plague		);
DECLARE_SPELL_FUN(	spell_plague_madness	);
DECLARE_SPELL_FUN(	spell_plantgrowth	);
DECLARE_SPELL_FUN(	spell_poison		);
DECLARE_SPELL_FUN(	spell_portal		);
DECLARE_SPELL_FUN(	spell_pox		);
DECLARE_SPELL_FUN(	spell_protection_evil	);
DECLARE_SPELL_FUN(	spell_protection_good	);
DECLARE_SPELL_FUN(	spell_protective_shield );
DECLARE_SPELL_FUN(	spell_protnormalmissiles);
DECLARE_SPELL_FUN(	spell_purify		);
DECLARE_SPELL_FUN(	spell_pyrotechnics	);
DECLARE_SPELL_FUN(	spell_quicksand		);
DECLARE_SPELL_FUN(	spell_rangedhealing	);
DECLARE_SPELL_FUN(	spell_ray_of_truth	);
DECLARE_SPELL_FUN(	spell_rayofpurity	);
DECLARE_SPELL_FUN(	spell_rearrange		);
DECLARE_SPELL_FUN(	spell_rebirth		);
DECLARE_SPELL_FUN(	spell_recharge		);
DECLARE_SPELL_FUN(	spell_refresh		);
DECLARE_SPELL_FUN(	spell_regeneration	);
DECLARE_SPELL_FUN(	spell_remove_curse	);
DECLARE_SPELL_FUN(	spell_resonance		);
DECLARE_SPELL_FUN(	spell_ringoffire	);
DECLARE_SPELL_FUN(	spell_riteofkaagn	);
DECLARE_SPELL_FUN(	spell_sanctuary		);
DECLARE_SPELL_FUN(	spell_sandspray		);
DECLARE_SPELL_FUN(	spell_sandstorm_old	);
DECLARE_SPELL_FUN(	spell_scare		);
DECLARE_SPELL_FUN(	spell_scorch		);
DECLARE_SPELL_FUN(	spell_shatter		);
DECLARE_SPELL_FUN(	spell_shocking_grasp	);
DECLARE_SPELL_FUN(	spell_shield		);
DECLARE_SPELL_FUN(      spell_siphonmana        );
DECLARE_SPELL_FUN(	spell_sleep		);
DECLARE_SPELL_FUN(	spell_slow		);
DECLARE_SPELL_FUN(	spell_smoke		);
DECLARE_SPELL_FUN(      spell_sanctify          );
DECLARE_SPELL_FUN(	spell_spirit_sanctuary	);
DECLARE_SPELL_FUN(	spell_soulburn		);
DECLARE_SPELL_FUN(	spell_stickstosnakes	);
DECLARE_SPELL_FUN(	spell_spiritwrack	);
DECLARE_SPELL_FUN(	spell_mantleofearth	);
DECLARE_SPELL_FUN(	spell_stone_skin	);
DECLARE_SPELL_FUN(	spell_stonefist		);
DECLARE_SPELL_FUN(	spell_subdue		);
DECLARE_SPELL_FUN(	spell_summon		);
DECLARE_SPELL_FUN(	spell_sustenance	);
DECLARE_SPELL_FUN(	spell_teleport		);
DECLARE_SPELL_FUN(	spell_scatter		);
DECLARE_SPELL_FUN(	spell_tempest		);
DECLARE_SPELL_FUN(	spell_windbind		);
DECLARE_SPELL_FUN(	spell_slow_cure_light	);
DECLARE_SPELL_FUN(	spell_slow_cure_serious );
DECLARE_SPELL_FUN(	spell_slow_cure_critical);
DECLARE_SPELL_FUN(	spell_slow_heal		);
DECLARE_SPELL_FUN(	spell_slow_cure_poison	);
DECLARE_SPELL_FUN(	spell_slow_cure_disease	);
//DECLARE_SPELL_FUN(	spell_thunderstrike	);
DECLARE_SPELL_FUN(	spell_powerwordfear	);
DECLARE_SPELL_FUN(	spell_veilsight		);
DECLARE_SPELL_FUN(	spell_vengeance		);
DECLARE_SPELL_FUN(	spell_ventriloquate	);
DECLARE_SPELL_FUN(	spell_visions		);
DECLARE_SPELL_FUN(      spell_vortex            );
DECLARE_SPELL_FUN(	spell_wallofair		);
DECLARE_SPELL_FUN(	spell_wallofwater	);
DECLARE_SPELL_FUN(	spell_wardoffire	);
DECLARE_SPELL_FUN(	spell_weaken		);
DECLARE_SPELL_FUN(	spell_water_breathing	);
DECLARE_SPELL_FUN(	spell_weariness		);
DECLARE_SPELL_FUN(	spell_windblast		);
DECLARE_SPELL_FUN(	spell_word_of_recall	);
DECLARE_SPELL_FUN(	spell_acid_breath	);
DECLARE_SPELL_FUN(	spell_fire_breath	);
DECLARE_SPELL_FUN(	spell_frost_breath	);
DECLARE_SPELL_FUN(	spell_gas_breath	);
DECLARE_SPELL_FUN(	spell_lightning_breath	);
DECLARE_SPELL_FUN(	spell_general_purpose	);
DECLARE_SPELL_FUN(	spell_high_explosive	);
DECLARE_SPELL_FUN(	spell_lesserhandwarp	);
DECLARE_SPELL_FUN(	spell_handwarp		);
DECLARE_SPELL_FUN(      spell_greaterhandwarp	);
DECLARE_SPELL_FUN(      spell_lessertentaclegrowth);
DECLARE_SPELL_FUN(      spell_greatertentaclegrowth);
DECLARE_SPELL_FUN(      spell_lessercarapace	);
DECLARE_SPELL_FUN(      spell_greatercarapace	);
DECLARE_SPELL_FUN(      spell_lycanthropy	);
DECLARE_SPELL_FUN(      spell_thirdeye		);
DECLARE_SPELL_FUN(      spell_cateyes		);
DECLARE_SPELL_FUN(      spell_eagleeyes		);
DECLARE_SPELL_FUN(      spell_gills		);
DECLARE_SPELL_FUN(      spell_drunkenness	);
DECLARE_SPELL_FUN(      spell_lovepotion	);
DECLARE_SPELL_FUN(      spell_susceptibility	);
DECLARE_SPELL_FUN(      spell_age		);
DECLARE_SPELL_FUN(      spell_polyglot		);
DECLARE_SPELL_FUN(      spell_resistance	);
DECLARE_SPELL_FUN(      spell_heroism		);
DECLARE_SPELL_FUN(      spell_divinesight	);
DECLARE_SPELL_FUN(      spell_youth		);
DECLARE_SPELL_FUN(      spell_invulnerability	);
DECLARE_SPELL_FUN(      spell_perfection	);
DECLARE_SPELL_FUN(      spell_improvement	);
DECLARE_SPELL_FUN(	spell_teleportcurse	);
DECLARE_SPELL_FUN(	spell_aviancurse	);
DECLARE_SPELL_FUN(	spell_prismaticray	);
DECLARE_SPELL_FUN(	spell_prismaticspray	);
DECLARE_SPELL_FUN(	spell_maze		);
DECLARE_SPELL_FUN(	spell_transmutation	);

DECLARE_SPELL_FUN(	spell_sealofthegoldenflames	);
DECLARE_SPELL_FUN(	spell_temper	            );
DECLARE_SPELL_FUN(	spell_flamesight	        );
DECLARE_SPELL_FUN(	spell_bloodpyre             );
DECLARE_SPELL_FUN(	spell_heartoftheinferno     );
DECLARE_SPELL_FUN(	spell_heartfire             );
DECLARE_SPELL_FUN(	spell_heatmine              );
DECLARE_SPELL_FUN(	spell_wrathofanakarta       );
DECLARE_SPELL_FUN(	spell_flameunity            );
DECLARE_SPELL_FUN(	spell_conflagration         );
DECLARE_SPELL_FUN(	spell_aspectoftheinferno    );

DECLARE_SPELL_FUN(	spell_moltenshield          );
DECLARE_SPELL_FUN(	spell_summonlavaelemental   );
DECLARE_SPELL_FUN(	spell_harrudimfire          );

DECLARE_SPELL_FUN(  spell_chillfireshield       );
DECLARE_SPELL_FUN(  spell_nightflamelash        );
DECLARE_SPELL_FUN(  spell_ashesoflogor          );

DECLARE_SPELL_FUN(  spell_burningwisp           );
DECLARE_SPELL_FUN(  spell_heatwave              );
DECLARE_SPELL_FUN(  spell_pyrokineticmirror     );

DECLARE_SPELL_FUN(  spell_sauna                 );
DECLARE_SPELL_FUN(  spell_flamesofthemartyr     );
DECLARE_SPELL_FUN(  spell_martyrsfire           );
DECLARE_SPELL_FUN(  spell_boilblood             );

DECLARE_SPELL_FUN(  spell_soulfireshield        );
DECLARE_SPELL_FUN(  spell_oriflamme             );
DECLARE_SPELL_FUN(  spell_holyflame             );
DECLARE_SPELL_FUN(  spell_soulbrand             );
DECLARE_SPELL_FUN(  spell_manaburn              );

DECLARE_SPELL_FUN(  spell_aetherealcommunion    );
DECLARE_SPELL_FUN(  spell_unfettermana          );
DECLARE_SPELL_FUN(  spell_dreamshape            );
DECLARE_SPELL_FUN(  spell_pacify                );
DECLARE_SPELL_FUN(  spell_absolvespirit         );
DECLARE_SPELL_FUN(  spell_unweave               );
DECLARE_SPELL_FUN(  spell_crystallizeaether     );
DECLARE_SPELL_FUN(  spell_leapoffaith           );
DECLARE_SPELL_FUN(  spell_phaseshift            );
DECLARE_SPELL_FUN(  spell_searinglight          );
DECLARE_SPELL_FUN(  spell_aegisofgrace          );
DECLARE_SPELL_FUN(  spell_wardofgrace           );
DECLARE_SPELL_FUN(  spell_undyingradiance       );
DECLARE_SPELL_FUN(  spell_riteofablution        );
DECLARE_SPELL_FUN(  spell_bondofsouls           );
DECLARE_SPELL_FUN(  spell_spectrallantern       );
DECLARE_SPELL_FUN(  spell_shadeswarm            );
DECLARE_SPELL_FUN(  spell_fugue                 );
DECLARE_SPELL_FUN(  spell_etherealbrethren      );
DECLARE_SPELL_FUN(  spell_attunefount           );
DECLARE_SPELL_FUN(  spell_singularity           );
DECLARE_SPELL_FUN(  spell_leypulse              );
DECLARE_SPELL_FUN(  spell_quintessencerush      );
DECLARE_SPELL_FUN(  spell_manifestweave         );
DECLARE_SPELL_FUN(  spell_sunderweave           );
DECLARE_SPELL_FUN(  spell_etherealsplendor      );
DECLARE_SPELL_FUN(  spell_rebukeofinvesi        );
DECLARE_SPELL_FUN(  spell_wardoftheshieldbearer );
DECLARE_SPELL_FUN(  spell_callhost              );
DECLARE_SPELL_FUN(  spell_bindessence           );
DECLARE_SPELL_FUN(  spell_workessence           );
DECLARE_SPELL_FUN(  spell_forgepostern          );
DECLARE_SPELL_FUN(  spell_sealpostern           );
DECLARE_SPELL_FUN(  spell_avatarofthelodestar   );
DECLARE_SPELL_FUN(  spell_avataroftheprotector  );
DECLARE_SPELL_FUN(  spell_avataroftheannointed  );
DECLARE_SPELL_FUN(  spell_nourishspirit         );

DECLARE_SPELL_FUN(  spell_balmofthespirit         );
DECLARE_SPELL_FUN(  spell_martyrsshield           );
DECLARE_SPELL_FUN(  spell_distillmagic            );
DECLARE_SPELL_FUN(  spell_annointedone            );

DECLARE_SPELL_FUN(  spell_holyground              );
DECLARE_SPELL_FUN(  spell_crystalsoul             );
DECLARE_SPELL_FUN(  spell_stoneloupe              );
DECLARE_SPELL_FUN(  spell_earthenvessel           );

DECLARE_SPELL_FUN(  spell_dreammastery            );
DECLARE_SPELL_FUN(  spell_drowse                  );
DECLARE_SPELL_FUN(  spell_dreamstalk              );
DECLARE_SPELL_FUN(  spell_parasiticbond           );

DECLARE_SPELL_FUN(  spell_bilocation              );
DECLARE_SPELL_FUN(  spell_aurora                  );
DECLARE_SPELL_FUN(  spell_diffraction             );

DECLARE_SPELL_FUN(  spell_refinepotion          );
DECLARE_SPELL_FUN(  spell_wellspring            );
DECLARE_SPELL_FUN(  spell_frostblast            );
DECLARE_SPELL_FUN(  spell_markofthekaceajka     );
DECLARE_SPELL_FUN(  spell_draughtoftheseas      );
DECLARE_SPELL_FUN(  spell_physikersinstinct     );
DECLARE_SPELL_FUN(  spell_oceanswell            );
DECLARE_SPELL_FUN(  spell_maelstrom             );
DECLARE_SPELL_FUN(  spell_drown                 );
DECLARE_SPELL_FUN(  spell_arcticchill           );
DECLARE_SPELL_FUN(  spell_breathofelanthemir    );
DECLARE_SPELL_FUN(  spell_glaciersedge          );
DECLARE_SPELL_FUN(  spell_wintersstronghold     );
DECLARE_SPELL_FUN(  spell_wardoffrost           );
DECLARE_SPELL_FUN(  spell_glyphofulyon          );
DECLARE_SPELL_FUN(  spell_solaceoftheseas       );
DECLARE_SPELL_FUN(  spell_hoarfrost             );
DECLARE_SPELL_FUN(  spell_ordainsanctum         );
DECLARE_SPELL_FUN(  spell_corrosion             );
DECLARE_SPELL_FUN(  spell_contaminate           );
DECLARE_SPELL_FUN(  spell_darkchillburst        );
DECLARE_SPELL_FUN(  spell_stoneofsalyra         );
DECLARE_SPELL_FUN(  spell_fogelementalsummon    );
DECLARE_SPELL_FUN(  spell_monsoon               );
DECLARE_SPELL_FUN(  spell_brume                 );
DECLARE_SPELL_FUN(  spell_steam                 );
DECLARE_SPELL_FUN(  spell_boilseas              );

DECLARE_SPELL_FUN(  spell_overcharge            );
DECLARE_SPELL_FUN(  spell_clingingfog           );
DECLARE_SPELL_FUN(  spell_gralcianfunnel        );
DECLARE_SPELL_FUN(  spell_illusion              );
DECLARE_SPELL_FUN(  spell_borrowluck            );
DECLARE_SPELL_FUN(  spell_conjureairefreeti     );
DECLARE_SPELL_FUN(  spell_mistralward           );
DECLARE_SPELL_FUN(  spell_fatesdoor             );
DECLARE_SPELL_FUN(  spell_mirage                );
DECLARE_SPELL_FUN(  spell_sparkingcloud         );
DECLARE_SPELL_FUN(  spell_runeofair             );
DECLARE_SPELL_FUN(  spell_unrealincursion       );
DECLARE_SPELL_FUN(  spell_figmentscage          );
DECLARE_SPELL_FUN(  spell_phantasmalmirror      );
DECLARE_SPELL_FUN(  spell_empowerphantasm       );
DECLARE_SPELL_FUN(  spell_englamour             );
DECLARE_SPELL_FUN(  spell_sonicboom             );
DECLARE_SPELL_FUN(  spell_electricalstorm       );
DECLARE_SPELL_FUN(  spell_ionize                );
DECLARE_SPELL_FUN(  spell_skystrike             );
DECLARE_SPELL_FUN(  spell_joltward              );
DECLARE_SPELL_FUN(  spell_displacement          );
DECLARE_SPELL_FUN(  spell_typhoon               );
DECLARE_SPELL_FUN(  spell_unleashtwisters       );
DECLARE_SPELL_FUN(  spell_breezestep            );
DECLARE_SPELL_FUN(  spell_curseofeverchange     );
DECLARE_SPELL_FUN(  spell_beckonwindfall        );
DECLARE_SPELL_FUN(  spell_calluponwind          );
DECLARE_SPELL_FUN(  spell_mistsofarcing         );
DECLARE_SPELL_FUN(  spell_breathoflife          );
DECLARE_SPELL_FUN(  spell_mantleofrain          );
DECLARE_SPELL_FUN(  spell_sandstorm             );
DECLARE_SPELL_FUN(  spell_channelwind           );
DECLARE_SPELL_FUN(  spell_chargestone           );
DECLARE_SPELL_FUN(  spell_hoveringshield        );
DECLARE_SPELL_FUN(  spell_miasmaofwaning        );
DECLARE_SPELL_FUN(  spell_bewilderment          );

DECLARE_SPELL_FUN(  spell_clayshield);
DECLARE_SPELL_FUN(  spell_mudfootcurse);
DECLARE_SPELL_FUN(  spell_reforgemagic);
DECLARE_SPELL_FUN(  spell_tuningstone);
DECLARE_SPELL_FUN(  spell_reinforce);
DECLARE_SPELL_FUN(  spell_honeweapon);
DECLARE_SPELL_FUN(  spell_shellofstone);
DECLARE_SPELL_FUN(  spell_quake);
DECLARE_SPELL_FUN(  spell_rocktomud);
DECLARE_SPELL_FUN(  spell_shapematter);
DECLARE_SPELL_FUN(  spell_crush);
DECLARE_SPELL_FUN(  spell_quarry);
DECLARE_SPELL_FUN(  spell_gravenmind);
DECLARE_SPELL_FUN(  spell_markofloam);
DECLARE_SPELL_FUN(  spell_glyphofentombment);
DECLARE_SPELL_FUN(  spell_latticeofstone);
DECLARE_SPELL_FUN(  spell_saltoftheearth);
DECLARE_SPELL_FUN(  spell_cryofthebrokenlands);
DECLARE_SPELL_FUN(  spell_shakestride);
DECLARE_SPELL_FUN(  spell_stonehaven);
DECLARE_SPELL_FUN(  spell_conduitofstonesong);
DECLARE_SPELL_FUN(  spell_causticblast);
DECLARE_SPELL_FUN(  spell_clockworkgolem);
DECLARE_SPELL_FUN(  spell_kaagnsplaguestone);
DECLARE_SPELL_FUN(  spell_heartofstone);
DECLARE_SPELL_FUN(  spell_abodeofthespirit);
DECLARE_SPELL_FUN(  spell_mindofsteel);
DECLARE_SPELL_FUN(  spell_pillarofsparks);
DECLARE_SPELL_FUN(  spell_dispatchlodestone);
DECLARE_SPELL_FUN(  spell_scorchedearth);
DECLARE_SPELL_FUN(  spell_lavaforge);

DECLARE_SPELL_FUN(  spell_darktallow);
DECLARE_SPELL_FUN(  spell_wreathoffear);
DECLARE_SPELL_FUN(  spell_scritofkaagn);
DECLARE_SPELL_FUN(  spell_callbat);
DECLARE_SPELL_FUN(  spell_callcat);
DECLARE_SPELL_FUN(  spell_callraven);
DECLARE_SPELL_FUN(  spell_callfox);
DECLARE_SPELL_FUN(  spell_calltoad);
DECLARE_SPELL_FUN(  spell_callserpent);
DECLARE_SPELL_FUN(  spell_harvestofsouls);
DECLARE_SPELL_FUN(  spell_unholymight);
DECLARE_SPELL_FUN(  spell_deathlyvisage);
DECLARE_SPELL_FUN(  spell_blackamulet);
DECLARE_SPELL_FUN(  spell_barrowmist);
DECLARE_SPELL_FUN(  spell_imbuephylactery);
DECLARE_SPELL_FUN(  spell_reaping);
DECLARE_SPELL_FUN(  spell_fadeshroud);
DECLARE_SPELL_FUN(  spell_deathswarm);
DECLARE_SPELL_FUN(  spell_direfeast);
DECLARE_SPELL_FUN(  spell_duskfall);
DECLARE_SPELL_FUN(  spell_eyeblighttouch);
DECLARE_SPELL_FUN(  spell_dreadwave);
DECLARE_SPELL_FUN(  spell_shadowfiend);
DECLARE_SPELL_FUN(  spell_nightstalk);
DECLARE_SPELL_FUN(  spell_webofoame);
DECLARE_SPELL_FUN(  spell_gaveloflogor);
DECLARE_SPELL_FUN(  spell_fetiddivination);
DECLARE_SPELL_FUN(  spell_grimseep);
DECLARE_SPELL_FUN(  spell_bloodofthevizier);
DECLARE_SPELL_FUN(  spell_seedofmadness);
DECLARE_SPELL_FUN(  spell_bierofunmaking);
DECLARE_SPELL_FUN(  spell_stasisrift);
DECLARE_SPELL_FUN(  spell_hemoplague);
DECLARE_SPELL_FUN(  spell_baneblade);
DECLARE_SPELL_FUN(  spell_gravemaw);
DECLARE_SPELL_FUN(  spell_devouressence);
DECLARE_SPELL_FUN(  spell_feverwinds);
DECLARE_SPELL_FUN(  spell_fatebones);
DECLARE_SPELL_FUN(  spell_masquerade);
DECLARE_SPELL_FUN(  spell_barbsofalthajji);
DECLARE_SPELL_FUN(  spell_focusfury);

/* SONGS! */
DECLARE_SPELL_FUN(	song_courage		);
DECLARE_SPELL_FUN(	song_painful_thoughts	);
DECLARE_SPELL_FUN(	song_dispel_illusion	);
DECLARE_SPELL_FUN(	spell_windsurge		);
DECLARE_SPELL_FUN(	spell_compact_of_Logor  );
DECLARE_SPELL_FUN(	spell_aura_of_corruption);
DECLARE_SPELL_FUN(	spell_jawsofthemountain	);

void do_mprog_cast(CHAR_DATA *ch, int sn, int level, CHAR_DATA *mptarget);
extern bool saves_dispel( int dis_level, int spell_level, int duration );

extern char *target_name;