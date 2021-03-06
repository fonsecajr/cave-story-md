#include "ai.h"

#include <genesis.h>
#include "audio.h"
#include "player.h"
#include "stage.h"
#include "tables.h"
#include "tsc.h"
#include "vdp_ext.h"
#include "effect.h"

#define SUE_BASE				20
#define SUE_PREPARE_ATTACK		30
#define SUE_SOMERSAULT			40
#define SUE_DASH				50
#define SUE_SOMERSAULT_HIT		60

// both sue and misery
#define SIDEKICK_CORE_DEFEATED		99		// core defeated (script-triggered)
#define SIDEKICK_DEFEATED			100		// sidekick defeated
#define SIDEKICK_CORE_DEFEATED_2	110

#define savedhp		id
#define angle		jump_time
#define spawner		underwater

u8 sue_being_hurt;
u8 sue_was_killed;

// Prototypes
//static Entity *fm_spawn_missile(Entity *e, u8 angindex);
static void sue_somersault(Entity *e);
static void sue_dash(Entity *e);
static void set_ignore_solid(Entity *e);
static void sidekick_run_defeated(Entity *e, u16 health);

void ai_misery_frenzied(Entity *e) {
	sidekick_run_defeated(e, 600);
	
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			sue_being_hurt = sue_was_killed = FALSE;
			
			e->savedhp = e->health;
			
			sound_play(SND_TELEPORT, 5);
			e->timer = 1;
		}
		case 1:		// transforming
		{
			e->timer++;
			
			if (e->timer == 2) {	// frenzied
				//e->sprite = SPR_MISERY_FRENZIED;
				e->frame = 9;
				e->x -= 0x1000;
				e->y -= 0x2000;
			}
			
			if (e->timer == 4) {	// normal
				e->timer = 0;
				
				//e->sprite = SPR_MISERY;
				e->frame = 2;
				e->x += 0x1000;
				e->y += 0x2000;
			}
			
			if (++e->timer2 >= 50) {
				e->timer2 = 0;
				e->state = 2;
			}
		}
		break;
		
		case 10:	// hold at "being transformed" frame
		{
			e->state = 11;
			e->frame = 9;
		}
		break;
		
		case 20:	// fight begin / base state
		{
			e->state = 21;
			e->timer = 0;
			e->frame = 0;
			e->animtime = 0;
		}
		case 21:
		{
			e->x_speed *= 7; e->x_speed /= 8;
			e->y_speed *= 7; e->y_speed /= 8;
			
			ANIMATE(e, 20, 0,1);
			
			if (++e->timer > TIME(100)) e->state = 30;
			
			FACE_PLAYER(e);
		}
		break;
		
		case 30:
		{
			e->state = 31;
			e->timer = 0;
			e->frame = 2;
			e->savedhp = e->health;
		}
		case 31:
		{
			ANIMATE(e, 4, 2,3);
			
			if (e->grounded) e->y_speed = -SPEED(0x200);
			
			s32 core_x = bossEntity ? bossEntity->x : 0;
			
			e->x_speed += (e->x > core_x) ? -0x20 : 0x20;
			e->y_speed += (e->y > player.y) ? -0x10 : 0x10;
			LIMIT_X(0x200);
			LIMIT_Y(0x200);
			
			FACE_PLAYER(e);
			
			if (++e->timer > 150) {
				// she attacks with normal critters if you attack either her or Sue.
				if ((e->savedhp - e->health) > 20 || sue_being_hurt) {
					sue_being_hurt = FALSE;
					e->state = 40;
				}
			}
			
			// she attacks periodically with fishy missiles if you killed Sue.
			if (e->timer > 250 && sue_was_killed)
				e->state = 50;
		}
		break;
		
		case 40:	// spawn bats/critters
		{
			e->state = 41;
			e->timer = 0;
			e->x_speed = 0;
			e->y_speed = 0;
			FACE_PLAYER(e);
			sound_play(SND_CHARGE_GUN, 5);
			
			// if you are below the 2nd little platform on the left,
			// she spawns critters, else bats.
			e->spawner = player.y >= block_to_sub(10);
		}
		case 41:
		{
			e->timer++;
			e->frame = (e->timer & 2) ? 4 : 5;
			
			if ((e->timer & 7) == 1) {
				int x, y;
				
				if (e->spawner) {
					x = e->x - 64 + ((random() & 127) << CSF);
					y = e->y - 32 + ((random() & 63) << CSF);
				} else {
					x = e->x - 32 + ((random() & 63) << CSF);
					y = e->y - 64 + ((random() & 127) << CSF);
				}
				
				if (x < block_to_sub(2)) x = block_to_sub(2);
				if (x > block_to_sub(stageWidth - 3)) x = block_to_sub(stageWidth - 3);
				
				if (y < block_to_sub(2)) y = block_to_sub(2);
				if (y > block_to_sub(stageHeight - 3)) y = block_to_sub(stageHeight - 3);
				
				sound_play(SND_EM_FIRE, 5);
				entity_create(x, y, 
						e->spawner ? OBJ_MISERY_CRITTER : OBJ_MISERY_BAT, 0)->hidden = TRUE;
			}
			
			if (e->timer > 50) {
				e->state = 42;
				e->timer = 0;
				FACE_PLAYER(e);
			}
		}
		break;
		
		case 42:
		{
			e->frame = 6;
			
			if (++e->timer > 50) {
				e->y_speed = -0x200;
				MOVE_X(-0x200);
				
				e->state = 30;
			}
		}
		break;
		
		case 50:	// spawn fishy missiles
		{
			e->state = 51;
			e->timer = 0;
			e->x_speed = 0;
			e->y_speed = 0;
			FACE_PLAYER(e);
			sound_play(SND_CHARGE_GUN, 5);
		}
		case 51:
		{
			e->timer++;
			e->frame = (e->timer & 2) ? 4 : 5;
			
			//u8 rate = (playerEquipment & EQUIP_BOOSTER20) ? 10 : 24;
			
			//if ((e->timer % rate) == 1) {
				// pattern: booster=[0,1,3,1,2,0], no-booster=[0,0,0]:
			//	u8 angindex = (e->timer >> 3) & 3;
			//	fm_spawn_missile(e, angindex);
			//}
			
			if (++e->timer > 50) {
				e->state = 42;
				e->timer = 0;
				FACE_PLAYER(e);
			}
		}
		break;
	}
	
	e->x += e->x_speed;
	e->y += e->y_speed;
}

/*
// spawn a fishy missile in the given direction
static Entity *fm_spawn_missile(Entity *e, u8 angindex) {
	static const int ang_table_left[]  = { 0xD8, 0xEC, 0x14, 0x28 };
	static const int ang_table_right[] = { 0x58, 0x6C, 0x94, 0xA8 };

	Entity *shot = entity_create(e->x, e->y, OBJ_MISERY_MISSILE, 0);
	sound_play(SND_EM_FIRE, 3);
	
	if (!e->dir) {
		shot->x += (10 << CSF);
		shot->angle = ang_table_left[angindex];
	} else {
		shot->x -= (10 << CSF);
		shot->angle = ang_table_right[angindex];
	}
	
	return shot;
}
*/
void ai_misery_critter(Entity *e) {
	e->x_next = e->x + e->x_speed;
	e->y_next = e->y + e->y_speed;
	
	if(e->state < 12) {
		if(e->x_speed > 0) collide_stage_rightwall(e);
		if(e->x_speed < 0) collide_stage_leftwall(e);
		if(e->y_speed < 0) collide_stage_ceiling(e);
		else if(!e->grounded) e->grounded = collide_stage_floor(e);
	}
	
	switch(e->state) {
		case 0:
		{
			if (++e->timer > 16) {
				e->frame = 2;
				e->hidden = FALSE;
				FACE_PLAYER(e);
				
				e->state = 10;
				e->attack = 2;
				e->eflags |= NPC_SHOOTABLE;
			}
		}
		break;
		
		case 10:
		{
			if (e->grounded) {
				e->state = 11;
				e->frame = 0;
				e->timer = 0;
				e->x_speed = 0;
				
				FACE_PLAYER(e);
			}
		}
		break;
		
		case 11:
		{
			if (++e->timer > 10) {
				if (++e->timer2 > 4)
					e->state = 12;
				else
					e->state = 10;
				
				sound_play(SND_ENEMY_JUMP, 5);
				e->grounded = FALSE;
				e->y_speed = -SPEED(0x600);
				MOVE_X(SPEED(0x200));
				
				e->frame = 2;
			}
		}
		break;
		
		case 12:
		{
			//e->eflags |= NPC_IGNORESOLID;
			if (e->y > block_to_sub(stageHeight)) {
				e->state = STATE_DELETE;
			}
		}
		break;
	}
	
	e->x = e->x_next;
	e->y = e->y_next;
	if (e->state >= 10) {
		e->y_speed += SPEED(0x40);
		LIMIT_Y(SPEED(0x5FF));
	}
}

void ai_misery_bat(Entity *e) {
	switch(e->state) {
		case 0:
		{
			if (++e->timer > 16) {
				e->frame = 2;
				e->hidden = FALSE;
				FACE_PLAYER(e);
				
				e->state = 1;
				e->attack = 2;
				e->eflags |= (NPC_SHOOTABLE | NPC_IGNORESOLID);
				
				e->y_mark = e->y;
				e->y_speed = 0x400;
			}
		}
		break;
		
		case 1:
		{
			ANIMATE(e, 2, 0, 2);
			
			e->y_speed += (e->y < e->y_mark) ? 0x40 : -0x40;
			ACCEL_X(0x10);
			
			if (e->x < 0 || e->x > block_to_sub(stageWidth) ||
				e->y < 0 || e->y > block_to_sub(stageHeight)) {
				e->state = STATE_DELETE;
			}
		}
		break;
	}
	
	e->x += e->x_speed;
	e->y += e->y_speed;
}
/*
void ai_misery_missile(Entity *e) {
	// cut & pasted from ai_x_fishy_missile
	vector_from_angle(e->angle, 0x400, &e->x_speed, &e->y_speed);
	int desired_angle = GetAngle(e->x, e->y, player.x, player.y);
	
	if (e->angle >= desired_angle)
	{
		if ((e->angle - desired_angle) < 128)
		{
			e->angle--;
		}
		else
		{
			e->angle++;
		}
	}
	else
	{
		if ((e->angle - desired_angle) < 128)
		{
			e->angle++;
		}
		else
		{
			e->angle--;
		}
	}
	
	// smoke trails
	if (++e->timer2 > 2)
	{
		e->timer2 = 0;
		Caret *c = effect(e->ActionPointX(), e->ActionPointY(), EFFECT_SMOKETRAIL_SLOW);
		c->x_speed = -e->x_speed >> 2;
		c->y_speed = -e->y_speed >> 2;
	}
	
	e->frame = (e->angle + 16) / 32;
	if (e->frame > 7) e->frame = 7;
}
*/

void ai_sue_frenzied(Entity *e) {
	sidekick_run_defeated(e, 500);
	
	switch(e->state) {
		case 0:
		{
			e->state = 1;
			sue_being_hurt = sue_was_killed = FALSE;
			
			e->savedhp = e->health;
			
			sound_play(SND_TELEPORT, 5);
			e->timer = 1;
		}
		case 1:		// transforming
		{
			e->timer++;
			
			if (e->timer == 2) {	// frenzied sue
				//e->sprite = SPR_SUE_FRENZIED;
				e->frame = 11;
				e->x -= 0x1000;
				e->y -= 0x1800;
			}
			
			if (e->timer == 4) {	// normal sue
				e->timer = 0;
				
				//e->sprite = SPR_SUE;
				e->frame = 12;
				e->x += 0x1000;
				e->y += 0x1800;
			}
			
			if (++e->timer2 >= 50) {
				entities_clear_by_type(OBJ_RED_CRYSTAL);
				e->timer2 = 0;
				e->state = 2;
			}
		}
		break;
		
		// fight begin/base state (script-triggered)
		case SUE_BASE:
		{
			e->state++;
			e->timer = 0;
			e->frame = 0;
			e->animtime = 0;
			e->attack = 0;
			
			e->eflags |= NPC_SHOOTABLE;
			e->eflags &= ~NPC_IGNORESOLID;
		}
		case SUE_BASE+1:
		{
			ANIMATE(e, 20, 0,1);
			FACE_PLAYER(e);
			
			e->x_speed = (e->x_speed << 1) + (e->x_speed << 2); e->x_speed >>= 3;
			e->y_speed = (e->y_speed << 1) + (e->y_speed << 2); e->y_speed >>= 3;
			
			if ((e->savedhp - e->health) > 50) {
				e->savedhp = e->health;
				sue_being_hurt = TRUE;	// trigger Misery to spawn monsters
			}
			
			if (++e->timer > 80)
				e->state = SUE_PREPARE_ATTACK;
		}
		break;
		
		// prepare to attack
		case SUE_PREPARE_ATTACK:
		{
			e->state++;
			e->timer = 0;
			e->frame = 2;
			
			e->x_speed = 0;
			e->y_speed = 0;
		}
		case SUE_PREPARE_ATTACK+1:
		{
			if (++e->timer > 16) {
				e->state = (e->timer2 ^= 1) ? SUE_SOMERSAULT : SUE_DASH;
				e->timer = 0;
			}
		}
		break;
	}
	
	sue_somersault(e);
	sue_dash(e);
	
	e->x += e->x_speed;
	e->y += e->y_speed;
}

// somersault attack. this is the only time she can actually hurt you.
static void sue_somersault(Entity *e) {
	switch(e->state) {
		case SUE_SOMERSAULT:
		{
			e->state++;
			e->timer = 0;
			e->attack = 4;
			e->frame = 2;
			
			FACE_PLAYER(e);
			THROW_AT_TARGET(e, player.x, player.y, SPEED(0x600));
			set_ignore_solid(e);
		}
		case SUE_SOMERSAULT+1:
		{
			// passes through frame 3 (prepare/dash) before entering anim loop
			ANIMATE(e, 2, 4,5,6,7);
			e->timer++;
			
			if (e->damage_time && e->timer > 20) {	// hurt fall
				e->state = SUE_SOMERSAULT_HIT;
				break;
			}
			
			// hit wall or timeout?
			if (e->timer > 50 || (blk(e->x, -16, e->y, 0) == 0x41) || 
					(blk(e->x, 16, e->y, 0) == 0x41)) {	// back to base state
				e->state = SUE_BASE;
			}
			
			if ((e->timer & 7) == 1)
				sound_play(SND_CRITTER_FLY, 3);
		}
		break;
		
		// hit during somersault
		case SUE_SOMERSAULT_HIT:
		{
			e->state++;
			e->timer = 0;
			e->frame = 2;	// stop somersault; back to normal stand frame
			e->attack = 0;
			e->eflags &= ~NPC_IGNORESOLID;
		}
		case SUE_SOMERSAULT_HIT+1:	// slowing down
		{
			e->x_speed = (e->x_speed << 1) + (e->x_speed << 2); e->x_speed >>= 3;
			e->y_speed = (e->y_speed << 1) + (e->y_speed << 2); e->y_speed >>= 3;
			
			if (++e->timer > 6) {
				e->state++;
				e->timer = 0;
				e->y_speed = -0x200;
				MOVE_X(-0x200);
			}
		}
		break;
		
		// falling/egads
		case SUE_SOMERSAULT_HIT+2:
		{
			e->frame = 9;	// egads!
			
			if (e->grounded) {
				e->state++;
				e->timer = 0;
				e->frame = 2;	// standing
				
				FACE_PLAYER(e);
			}
			
			e->y_speed += 0x20;
			LIMIT_Y(0x5ff);
		}
		break;
		
		// hit ground: slide a bit then recover
		case SUE_SOMERSAULT_HIT+3:
		{
			if (++e->timer > 16)
				e->state = 20;
		}
		break;
	}
}

// non-harmful dash. she cannot be hurt, but cannot hurt you, either.
static void sue_dash(Entity *e) {
	int x;

	switch(e->state) {
		case SUE_DASH:
		{
			e->state++;
			e->timer = 0;
			
			FACE_PLAYER(e);
			e->eflags &= ~NPC_SHOOTABLE;
			
			if (player.x < e->x) x = player.x - (160<<CSF);
							 else x = player.x + (160<<CSF);
			
			THROW_AT_TARGET(e, x, player.y, SPEED(0x600));
			set_ignore_solid(e);
		}
		case SUE_DASH+1:
		{
			// flash
			e->frame = (++e->timer & 2) ? 8 : 3;	// frame 8 is invisible
			
			if (e->timer > 50 || (blk(e->x, -16, e->y, 0) == 0x41) || 
					(blk(e->x, 16, e->y, 0) == 0x41)) {
				e->hidden = FALSE;
				e->state = SUE_BASE;
			}
		}
		break;
	}
}

// sets NPC_IGNORESOLID if the object is heading towards the center
// of the room, clears it otherwise.
static void set_ignore_solid(Entity *e) {
	s32 map_right_half = block_to_sub(stageWidth) >> 1;
	s32 map_bottom_half = block_to_sub(stageHeight) >> 1;
	
	e->eflags &= ~NPC_IGNORESOLID;
	
	if ((e->x < map_right_half && e->x_speed > 0) ||
		(e->x > map_right_half && e->x_speed < 0)) {
		if ((e->y < map_bottom_half && e->y_speed > 0) ||
			(e->y > map_bottom_half && e->y_speed < 0)) {
			e->eflags |= NPC_IGNORESOLID;
		}
	}
}

// shared between both Sue and Misery.
static void sidekick_run_defeated(Entity *e, u16 health) {
	// die if still around when core explodes
	if (e->state == SIDEKICK_CORE_DEFEATED_2) {
		if (!bossEntity) e->health = 0;
	}
	
	// trigger die
	if (e->health < (1000 - health)) {
		e->eflags &= ~NPC_SHOOTABLE;
		e->health = 9999;	// don't re-trigger
		e->state = SIDEKICK_DEFEATED;
	}
	
	switch(e->state) {
		// the script triggers this if you defeat the core
		// without killing one or both sidekicks.
		//
		// once the core explodes and game.stageboss.object becomes NULL,
		// the sidekicks then enter the full defeated state and collapse.
		case SIDEKICK_CORE_DEFEATED:
		{
			if (e->health == 9999)
			{	// we were already dead when core was killed--ignore.
				e->state = SIDEKICK_DEFEATED+1;
			} else {
				e->eflags &= ~NPC_SHOOTABLE;
				e->health = 9999;
				
				e->x_speed = 0;
				e->y_speed = 0;
				e->frame = 9;
				
				e->state = SIDEKICK_CORE_DEFEATED_2; // cannot "state++"; that is SIDEKICK_DEFEATED
			}
		}
		break;
		
		case SIDEKICK_DEFEATED:
		{
			e->state++;
			e->frame = 9;
			e->attack = 0;
			e->eflags &= ~NPC_SHOOTABLE;
			e->eflags |= NPC_IGNORESOLID;
			
			e->y_speed = -0x200;
			e->damage_time += 50;
			
			if (e->type == OBJ_SUE_FRENZIED)
				sue_was_killed = TRUE;	// trigger Misery to start spawning missiles
		}
		case SIDEKICK_DEFEATED+1:
		{
			e->y_speed += 0x20;
			
			#define FLOOR	(((13 * 16) - 13) << CSF)
			if (e->y_speed > 0 && e->y > FLOOR) {
				e->y = FLOOR;
				e->state++;
				e->frame = 10;
				e->x_speed = 0;
				e->y_speed = 0;
			}
		}
		break;
		
		case SIDEKICK_CORE_DEFEATED_2: break;
	}
}
