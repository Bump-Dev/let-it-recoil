
//{{BLOCK(bg_floor)

//======================================================================
//
//	bg_floor, 256x256@4, 
//	Transparent color : FF,00,FF
//	+ palette 256 entries, not compressed
//	+ 33 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 1056 + 2048 = 3616
//
//	Time-stamp: 2026-08-26, 21:14:27
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_BG_FLOOR_H
#define GRIT_BG_FLOOR_H

#define bg_floorTilesLen 1056
extern const unsigned int bg_floorTiles[264];

#define bg_floorMapLen 2048
extern const unsigned short bg_floorMap[1024];

#define bg_floorPalLen 512
extern const unsigned short bg_floorPal[256];

#endif // GRIT_BG_FLOOR_H

//}}BLOCK(bg_floor)
