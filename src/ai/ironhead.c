#include "ai.h"

#include <genesis.h>
#include "audio.h"
#include "player.h"
#include "stage.h"
#include "tables.h"
#include "tsc.h"
#include "effect.h"
#include "camera.h"
#include "system.h"
#include "sheet.h"
#include "sprite.h"
#include "resources.h"
#include "npc.h"
#include "vdp_ext.h"

#define ARENA_TOP				2
#define ARENA_BOTTOM			13

#define IRONH_SPAWN_FISHIES		100
#define IRONH_SWIM				250
#define IRONH_DEFEATED			1000

#define player_hit	curly_target_time
#define dir2		jump_time

void onspawn_ironhead(Entity *e) {
	e->alwaysActive = TRUE;
	e->attack = 10;
	e->health = 400;
	e->hit_box = (bounding_box) { 20, 8, 20, 8 };
	e->display_box = (bounding_box) { 28, 12, 28, 12 };
	e->hurtSound = SND_ENEMY_HURT_COOL;
	e->state = IRONH_SPAWN_FISHIES;
	// Keep track if player gets hurt
	player_hit = FALSE;
}

void ondeath_ironhead(Entity *e) {
	e->state = IRONH_DEFEATED;
	tsc_call_event(1000);
	bossEntity = NULL;
}

void ai_ironhead(Entity *e) {
	if(!player_hit && playerIFrames) player_hit = TRUE;
	switch(e->state) {
		case IRONH_SPAWN_FISHIES:
		{
			e->dir = 1;
			e->timer = 0;
			e->state++;
		}
		case IRONH_SPAWN_FISHIES+1:		// wave of fishies comes in
		{
			if (++e->timer > 50) {
				e->timer = 0;
				e->state = IRONH_SWIM;
			}
			if (!(e->timer & 7)) {
				entity_create(((15 + (random() % 3)) * 16) << CSF,
						  	((ARENA_TOP + (random() % (ARENA_BOTTOM-ARENA_TOP))) * 16) << CSF,
						  	OBJ_IRONH_FISHY, 0);
			}
		}
		break;
		case IRONH_SWIM:		// swimming attack
		{
			e->state++;
			if (e->dir2) {	// coming up on player from left
				e->x = 0x1e000;
				e->y = player.y;
			} else {	// returning from right side of screen
				e->x = 0x5a000;
				e->y = ((ARENA_TOP+random() % (ARENA_BOTTOM-ARENA_TOP)) * 16) << CSF;
			}
			
			e->x_mark = e->x;
			e->y_mark = e->y;
			
			e->y_speed = -SPEED(0x200) + (random() % SPEED(0x400));
			e->x_speed = -SPEED(0x200) + (random() % SPEED(0x400));
			
			e->eflags |= NPC_SHOOTABLE;
		}
		case IRONH_SWIM+1:
		{
			ANIMATE(e, 8, 4,3,2,3,4,1,0,1);
			if (e->dir2) {
				e->x_mark += SPEED(0x400);
			} else {
				e->x_mark -= SPEED(0x200);
				e->y_mark += (e->y_mark < player.y) ? SPEED(0x200): -SPEED(0x200);
			}
			
			e->x_speed += (e->x > e->x_mark) ? -8 : 8;
			e->y_speed += (e->y > e->y_mark) ? -8 : 8;
			
			LIMIT_Y(SPEED(0x200));
			
			if (e->dir2) {
				if (e->x > 0x5a000) {
					e->dir2 = 0;
					e->state = IRONH_SPAWN_FISHIES;
				}
			} else {
				if (e->x < 0x22000) {
					e->dir2 = 1;
					e->state = IRONH_SPAWN_FISHIES;
				}
			}
			
			if (!e->dir2) {
				// fire bullets at player when retreating
				switch(++e->timer) {
					case 300:
					case 310:
					case 320:
					{
						Entity *shot = entity_create(e->x, e->y, OBJ_IRONH_SHOT, 0);
						shot->x_speed = -3 + ((random() % 4) << CSF);
						shot->y_speed = -3 + ((random() % 7) << CSF);
						sound_play(SND_EM_FIRE, 5);
					}
					break;
				}
			}
		}
		break;
		case IRONH_DEFEATED:
		{
			sound_play(SND_EXPL_SMALL, 8);
			e->state = IRONH_DEFEATED+1;
			e->eflags &= ~NPC_SHOOTABLE;
			e->frame = 5;
			e->attack = 0;
			e->x_mark = e->x;
			e->y_mark = e->y;
			e->x_speed = e->y_speed = 0;
			e->timer = 0;
			// I believe the screen should flash here since objects get deleted
			SCREEN_FLASH(10);
			entities_clear_by_type(OBJ_IRONH_FISHY);
			entities_clear_by_type(OBJ_IRONH_BRICK);
			entities_clear_by_type(OBJ_BRICK_SPAWNER);
			camera_shake(20);
			
			//for(int i=0;i<32;i++)
			//	ironh_smokecloud(o);
		}
		case IRONH_DEFEATED+1:			// retreat back to left...
		{
			e->x_mark -= (1<<CSF);
			
			e->x = e->x_mark - 1 + ((random() % 2) << CSF);
			e->y = e->y_mark - 1 + ((random() % 2) << CSF);
			
			e->timer++;
			//if ((e->timer & 3)==0) ironh_smokecloud(o);
		}
		break;
	}
	
	// show pink "hit" frame when he's taking damage
	//e->sprite = SPR_IRONH;
	//if (e->shaketime)
	//{
	//	this->hittimer++;
	//	if (this->hittimer & 2)
	//	{
	//		e->sprite = SPR_IRONH_HURT;
	//	}
	//}
	//else
	//{
	//	this->hittimer = 0;
	//}
	
	e->x += e->x_speed;
	e->y += e->y_speed;
}

//static void ironh_smokecloud(Entity *e) {
//	Entity *smoke;
//
//	smoke = CreateEntity(e->CenterX() + (random(-128, 128)<<CSF),
//						 e->CenterY() + (random(-64, 64)<<CSF),
//						 OBJ_SMOKE_CLOUD);
//	
//	smoke->x_speed = random(-128, 128);
//	smoke->y_speed = random(-128, 128);
//}

void ai_ironh_fishy(Entity *e) {
	e->x_next = e->x + e->x_speed;
	e->y_next = e->y + e->y_speed;
	switch(e->state) {
		case 0:
		{
			e->state = 10;
			e->animtime = 0;
			e->y_speed = -SPEED(0x200) + (random() % SPEED(0x400));
			e->x_speed = SPEED(0x800);
		}
		case 10:			// harmless fishy
		{
			ANIMATE(e, 8, 0,1);
			if (e->x_speed < 0) {
				e->attack = 3;
				e->state = 20;
			}
		}
		break;
		case 20:			// puffer fish
		{
			ANIMATE(e, 8, 2,3);
		}
		break;
	}
	
	if (e->y_speed < 0 && collide_stage_ceiling(e)) e->y_speed = SPEED(0x200);
	if (e->y_speed > 0 && collide_stage_floor(e)) e->y_speed = -SPEED(0x200);
	e->x_speed -= SPEED(0x0c);
	e->x = e->x_next;
	e->y = e->y_next;
	
	if (e->x_speed < 0 && e->x < camera.x - (SCREEN_HALF_W << CSF)) 
			e->state = STATE_DELETE;
}

void ai_ironh_shot(Entity *e) {
	if (!e->state) {
		e->dir = 1;
		if (++e->timer > 20) {
			e->state = 1;
			e->x_speed = e->y_speed = 0;
			e->timer2 = 0;
		}
	} else {
		e->x_speed += SPEED(0x20);
	}
	
	ANIMATE(e, 8, 0,1,2);
	
	if (++e->timer2 > 100 && !entity_on_screen(e)) {
		e->state = STATE_DELETE;
	}
	
	if ((e->timer2 & 3)==1) sound_play(SND_IRONH_SHOT_FLY, 3);
	
	e->x += e->x_speed;
	e->y += e->y_speed;
}


void ai_brick_spawner(Entity *e) {
	if (!e->state) {
		e->state = 1;
		e->timer = TIME(25) + (random() % TIME(150));
	}
	
	if (!e->timer) {	// time to spawn a block
		e->state = 0;
		Entity *brick = entity_create(e->x, e->y - (20<<CSF) + ((random() % 40)<<CSF), 
				OBJ_IRONH_BRICK, 0);
		brick->dir = e->dir;
	} else e->timer--;
}

void ai_ironh_brick(Entity *e) {
	if (!e->state) {
		u8 r = random() % 10;
		if (r) {
			e->frame = e->oframe = 255;
			e->vramindex = sheets[e->sheet].index + 16 + (r % 4) * 4;
			e->sprite[0].size = SPRITE_SIZE(2, 2);
			e->sprite[0].attribut = TILE_ATTR_FULL(PAL2,0,0,0,e->vramindex);
			e->hit_box = e->display_box = (bounding_box) { 8, 8, 8, 8 };
			e->sheet = NOSHEET;
		} else {
			e->hit_box = e->display_box = (bounding_box) { 16, 16, 16, 16 };
		}
		
		e->x_speed = SPEED(0x100) + (random() % SPEED(0x100));
		e->x_speed *= e->dir ? 2 : -2;
		
		e->y_speed = -SPEED(0x200) + (random() % SPEED(0x400));
		e->state = 1;
	}
	
	// bounce off the walls
	if (e->y_speed < 0 && e->y <= (16<<CSF)) {
		//effect(e->CenterX(), e->y, EFFECT_BONKPLUS);
		e->y_speed = -e->y_speed;
	}
	
	if (e->y_speed > 0 && (e->y + (e->hit_box.bottom<<CSF) >= (239<<CSF))) {
		//effect(e->CenterX(), e->Bottom(), EFFECT_BONKPLUS);
		e->y_speed = -e->y_speed;
	}
	
	
	if ((e->x_speed < 0 && (e->x < -0x2000)) ||
		(e->x > (stageWidth * 16) << CSF)) {
		e->state = STATE_DELETE;
	}
	e->x += e->x_speed;
	e->y += e->y_speed;
	
	// Have to manually add the small one
	if(e->frame == 255) {
		sprite_pos(e->sprite[0],
				(e->x>>CSF) - (camera.x>>CSF) + SCREEN_HALF_W - e->display_box.left,
				(e->y>>CSF) - (camera.y>>CSF) + SCREEN_HALF_H - e->display_box.top);
		sprite_add(e->sprite[0]);
	}
}

void ai_ikachan_spawner(Entity *e) {
	switch(e->state) {
		case 0:
		{
			// oops player got hurt--no ikachans for you!
			// the deletion of the object causes the flag matching it's id2 to be set,
			// which is how the scripts know not to give the alien medal.
			if (player_hit)
				e->state = STATE_DELETE;
		}
		break;
		
		case 10:	// yay spawn ikachans!
		{
			e->timer++;
			if ((e->timer & 3) == 1) {
				entity_create(e->x, e->y + (((random() % 14) * 16) << CSF), OBJ_IKACHAN, 0);
			}
		}
		break;
	}
}

void ai_ikachan(Entity *e) {
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			e->timer = 3 + (random() % 17);
		}
		case 1:		// he pushes ahead
		{
			if (--e->timer == 0) {
				e->state = 2;
				e->timer = 10 + (random() % 40);
				e->frame = 1;
				e->x_speed = 0x600;
			}
		}
		break;
		
		case 2:		// after a short time his tentacles look less whooshed-back
		{
			if (--e->timer == 0) {
				e->state = 3;
				e->timer = 10 + random() % 40;
				e->frame = 2;
				e->y_speed = -0x100 + (random() % 0x200);
			}
		}
		break;
		
		case 3:		// gliding
		{
			if (--e->timer == 0) {
				e->state = 1;
				e->timer = 1;
				e->frame = 0;
			}
			
			e->x_speed -= 0x10;
		}
		break;
	}
	
	e->x += e->x_speed;
	e->y += e->y_speed;
	
	if (e->x > 720<<CSF) e->state = STATE_DELETE;
}
