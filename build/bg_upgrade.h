
//{{BLOCK(bg_upgrade)

//======================================================================
//
//	bg_upgrade, 256x256@4, 
//	Transparent color : FF,00,FF
//	+ palette 256 entries, not compressed
//	+ 56 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 1792 + 2048 = 4352
//
//	Time-stamp: 2026-08-25, 14:12:53
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_BG_UPGRADE_H
#define GRIT_BG_UPGRADE_H

#define bg_upgradeTilesLen 1792
extern const unsigned int bg_upgradeTiles[448];

#define bg_upgradeMapLen 2048
extern const unsigned short bg_upgradeMap[1024];

#define bg_upgradePalLen 512
extern const unsigned short bg_upgradePal[256];

#endif // GRIT_BG_UPGRADE_H

//}}BLOCK(bg_upgrade)
