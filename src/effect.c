#include "effect.h"

#include <genesis.h>
#include "tables.h"
#include "sprite.h"
#include "resources.h"
#include "camera.h"

#define MAX_DAMAGE_STRINGS 4

#define MAX_SMOKE 8
#define SMOKE_NONE 0
#define SMOKE_SMALL 1
#define SMOKE_MID 2
#define SMOKE_BIG 3
#define SMOKE_PLAYER 4

typedef struct {
	u8 sprite[4];
	u8 sprite_count;
	s16 x, y;
	u8 ttl;
} damage_string_def;

damage_string_def damage_string[MAX_DAMAGE_STRINGS];

typedef struct {
	u8 sprite;
	s16 x, y;
	u8 ttl;
} smoke_def;

smoke_def smoke[MAX_SMOKE];

// Internal functions
void damage_string_update(damage_string_def *d);
void damage_string_cleanup(damage_string_def *d);
void smoke_update(smoke_def *s);
s16 abs(s16 v);
s16 pow(s16 base, s16 exp);

void effects_clear() {
	for(u8 i = 0; i < MAX_DAMAGE_STRINGS; i++) {
		damage_string[i].ttl = 0;
		damage_string_cleanup(&damage_string[i]);
	}
	for(u8 i = 0; i < MAX_SMOKE; i++) {
		smoke[i].ttl = 0;
		sprite_delete(smoke[i].sprite);
	}
}

void effects_update() {
	for(u8 i = 0; i < MAX_DAMAGE_STRINGS; i++) {
		if(damage_string[i].ttl == 0) continue;
		damage_string[i].ttl--;
		if(damage_string[i].ttl == 0) {
			damage_string_cleanup(&damage_string[i]);
		} else {
			damage_string_update(&damage_string[i]);
		}
	}
	for(u8 i = 0; i < MAX_SMOKE; i++) {
		if(smoke[i].ttl == 0) continue;
		smoke[i].ttl--;
		if(smoke[i].ttl == 0) {
			sprite_delete(smoke[i].sprite);
		} else {
			smoke_update(&smoke[i]);
		}
	}
}

void effect_create_damage_string(s16 num, s16 x, s16 y, u8 ttl) {
	for(u8 i = 0; i < MAX_DAMAGE_STRINGS; i++) {
		if(damage_string[i].ttl > 0) continue;
		damage_string[i].x = x;
		damage_string[i].y = y;
		damage_string[i].ttl = 60;
		u8 digits = 1;
		if(abs(num) >= 10) digits++;
		if(abs(num) >= 100) digits++;
		damage_string[i].sprite_count = digits + 1;
		u8 anim = num < 0;
		damage_string[i].sprite[0] = sprite_create(&SPR_Numbers, PAL0, true);
		sprite_set_animframe(damage_string[i].sprite[0], anim, 10);
		for(u8 d = 0; d < digits; d++) {
			damage_string[i].sprite[d+1] = sprite_create(&SPR_Numbers, PAL0, true);
			sprite_set_animframe(damage_string[i].sprite[d+1], anim, (abs(num) / pow(10,d)) % 10);
		}
		break; // Oh that's why they lag so much
	}
}

void damage_string_update(damage_string_def *d) {
	if(d->ttl%2) d->y -= 1;
	for(u8 i = 0; i < d->sprite_count; i++) {
		sprite_set_position(d->sprite[i],
			d->x - sub_to_pixel(camera.x) + SCREEN_HALF_W - 16 + (8*i),
			d->y - sub_to_pixel(camera.y) + SCREEN_HALF_H - 8);
	}
}

void damage_string_cleanup(damage_string_def *d) {
	for(u8 s = 0; s < d->sprite_count; s++) sprite_delete(d->sprite[s]);
}

void effect_create_smoke(u8 type, s16 x, s16 y) {
	for(u8 i = 0; i < MAX_SMOKE; i++) {
		if(smoke[i].ttl > 0) continue;
		smoke[i].x = x;
		smoke[i].y = y;
		smoke[i].ttl = 48;
		smoke[i].sprite = sprite_create(&SPR_Smoke, PAL1, false);
		sprite_set_position(smoke[i].sprite,
				smoke[i].x - sub_to_pixel(camera.x) + SCREEN_HALF_W - 8,
				smoke[i].y - sub_to_pixel(camera.y) + SCREEN_HALF_H - 8);
		break;
	}
}

void smoke_update(smoke_def *s) {
	sprite_set_position(s->sprite,
			s->x - sub_to_pixel(camera.x) + SCREEN_HALF_W - 8,
			s->y - sub_to_pixel(camera.y) + SCREEN_HALF_H - 8);
}

s16 abs(s16 v) {
	return v * ((v>0) - (v<0));
}

s16 pow(s16 base, s16 exp) {
    s16 result = 1;
    while(exp) { result *= base; exp--; }
    return result;
}
