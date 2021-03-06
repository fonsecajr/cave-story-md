#ifndef INC_SHEET_H_
#define INC_SHEET_H_

#include <genesis.h>
#include "common.h"
#include "weapon.h"

/*
 * Handles pre-loaded shared sprite sheets and individually owned sprite tile allocations
 */

#define MAX_SHEETS	24
#define MAX_TILOCS	96

#define NOSHEET 255
#define NOTILOC 255
 
// Reduces the copy paste mess of VDP_loadTileData calls
#define SHEET_ADD(sheetid, sdef, frames, width, height, ...) {                                            \
	if(sheet_num < MAX_SHEETS) {                                                               \
		u16 index = sheet_num ? sheets[sheet_num-1].index + sheets[sheet_num-1].size           \
							  : TILE_SHEETINDEX;                                               \
		sheets[sheet_num] = (Sheet) { sheetid, frames*width*height, index, width, height };    \
		tiloc_index = sheets[sheet_num].index + sheets[sheet_num].size;                        \
		SHEET_LOAD(sdef, frames, (width)*(height), sheets[sheet_num].index, 1, __VA_ARGS__);   \
		for(u8 i = 0; i < 16; i++) frameOffset[sheet_num][i] = width * height * i;             \
		sheet_num++;                                                                           \
	}                                                                                          \
}
// Replaces the tiles in a previously loaded sprite sheet
#define SHEET_MOD(sheetid, sdef, frames, width, height, ...) {                                 \
	u8 sindex = NOSHEET;                                                                       \
	SHEET_FIND(sindex, sheetid);                                                               \
	if(sindex != NOSHEET) {                                                                    \
		SHEET_LOAD(sdef, frames, (width)*(height), sheets[sindex].index, 1, __VA_ARGS__);      \
	}                                                                                          \
}
// The end params are anim,frame value couples from the sprite definition
#define SHEET_LOAD(sdef, frames, fsize, index, dma, ...) {                                     \
	const u8 fa[frames<<1] = { __VA_ARGS__ };                                                  \
	for(u8 i = 0; i < frames; i++) {                                                           \
		VDP_loadTileData(SPR_TILES(sdef,fa[i<<1],fa[(i<<1)+1]),index+i*fsize,fsize,dma);       \
	}                                                                                          \
}
#define SHEET_FIND(index, sid) {                                                               \
	for(u8 i = MAX_SHEETS; i--; ) {                                                            \
		if(sheets[i].id == sid) {                                                              \
			index = i;                                                                         \
			break;                                                                             \
		}                                                                                      \
	}                                                                                          \
}

// Get the first available space in VRAM and allocate it
#define TILOC_ADD(myindex, framesize) {                                                        \
	myindex = NOTILOC;                                                                         \
	u8 freeCount = 0;                                                                          \
	for(u8 i = 0; i < MAX_TILOCS; i++) {                                                       \
		if(tilocs[i]) {                                                                        \
			freeCount = 0;                                                                     \
		} else {                                                                               \
			freeCount++;                                                                       \
			if(freeCount << 2 >= (framesize)) {                                                \
				myindex = i;                                                                   \
				break;                                                                         \
			}                                                                                  \
		}                                                                                      \
	}                                                                                          \
	if(myindex != NOTILOC) {                                                                   \
		myindex -= freeCount-1;                                                                \
		while(freeCount--) tilocs[myindex+freeCount] = TRUE;                                   \
	}                                                                                          \
}
#define TILOC_FREE(myindex, framesize) {                                                       \
	u8 freeCount = ((framesize) >> 2) + (((framesize) & 3) ? 1 : 0);                           \
	while(freeCount--) tilocs[(myindex)+freeCount] = FALSE;                                    \
}
#define TILES_QUEUE(tiles, index, count) {                                                     \
	DMA_queueDma(DMA_VRAM, (u32)(tiles), (index) << 5, (count) << 4, 2);                       \
}

enum { 
	SHEET_NONE,    SHEET_PSTAR,  SHEET_MGUN,    SHEET_FBALL,  SHEET_HEART,  SHEET_MISSILE, 
	SHEET_ENERGY,  SHEET_ENERGYL,SHEET_BAT,     SHEET_CRITTER,SHEET_PIGNON, SHEET_FROGFEET, 
	SHEET_BALFROG, SHEET_CROW,   SHEET_GAUDI,   SHEET_FUZZ,   SHEET_SPIKE,  SHEET_BEETLE,
	SHEET_BEHEM,   SHEET_TELE,   SHEET_BASU,    SHEET_BASIL,  SHEET_TRAP,   SHEET_MANNAN,
	SHEET_PUCHI,   SHEET_SKULLH, SHEET_IGORSHOT,SHEET_REDSHOT,SHEET_LABSHOT,SHEET_IRONHBLK,
	SHEET_PCRITTER,SHEET_FAN,    SHEET_BARMIMI, SHEET_DARK,   SHEET_DARKBUB,SHEET_BLOWFISH,
	SHEET_POWERF,  SHEET_FLOWER, SHEET_BASUSHOT,SHEET_POLISH, SHEET_BABY,   SHEET_FIREWSHOT,
	SHEET_TERM,    SHEET_FFIELD, SHEET_FROG,    SHEET_DROP,   SHEET_PIGNONB,SHEET_OMGSHOT,
	SHEET_CURLYB,  SHEET_OMGLEG, SHEET_MISSL,   SHEET_XTARGET,SHEET_XFISHY, SHEET_XTREAD,
	SHEET_BLADE,   SHEET_FUZZC,  SHEET_XBODY,   SHEET_SPUR,   SHEET_CROC,   SHEET_GAUDISHOT,
	SHEET_SNAKE,   SHEET_BUBB,   SHEET_NEMES,   SHEET_CGUN,   SHEET_JELLY,  SHEET_GAUDIEGG,
	SHEET_ZZZ,	   SHEET_GAUDID, SHEET_IKACHAN, SHEET_POWERS, SHEET_SMSTAL, SHEET_LGSTAL,
	SHEET_SISHEAD, SHEET_BUYOB,  SHEET_BUYO,	SHEET_HOPPY,  SHEET_ACID,   SHEET_NIGHTSHOT,
	SHEET_GUNFSHOT,SHEET_MIDO,   SHEET_PRESS,	SHEET_STUMPY, SHEET_CORES1, SHEET_CORES3,
	SHEET_CORES4,  SHEET_REDDOT, SHEET_MIMI,	SHEET_DOCSHOT,SHEET_RING,	SHEET_SHOCK,
};

u8 sheet_num;
typedef struct {
	u8 id; // One of the values in the enum above - so an entity can find its sheet
	u8 size; // Total number of tiles used by the complete sheet
	u16 index; // VDP tile index
	u8 w, h; // Size of each frame
} Sheet;
Sheet sheets[MAX_SHEETS];

// Avoids MULU in entity update
u8 frameOffset[MAX_SHEETS][16];

u16 tiloc_index;
u8 tilocs[MAX_TILOCS];

void sheets_load_weapon(Weapon *w);
void sheets_refresh_weapon(Weapon *w);
void sheets_load_stage(u16 sid, u8 init_base, u8 init_tiloc);
void sheets_load_splash();

#endif
