#include "Gtimer.h"
#include "Game.h"

void GTInit(GTimer *gt, f64 trigger) {
    gt->state = TS_STOPED;
    gt->acm = 0.0;
    gt->trigger = trigger;
}
void GTReset(GTimer *gt) {
    gt->state = TS_RUNNING;
    gt->acm = 0;
}
BOOL GTIsTriggeredReset(GTimer *gt) {
    BOOL triggered = TS_TRIGGERED == gt->state;
    if (triggered) {
        GTReset(gt);
    }

    return triggered;
}
void GTTick(GTimer *gt, f64 tickL) {
    if (TS_RUNNING != gt->state) {
        return;
    }

    gt->acm += tickL;
    
    if (gt->acm >= gt->trigger) {
        gt->state = TS_TRIGGERED;
    }
}
void GTTickAllTimers(GameData *gd, f64 tickL) {
    GTTick(&gd->spawns[gd->spawnCurrent].spawnTime, tickL);

    GTTick(&gd->player->moveTX, tickL);
    GTTick(&gd->player->moveTY, tickL);

    for (usize i = 0; i < gd->player->equipedWeaponsCount; i++) {
        GTTick(&gd->player->equipedWeapons[i].actionT, tickL);
    }

    for (usize i = 0; i < gd->hostiles.l; i++) {
        Entity *e = ((Entity *)gd->hostiles.data) + i;
        if (FALSE == e->active) {
            continue;
        }

        GTTick(&e->moveTX, tickL);
        GTTick(&e->moveTY, tickL);

        for (usize i = 0; i < e->equipedWeaponsCount; i++) {
            GTTick(&e->equipedWeapons[i].actionT, tickL);
        }
    }

    for (usize i = 0; i < gd->hostileBullets.l; i++) {
        Projectile *p = ((Projectile *)gd->hostileBullets.data) + i;
        if (FALSE == p->active) {
            continue;
        }

        GTTick(&p->moveTX, tickL);
        GTTick(&p->moveTY, tickL);
    }
    for (usize i = 0; i < gd->playerBullets.l; i++) {
        Projectile *p = ((Projectile *)gd->playerBullets.data) + i;
        if (FALSE == p->active) {
            continue;
        }

        GTTick(&p->moveTX, tickL);
        GTTick(&p->moveTY, tickL);
    }

    for (usize i = 0; i < gd->pickupsActive.l; i++) {
        Pickup *p = ((Pickup *)gd->pickupsActive.data) + i;
        if (FALSE == p->active) {
            continue;
        }

        GTTick(&p->moveTX, tickL);
        GTTick(&p->moveTY, tickL);
    }
}
