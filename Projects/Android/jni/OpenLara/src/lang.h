#ifndef H_LANG
#define H_LANG

// Thanks: SuiKaze Raider

enum StringID {
      STR_EMPTY
// common
    , STR_LOADING
    , STR_HELP_PRESS
    , STR_HELP_TEXT
    , STR_LEVEL_STATS
    , STR_HINT_SAVING
    , STR_HINT_SAVING_DONE
    , STR_HINT_SAVING_ERROR
    , STR_YES
    , STR_NO
    , STR_OFF
    , STR_ON
    , STR_RIGHT_HANDED
    , STR_LEFT_HANDED
    , STR_SNAP_TURN
    , STR_SNAP_AND_SMOOTH
    , STR_SMOOTH_TURN_SLOW
    , STR_SMOOTH_TURN_FAST
    , STR_QUALITY_LOW
    , STR_QUALITY_MEDIUM
    , STR_QUALITY_HIGH
    , STR_LANG_EN
    , STR_LANG_FR
    , STR_LANG_DE
    , STR_LANG_ES
    , STR_LANG_IT
    , STR_LANG_PL
    , STR_LANG_PT
    , STR_LANG_RU
    , STR_LANG_JA
    , STR_LANG_GR
    , STR_LANG_FI
    , STR_LANG_CZ
    , STR_LANG_CN
    , STR_APPLY
    , STR_GAMEPAD_1
    , STR_GAMEPAD_2
    , STR_GAMEPAD_3
    , STR_GAMEPAD_4
    , STR_NOT_READY
    , STR_PLAYER_1
    , STR_PLAYER_2
    , STR_PRESS_ANY_KEY
    , STR_HELP_SELECT
    , STR_HELP_BACK
// inventory pages
    , STR_OPTION
    , STR_INVENTORY
    , STR_ITEMS
// save game page
    , STR_SAVEGAME
    , STR_CURRENT_POSITION
// inventory option
    , STR_GAME
    , STR_MAP
    , STR_COMPASS
    , STR_STOPWATCH
    , STR_HOME
    , STR_DETAIL
    , STR_SOUND
    , STR_CONTROLS
    , STR_GAMMA
// passport menu
    , STR_LOAD_GAME
    , STR_START_GAME
    , STR_RESTART_LEVEL
    , STR_EXIT_TO_TITLE
    , STR_EXIT_GAME
    , STR_SELECT_LEVEL
// detail options
    , STR_SELECT_DETAIL
    , STR_OPT_DETAIL_FILTER
    , STR_OPT_DETAIL_LIGHTING
    , STR_OPT_DETAIL_SHADOWS
    , STR_OPT_DETAIL_WATER
    , STR_OPT_DETAIL_VSYNC
    , STR_OPT_DETAIL_HANDEDNESS
    , STR_OPT_DETAIL_CAM_MODE
    , STR_OPT_DETAIL_TURNMODE
    , STR_OPT_DETAIL_AUTOAIM
    , STR_OPT_RESOLUTION
    , STR_SCALE_100
    , STR_SCALE_75
    , STR_SCALE_50
    , STR_SCALE_25
// sound options
    , STR_SET_VOLUMES
    , STR_REVERBERATION
    , STR_OPT_SUBTITLES
    , STR_OPT_LANGUAGE
// controls options
    , STR_SET_GAME_OPTIONS
    , STR_OPT_CONTROLS_KEYBOARD
    , STR_OPT_CONTROLS_GAMEPAD
    , STR_OPT_CONTROLS_VIBRATION
    , STR_OPT_CONTROLS_RETARGET
    , STR_OPT_CONTROLS_MULTIAIM
    // controls
    , STR_CTRL_FIRST
    , STR_CTRL_LAST = STR_CTRL_FIRST + cMAX - 1
    // keys
    , STR_KEY_FIRST
    , STR_KEY_LAST  = STR_KEY_FIRST + ikBack
    // gamepad
    , STR_JOY_FIRST
    , STR_JOY_LAST  = STR_JOY_FIRST + jkMAX - 1
// inventory items
    , STR_UNKNOWN
    , STR_EXPLOSIVE
    , STR_PISTOLS
    , STR_SHOTGUN
    , STR_MAGNUMS
    , STR_UZIS
    , STR_AMMO_PISTOLS
    , STR_AMMO_SHOTGUN
    , STR_AMMO_MAGNUMS
    , STR_AMMO_UZIS
    , STR_MEDI_SMALL
    , STR_MEDI_BIG
    , STR_LEAD_BAR
    , STR_SCION
// keys
    , STR_KEY
    , STR_KEY_SILVER
    , STR_KEY_RUSTY
    , STR_KEY_GOLD
    , STR_KEY_SAPPHIRE
    , STR_KEY_NEPTUNE
    , STR_KEY_ATLAS
    , STR_KEY_DAMOCLES
    , STR_KEY_THOR
    , STR_KEY_ORNATE
// puzzles
    , STR_PUZZLE
    , STR_PUZZLE_GOLD_IDOL
    , STR_PUZZLE_GOLD_BAR
    , STR_PUZZLE_COG
    , STR_PUZZLE_FUSE
    , STR_PUZZLE_ANKH
    , STR_PUZZLE_HORUS
    , STR_PUZZLE_ANUBIS
    , STR_PUZZLE_SCARAB
    , STR_PUZZLE_PYRAMID
// TR1 subtitles
    , STR_TR1_SUB_CAFE
    , STR_TR1_SUB_LIFT
    , STR_TR1_SUB_CANYON
    , STR_TR1_SUB_PRISON
    , STR_TR1_SUB_22 // CUT4
    , STR_TR1_SUB_23 // CUT1
    , STR_TR1_SUB_24
    , STR_TR1_SUB_25 // CUT3
    , STR_TR1_SUB_26
    , STR_TR1_SUB_27
    , STR_TR1_SUB_28
    , STR_TR1_SUB_29
    , STR_TR1_SUB_30
    , STR_TR1_SUB_31
    , STR_TR1_SUB_32
    , STR_TR1_SUB_33
    , STR_TR1_SUB_34
    , STR_TR1_SUB_35
    , STR_TR1_SUB_36
    , STR_TR1_SUB_37
    , STR_TR1_SUB_38
    , STR_TR1_SUB_39
    , STR_TR1_SUB_40
    , STR_TR1_SUB_41
    , STR_TR1_SUB_42
    , STR_TR1_SUB_43
    , STR_TR1_SUB_44
    , STR_TR1_SUB_45
    , STR_TR1_SUB_46
    , STR_TR1_SUB_47
    , STR_TR1_SUB_48
    , STR_TR1_SUB_49
    , STR_TR1_SUB_50
    , STR_TR1_SUB_51
    , STR_TR1_SUB_52
    , STR_TR1_SUB_53
    , STR_TR1_SUB_54
    , STR_TR1_SUB_55
    , STR_TR1_SUB_56
// TR1 levels
    , STR_TR1_GYM
    , STR_TR1_LEVEL1
    , STR_TR1_LEVEL2
    , STR_TR1_LEVEL3A
    , STR_TR1_LEVEL3B
    , STR_TR1_LEVEL4
    , STR_TR1_LEVEL5
    , STR_TR1_LEVEL6
    , STR_TR1_LEVEL7A
    , STR_TR1_LEVEL7B
    , STR_TR1_LEVEL8A
    , STR_TR1_LEVEL8B
    , STR_TR1_LEVEL8C
    , STR_TR1_LEVEL10A
    , STR_TR1_LEVEL10B
    , STR_TR1_LEVEL10C
    , STR_TR1_EGYPT
    , STR_TR1_CAT
    , STR_TR1_END
    , STR_TR1_END2
// TR2 levels
    , STR_TR2_ASSAULT
    , STR_TR2_WALL
    , STR_TR2_BOAT
    , STR_TR2_VENICE
    , STR_TR2_OPERA
    , STR_TR2_RIG
    , STR_TR2_PLATFORM
    , STR_TR2_UNWATER
    , STR_TR2_KEEL
    , STR_TR2_LIVING
    , STR_TR2_DECK
    , STR_TR2_SKIDOO
    , STR_TR2_MONASTRY
    , STR_TR2_CATACOMB
    , STR_TR2_ICECAVE
    , STR_TR2_EMPRTOMB
    , STR_TR2_FLOATING
    , STR_TR2_XIAN
    , STR_TR2_HOUSE
// TR3 levels
    , STR_TR3_HOUSE
    , STR_TR3_JUNGLE
    , STR_TR3_TEMPLE
    , STR_TR3_QUADCHAS
    , STR_TR3_TONYBOSS
    , STR_TR3_SHORE
    , STR_TR3_CRASH
    , STR_TR3_RAPIDS
    , STR_TR3_TRIBOSS
    , STR_TR3_ROOFS
    , STR_TR3_SEWER
    , STR_TR3_TOWER
    , STR_TR3_OFFICE
    , STR_TR3_NEVADA
    , STR_TR3_COMPOUND
    , STR_TR3_AREA51
    , STR_TR3_ANTARC
    , STR_TR3_MINES
    , STR_TR3_CITY
    , STR_TR3_CHAMBER
    , STR_TR3_STPAUL


// VR ADDITIONS
    ,STR_OPT_DETAIL_BRAID
    ,STR_OPT_DETAIL_INVERT_STICK_SWIMMING
    ,STR_OPT_DETAIL_POV
    ,STR_OPT_DETAIL_POV_1
    ,STR_OPT_DETAIL_POV_2
    ,STR_OPT_DETAIL_POV_3
    ,STR_OPT_DETAIL_TOY_MODE
    ,STR_OPT_DETAIL_HIDE_IK_BODY
    ,STR_OPT_DETAIL_AUTO_3RD_PERSON
    ,STR_OPT_DETAIL_MIXED_REALITY
    ,STR_MR_OFF
    ,STR_MR_ON
    ,STR_MR_BATTERY_SAVER
    ,STR_OPT_DETAIL_FIXED_CAMERA
    ,STR_OPT_DETAIL_SKY
    ,STR_OPT_CAM_MODE_MODERN
    ,STR_OPT_CHASE_CAM_MODERN
    ,STR_OPT_CHASE_CAM_CLASSIC

    ,STR_OPT_GAME_DIFFICULTY
    ,STR_OPT_GAME_DIFFICULTY_EASY
    ,STR_OPT_GAME_DIFFICULTY_NORMAL
    ,STR_OPT_GAME_DIFFICULTY_HARD

//VERSION
    ,STR_BEEFRAIDERXR_VERSION

    , STR_MAX
};

#define STR_LANGUAGES \
      "English"       \
    , "Fran|cais"     \
    , "Deutsch"       \
    , "Espa+nol"      \
    , "Italiano"      \
    , "Polski"        \
    , "Portugu(es"    \
    , "������{�"      \
    , "\x11\x02\x70\x01\x97\x01\xD6\xFF\xFF" \
    , "\x11\x01\x22\x01\x0F\x01\x0F\x01\x0E\x01\x06\x01\x04\x01\x0C\x01\x0B\xFF\xFF" \
    , "Suomi"         \
    , "{Cesky"        \
    , "\x11\x02\x8A\x02\x6C\x01\x54\x03\x02\xFF\xFF"

#define LANG_PREFIXES "_EN", "_FR", "_DE", "_ES", "_IT", "_PL", "_PT", "_RU", "_JA", "_GR", "_FI", "_CZ", "_CN"

#define STR_KEYS \
      "NONE", "LEFT", "RIGHT", "UP", "DOWN", "SPACE", "TAB", "ENTER", "ESCAPE", "SHIFT", "CTRL", "ALT" \
    , "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" \
    , "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M" \
    , "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" \
    , "PAD0", "PAD1", "PAD2", "PAD3", "PAD4", "PAD5", "PAD6", "PAD7", "PAD8", "PAD9", "PAD+", "PAD-", "PADx", "PAD/", "PAD." \
    , "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" \
    , "-", "+", "<", ">", "/", "\\", ",", ".", "$", ":", "'", "PGUP", "PGDN", "HOME", "END", "DEL", "INS", "BKSP" \
    , "NONE", "A", "B", "X", "Y", "L BUMPER", "R BUMPER", "SELECT", "START", "L STICK", "R STICK", "L TRIGGER", "R TRIGGER", "D-LEFT", "D-RIGHT", "D-UP", "D-DOWN"

#define STR_SCALE "25", "50", "75", "100"

const char *helpText = 
    "Start - add second player or restore Lara@"
    "H - Show or hide this help@"
    "ALT and ENTER - Fullscreen@"
    "5 - Save Game@"
    "9 - Load Game@"
    "C - Look@"
    "R - Slow motion@"
    "T - Fast motion@"
    "Roll - Up & Down@"
    "Step Left - Walk & Left@"
    "Step Right - Walk & Right@"
    "Out of water - Up & Action@"
    "Handstand - Up & Walk@"
    "Swan dive - Up & Walk & Jump@"
    "First Person View - Look & Action@"
    "DOZY on - Look & Duck & Action & Jump@"
    "DOZY off - Walk@"
    "Free Camera - hold L & R stick";

#define BEEFRAIDERXR_VERSION    "BeefRaiderXR 1.0.1"

#include "lang/en.h"
#include "lang/fr.h"
#include "lang/de.h"
#include "lang/es.h"
#include "lang/it.h"
#include "lang/pl.h"
#include "lang/pt.h"
#include "lang/ru.h"
#include "lang/ja.h"
#include "lang/gr.h"
#include "lang/fi.h"
#include "lang/cz.h"
#include "lang/cn.h"

char **STR = NULL;

void ensureLanguage(int lang) {
    ASSERT(COUNT(STR_EN) == STR_MAX);
    ASSERT(COUNT(STR_FR) == STR_MAX);
    ASSERT(COUNT(STR_DE) == STR_MAX);
    ASSERT(COUNT(STR_ES) == STR_MAX);
    ASSERT(COUNT(STR_IT) == STR_MAX);
    ASSERT(COUNT(STR_PL) == STR_MAX);
    ASSERT(COUNT(STR_PT) == STR_MAX);
    ASSERT(COUNT(STR_RU) == STR_MAX);
    ASSERT(COUNT(STR_JA) == STR_MAX);
    ASSERT(COUNT(STR_GR) == STR_MAX);
    ASSERT(COUNT(STR_FI) == STR_MAX);
    ASSERT(COUNT(STR_CZ) == STR_MAX);
    ASSERT(COUNT(STR_CN) == STR_MAX);

    lang += STR_LANG_EN;

    switch (lang) {
        case STR_LANG_FR : STR = (char**)STR_FR; break;
        case STR_LANG_DE : STR = (char**)STR_DE; break;
        case STR_LANG_ES : STR = (char**)STR_ES; break;
        case STR_LANG_IT : STR = (char**)STR_IT; break;
        case STR_LANG_PL : STR = (char**)STR_PL; break;
        case STR_LANG_PT : STR = (char**)STR_PT; break;
        case STR_LANG_RU : STR = (char**)STR_RU; break;
        case STR_LANG_JA : STR = (char**)STR_JA; break;
        case STR_LANG_GR : STR = (char**)STR_GR; break;
        case STR_LANG_FI : STR = (char**)STR_FI; break;
        case STR_LANG_CZ : STR = (char**)STR_CZ; break;
        case STR_LANG_CN : STR = (char**)STR_CN; break;
        default          : STR = (char**)STR_EN; break;
    }
}

#endif
