#pragma once
#define FONT_SMALL_DEV "fonts/smallDevFont"
#define FONT_BIG_DEV "fonts/bigDevFont"
#define FONT_CONSOLE "fonts/consoleFont"
#define FONT_BIG "fonts/bigFont"
#define FONT_SMALL "fonts/smallFont"
#define FONT_BOLD "fonts/boldFont"
#define FONT_NORMAL "fonts/normalFont"
#define FONT_EXTRA_BIG "fonts/extraBigFont"
#define FONT_OBJECTIVE "fonts/objectiveFont"

#define MTL_ARG_MATERIAL_VERTEX_CONST 0x0
#define MTL_ARG_LITERAL_VERTEX_CONST 0x1
#define MTL_ARG_MATERIAL_PIXEL_SAMPLER 0x2
#define MTL_ARG_CODE_VERTEX_CONST 0x3
#define MTL_ARG_CODE_PIXEL_SAMPLER 0x4
#define MTL_ARG_CODE_PIXEL_CONST 0x5
#define MTL_ARG_MATERIAL_PIXEL_CONST 0x6
#define MTL_ARG_LITERAL_PIXEL_CONST 0x7

#define TS_2D 0x0
#define TS_FUNCTION 0x1
#define TS_COLOR_MAP 0x2
#define TS_UNUSED_1 0x3
#define TS_UNUSED_2 0x4
#define TS_NORMAL_MAP 0x5
#define TS_UNUSED_3 0x6
#define TS_UNUSED_4 0x7
#define TS_SPECULAR_MAP 0x8
#define TS_UNUSED_5 0x9
#define TS_UNUSED_6 0xA
#define TS_WATER_MAP 0xB

#define CONTENTS_SOLID 0x1		   // An eye is never valid in a solid
#define CONTENTS_WINDOW 0x2		   // Translucent, but not watery (glass)
#define CONTENTS_GRATE 0x8		   // Alpha-tested "grate" textures. Bullets/sight pass through, but solids don't
#define CONTENTS_MOVEABLE 0x4000   // Hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MONSTER 0x2000000 // Should never be on a brush, only in game

#define MASK_SOLID (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE)
#define MASK_SHOT (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | 0x4000000 | 0x40000000)
#define MASK_PLAYERSOLID 0x02810011

#define GENTITYNUM_BITS 10
#define MAX_GENTITIES (1 << GENTITYNUM_BITS)
#define ENTITYNUM_NONE 1023

#define KEY_MASK_FIRE 1
#define KEY_MASK_SPRINT 2
#define KEY_MASK_MELEE 4
#define KEY_MASK_RELOAD 16
#define KEY_MASK_LEANLEFT 64
#define KEY_MASK_FORWARD 127
#define KEY_MASK_MOVERIGHT 127
#define KEY_MASK_LEANRIGHT 128
#define KEY_MASK_BACK 129
#define KEY_MASK_MOVELEFT 129
#define KEY_MASK_PRONE 256
#define KEY_MASK_CROUCH 512
#define KEY_MASK_JUMP 1024
#define KEY_MASK_ADS_MODE 2048
#define KEY_MASK_TEMP_ACTION 4096
#define KEY_MASK_HOLDBREATH 8192
#define KEY_MASK_FRAG 16384
#define KEY_MASK_SMOKE 32768
#define KEY_MASK_NIGHTVISION 262144
#define KEY_MASK_ADS 524288
#define KEY_MASK_USE 8
#define KEY_MASK_USERELOAD 32
