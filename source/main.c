#include <tonc.h>
#include <maxmod.h>
#include "font.h"
#include "hud.h"
#include "bg_floor.h"
#include "bg_upgrade.h"
#include "player.h"
#include "reticle.h"
#include "bullet.h"
#include "enemy_dino.h"
#include "enemy_green.h"
#include "enemy_charger.h"
#include "enemy_egg.h"
#include "game_over.h"
#include "title_banner.h"
#include "soundbank.h"
#include "soundbank_bin.h"

#define GBA_SRAM ((volatile u8 *)0x0E000000)

int best_normal = 0;
int best_hardcore = 0;

#define MAX_BULLETS 24
#define MAX_ENEMIES 24

// OAM Map
#define OAM_HEART_START 0		 // Slots 0 - 11
#define OAM_WAVE_START 12		 // Slots 12 - 13
#define OAM_TIME_START 14		 // Slots 14 - 15
#define OAM_MENU_BANNER_START 16 // Slots 16 - 19
#define OAM_PLAYER 20			 // Slot 20
#define OAM_RETICLE 21			 // Slot 21
#define OAM_BULLET_START 22		 // Slots 22 - 45
#define OAM_ENEMY_START 46		 // Slots 46 - 69
#define OAM_STOCK_START 70		 // Slots 70 - 72

typedef enum
{
	TITLE_SCREEN,
	GAMEPLAY,
	INTERMISSION,
	INTERMISSION_OUT,
	UPGRADE,
	GAME_OVER,
	PAUSE
} GameState;
GameState current_state;
GameState paused_state;

typedef enum
{
	// Simple Upgrades //
	ONE_UP,		  // +1 Stock
	BUBBLE_WRAP,  // +1 Max HP
	GUST_OF_WIND, // + Cruise Speed
	ANVIL,		  // - Cruise Speed
		   // QUICK_TRIGGER,	// +1 Fired Bullet Amount
	JETPACK,		// + Recoil Modifier
	SHOCK_ABSORBER, // - Recoil Modifier
	BULLET_GLIDER,	// + Bullet Lifetime
				   // Complex Upgrades //
				   // JETBACKPACK,  //  +1 Fired Bullet Amount, + Recoil Modifier
	HEAVY_CANNON, // + Recoil Modifier, -- Cruise Speed
	ARMOR,		  // +2 Max HP, -- Cruise Speed, -- Bullet Lifetime
	FEATHER,	  // -1 Max HP, + Cruise Speed, ++ Bullet Lifetime
	SUGAR_RUSH,
	ANCHOR,
	WD_40,
	BOWLING_BALL,
	LONG_BARREL,
	// Unique Upgrades
	SHOTGUN,
	PISTOL,
	RICOCHET,
	WRONG_WAY,
	BACK_ON_TRACK,
	NUM_UPGRADES
} UpgradeType;

typedef struct
{
	const char *name;
	const char *description[4];
	bool is_unique;
} UpgradeDef;

const UpgradeDef upgrade_defs[] = {
	[ONE_UP] = {"#{cx:0x0000}1 UP", {"#{cx:0x2000}+1#{cx:0x0000} Life"}, false},
	[BUBBLE_WRAP] = {"#{cx:0x0000}BUBBLE WRAP", {"#{cx:0x2000}+1#{cx:0x0000} Max HP"}, false},
	[GUST_OF_WIND] = {"#{cx:0x0000}GUST OF WIND", {"#{cx:0x2000}+1#{cx:0x0000} Speed"}, false},
	[ANVIL] = {"#{cx:0x0000}ANVIL", {"#{cx:0x3000}-1#{cx:0x0000} Speed"}, false},
	// [QUICK_TRIGGER] = {"EXTRA POCKET", {"#{cx:0x2000}-2#{cx:0x0000} Fire Delay"}},
	[JETPACK] = {"#{cx:0x0000}JETPACK", {"#{cx:0x2000}+1#{cx:0x0000} Recoil"}, false},
	[SHOCK_ABSORBER] = {"#{cx:0x0000}SHOCK ABSORBER", {"#{cx:0x3000}-1#{cx:0x0000} Recoil"}, false},
	[BULLET_GLIDER] = {"#{cx:0x0000}BULLET GLIDER", {"#{cx:0x2000}+10#{cx:0x0000} Bullet Lifetime"}, false},
	// [JETBACKPACK] = {"JETBACKPACK", {"#{cx:0x2000}-2#{cx:0x0000} Fire Delay", "#{cx:0x2000}+1#{cx:0x0000} Recoil"}},
	[HEAVY_CANNON] = {"#{cx:0x0000}HEAVY CANNON", {"#{cx:0x2000}+1#{cx:0x0000} Recoil", "#{cx:0x3000}-2#{cx:0x0000} Speed"}, false},
	[ARMOR] = {"#{cx:0x0000}ARMOR", {"#{cx:0x2000}+2#{cx:0x0000} Max HP", "#{cx:0x3000}-2#{cx:0x0000} Speed", "#{cx:0x3000}-20#{cx:0x0000} Bullet Lifetime"}, false},
	[FEATHER] = {"#{cx:0x0000}FEATHER", {"#{cx:0x3000}-1#{cx:0x0000} Max HP", "#{cx:0x2000}+1#{cx:0x0000} Speed", "#{cx:0x2000}+20#{cx:0x0000} Bullet Lifetime"}, false},
	[SUGAR_RUSH] = {"#{cx:0x0000}SUGAR RUSH", {"#{cx:0x2000}+1#{cx:0x0000} Speed", "#{cx:0x3000}-10#{cx:0x0000} Bullet Lifetime"}, false},
	[ANCHOR] = {"#{cx:0x0000}ANCHOR", {"#{cx:0x3000}-1#{cx:0x0000} Speed", "#{cx:0x2000}+15#{cx:0x0000} Bullet Lifetime"}, false},
	[WD_40] = {"#{cx:0x0000}WD-40", {"#{cx:0x2000}+1#{cx:0x0000} Speed", "#{cx:0x3000}-1#{cx:0x0000} Recoil"}, false},
	[BOWLING_BALL] = {"#{cx:0x0000}BOWLING BALL", {"#{cx:0x2000}+1#{cx:0x0000} Max HP", "#{cx:0x2000}+1#{cx:0x0000} Recoil", "#{cx:0x3000}-1#{cx:0x0000} Speed"}, false},
	[LONG_BARREL] = {"#{cx:0x0000}LONG BARREL", {"#{cx:0x2000}+15#{cx:0x0000} Bullet Lifetime", "#{cx:0x2000}+1#{cx:0x0000} Recoil"}, false},
	[SHOTGUN] = {"#{cx:0x4000}SHOTGUN", {"#{cx:0x2000}3#{cx:0x0000} Bullets", "#{cx:0x3000}-40#{cx:0x0000} Bullet Lifetime"}, true},
	[PISTOL] = {"#{cx:0x4000}PISTOL", {"#{cx:0x3000}1#{cx:0x0000} Bullets", "#{cx:0x2000}+40#{cx:0x0000} Bullet Lifetime"}, true},
	[RICOCHET] = {"#{cx:0x4000}RICOCHET", {"#{cx:0x0000}Bullets #{cx:0x2000}bounce#{cx:0x0000} once", "off enemies"}, true},
	[WRONG_WAY] = {"#{cx:0x4000}WRONG WAY", {"#{cx:0x0000}Recoil towards", "bullets instead"}, true},
	[BACK_ON_TRACK] = {"#{cx:0x4000}BACK ON TRACK", {"#{cx:0x0000}Recoil away from", "bullets again"}, true},
};

bool taken_upgrades[NUM_UPGRADES] = {false};

typedef struct
{
	int x, y;
	int vx, vy;
	int max_hp, hp;
	int stocks;
	int recoil_intensity, recoil_frames;
	int recoil_modifier, cruise_modifier;
	int flash_timer;
	int stun_timer;
	int invincibility_timer;
	int bullet_lifetime;
	int bullet_amount;
	u16 angle;
} Player;

typedef struct
{
	int x, y;
	int vx, vy;
	int active;
	int lifetime;
	int bounces_left;
	int last_enemy_hit;
	bool is_enemy;
} Bullet;

typedef enum
{
	ENEMY_DINO,
	ENEMY_GREEN,
	ENEMY_CHARGER,
	ENEMY_EGG,
} EnemyType;

typedef enum
{
	STATE_CHASE,
	STATE_WINDUP,
	STATE_ATTACK
} EnemyState;

typedef struct
{
	int x, y;
	int vx, vy;
	u16 target_angle;
	int active;
	EnemyType type;
	int hp;
	int flash_timer;
	int action_timer;
	int state;
} Enemy;

Player player;
Bullet bullet_pool[MAX_BULLETS];
Enemy enemy_pool[MAX_ENEMIES];

int current_mode = 0;
int beat_timer = 0;
int beat_state = 0; // 0 = WIND UP, 1 = THUD
const int max_cooldown = 120;

const u16 sfx_shoot[] = {
	SFX_SHOOT0,
	SFX_SHOOT1,
	SFX_SHOOT2};
#define NUM_SHOOT_SFX (sizeof(sfx_shoot) / sizeof(sfx_shoot[0]))

const u16 sfx_hit[] = {
	SFX_HIT0,
	SFX_HIT1,
	SFX_HIT2};
#define NUM_HIT_SFX (sizeof(sfx_hit) / sizeof(sfx_hit[0]))

const u16 sfx_thud[] = {
	SFX_THUD0,
	SFX_THUD1};
#define NUM_THUD_SFX (sizeof(sfx_thud) / sizeof(sfx_thud[0]))

void init_save_data()
{
	if (GBA_SRAM[0] == 'R' && GBA_SRAM[1] == 'E' && GBA_SRAM[2] == 'C' && GBA_SRAM[3] == 'O')
	{
		best_normal = GBA_SRAM[4];
		best_hardcore = GBA_SRAM[5];
	}
	else
	{
		GBA_SRAM[0] = 'R';
		GBA_SRAM[1] = 'E';
		GBA_SRAM[2] = 'C';
		GBA_SRAM[3] = 'O';
		GBA_SRAM[4] = 1;
		GBA_SRAM[5] = 1;
		best_normal = 0;
		best_hardcore = 0;
	}
}

int check_collision(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
	return (ax < bx + bw &&
			ax + aw > bx &&
			ay < by + bh &&
			ay + ah > by);
}

void shake_screen(int intensity, int duration)
{
	static int frames = 0;
	static int pwr = 0;

	if (duration > 0)
	{
		frames = duration;
		pwr = intensity;
		return;
	}

	if (frames > 0)
	{
		REG_BG3HOFS = (qran() % (pwr * 2 + 1)) - pwr;
		REG_BG3VOFS = (qran() % (pwr * 2 + 1)) - pwr;
		frames--;
	}
	else
	{
		REG_BG3HOFS = 0;
		REG_BG3VOFS = 0;
	}
}

void tte_write_int(char *str_val, int value)
{
	char text_buffer[64];
	siprintf(text_buffer, str_val, value);
	tte_write(text_buffer);
}

void hit_stop(int duration)
{
	for (int i = 0; i < duration; i++)
	{
		vid_vsync();
		mmFrame();
	}
}

void hide_hud_sprites(OBJ_ATTR *obj_buffer)
{
	// Hide Hearts
	for (int i = 0; i < 12; i++)
		obj_set_pos(&obj_buffer[OAM_HEART_START + i], 240, 160);
	// Hide Stocks
	for (int i = 0; i < 2; i++)
		obj_set_pos(&obj_buffer[OAM_STOCK_START + i], 240, 160);

	// Hide Wave & Timer digits
	obj_set_pos(&obj_buffer[OAM_WAVE_START], 240, 160);
	obj_set_pos(&obj_buffer[OAM_WAVE_START + 1], 240, 160);
	obj_set_pos(&obj_buffer[OAM_TIME_START], 240, 160);
	obj_set_pos(&obj_buffer[OAM_TIME_START + 1], 240, 160);
}

void draw_game_over_screen(OBJ_ATTR *obj_buffer, int current_wave)
{
	shake_screen(6, 20);
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG1 | DCNT_BG3;
	REG_BLDCNT = 0x00C8;
	REG_BLDY = 12;

	// Set Sprites Rendering Priority To 1
	for (int i = 0; i < 128; i++)
	{
		obj_buffer[i].attr2 &= ~ATTR2_PRIO_MASK;
		obj_buffer[i].attr2 |= ATTR2_PRIO(1);

		obj_buffer[i].attr0 |= ATTR0_BLEND;
	}
	// Dim Sprite Palette Banks
	for (int i = 0; i < 10 * 16; i++)
	{
		u16 color = pal_obj_mem[i];
		u32 r = (color & 0x001F) >> 1;
		u32 g = ((color >> 5) & 0x001F) >> 1;
		u32 b = ((color >> 10) & 0x001F) >> 1;
		pal_obj_mem[i] = RGB15(r, g, b);
	}

	hide_hud_sprites(obj_buffer);

	// Left Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(10) | 35);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START], 56, 20);
	// Right Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START + 1], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(10) | 67);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START + 1], 120, 20);

	tte_write("#{cx:0x0000}");
	tte_erase_screen();

	tte_write("#{P:56,56}#{cx:0x3000}OH NO! You lost!");
	tte_write_int("#{P:88,72}#{cx:0x0000}WAVE: %02d", current_wave);
	tte_write_int("#{P:88,88}#{cx:0x0000}BEST: %02d", (current_mode == 0) ? best_normal : best_hardcore);
	tte_write("#{P:40,104}#{cx:0x4000}PRESS START TO RETRY");
	tte_write("#{P:40,120}#{cx:0x1000}PRESS SELECT TO TITLE");
}

void toggle_pause_menu(OBJ_ATTR *obj_buffer)
{
	shake_screen(2, 5);
	mmEffect(SFX_BLIP0);
	if (current_state != PAUSE)
	{
		paused_state = current_state;
		current_state = PAUSE;
		REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG1 | DCNT_BG3;
		REG_BLDCNT = 0x00C8;
		REG_BLDY = 12;

		// Set Sprites Rendering Priority To 1
		for (int i = 0; i < 128; i++)
		{
			obj_buffer[i].attr2 &= ~ATTR2_PRIO_MASK;
			obj_buffer[i].attr2 |= ATTR2_PRIO(1);

			obj_buffer[i].attr0 |= ATTR0_BLEND;
		}
		// Dim Sprite Palette Banks
		for (int i = 0; i < 11 * 16; i++)
		{
			u16 color = pal_obj_mem[i];
			u32 r = (color & 0x001F) >> 1;
			u32 g = ((color >> 5) & 0x001F) >> 1;
			u32 b = ((color >> 10) & 0x001F) >> 1;
			pal_obj_mem[i] = RGB15(r, g, b);
		}

		tte_erase_screen();
		tte_write("#{P:96,72}#{cx:0x4000}PAUSED");
	}
	else
	{
		tte_erase_screen();
		// Restore original sprite palettes
		memcpy16(pal_obj_mem, playerPal, 16);
		memcpy16(&pal_obj_mem[16], reticlePal, 16);
		memcpy16(&pal_obj_mem[32], bulletPal, 16);
		memcpy16(&pal_obj_mem[48], enemy_chargerPal, 16);
		memcpy16(&pal_obj_mem[64], enemy_dinoPal, 16);
		memcpy16(&pal_obj_mem[80], enemy_greenPal, 16);
		memcpy16(&pal_obj_mem[192], enemy_eggPal, 16);
		memcpy16(&pal_obj_mem[96], hudPal, 16);
		memcpy16(&pal_obj_mem[144], fontPal, 16);

		// Enemy bullet palette reset
		pal_obj_mem[8 * 16 + 0] = 0;
		pal_obj_mem[8 * 16 + 1] = RGB15(22, 5, 8);
		pal_obj_mem[8 * 16 + 2] = RGB15(29, 29, 29);

		// Flash Palette
		pal_obj_mem[112] = 0; // Transparent Palette
		for (int c = 1; c < 16; c++)
			pal_obj_mem[112 + c] = RGB15(31, 31, 31);

		current_state = paused_state;
		REG_BLDCNT = 0;
		REG_BLDY = 0;
		REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG3;
	}
}

void draw_title_menu(int selection)
{
	if (selection == 0)
	{
		tte_write("#{P:80,80}#{cx:0x0000}> NORMAL");
		tte_write("#{P:72,96}#{cx:0x0000}  HARDCORE");
	}
	else
	{
		tte_write("#{P:80,80}#{cx:0x0000}  NORMAL");
		tte_write("#{P:72,96}#{cx:0x0000}> HARDCORE");
	}
	tte_write_int("#{P:72,112}#{cx:0x4000}BEST WAVE: %02d", (selection == 0) ? best_normal : best_hardcore);
}

void draw_title_screen(OBJ_ATTR *obj_buffer)
{
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG1 | DCNT_BG3;
	REG_BLDCNT = 0x00C8;
	REG_BLDY = 12;

	// Set Sprites Rendering Priority To 1
	for (int i = 0; i < 128; i++)
	{
		obj_buffer[i].attr2 &= ~ATTR2_PRIO_MASK;
		obj_buffer[i].attr2 |= ATTR2_PRIO(1);

		obj_buffer[i].attr0 |= ATTR0_BLEND;
	}
	tte_erase_screen();
	hide_hud_sprites(obj_buffer);

	int h_margin = 56;
	int v_margin = 16;
	// Top Left Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(11) | 99);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START], h_margin, v_margin);
	// Top Right Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START + 1], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(11) | 131);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START + 1], h_margin + 64, v_margin);
	// Bottom Left Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START + 2], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(11) | 163);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START + 2], h_margin, v_margin + 32);
	// Bottom Right Sprite
	obj_set_attr(&obj_buffer[OAM_MENU_BANNER_START + 3], ATTR0_WIDE, ATTR1_SIZE_64, ATTR2_PALBANK(11) | 195);
	obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START + 3], h_margin + 64, v_margin + 32);

	draw_title_menu(0);
}

void update_hearts(OBJ_ATTR *obj_buffer, int val, int current_wave)
{
	player.hp += val;
	if (player.hp <= 0)
	{
		if (player.stocks > 1)
		{
			player.stocks--;
			player.hp = player.max_hp;
			player.invincibility_timer = 180;
			// hit_stop(10);
			shake_screen(12, 30);
			mmEffect(SFX_WAVE_START);
		}
		else
		{
			current_state = GAME_OVER;
			if (current_mode == 0 && current_wave > best_normal)
			{
				best_normal = current_wave;
				GBA_SRAM[4] = best_normal;
			}
			else if (current_mode == 1 && current_wave > best_hardcore)
			{
				best_hardcore = current_wave;
				GBA_SRAM[5] = best_hardcore;
			}
			draw_game_over_screen(obj_buffer, current_wave);
			return;
		}
	}
	for (int i = 0; i < 12; i++) // 12 is the max displayable hearts
	{
		if (i < player.max_hp)
		{
			int tile_id = (i < player.hp) ? 18 : 19;
			obj_set_attr(&obj_buffer[OAM_HEART_START + i], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(6) | tile_id);
			obj_set_pos(&obj_buffer[OAM_HEART_START + i], 5 + i * 6, 7);
		}
		else
		{
			obj_set_pos(&obj_buffer[OAM_HEART_START + i], 240, 160);
		}
	}

	// Stocks
	obj_set_attr(&obj_buffer[OAM_STOCK_START + 1], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(6) | 20);
	obj_set_pos(&obj_buffer[OAM_STOCK_START + 1], 5, 13);

	int stock_amt = player.stocks % 10;
	obj_set_attr(&obj_buffer[OAM_STOCK_START], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(6) | (20 + stock_amt));
	obj_set_pos(&obj_buffer[OAM_STOCK_START], 12, 15);
}

void draw_upgrade_menu(UpgradeType choices[], int selection, bool init)
{
	if (init)
	{
		tte_write("#{cx:0x0000}");
		tte_erase_screen();

		tte_write("#{cx:0x1000}#{P:8,8}SELECT UPGRADE#{cx:0x0000}");
		tte_write("#{cx:0x1000}#{P:184,8}STATS#{cx:0x0000}");

		char stat_buffer[32];
		int stat_x = 208;
		int base_y = 24;
		int stat_spacing = 16;

		// Max HP (Heart Icon)
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (0 * stat_spacing), player.max_hp);
		tte_write(stat_buffer);

		// Stocks
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (1 * stat_spacing), player.stocks);
		tte_write(stat_buffer);

		// Speed
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (2 * stat_spacing), (player.cruise_modifier - 256) / 32);
		tte_write(stat_buffer);

		// Recoil
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (3 * stat_spacing), (player.recoil_modifier - 256) / 128);
		tte_write(stat_buffer);

		// Bullet Lifetime
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (4 * stat_spacing), player.bullet_lifetime);
		tte_write(stat_buffer);

		// Bullet Amount
		siprintf(stat_buffer, "#{P:%d,%d}%d", stat_x, base_y + (5 * stat_spacing), player.bullet_amount);
		tte_write(stat_buffer);
	}

	for (int i = 0; i < 3; i++)
	{
		UpgradeType type = choices[i];
		char choice_buffer[64];

		int y_pos = 24 + (i * 16);

		if (i == selection)
			siprintf(choice_buffer, "#{cx:0x0000}#{P:8,%d}> %s", y_pos, upgrade_defs[type].name);
		else
			siprintf(choice_buffer, "#{cx:0x0000}#{P:8,%d}  %s", y_pos, upgrade_defs[type].name);
		tte_write(choice_buffer);
	}

	for (int i = 0; i < 4; i++)
	{
		tte_set_pos(8, 80 + (i * 16));
		tte_write("                            ");
	}

	UpgradeType selected_type = choices[selection];
	int start_x = 8;
	int start_y = 80;
	int line_spacing = 16;
	for (int i = 0; i < 4; i++)
	{
		if (upgrade_defs[selected_type].description[i] == NULL)
			break;
		tte_set_pos(start_x, start_y + (i * line_spacing));
		tte_write(upgrade_defs[selected_type].description[i]);
	}
}

bool spawn_enemy(EnemyType type, int x_px, int y_px, int current_wave)
{
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (!enemy_pool[i].active)
		{
			enemy_pool[i].active = 1;
			enemy_pool[i].type = type;
			enemy_pool[i].x = x_px << 8;
			enemy_pool[i].y = y_px << 8;
			enemy_pool[i].flash_timer = 0;
			enemy_pool[i].state = STATE_CHASE;
			switch (enemy_pool[i].type)
			{
			case ENEMY_CHARGER:
			{
				enemy_pool[i].hp = 2; //+ (current_wave / 8);
				enemy_pool[i].action_timer = 60;
			}
			break;
			case ENEMY_DINO:
			{
				enemy_pool[i].hp = 1; //+ (current_wave / 5);
			}
			break;
			case ENEMY_GREEN:
			{
				enemy_pool[i].hp = 2; // + (current_wave / 6);
				enemy_pool[i].action_timer = 90;
			}
			break;
			case ENEMY_EGG:
			{
				enemy_pool[i].hp = 3; // + (current_wave / 6);
			}
			break;
			}
			return true;
		}
	}
	return false;
}

void reset_game(OBJ_ATTR *obj_buffer, int *current_wave, int *wave_timer, int *enemies_on_screen, int *spawn_cooldown, int mode)
{
	// Restore original sprite palettes
	memcpy16(pal_obj_mem, playerPal, 16);
	memcpy16(&pal_obj_mem[16], reticlePal, 16);
	memcpy16(&pal_obj_mem[32], bulletPal, 16);
	memcpy16(&pal_obj_mem[48], enemy_chargerPal, 16);
	memcpy16(&pal_obj_mem[64], enemy_dinoPal, 16);
	memcpy16(&pal_obj_mem[80], enemy_greenPal, 16);
	memcpy16(&pal_obj_mem[192], enemy_eggPal, 16);
	memcpy16(&pal_obj_mem[96], hudPal, 16);
	memcpy16(&pal_obj_mem[144], fontPal, 16);

	// Enemy bullet palette reset
	pal_obj_mem[8 * 16 + 0] = 0;
	pal_obj_mem[8 * 16 + 1] = RGB15(22, 5, 8);
	pal_obj_mem[8 * 16 + 2] = RGB15(29, 29, 29);

	// Flash Palette
	pal_obj_mem[112] = 0;
	for (int c = 1; c < 16; c++)
		pal_obj_mem[112 + c] = RGB15(31, 31, 31);

	// Player
	player.x = 112 << 8;
	player.y = 72 << 8;
	player.max_hp = 3, player.hp = 3;
	player.stocks = (mode == 0) ? 3 : 1;
	player.vx = 0, player.vy = 0;
	player.recoil_modifier = 256, player.cruise_modifier = 256;
	player.bullet_amount = 1, player.bullet_lifetime = 60;
	player.flash_timer = 0, player.stun_timer = 0, player.invincibility_timer = 0;
	player.angle = 0;

	// Clear All Bullets
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		bullet_pool[i].active = 0;
		obj_set_pos(&obj_buffer[OAM_BULLET_START + i], 240, 160);
	}
	// Clear All Enemies
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		enemy_pool[i].active = 0;
		enemy_pool[i].vx = 0;
		enemy_pool[i].vy = 0;
		enemy_pool[i].hp = 0;
		enemy_pool[i].flash_timer = 0;
		enemy_pool[i].action_timer = 0;
		enemy_pool[i].state = STATE_CHASE;
		obj_set_pos(&obj_buffer[OAM_ENEMY_START + i], 240, 160);
	}

	// Reset Variables
	*current_wave = 1;
	*wave_timer = 20 * 60;
	*enemies_on_screen = 0;
	*spawn_cooldown = max_cooldown;
	for (int i = 0; i < NUM_UPGRADES; i++)
	{
		taken_upgrades[i] = false;
		if (i == PISTOL)
			taken_upgrades[PISTOL] = true;
		if (i == BACK_ON_TRACK)
			taken_upgrades[BACK_ON_TRACK] = true;
	}

	beat_timer = 0;
	beat_state = 0;
	update_hearts(obj_buffer, 0, *current_wave);
	for (int i = 0; i < 4; i++)
		obj_set_pos(&obj_buffer[OAM_MENU_BANNER_START + i], 240, 160);
	current_state = GAMEPLAY;
	REG_BLDCNT = 0;
	REG_BLDY = 0;
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG3;
}

void play_random_sfx(const u16 sounds[], int amount)
{
	static int last_sfx = -1;
	int random_sfx;
	do
		random_sfx = qran_range(0, amount);
	while (random_sfx == last_sfx && amount > 1);

	last_sfx = random_sfx;
	mmEffect(sounds[random_sfx]);
}

int main()
{
	// INTERRUPTS
	REG_DISPSTAT |= DSTAT_VBL_IRQ;
	irq_init(NULL);
	irq_add(II_VBLANK, mmVBlank);

	// SOUND
	mmInitDefault((mm_addr)soundbank_bin, 8);

	// SAVE DATA
	init_save_data();

	// GRAPHICS
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG1 | DCNT_BG3;
	REG_BG2CNT = BG_CBB(1) | BG_SBB(30) | BG_PRIO(2);
	REG_BG3CNT = BG_CBB(2) | BG_SBB(29) | BG_PRIO(3) | BG_MOSAIC;

	OBJ_ATTR obj_buffer[128];
	OBJ_AFFINE *obj_aff_buffer = (OBJ_AFFINE *)obj_buffer;
	oam_init(obj_buffer, 128);

	pal_bg_bank[0][1] = RGB15(29, 29, 29); // White
	pal_bg_bank[1][1] = RGB15(8, 24, 30);  // Blue
	pal_bg_bank[2][1] = RGB15(4, 30, 15);  // Green
	pal_bg_bank[3][1] = RGB15(26, 7, 11);  // Red
	pal_bg_bank[4][1] = RGB15(31, 24, 2);  // Yellow
	tte_init_se_default(1, BG_CBB(0) | BG_SBB(31));
	REG_BG1CNT |= BG_MOSAIC;

	for (int i = 0; i < MAX_BULLETS; i++)
		bullet_pool[i].active = 0;
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		enemy_pool[i].active = 0;
		enemy_pool[i].vx = 0;
		enemy_pool[i].vy = 0;
		enemy_pool[i].hp = 0;
		enemy_pool[i].flash_timer = 0;
		enemy_pool[i].action_timer = 0;
	}

	// Waves
	int current_wave = 0;
	int wave_timer = 0;
	int intermission_timer = 0;
	int spawn_cooldown = 0;
	int enemies_on_screen = 0;
	current_state = TITLE_SCREEN;
	UpgradeType pool_choices[3];
	int menu_selection = 0;

	// Player
	memcpy16(pal_obj_mem, playerPal, 16);
	memcpy16(&tile_mem[4][0], playerTiles, playerTilesLen / 2);
	obj_set_attr(&obj_buffer[OAM_PLAYER], ATTR0_SQUARE | ATTR0_AFF_DBL, ATTR1_SIZE_16 | ATTR1_AFF_ID(0), ATTR2_PALBANK(0) | 0);
	obj_set_pos(&obj_buffer[OAM_PLAYER], 240, 160);

	// Reticle
	int ret_x = 0, ret_y = 0;
	u16 aim_angle = 0;
	const int MIN_AIM_SPEED = 550, MAX_AIM_SPEED = 1650, AIM_ACCEL = 55;
	const int aim_dist = 32;
	int aim_speed = MIN_AIM_SPEED;
	memcpy16(&pal_obj_mem[16], reticlePal, 16);
	memcpy16(&tile_mem[4][4], reticleTiles, reticleTilesLen / 2);
	obj_set_attr(&obj_buffer[OAM_RETICLE], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(1) | 4);
	obj_set_pos(&obj_buffer[OAM_RETICLE], 240, 160);

	// Bullet
	memcpy16(&pal_obj_mem[32], bulletPal, 16);
	memcpy16(&tile_mem[4][5], bulletTiles, bulletTilesLen / 2);
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		obj_set_attr(&obj_buffer[OAM_BULLET_START + i], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(2) | 5);
		obj_set_pos(&obj_buffer[OAM_BULLET_START + i], 240, 160);
	}
	// Enemy Bullet Palette Bank (8)
	pal_obj_mem[8 * 16 + 0] = 0;
	pal_obj_mem[8 * 16 + 1] = RGB15(22, 5, 8);
	pal_obj_mem[8 * 16 + 2] = RGB15(29, 29, 29);

	// Enemy - Charger
	memcpy16(&pal_obj_mem[48], enemy_chargerPal, 16);
	memcpy16(&tile_mem[4][6], enemy_chargerTiles, enemy_chargerTilesLen / 2);

	// Enemy - Dino
	memcpy16(&pal_obj_mem[64], enemy_dinoPal, 16);
	memcpy16(&tile_mem[4][10], enemy_dinoTiles, enemy_dinoTilesLen / 2);

	// Enemy - Green Man
	memcpy16(&pal_obj_mem[80], enemy_greenPal, 16);
	memcpy16(&tile_mem[4][14], enemy_greenTiles, enemy_greenTilesLen / 2);

	// Enemy - Surprise Egg
	memcpy16(&pal_obj_mem[192], enemy_eggPal, 16);
	memcpy16(&tile_mem[4][228], enemy_eggTiles, enemy_eggTilesLen / 2);

	// Enemy OAM Initilization
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		obj_set_attr(&obj_buffer[OAM_ENEMY_START + i], ATTR0_SQUARE, ATTR1_SIZE_16, 0);
		obj_set_pos(&obj_buffer[OAM_ENEMY_START + i], 240, 160);
	}

	pal_obj_mem[112] = 0; // Sets Palette at 112 to Transparent
	for (int c = 1; c < 16; c++)
		pal_obj_mem[112 + c] = RGB15(31, 31, 31);

	// HUD
	memcpy16(&pal_obj_mem[96], hudPal, 16);
	memcpy16(&tile_mem[4][18], hudTiles, hudTilesLen / 2);

	// Numbers Font
	memcpy16(&pal_obj_mem[144], fontPal, 16);
	memcpy16(&tile_mem[4][24], fontTiles, fontTilesLen / 2);

	// Floor Background
	memcpy16(&pal_bg_bank[6][0], bg_floorPal, 16);
	memcpy16(&tile_mem[2][0], bg_floorTiles, bg_floorTilesLen / 2);
	memcpy16(&se_mem[29][0], bg_floorMap, bg_floorMapLen / 2);
	for (int i = 0; i < bg_floorMapLen / 2; i++)
	{
		se_mem[29][i] = bg_floorMap[i] | SE_PALBANK(6);
	}

	// Upgrade Menu
	memcpy16(&pal_bg_bank[5][0], bg_upgradePal, 16);
	memcpy16(&tile_mem[1][0], bg_upgradeTiles, bg_upgradeTilesLen / 2);
	memcpy16(&se_mem[30][0], bg_upgradeMap, bg_upgradeMapLen / 2);
	for (int i = 0; i < bg_upgradeMapLen / 2; i++)
	{
		se_mem[30][i] = bg_upgradeMap[i] | SE_PALBANK(5);
	}

	// Game Over Screen
	memcpy16(&pal_obj_mem[10 * 16], game_overPal, 16);
	memcpy16(&tile_mem[4][35], game_overTiles, game_overTilesLen / 2);

	// Title Screen
	memcpy16(&pal_obj_mem[11 * 16], title_bannerPal, 16);
	memcpy16(&tile_mem[4][99], title_bannerTiles, title_bannerTilesLen / 2);

	int title_selection = 0;
	u32 frame_counter = 0;
	draw_title_screen(obj_buffer);

	while (1)
	{
		vid_vsync();
		mmFrame();
		shake_screen(0, 0);
		frame_counter++;
		if (current_state == TITLE_SCREEN)
		{
			if (key_hit(KEY_DOWN))
			{
				title_selection++;
				if (title_selection > 1)
					title_selection = 0;
				draw_title_menu(title_selection);
				mmEffect(SFX_BLIP0);
			}
			else if (key_hit(KEY_UP))
			{
				title_selection--;
				if (title_selection < 0)
					title_selection = 1;
				draw_title_menu(title_selection);
				mmEffect(SFX_BLIP0);
			}
			if (key_hit(KEY_A) || key_hit(KEY_START))
			{
				sqran(frame_counter);
				mmEffect(SFX_SELECT0);
				current_mode = title_selection;
				reset_game(obj_buffer, &current_wave, &wave_timer, &enemies_on_screen, &spawn_cooldown, title_selection);
			}
		}
		if (current_state == INTERMISSION)
		{
			if (intermission_timer > 0)
			{
				intermission_timer--;
				int m = (60 - intermission_timer) / 4;
				REG_MOSAIC = MOS_BUILD(m, m, m, m);
			}
			else
			{
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemy_pool[i].active)
					{
						enemy_pool[i].active = 0;
						enemy_pool[i].y = 160 << 8;
					}
				}
				enemies_on_screen = 0;
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullet_pool[i].active && bullet_pool[i].is_enemy)
					{
						bullet_pool[i].active = 0;
						obj_set_pos(&obj_buffer[OAM_BULLET_START + i], 240, 160);
					}
				}
				REG_MOSAIC = 0;
				for (int i = 0; i < 128; i++)
					obj_buffer[i].attr0 &= ~ATTR0_MOSAIC;
				tte_erase_screen();
				REG_DISPCNT = DCNT_BG1 | DCNT_BG2;
				current_state = UPGRADE;
				menu_selection = 0;
				for (int i = 0; i < 3; i++)
				{
					bool duplicate;
					do
					{
						pool_choices[i] = qran_range(0, NUM_UPGRADES);
						duplicate = false;

						if (upgrade_defs[pool_choices[i]].is_unique && taken_upgrades[pool_choices[i]])
							duplicate = true;

						for (int j = 0; j < i; j++)
						{
							if (pool_choices[i] == pool_choices[j])
								duplicate = true;
						}
					} while (duplicate);
				}
				draw_upgrade_menu(pool_choices, menu_selection, true);
			}
		}
		if (current_state == INTERMISSION_OUT)
		{
			if (intermission_timer > 0)
			{
				intermission_timer--;
				int m = intermission_timer / 4;
				REG_MOSAIC = MOS_BUILD(m, m, m, m);
			}
			else
			{
				REG_MOSAIC = 0;
				for (int i = 0; i < 128; i++)
					obj_buffer[i].attr0 &= ~ATTR0_MOSAIC;
				current_state = GAMEPLAY;
			}
		}
		if (wave_timer > 0 && current_state == GAMEPLAY) // Gameplay
		{
			wave_timer--;

			int urgency_delay = 15 + (wave_timer / 10);
			if (urgency_delay > 90)
				urgency_delay = 90;

			int current_beat_delay = (beat_state == 0) ? urgency_delay : (urgency_delay / 2);
			beat_timer += 1;
			if (beat_timer >= current_beat_delay)
			{
				beat_timer = 0;
				if (beat_state == 0)
				{
					mmEffect(SFX_WIND_UP);
					beat_state = 1;
				}
				else
				{
					mmEffect(SFX_CLANK);
					beat_state = 0;
				}
			}
			if (spawn_cooldown > 0)
				spawn_cooldown--;
			if (spawn_cooldown == 0 && enemies_on_screen < MAX_ENEMIES)
			{
				EnemyType random_type = ENEMY_DINO;
				int roll = qran_range(0, 100);

				if (current_wave <= 1)
					random_type = ENEMY_DINO;
				else if (current_wave <= 3)
					random_type = (roll < 60) ? ENEMY_DINO : ENEMY_GREEN;
				else if (current_wave <= 5)
				{
					if (roll < 35)
						random_type = ENEMY_DINO;
					else if (roll < 35 + 25)
						random_type = ENEMY_GREEN;
					else
						random_type = ENEMY_CHARGER;
				}
				else
				{
					if (enemies_on_screen <= MAX_ENEMIES - 10 && roll >= 90)
					{
						random_type = ENEMY_EGG;
					}
					else
					{
						if (roll < 35)
							random_type = ENEMY_DINO;
						else if (roll < 35 + 35)
							random_type = ENEMY_GREEN;
						else
							random_type = ENEMY_CHARGER;
					}
				}
				int spawn_x = qran_range(0, 224);
				int spawn_y = qran_range(0, 144);

				if (ABS(spawn_x - (player.x >> 8)) < 40)
					spawn_x = (spawn_x + 112) % 224;
				if (ABS(spawn_y - (player.y >> 8)) < 40)
					spawn_y = (spawn_y + 72) % 144;

				if (spawn_enemy(random_type, spawn_x, spawn_y, current_wave))
				{
					enemies_on_screen++;
					spawn_cooldown = max_cooldown - ((current_wave - 1) * 10);
					if (spawn_cooldown < 15)
						spawn_cooldown = 15;
				}
			}
		}
		else if (wave_timer == 0 && current_state == GAMEPLAY) // Wave Ended
		{
			intermission_timer = 60; // Pause >> Open Upgrade Menu
			current_state = INTERMISSION;
			mmEffect(SFX_WAVE_END);
		}
		if (current_state == GAME_OVER)
		{
			if (key_hit(KEY_START))
				reset_game(obj_buffer, &current_wave, &wave_timer, &enemies_on_screen, &spawn_cooldown, current_mode);
			else if (key_hit(KEY_SELECT))
			{
				title_selection = 0;
				current_mode = 0;
				current_state = TITLE_SCREEN;
				draw_title_screen(obj_buffer);
			}
		}

		key_poll();
		if (key_is_down(KEY_ANY))
		{ // making seed more random based on input
			sqran(qran() + frame_counter + player.x + aim_angle);
		}

		if (key_hit(KEY_START) && current_state != TITLE_SCREEN && current_state != GAME_OVER && current_state != UPGRADE)
			toggle_pause_menu(obj_buffer);

		if (current_state == INTERMISSION || current_state == INTERMISSION_OUT)
		{
			for (int i = 0; i < 128; i++)
				obj_buffer[i].attr0 |= ATTR0_MOSAIC;
		}
		oam_copy(oam_mem, obj_buffer, 128);

		if (current_state == GAMEPLAY || current_state == INTERMISSION || current_state == INTERMISSION_OUT)
		{
			// 	if (key_hit(KEY_UP))
			// 		aim_angle = 49152; // Snaps aim straight UP (270 degrees)
			// 	else if (key_hit(KEY_DOWN))
			// 		aim_angle = 16384; // Snaps aim straight DOWN (90 degrees)
			if (key_is_down(KEY_RIGHT))
			{
				aim_angle += aim_speed;
				aim_speed += AIM_ACCEL;
				if (aim_speed > MAX_AIM_SPEED)
					aim_speed = MAX_AIM_SPEED;
			}
			else if (key_is_down(KEY_LEFT))
			{
				aim_angle -= aim_speed;
				aim_speed += AIM_ACCEL;
				if (aim_speed > MAX_AIM_SPEED)
					aim_speed = MAX_AIM_SPEED;
			}
			else
				aim_speed = MIN_AIM_SPEED;

			// Converting Player Fixed-Point Back to Normal Raw Pixels
			int px = player.x >> 8;
			int py = player.y >> 8;

			const int CORNER_SIZE = 12;
			bool in_left = (px < CORNER_SIZE);
			bool in_right = (px > 224 - CORNER_SIZE);
			bool in_top = (py < CORNER_SIZE);
			bool in_bottom = (py > 144 - CORNER_SIZE);

			if ((in_left || in_right) && (in_top || in_bottom) && player.stun_timer == 0)
			{
				int target_dx = 112 - px;
				int target_dy = 72 - py;
				u16 launch_angle = ArcTan2(target_dx, target_dy);

				int launch_speed = 200;
				player.vx = (launch_speed * lu_cos(launch_angle)) >> 12;
				player.vy = (launch_speed * lu_sin(launch_angle)) >> 12;

				player.recoil_frames = 20;
				player.recoil_intensity = 100;
				player.stun_timer = 20;
				player.invincibility_timer = 20;
				mmEffect(SFX_BOING0);
				shake_screen(6, 10);
			}
			else
			{
				// Bounce off Screen Edge
				if (px < 0)
				{
					player.x = 0 << 8;
					player.vx = -player.vx;
					play_random_sfx(sfx_thud, NUM_THUD_SFX);
				}
				else if (px > 224)
				{
					player.x = 224 << 8;
					player.vx = -player.vx;
					play_random_sfx(sfx_thud, NUM_THUD_SFX);
				}
				if (py < 0)
				{
					player.y = 0 << 8;
					player.vy = -player.vy;
					play_random_sfx(sfx_thud, NUM_THUD_SFX);
				}
				else if (py > 144)
				{
					player.y = 144 << 8;
					player.vy = -player.vy;
					play_random_sfx(sfx_thud, NUM_THUD_SFX);
				}
			}

			ret_x = px + 4 + ((aim_dist * lu_cos(aim_angle)) >> 12);
			ret_y = py + 4 + ((aim_dist * lu_sin(aim_angle)) >> 12);

			// Shoot
			if (key_hit(KEY_A) && player.stun_timer == 0)
			{
				play_random_sfx(sfx_shoot, NUM_SHOOT_SFX);
				u16 t_aim_angle = aim_angle;
				int closest_dist_sq = 140 * 140;
				int cone_threshold = 30 * 182;

				for (int e = 0; e < MAX_ENEMIES; e++)
				{
					if (!enemy_pool[e].active || enemy_pool[e].hp <= 0)
						continue;
					int dx = (enemy_pool[e].x - player.x) >> 8;
					int dy = (enemy_pool[e].y - player.y) >> 8;
					int dist_sq = (dx * dx) + (dy * dy);
					if (dist_sq < closest_dist_sq)
					{
						u16 enemy_angle = ArcTan2(dx, dy);
						s16 angle_difference = (s16)(enemy_angle - aim_angle);
						if (angle_difference >= -cone_threshold && angle_difference <= cone_threshold)
						{
							closest_dist_sq = dist_sq;
							t_aim_angle = enemy_angle;
						}
					}
				}

				u16 recoil_angle = t_aim_angle + (taken_upgrades[WRONG_WAY] ? 0 : 32768);
				int cruise_force = (280 * player.cruise_modifier) >> 8;
				player.vx = (cruise_force * lu_cos(recoil_angle)) >> 12;
				player.vy = (cruise_force * lu_sin(recoil_angle)) >> 12;
				player.recoil_intensity = (32 * player.recoil_modifier) >> 8;
				player.recoil_frames = 16;
				shake_screen(2, 5);
				for (int i = 0; i < player.bullet_amount; i++)
				{
					for (int b = 0; b < MAX_BULLETS; b++)
					{
						if (!bullet_pool[b].active)
						{
							bullet_pool[b].active = 1;
							bullet_pool[b].is_enemy = false;
							bullet_pool[b].x = player.x + (4 << 8);
							bullet_pool[b].y = player.y + (4 << 8);
							bullet_pool[b].lifetime = player.bullet_lifetime;
							if (taken_upgrades[RICOCHET] == true)
								bullet_pool[b].bounces_left = 1;
							else
								bullet_pool[b].bounces_left = 0;
							bullet_pool[b].last_enemy_hit = -1;

							int bullet_speed = 3 << 8;
							u16 spread_angle = t_aim_angle + (i * 3000) - ((player.bullet_amount - 1) * 1000);
							bullet_pool[b].vx = (bullet_speed * lu_cos(spread_angle)) >> 12;
							bullet_pool[b].vy = (bullet_speed * lu_sin(spread_angle)) >> 12;
							break;
						}
					}
				}
			}

			int boost = 256 + (player.recoil_frames * player.recoil_intensity);

			// Apply boost
			player.x += (player.vx * boost) >> 8;
			player.y += (player.vy * boost) >> 8;

			if (player.recoil_frames > 0)
				player.recoil_frames--;

			// Update Bullet
			for (int i = 0; i < MAX_BULLETS; i++)
			{
				if (bullet_pool[i].active)
				{
					bullet_pool[i].lifetime--;
					if (bullet_pool[i].lifetime <= 0)
					{
						bullet_pool[i].active = 0;
						continue;
					}
					bullet_pool[i].x += bullet_pool[i].vx;
					bullet_pool[i].y += bullet_pool[i].vy;
					int bx = bullet_pool[i].x >> 8;
					int by = bullet_pool[i].y >> 8;
					if (!bullet_pool[i].is_enemy)
					{
						if (bx < 0)
						{
							bullet_pool[i].x = 0 << 8;
							bullet_pool[i].vx = -bullet_pool[i].vx;
						}
						else if (bx > 232)
						{
							bullet_pool[i].x = 232 << 8;
							bullet_pool[i].vx = -bullet_pool[i].vx;
						}
						if (by < 0)
						{
							bullet_pool[i].y = 0 << 8;
							bullet_pool[i].vy = -bullet_pool[i].vy;
						}
						else if (by > 152)
						{
							bullet_pool[i].y = 152 << 8;
							bullet_pool[i].vy = -bullet_pool[i].vy;
						}
					}
					else if (bx < 0 || bx > 240 || by < 0 || by > 160)
						bullet_pool[i].active = 0;

					int b_pb = bullet_pool[i].is_enemy ? 8 : 2;
					obj_set_attr(&obj_buffer[OAM_BULLET_START + i], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(b_pb) | 5);
					obj_set_pos(&obj_buffer[OAM_BULLET_START + i], bx, by);
				}
				else
					obj_set_pos(&obj_buffer[OAM_BULLET_START + i], 240, 160);
			}

			// Update Enemies
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (enemy_pool[i].active)
				{
					if (enemy_pool[i].flash_timer > 0)
						enemy_pool[i].flash_timer--;
					if (enemy_pool[i].hp <= 0)
					{
						if (enemy_pool[i].flash_timer == 0)
						{
							if (enemy_pool[i].type == ENEMY_EGG)
							{
								for (int s = 0; s < 2; s++)
								{
									EnemyType r_enemy = (EnemyType)qran_range(0, 3);
									int spawn_x = (enemy_pool[i].x >> 8) + qran_range(-12, 12);
									int spawn_y = (enemy_pool[i].y >> 8) + qran_range(-12, 12);
									spawn_enemy(r_enemy, spawn_x, spawn_y, current_wave);
									enemies_on_screen++;
								}
							}
							enemy_pool[i].active = 0;
							obj_set_pos(&obj_buffer[OAM_ENEMY_START + i], 240, 160);
							continue;
						}
						enemy_pool[i].vx = 0;
						enemy_pool[i].vy = 0;
					}

					int dx = (player.x - enemy_pool[i].x) >> 8;
					int dy = (player.y - enemy_pool[i].y) >> 8;
					int total_dist_squared = (dx * dx) + (dy * dy);
					u16 angle = ArcTan2(dx, dy);
					u16 flip = 0;
					if (dx < 0)
						flip = ATTR1_HFLIP;

					switch (enemy_pool[i].type)
					{
					case ENEMY_CHARGER:
					{
						int action_range = 80;
						switch (enemy_pool[i].state)
						{
						case STATE_CHASE:
						{
							int enemy_speed = 0.3 * 256;
							enemy_pool[i].vx = (enemy_speed * lu_cos(angle)) >> 12;
							enemy_pool[i].vy = (enemy_speed * lu_sin(angle)) >> 12;
							if (enemy_pool[i].action_timer > 0)
								enemy_pool[i].action_timer--;
							else
							{
								if (total_dist_squared <= action_range * action_range)
								{
									mmEffect(SFX_WIND0);
									enemy_pool[i].state = STATE_WINDUP;
									enemy_pool[i].action_timer = 30;
									enemy_pool[i].target_angle = angle;
								}
							}
							break;
						}
						case STATE_WINDUP:
						{
							enemy_pool[i].vx = 0;
							enemy_pool[i].vy = 0;

							if (enemy_pool[i].action_timer > 0)
								enemy_pool[i].action_timer--;
							else
							{
								int enemy_speed = 4 * 256;
								enemy_pool[i].vx = (enemy_speed * lu_cos(enemy_pool[i].target_angle)) >> 12;
								enemy_pool[i].vy = (enemy_speed * lu_sin(enemy_pool[i].target_angle)) >> 12;

								mmEffect(SFX_LAUNCH0);
								enemy_pool[i].state = STATE_ATTACK;
								enemy_pool[i].action_timer = 20;
							}
							break;
						}
						case STATE_ATTACK:
						{
							if (enemy_pool[i].action_timer > 0)
								enemy_pool[i].action_timer--;
							else
							{
								enemy_pool[i].state = STATE_CHASE;
								enemy_pool[i].action_timer = 60;
							}
							break;
						}
						}
						int pb = (enemy_pool[i].flash_timer > 0) ? 7 : 3;
						obj_set_attr(&obj_buffer[OAM_ENEMY_START + i], ATTR0_SQUARE, ATTR1_SIZE_16 | flip, ATTR2_PALBANK(pb) | 6);
					}
					break;
					case ENEMY_DINO:
					{
						int enemy_speed = 0.4 * 256;
						enemy_pool[i].vx = (enemy_speed * lu_cos(angle)) >> 12;
						enemy_pool[i].vy = (enemy_speed * lu_sin(angle)) >> 12;

						int pb = (enemy_pool[i].flash_timer > 0) ? 7 : 4;
						obj_set_attr(&obj_buffer[OAM_ENEMY_START + i], ATTR0_SQUARE, ATTR1_SIZE_16 | flip, ATTR2_PALBANK(pb) | 10);
					}
					break;
					case ENEMY_GREEN:
					{
						int action_range = 100;
						if (total_dist_squared <= action_range * action_range)
						{
							enemy_pool[i].vx = 0;
							enemy_pool[i].vy = 0;

							if (enemy_pool[i].action_timer > 0)
								enemy_pool[i].action_timer--;
							else
							{
								for (int b = 0; b < MAX_BULLETS; b++)
								{
									if (!bullet_pool[b].active)
									{
										bullet_pool[b].active = 1;
										bullet_pool[b].is_enemy = true;
										bullet_pool[b].x = enemy_pool[i].x + (4 << 8);
										bullet_pool[b].y = enemy_pool[i].y + (4 << 8);
										bullet_pool[b].lifetime = 90;

										int bullet_speed = 1 << 8;
										bullet_pool[b].vx = (bullet_speed * lu_cos(angle)) >> 12;
										bullet_pool[b].vy = (bullet_speed * lu_sin(angle)) >> 12;
										play_random_sfx(sfx_shoot, NUM_SHOOT_SFX);
										break;
									}
								}
								enemy_pool[i].action_timer = 90;
							}
						}
						else
						{
							int enemy_speed = 0.2 * 256;
							enemy_pool[i].vx = (enemy_speed * lu_cos(angle)) >> 12;
							enemy_pool[i].vy = (enemy_speed * lu_sin(angle)) >> 12;
						}
						int pb = (enemy_pool[i].flash_timer > 0) ? 7 : 5;
						obj_set_attr(&obj_buffer[OAM_ENEMY_START + i], ATTR0_SQUARE, ATTR1_SIZE_16 | flip, ATTR2_PALBANK(pb) | 14);
					}
					break;
					case ENEMY_EGG:
					{
						int enemy_speed = 0.1 * 256;
						enemy_pool[i].vx = (enemy_speed * lu_cos(angle)) >> 12;
						enemy_pool[i].vy = (enemy_speed * lu_sin(angle)) >> 12;

						int pb = (enemy_pool[i].flash_timer > 0) ? 7 : 12;
						obj_set_attr(&obj_buffer[OAM_ENEMY_START + i], ATTR0_TALL, ATTR1_SIZE_16x32 | flip, ATTR2_PALBANK(pb) | 228);
					}
					break;
					}
					enemy_pool[i].x += enemy_pool[i].vx;
					enemy_pool[i].y += enemy_pool[i].vy;
					int ex = enemy_pool[i].x >> 8;
					int ey = enemy_pool[i].y >> 8;
					if (enemy_pool[i].state == STATE_WINDUP)
					{
						ex += (qran() % 3) - 1;
						ey += (qran() % 3) - 1;
					}
					// Bounce off Left / Right walls
					if (ex < 0)
					{
						enemy_pool[i].x = 0;
						enemy_pool[i].vx = -enemy_pool[i].vx;
						ex = 0;
					}
					else if (ex > 224)
					{
						enemy_pool[i].x = 224 << 8;
						enemy_pool[i].vx = -enemy_pool[i].vx;
						ex = 224;
					}

					// Bounce off Top / Bottom walls
					if (ey < 0)
					{
						enemy_pool[i].y = 0;
						enemy_pool[i].vy = -enemy_pool[i].vy;
						ey = 0;
					}
					else if (ey > 144)
					{
						enemy_pool[i].y = 144 << 8;
						enemy_pool[i].vy = -enemy_pool[i].vy;
						ey = 144;
					}

					obj_set_pos(&obj_buffer[OAM_ENEMY_START + i], ex, ey);
				}
				else
					obj_set_pos(&obj_buffer[OAM_ENEMY_START + i], 240, 160);
			}

			int ppb = (player.flash_timer > 0) ? 7 : 0;

			// SPINNN
			int current_vx = (player.vx * boost) >> 8;
			int current_vy = (player.vy * boost) >> 8;
			int abs_vx = (current_vx < 0) ? -current_vx : current_vx;
			int abs_vy = (current_vy < 0) ? -current_vy : current_vy;
			int total_speed = abs_vx + abs_vy;
			int spin_rate = 4;
			if (player.vx < 0)
				player.angle += (total_speed * spin_rate);
			else
				player.angle -= (total_speed * spin_rate);
			obj_aff_rotate(&obj_aff_buffer[0], player.angle);
			obj_set_attr(&obj_buffer[OAM_PLAYER], ATTR0_SQUARE | ATTR0_AFF_DBL, ATTR1_SIZE_16 | ATTR1_AFF_ID(0), ATTR2_PALBANK(ppb) | 0);

			// Set Object Positions
			obj_set_pos(&obj_buffer[OAM_PLAYER], px - 8, py - 8);
			obj_set_pos(&obj_buffer[OAM_RETICLE], ret_x, ret_y);

			if (player.flash_timer > 0)
				player.flash_timer--;
			if (player.stun_timer > 0)
				player.stun_timer--;
			if (player.invincibility_timer > 0)
			{
				player.invincibility_timer--;
				if (player.invincibility_timer & 4)
					obj_set_pos(&obj_buffer[OAM_PLAYER], 240, 160);
			}

			// COLLISION DETECTION 😨
			for (int b = 0; b < MAX_BULLETS; b++)
			{
				if (!bullet_pool[b].active)
					continue;
				if (!bullet_pool[b].is_enemy)
				{
					for (int e = 0; e < MAX_ENEMIES; e++)
					{
						if (!enemy_pool[e].active || enemy_pool[e].hp <= 0)
							continue;

						if (e != bullet_pool[b].last_enemy_hit && check_collision(bullet_pool[b].x, bullet_pool[b].y, 8 << 8, 8 << 8, enemy_pool[e].x, enemy_pool[e].y, 16 << 8, 16 << 8))
						{
							enemy_pool[e].flash_timer = 8;
							enemy_pool[e].hp--;
							bullet_pool[b].last_enemy_hit = e;

							if (enemy_pool[e].hp <= 0)
							{
								enemies_on_screen--;
							}
							play_random_sfx(sfx_hit, NUM_HIT_SFX);

							if (bullet_pool[b].bounces_left > 0)
							{
								int target_enemy = -1;
								int closest_dist_sq = 150 * 150;

								for (int next_e = 0; next_e < MAX_ENEMIES; next_e++)
								{
									if (next_e == e || !enemy_pool[next_e].active || enemy_pool[next_e].hp <= 0)
										continue;
									int dx = (enemy_pool[next_e].x - bullet_pool[b].x) >> 8;
									int dy = (enemy_pool[next_e].y - bullet_pool[b].y) >> 8;
									int dist_sq = (dx * dx) + (dy * dy);

									if (dist_sq < closest_dist_sq)
									{
										closest_dist_sq = dist_sq;
										target_enemy = next_e;
									}
								}
								if (target_enemy != -1)
								{
									bullet_pool[b].bounces_left--;
									int dx = (enemy_pool[target_enemy].x - bullet_pool[b].x) >> 8;
									int dy = (enemy_pool[target_enemy].y - bullet_pool[b].y) >> 8;
									u16 bounce_angle = ArcTan2(dx, dy);

									int speed = 3 << 8;
									bullet_pool[b].vx = (speed * lu_cos(bounce_angle)) >> 12;
									bullet_pool[b].vy = (speed * lu_sin(bounce_angle)) >> 12;
									bullet_pool[b].lifetime += 20;
								}
								else
									bullet_pool[b].active = 0;
							}
							else
								bullet_pool[b].active = 0;
							break;
						}
					}
					if (!bullet_pool[b].active)
						continue;
					for (int b2 = 0; b2 < MAX_BULLETS; b2++)
					{
						if (!bullet_pool[b2].active || !bullet_pool[b2].is_enemy)
							continue;

						if (check_collision(bullet_pool[b].x, bullet_pool[b].y, 8 << 8, 8 << 8, bullet_pool[b2].x, bullet_pool[b2].y, 8 << 8, 8 << 8))
						{
							bullet_pool[b].active = 0;
							bullet_pool[b2].active = 0;
							play_random_sfx(sfx_thud, NUM_THUD_SFX);
							break;
						}
					}
				}
				else
				{
					if (current_state == GAMEPLAY && player.flash_timer == 0 && player.invincibility_timer == 0 &&
						check_collision(player.x + (4 << 8), player.y + (4 << 8), 8 << 8, 8 << 8, bullet_pool[b].x, bullet_pool[b].y, 8 << 8, 8 << 8))
					{
						int dx = (player.x - bullet_pool[b].x) >> 8;
						int dy = (player.y - bullet_pool[b].y) >> 8;
						bullet_pool[b].active = 0;
						u16 recoil_angle = ArcTan2(dx, dy);
						int cruise_force = (280 * player.cruise_modifier) >> 8;
						player.vx = (cruise_force * lu_cos(recoil_angle)) >> 12;
						player.vy = (cruise_force * lu_sin(recoil_angle)) >> 12;
						player.recoil_frames = 8;
						player.recoil_intensity = 80;
						shake_screen(4, 10);
						play_random_sfx(sfx_hit, NUM_HIT_SFX);
						update_hearts(obj_buffer, -1, current_wave);
						player.flash_timer = 16;
						break;
					}
				}
			}
			for (int e = 0; e < MAX_ENEMIES; e++)
			{
				if (!enemy_pool[e].active || enemy_pool[e].hp <= 0)
					continue;

				if (current_state == GAMEPLAY && player.flash_timer == 0 && player.invincibility_timer == 0 &&
					check_collision(player.x + (4 << 8), player.y + (4 << 8), 8 << 8, 8 << 8, enemy_pool[e].x, enemy_pool[e].y, 16 << 8, 16 << 8))
				{
					int dx = (player.x - enemy_pool[e].x) >> 8;
					int dy = (player.y - enemy_pool[e].y) >> 8;
					u16 recoil_angle = ArcTan2(dx, dy);
					int cruise_force = (280 * player.cruise_modifier) >> 8;
					player.vx = (cruise_force * lu_cos(recoil_angle)) >> 12;
					player.vy = (cruise_force * lu_sin(recoil_angle)) >> 12;
					player.recoil_frames = 8;
					player.recoil_intensity = 80;
					shake_screen(4, 10);
					play_random_sfx(sfx_hit, NUM_HIT_SFX);
					update_hearts(obj_buffer, -1, current_wave);
					player.flash_timer = 16;
					break;
				}
			}
		}
		else if (current_state == UPGRADE)
		{
			if (key_hit(KEY_DOWN))
			{
				menu_selection++;
				if (menu_selection > 2)
					menu_selection = 0;
				draw_upgrade_menu(pool_choices, menu_selection, false);
				mmEffect(SFX_BLIP0);
			}
			else if (key_hit(KEY_UP))
			{
				menu_selection--;
				if (menu_selection < 0)
					menu_selection = 2;
				draw_upgrade_menu(pool_choices, menu_selection, false);
				mmEffect(SFX_BLIP0);
			}
			if (key_hit(KEY_A) || key_hit(KEY_START))
			{
				mmEffect(SFX_SELECT0);
				switch (pool_choices[menu_selection])
				{
				case ONE_UP:
					player.stocks++;
					break;
				case BUBBLE_WRAP:
					player.max_hp++;
					break;
				case GUST_OF_WIND:
					player.cruise_modifier += 32; // Value uncertain
					break;
				case ANVIL:
					player.cruise_modifier -= 32; // Value uncertain
					break;
				// case QUICK_TRIGGER:
				// player.fire_delay -= 2; // Downside uncertain
				// break;
				case JETPACK:
					player.recoil_modifier += 128; // Value uncertain
					break;
				case SHOCK_ABSORBER:
					player.recoil_modifier -= 128; // Value uncertain
					break;
				case BULLET_GLIDER:
					player.bullet_lifetime += 10; // Value to be balanced
					break;
				// case JETBACKPACK:
				// 	player.fire_delay -= 2;
				// 	player.recoil_modifier += 128; // Value uncertain
				// break;
				case HEAVY_CANNON:
					player.recoil_modifier += 128; // Value uncertain
					player.cruise_modifier -= 64;  // Value bit more uncertain
					break;
				case ARMOR:
					player.max_hp += 2;
					player.cruise_modifier -= 64; // Value bit more uncertain
					player.bullet_lifetime -= 20; // Value bit more uncertain and balanced
					break;
				case FEATHER:
					player.max_hp--;
					player.cruise_modifier += 32; // Value bit more uncertain
					player.bullet_lifetime += 20; // Value bit more uncertain and balanced
					break;
				case SUGAR_RUSH:
					player.cruise_modifier += 32;
					player.bullet_lifetime -= 10;
					break;
				case ANCHOR:
					player.cruise_modifier -= 32;
					player.bullet_lifetime += 15;
					break;
				case WD_40:
					player.cruise_modifier += 32;
					player.recoil_modifier -= 128;
					break;
				case BOWLING_BALL:
					player.max_hp++;
					player.recoil_modifier += 128;
					player.cruise_modifier -= 32;
					break;
				case LONG_BARREL:
					player.bullet_lifetime += 15;
					player.recoil_modifier += 128;
					break;
				case SHOTGUN:
					player.bullet_amount = 3;
					player.bullet_lifetime -= 40;
					taken_upgrades[PISTOL] = false;
					taken_upgrades[SHOTGUN] = true;
					break;
				case PISTOL:
					player.bullet_amount = 1;
					player.bullet_lifetime += 40;
					taken_upgrades[SHOTGUN] = false;
					taken_upgrades[PISTOL] = true;
					break;
				case RICOCHET:
					taken_upgrades[RICOCHET] = true;
					break;
				case WRONG_WAY:
					taken_upgrades[WRONG_WAY] = true;
					taken_upgrades[BACK_ON_TRACK] = false;
					break;
				case BACK_ON_TRACK:
					taken_upgrades[BACK_ON_TRACK] = true;
					taken_upgrades[WRONG_WAY] = false;
					break;
				default:
					break;
				}

				if (player.cruise_modifier < 96)
					player.cruise_modifier = 96;
				if (player.recoil_modifier < 64)
					player.recoil_modifier = 64;
				if (player.max_hp < 1)
					player.max_hp = 1;
				if (player.bullet_lifetime < 10)
					player.bullet_lifetime = 10;

				current_wave++;
				wave_timer = (20 + (current_wave - 1) * 3) * 60;
				REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG3;
				player.hp = player.max_hp;
				update_hearts(obj_buffer, 0, current_wave);
				intermission_timer = 60;
				current_state = INTERMISSION_OUT;
				mmEffect(SFX_WAVE_START);
			}
		}

		// -- HUD Display --
		// Wave Number
		int v_margin = 6;
		if (current_state != GAME_OVER && current_state != TITLE_SCREEN && current_state != PAUSE)
		{
			int wave_tens = current_wave / 10;
			int wave_ones = current_wave % 10;

			// Tens Digit Sprite
			obj_set_attr(&obj_buffer[OAM_WAVE_START], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(9) | (24 + wave_tens));
			obj_set_pos(&obj_buffer[OAM_WAVE_START], 220, v_margin);

			// Ones Digit Sprite
			obj_set_attr(&obj_buffer[OAM_WAVE_START + 1], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(9) | (24 + wave_ones));
			obj_set_pos(&obj_buffer[OAM_WAVE_START + 1], 228, v_margin);

			// Wave Timer
			int seconds_left = wave_timer / 60;
			if (seconds_left < 0)
				seconds_left = 0;

			int timer_tens = seconds_left / 10;
			int timer_ones = seconds_left % 10;

			// Tens Digit Sprite
			obj_set_attr(&obj_buffer[OAM_TIME_START], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(9) | (24 + timer_tens));
			obj_set_pos(&obj_buffer[OAM_TIME_START], 112, v_margin);

			// Ones Digit Sprite
			obj_set_attr(&obj_buffer[OAM_TIME_START + 1], ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(9) | (24 + timer_ones));
			obj_set_pos(&obj_buffer[OAM_TIME_START + 1], 120, v_margin);
		}
	}
	return 0;
}