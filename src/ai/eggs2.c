#include "ai.h"

#include <genesis.h>
#include "audio.h"
#include "player.h"
#include "stage.h"
#include "tables.h"
#include "tsc.h"
#include "camera.h"
#include "effect.h"

void ai_dragon_zombie(Entity *e) {
	if (e->health < 950 && e->state < 50) {
		sound_play(SND_BIG_CRASH, 5);
		effect_create_smoke(e->x << CSF, e->y << CSF);
		entity_drop_powerup(e);
		
		e->nflags &= ~NPC_SHOOTABLE;
		e->attack = 0;
		
		e->frame = 5;	// dead
		e->state = 50;	// dead
	}
	
	switch(e->state) {
		case 0:
		case 1:		// ready
		{
			ANIMATE(e, 30, 0,1);
			
			if (e->timer == 0) {
				if (PLAYER_DIST_X(112 << CSF)) {
					e->state = 2;
					e->timer = 0;
				}
			} else {
				e->timer--;
			}
		}
		break;
		
		case 2:		// flashing, prepare to fire
		{
			FACE_PLAYER(e);
			
			e->timer++;
			e->frame = (e->timer & 4) ? 2 : 3;
			
			if (e->timer > TIME(30))
				e->state = 3;
		}
		break;
		
		case 3:
		{
			e->state = 4;
			e->timer = 0;
			e->frame = 4;
			
			// save point we'll fire at--these enemies don't update
			// the position of their target for each shot
			e->x_mark = player.x;
			e->y_mark = player.y;
		}
		case 4:
		{
			e->timer++;
			
			if (e->timer < TIME(40) && (e->timer % 8) == 1) {
				Entity *fire = entity_create(e->x, e->y, OBJ_DRAGON_ZOMBIE_SHOT, 0);
				//ThrowEntity(fire, e->xmark, e->ymark, 6, 0x600);
				
				sound_play(SND_SNAKE_FIRE, 3);
			}
			
			if (e->timer > TIME(60)) {
				e->state = 1;
				e->frame = 0;
				e->timer = (random() % TIME(100)) + TIME(100);	// random time till can fire again
			}
		}
		break;
	}
}

void ai_falling_spike_small(Entity *e) {
	switch(e->state) {
		case 0:
		{
			e->x_mark = e->x;
			
			if (PLAYER_DIST_X(12 << CSF))
				e->state = 1;
		}
		break;
		
		case 1:		// shaking
		{
			if (++e->animtime >= 12)
				e->animtime = 0;
			
			e->x = e->x_mark;
			if (e->animtime >= 6) e->x += (1 << CSF);
			
			if (++e->timer > TIME(30)) {
				e->state = 2;	// fall
				e->frame = 1;	// slightly brighter frame at top
			}
		}
		break;
		
		case 2:		// falling
		{
			e->y_speed += SPEED(0x20);
			LIMIT_Y(SPEED(0xC00));
			
			e->x_next = e->x;
			e->y_next = e->y + e->y_speed;
			if (collide_stage_floor(e)) {
				if (!controlsLocked)	// no sound in ending cutscene
					sound_play(SND_BLOCK_DESTROY, 5);
				
				//SmokeClouds(o, 4, 2, 2);
				//effect(e->CenterX(), e->CenterY(), EFFECT_BOOMFLASH);
				e->state = STATE_DELETE;
			}
			e->y = e->y_next;
		}
		break;
	}
}


void ai_falling_spike_large(Entity *e) {
	switch(e->state) {
		case 0:
		{
			e->x_mark = e->x;
			
			if (PLAYER_DIST_X(12 << CSF))
				e->state = 1;
		}
		break;
		
		case 1:		// shaking
		{
			if (++e->animtime >= 12)
				e->animtime = 0;
			
			e->x = e->x_mark;
			if (e->animtime >= 6)	// scuttle:: big spikes shake in the other direction
				e->x -= (1 << CSF);
			
			if (++e->timer > TIME(30)) {
				e->state = 2;	// fall
				e->frame = 1;	// slightly brighter frame at top
			}
		}
		break;
		
		case 2:		// falling
		{
			e->y_speed += SPEED(0x20);
			LIMIT_Y(SPEED(0xC00));
			
			if (e->y + (e->hit_box.bottom<<CSF) < player.y + (player.hit_box.top<<CSF)) {	
				// could fall on player
				e->nflags &= ~NPC_SPECIALSOLID;
				e->attack = 127;	// ouch!
			} else {	// player could only touch side from this position
				e->nflags |= NPC_SPECIALSOLID;
				e->attack = 0;
			}
			
			// damage NPC's as well (it kills that one Dragon Zombie)
			//Entity *enemy;
			//FOREACH_OBJECT(enemy)
			//{
			//	if ((enemy->flags & FLAG_SHOOTABLE) && \
			//		e->Bottom() >= enemy->CenterY() && hitdetect(o, enemy))
			//	{
			//		if (!(enemy->flags & FLAG_INVULNERABLE))
			//			enemy->DealDamage(127);
			//	}
			//}
			e->x_next = e->x;
			e->y_next = e->y + e->y_speed;
			if (++e->timer > 8 && collide_stage_floor(e)) {
				e->nflags |= NPC_SPECIALSOLID;
				e->attack = 0;
				e->y_speed = 0;
				
				e->state = 3;	// fall complete
				e->timer = 0;
				
				sound_play(SND_BLOCK_DESTROY, 5);
				//SmokeClouds(o, 4, 2, 2);
				
				//effect(e->CenterX(), e->y + (sprites[e->sprite].block_d[0].y << CSF),
				//	EFFECT_STARSOLID);
			}
			e->y = e->y_next;
		}
		break;
		
		case 3:		// hit ground
		{
			if (++e->timer > 4) {	// make it destroyable
				e->eflags |= NPC_SHOOTABLE;
				e->eflags &= ~NPC_INVINCIBLE;
				e->state = 4;
			}
		}
		break;
	}
}

void ai_counter_bomb(Entity *e) {
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			e->y_mark = e->y;
			
			e->timer = random() % 50;
			e->timer2 = 0;
		}
		case 1:
		{	// desync if multiple enemies
			if (e->timer == 0) {
				e->timer = 0;
				e->state = 2;
				e->y_speed = SPEED(0x300);
			} else {
				e->timer--;
			}
		}
		break;
		
		case 2:		// ready
		{
			if (PLAYER_DIST_X(80 << CSF) || e->damage_time) {
				e->state = 3;
				e->timer = 0;
			}
		}
		break;
		
		case 3:		// counting down...
		{
			if (e->timer == 0) {
				if (e->timer2 < 5) {
					Entity *number = entity_create(e->x + (8 << CSF), e->y + (16 << CSF),
												  OBJ_COUNTER_BOMB_NUMBER, 0);
					number->frame = e->timer2++;
					e->timer = 60;
				} else {
					// expand bounding box to cover explosion area
					//e->x = e->CenterX();
					//e->y = e->CenterY();
					e->hidden = TRUE;
					//e->sprite = SPR_BBOX_PUPPET_1;
					//sprites[e->sprite].bbox.x1 = -128;
					//sprites[e->sprite].bbox.y1 = -100;
					//sprites[e->sprite].bbox.x2 = 128;
					//sprites[e->sprite].bbox.y2 = 100;
					e->attack = 30;
					
					e->y_speed = 0;
					e->state = 4;
					
					// make kaboom
					sound_play(SND_EXPLOSION1, 5);
					camera_shake(20);
					//SmokeXY(e->CenterX(), e->CenterY(), 100, 128, 100);
					
					return;
				}
			} else {
				e->timer--;
			}
		}
		break;
		
		case 4:		// exploding (one frame only to give time for bbox to damage player)
			e->state = STATE_DELETE;
			return;
	}
	
	ANIMATE(e, 4, 0,1,2);
	
	if (e->state == 2 || e->state == 3) {
		e->y_speed += (e->y > e->y_mark) ? -0x10 : 0x10;
		LIMIT_Y(0x100);
	}
}

void ai_counter_bomb_number(Entity *e) {
	switch(e->state) {
		case 0:
		{
			sound_play(SND_COMPUTER_BEEP, 5);
			e->state = 1;
		}
		case 1:
		{
			e->x += (1 << CSF);
			if (++e->timer > 8) {
				e->state = 2;
				e->timer = 0;
			}
		}
		break;
		
		case 2:
		{
			if (++e->timer > TIME(30))
				e->state = STATE_DELETE;
		}
		break;
	}
}
