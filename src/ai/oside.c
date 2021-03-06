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
#include "resources.h"
#include "npc.h"

#define NS_WAIT					1
#define NS_SEEK_PLAYER			2
#define NS_PREPARE_FIRE			3
#define NS_FIRING				4
#define NS_RETURN_TO_SET_POINT	5
#define NS_GUARD_SET_POINT		6
void ai_night_spirit(Entity *e) {
	switch(e->state) {
		case 0:
		{
			e->state = NS_WAIT;
			e->hidden = TRUE;
			e->y_mark = e->y + (12 << CSF);
		}
		case NS_WAIT:
		{
			if (PLAYER_DIST_Y(8 << CSF)) {
				static const s32 distance = (240 << CSF);
				e->y += e->dir ? distance : -distance;
				
				e->state = NS_SEEK_PLAYER;
				e->timer = 0;
				e->hidden = FALSE;
				
				e->y_speed = 0;
				e->nflags |= NPC_SHOOTABLE;
			}
		}
		break;
		
		case NS_SEEK_PLAYER:
		{
			ANIMATE(e, 4, 0,1,2);
			
			if (++e->timer > TIME(200)) {
				e->state = NS_PREPARE_FIRE;
				e->timer = 0;
				e->frame += 3;
			}
		}
		break;
		
		case NS_PREPARE_FIRE:
		{
			ANIMATE(e, 4, 3,4,5);
			if (++e->timer > TIME(50)) {
				e->state = NS_FIRING;
				e->timer = 0;
				e->frame += 3;
			}
		}
		break;
		
		case NS_FIRING:
		{
			ANIMATE(e, 4, 6,7,8);
			
			if (!(e->timer % 8)) {
				Entity *shot = entity_create(e->x, e->y, OBJ_NIGHT_SPIRIT_SHOT, 0);
				shot->x_speed = SPEED(0x100) + (random() % SPEED(0x500));
				shot->y_speed = -SPEED(0x200) + (random() % SPEED(0x400));
				
				sound_play(SND_BUBBLE, 3);
			}
			
			if (++e->timer > TIME(50)) {
				e->state = NS_SEEK_PLAYER;
				e->timer = 0;
				e->frame -= 6;
			}
		}
		break;
		
		case NS_RETURN_TO_SET_POINT:
		{
			ANIMATE(e, 4, 3,4,5);
			
			// lie in wait at original set point
			e->y_speed += (e->y > e->y_mark) ? -SPEED(0x40) : SPEED(0x40);
			LIMIT_Y(SPEED(0x400));
			
			if (abs(e->y - e->y_mark) < (SCREEN_HEIGHT/2)<<CSF) {
				e->state = NS_GUARD_SET_POINT;
			}
		}
		break;
		
		case NS_GUARD_SET_POINT:
		{
			ANIMATE(e, 4, 3,4,5);
			
			// lie in wait at original set point
			e->y_speed += (e->y > e->y_mark) ? -SPEED(0x40) : SPEED(0x40);
			LIMIT_Y(SPEED(0x400));
			
			// and if player appears again...
			if (PLAYER_DIST_Y(SCREEN_HEIGHT << CSF)) {	
				// ..jump out and fire immediately
				e->state = NS_PREPARE_FIRE;
				e->timer = 0;
			}
		}
		break;
	}
	
	if (e->state >= NS_SEEK_PLAYER && e->state < NS_GUARD_SET_POINT) {
		// sinusoidal player seek
		e->y_speed += (e->y < player.y) ? SPEED(0x19) : -SPEED(0x19);
		
		// rarely seen, but they do bounce off walls
		e->x_next = e->x;
		e->y_next = e->y + e->y_speed;
		if (collide_stage_ceiling(e)) e->y_speed = SPEED(0x200);
		if (collide_stage_floor(e)) e->y_speed = -SPEED(0x200);
		e->y = e->y_next;
		
		// avoid leaving designated area
		if (abs(e->y - e->y_mark) > SCREEN_HEIGHT<<CSF) {
			if (e->state != NS_FIRING) {
				e->state = NS_RETURN_TO_SET_POINT;
			}
		}
	}
	LIMIT_Y(SPEED(0x400));
}

void ai_night_spirit_shot(Entity *e) {
	ANIMATE(e, 4, 0,1,2);
	e->x_speed -= SPEED(0x19);
	e->x_next = e->x + e->x_speed;
	e->y_next = e->y + e->y_speed;
	if (e->x_speed < 0 && collide_stage_leftwall(e)) {
		effect_create_smoke(e->x >> CSF, e->y >> CSF);
		sound_play(SND_SHOT_HIT, 3);
		e->state = STATE_DELETE;
	}
	e->x = e->x_next;
	e->y = e->y_next;
}

// Hoppy jumps off the left walls, physics needs to be handled a bit weird here
void ai_hoppy(Entity *e) {
	e->x_next = e->x + e->x_speed;
	e->y_next = e->y + e->y_speed;
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			e->enableSlopes = FALSE;
			e->grounded = TRUE;
		}
		case 1:		// wait for player...
		{
			e->frame = 0;
			if (PLAYER_DIST_Y(0x10000)) {
				e->state = 2;
				e->timer = 0;
				e->frame = 1;
			}
		}
		break;
		
		case 2:	// jump
		{
			e->timer++;
			
			if (e->timer == 4)
				e->frame = 2;
			
			if (e->timer > 12) {
				e->state = 3;
				e->frame = 3;
				
				sound_play(SND_HOPPY_JUMP, 5);
				e->x_speed = SPEED(0x700);
				e->grounded = FALSE;
			}
		}
		break;
		
		case 3:	// in air...
		{
			if (e->y < player.y)	  e->y_speed = SPEED(0xAA);
			else if (e->y > player.y) e->y_speed = -SPEED(0xAA);
			
			// Top/bottom
			if (e->x_speed > 0) {
				collide_stage_rightwall(e);
			} else if (e->x_speed < 0) {
				if((e->grounded = collide_stage_leftwall(e))) {
					e->y_speed = 0;
					
					e->state = 4;
					e->frame = 2;
					e->timer = 0;
				}
			}
			
			// Sides
			if(e->y_speed < 0) {
				collide_stage_ceiling(e);
			} else if(e->y_speed > 0) {
				collide_stage_floor(e);
			}
		}
		break;
		
		case 4:
		{
			e->timer++;
			if (e->timer == 2) e->frame = 1;
			if (e->timer == 6) e->frame = 0;
			
			if (e->timer > 16)
				e->state = 1;
		}
		break;
	}
	e->x = e->x_next;
	e->y = e->y_next;
	
	if(!e->grounded) e->x_speed -= SPEED(0x2A);
	LIMIT_X(SPEED(0x5ff));
}

void ai_sky_dragon(Entity *e) {
	switch(e->state) {
		case 0:		// standing
		{
			ANIMATE(e, 30, 0,1);
			e->dir = 1; // Dragon always faces right
		}
		break;
		
		case 10:	// player and kazuma gets on, dragon floats up
		{
			e->state = 11;
			// There is a separate 2 frames for the player wearing the Mimiga Mask
			if(player_has_item(0x10))
				e->frame = 4;
			else
				e->frame = 2;
			e->animtime = 0;
			
			e->x_mark = e->x - (6 << CSF);
			e->y_mark = e->y - (16 << CSF);
			
			e->y_speed = 0;
		}
		case 11:
		{
			if(++e->animtime > 8) {
				e->frame ^= 1; // swap between 2-3 or 4-5 for mimiga mask
				e->animtime = 0;
			}
			e->x_speed += (e->x < e->x_mark) ? 0x08 : -0x08;
			e->y_speed += (e->y < e->y_mark) ? 0x08 : -0x08;
		}
		break;
		
		case 20:	// fly away
		{
			if(++e->animtime > 8) {
				e->frame ^= 1;
				e->animtime = 0;
			}
			e->y_speed += (e->y < e->y_mark) ? 0x08 : -0x08;
			e->x_speed += 0x20;
			LIMIT_X(SPEED(0x600));
		}
		break;
		
		case 30:	// spawn a Sue hanging from mouth
		{
		}
		break;
	}
	e->x += e->x_speed;
	e->y += e->y_speed;
}

void ai_pixel_cat(Entity *e) {
	if (e->state == 0) {
		e->y -= (32 << CSF);
		e->state = 1;
	}
}

void ai_little_family(Entity *e) {
	e->frame &= 1;
	
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			e->frame = 0;
			e->x_speed = 0;
		}
		case 1:
		{
			if (!(random() % 60)) {
				e->state = (random() & 1) ? 2 : 10;
				e->timer = 0;
				e->frame = 1;
			}
		}
		break;
		
		case 2:
		{
			if (++e->timer > 8) {
				e->state = 1;
				e->frame = 0;
			}
		}
		break;
		
		case 10:
		{
			e->state = 11;
			e->frame = 0;
			e->animtime = 0;
			e->dir = random() & 1;
			e->timer = 16 + (random() % 16);
		}
		case 11:
		{
			if ((!e->dir && collide_stage_leftwall(e)) ||
				(e->dir && collide_stage_rightwall(e))) {
				e->dir ^= 1;
			}
			
			MOVE_X(SPEED(0x100));
			ANIMATE(e, 4, 0,1);
			
			if (e->timer == 0) e->state = 0;
			else e->timer--;
		}
		break;
	}
	
	switch(e->event) {
		case 210: e->frame += 2; break;		// red mom
		case 220: e->frame += 4; break;		// little son
	}
	
	e->y_speed += SPEED(0x20);
	LIMIT_Y(SPEED(0x5ff));
}
