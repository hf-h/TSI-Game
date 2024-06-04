#ifndef GAME_C
#define GAME_C

#include "Game.h"
#include "GameInput.h"
#include "cutils/UtMem.h"
#include "cutils/UtString.h"

#ifdef _WIN32
#include "Win32Defs.h"
#endif

//TODO: remove debug var
static u64 activeHostiles = 0;

void DebugRenderTextL(RenderTarget *rt, char *text, usize length, u64 startX, u64 startY) {
    for (usize i = 0; i < length; i++) {
        usize buffI = startX + i + startY * rt->w;
        rt->chs[buffI] = text[i];
        rt->attrs[buffI] = FOREGROUND_RED;
    }
}
void DebugRenderText(RenderTarget *rt, char *text, u64 startX, u64 startY) {
    usize i = 0;
    while ('\0' != text[i]) {
        usize buffI = startX + i + startY * rt->w;
        rt->chs[buffI] = text[i];
        rt->attrs[buffI] = FOREGROUND_RED;
        i++;
    }
}

void SpawnProjectileFromTemplate(CBuff *ecb, Projectile *template, i64 x, i64 y, i64 mdX, i64 mdY) {
    Projectile *projectile = ((Projectile *)ecb->data) + ecb->wp;

    *projectile = *template;

    projectile->sprite.x = x;
    projectile->sprite.y = y;
    projectile->mdX = mdX;
    projectile->mdY = mdY;

    projectile->active = 1;
    GTReset(&projectile->moveTX);
    GTReset(&projectile->moveTY);

    ecb->wp = (ecb->wp + 1) % ecb->l;
}

void SpawnEntityFromTemplates(CBuff *ecb, Entity *template, usize movementBehaviourId, usize actionBehaviourId, i64 x, i64 y, i64 mdX, i64 mdY) {
    Entity *ent = ((Entity *)ecb->data) + ecb->wp;

    *ent = *template;

    ent->sprite.x = x;
    ent->sprite.y = y;
    ent->mdX = mdX;
    ent->mdY = mdY;
    ent->active = 1;
    ent->moveBehaviourId = movementBehaviourId;
    ent->actionBehaviourId = actionBehaviourId;
    GTReset(&ent->moveTX);
    GTReset(&ent->moveTY);

    ecb->wp = (ecb->wp + 1) % ecb->l;
}

void SpawnPickupsFromTemplate(CBuff *cb, Pickup *template, i64 x, i64 y, i64 mdX, i64 mdY) {
    Pickup *p = ((Pickup *)cb->data) + cb->wp;

    *p = *template;

    p->active = 1;
    p->sprite.x = x;
    p->sprite.y = y;
    p->mdX = mdX;
    p->mdY = mdY;

    GTReset(&p->moveTX);
    GTReset(&p->moveTY);

    cb->wp = (cb->wp + 1) % cb->l;
}

void RenderSprite(RenderTarget *rt, Sprite *sprt) {
    for (int i = 0; i < sprt->ptrn.w * sprt->ptrn.h; i++) {
        int piX = i % sprt->ptrn.w;
        int piY = i / sprt->ptrn.w;
        int buffI = (sprt->x + sprt->y * rt->w) + piX + (piY * rt->w);

        if (buffI > rt->w * rt->h || buffI < 0) {
            continue;
        }

        rt->chs[buffI] = sprt->ptrn.data[i];
        rt->attrs[buffI] = sprt->attr;
    }
}

void RenderGameData(GameData *gd, RenderTarget *rt) {
    for (usize i = 0; i < gd->hostiles.l; i++) {
        Entity *hostile = ((Entity *)gd->hostiles.data) + i; 
        if (FALSE == hostile->active) {
            continue;
        }

        RenderSprite(rt, &hostile->sprite);
    }

    for (usize i = 0; i < gd->hostileBullets.l; i++) {
        Projectile *projectile = ((Projectile *)gd->hostileBullets.data) + i;
        if (FALSE == projectile->active) {
            continue;
        }

        RenderSprite(rt, &projectile->sprite);
    }

    for (usize i = 0; i < gd->playerBullets.l; i++) {
        Projectile *projectile = ((Projectile *)gd->playerBullets.data) + i;
        if (FALSE == projectile->active) {
            continue;
        }

        RenderSprite(rt, &projectile->sprite);
    }

    for (usize i = 0; i < gd->pickupsActive.l; i++) {
        Pickup *pickup = ((Pickup *)gd->pickupsActive.data) + i;
        if (FALSE == pickup->active) {
            continue;
        }

        RenderSprite(rt, &pickup->sprite);
    }

    RenderSprite(rt, &gd->player->sprite);
}

void RenderBorder(RenderTarget *rt) {
    for (usize i = 0; i < FIELD_WIDTH; i++) {
        rt->chs[i] = 'O';
        rt->attrs[i] = FOREGROUND_BLUE;

        usize lastRow = ((FIELD_HEIGHT -1) * FIELD_WIDTH);
        rt->chs[lastRow + i] = 'O';
        rt->attrs[lastRow + i] = FOREGROUND_BLUE;
    }

    for (usize i = 0; i < FIELD_HEIGHT; i++) {
        rt->chs[i * FIELD_WIDTH] = 'O';
        rt->attrs[i * FIELD_WIDTH] = FOREGROUND_BLUE;

        rt->chs[i * FIELD_WIDTH + FIELD_WIDTH -1] = 'O';
        rt->attrs[i * FIELD_WIDTH + FIELD_WIDTH -1] = FOREGROUND_BLUE;
    }
}

void MakeWeaponTemplate(Weapon *wps, usize tmplId, usize wpId, f64 actionTimerTrigger) {
    wps[tmplId].type = wpId;
    GTInit(&wps[tmplId].actionT, actionTimerTrigger);
}

Projectile MakeProjectileTemplate(i64 dmg, Pattern ptrn, u16 spriteAttr, f64 moveTX, f64 moveTY) {
    Projectile p;
    p.dmg = dmg;
    p.sprite.ptrn = ptrn;
    p.sprite.attr = spriteAttr;
    p.active = FALSE;

    GTInit(&p.moveTX, moveTX);
    GTInit(&p.moveTY, moveTY);

    return p;
}

void MakePattern(GameData *gd, usize patternId, u8 *patternData, usize patternW, usize patternH) {
    usize patternDataPos = PATTERN_CHAR_LEN * patternId;
    Pattern pattern;
    //Jump to just after struct
    pattern.data = gd->patternsData + patternDataPos;
    pattern.h = patternH;
    pattern.w = patternW;
    gd->patterns[patternId] = pattern;
    CopyMem(gd->patternsData + patternDataPos, patternData, patternW * patternH);
}

void EquipWeapon(Entity *e, Weapon wpTmpl) {
    e->equipedWeapons[e->equipedWeaponsCount] = wpTmpl;
    GTReset(&e->equipedWeapons[e->equipedWeaponsCount].actionT);
    e->equipedWeaponsCount += 1;
}

Entity MakeEntityTemplate(Weapon *weaponTmpls
                          , i64 hp
                          , i64 dmg
                          , Pattern ptrn
                          , u16 spriteAttr
                          , u64 moveBehaviourId
                          , u64 templateType
                          , f64 moveTX
                          , f64 moveTY
                          , u64 weaponsC
                          , u64 *weapons
                          ) {
    Entity ent = {0};
    ent.hp = hp;
    ent.dmg = dmg;
    ent.sprite.ptrn = ptrn;
    ent.sprite.attr = spriteAttr;
    ent.templateType = templateType;


    GTInit(&ent.moveTX, moveTX);
    GTInit(&ent.moveTY, moveTY);

    for (usize i = 0; i < weaponsC; i++) {
        EquipWeapon(&ent, weaponTmpls[weapons[i]]);
    }

    return ent;
}

void InitPatterns(GameData *gd) {
    //Sprites
    u8 playerPattern[9] = {
        ' ', 'A', ' ',
        'A', 'A', 'A',
        ' ', 'A', ' '
    };
    u8 test = 'A';
    MakePattern(gd, SPID_PLAYER, &test, 1, 1);

    u8 singleshotPattern[1] = { '|' };
    MakePattern(gd, SPID_SINGLESHOT, singleshotPattern, 1, 1);

    u8 energyBallPattern[1] = {'@'};
    MakePattern(gd, SPID_ENERGY_BALL, energyBallPattern, 1, 1);

    u8 missileUpPattern[1] = {'^'};
    MakePattern(gd, SPID_MISSILE_UP, missileUpPattern, 1, 1);

    u8 enemyOnePattern[6] = {
        '#', '#',
        '%', '#',
        ' ', '#'
    };
    MakePattern(gd, SPID_ENEMY_ONE, enemyOnePattern, 2, 3);

    u8 hostileStriker[6] = {
        'Y', ' ', 'Y',
        ' ', 'Y', ' '
    };
    MakePattern(gd, SPID_HOSTILE_STRIKER, hostileStriker, 3, 2);

    u8 hostileBarge[9] = {
        '<', '_', '>',
        '<', '#', '>',
        '<', '#', '>'
    };
    MakePattern(gd, SPID_HOSTILE_BARGE, hostileBarge, 3, 3);

    u8 hostileDreadnaught[20] = {
        '<', '_', '_', '_', '>',
        '<', '#', '#', '#', '>',
        '<', '#', '#', '#', '>',
        '<', '#', '#', '#', '>'
    };
    MakePattern(gd, SPID_HOSTILE_DREADNAUGHT, hostileDreadnaught, 5, 4);

    u8 pickup[1] = {
        'P'
    };
    MakePattern(gd, SPID_PICKUP, pickup, 1, 1);
}

void MakeWeaponPickupTemplate(GameData *gd, u64 tmplId, u64 wpId) {
    gd->pickupTemplates[tmplId].data.weaponId = wpId;
    gd->pickupTemplates[tmplId].type = PUT_WEAPON;
    gd->pickupTemplates[tmplId].sprite.ptrn = gd->patterns[SPID_PICKUP];
    gd->pickupTemplates[tmplId].sprite.attr = FOREGROUND_BLUE;
    GTInit(&gd->pickupTemplates[tmplId].moveTX, 1.0);
    GTInit(&gd->pickupTemplates[tmplId].moveTY, 1.0);
}

void MakeHealingPickupTemplate(GameData *gd, u64 tmplId, i64 hp) {
    gd->pickupTemplates[tmplId].data.hp = hp;
    gd->pickupTemplates[tmplId].type = PUT_HEALING;
    gd->pickupTemplates[tmplId].sprite.ptrn = gd->patterns[SPID_PICKUP];
    gd->pickupTemplates[tmplId].sprite.attr = FOREGROUND_GREEN;
    GTInit(&gd->pickupTemplates[tmplId].moveTX, 1.0);
    GTInit(&gd->pickupTemplates[tmplId].moveTY, 1.0);
}

void InitPickupTemplates(GameData *gd) {

    MakeWeaponPickupTemplate(gd, PU_LASAER, PL_LASER_TMPL);
    MakeWeaponPickupTemplate(gd, PU_ENERGY_BALL, PL_ENERGY_BALL_TMPL);
    MakeWeaponPickupTemplate(gd, PU_SCATTER_GUN, PL_SCATTER_GUN_TMPL);
    MakeWeaponPickupTemplate(gd, PU_MISSILES, PL_MISSILES_TMPL);

    MakeHealingPickupTemplate(gd, PU_HP_SMALL, 10);
    MakeHealingPickupTemplate(gd, PU_HP_MEDIUM, 10);
    MakeHealingPickupTemplate(gd, PU_HP_LARGE, 10);
}

void InitProjectileTemplates(GameData *gd) {
    Projectile laserTmpl = MakeProjectileTemplate(1, gd->patterns[SPID_SINGLESHOT], FOREGROUND_RED, 0.03, 0.03);
    gd->projectileTemplates[TMPL_SINGLESHOT] = laserTmpl;

    Projectile energyBallTmpl = MakeProjectileTemplate(2, gd->patterns[SPID_ENERGY_BALL], FOREGROUND_GREEN | FOREGROUND_BLUE, 0.1, 0.05);
    gd->projectileTemplates[TMPL_ENERGY_BALL] = energyBallTmpl;

    Projectile missileUpTmpl = MakeProjectileTemplate(3, gd->patterns[SPID_MISSILE_UP], FOREGROUND_RED, 0.06, 0.08);
    gd->projectileTemplates[TMPL_MISSILE_UP] = missileUpTmpl;
}

void InitWeaponTemplates(GameData *gd) {
    MakeWeaponTemplate(gd->weaponTemplates, PL_LASER_TMPL, WP_LAZER, 0.2);
    MakeWeaponTemplate(gd->weaponTemplates, PL_ENERGY_BALL_TMPL, WP_ENERGY_BALL, 0.2);
    MakeWeaponTemplate(gd->weaponTemplates, PL_SCATTER_GUN_TMPL, WP_SCATTER_GUN, 0.25);
    MakeWeaponTemplate(gd->weaponTemplates, PL_MISSILES_TMPL, WP_MISSILES, 0.8);

    MakeWeaponTemplate(gd->weaponTemplates, HOSTILE_ENERGY_BALL_TMPL, WP_ENERGY_BALL, 1.5);
    MakeWeaponTemplate(gd->weaponTemplates, HOSTILE_LASER_TMPL, WP_LAZER, 0.4);
    MakeWeaponTemplate(gd->weaponTemplates, HOSTILE_SCATTER_TMPL, WP_SCATTER_GUN, 1.3);
}

void InitHostilesTemplates(GameData *gd) {
    u64 weapons[MAX_EQUIPED_WEAPONS];

    weapons[0] = HOSTILE_ENERGY_BALL_TMPL;
    Entity enemyMedium = MakeEntityTemplate(gd->weaponTemplates, 5, 5, gd->patterns[SPID_ENEMY_ONE], FOREGROUND_BLUE | FOREGROUND_RED, TMPL_EMENY_MEDIUM, BH_HEAD_ON, 1.0, 1.0, 1, weapons);
    gd->entityTemplates[TMPL_EMENY_MEDIUM] = enemyMedium;

    weapons[0] = HOSTILE_LASER_TMPL;
    Entity hostileStriker = MakeEntityTemplate(gd->weaponTemplates, 3, 1, gd->patterns[SPID_HOSTILE_STRIKER], FOREGROUND_YELLOW | FOREGROUND_BLUE, TMPL_HOSTILE_STRIKER, BH_HEAD_ON, 1.0, 0.3, 1, weapons);
    gd->entityTemplates[TMPL_HOSTILE_STRIKER] = hostileStriker;
    
    weapons[0] = HOSTILE_ENERGY_BALL_TMPL;
    Entity hostileBarge = MakeEntityTemplate(gd->weaponTemplates, 6, 1, gd->patterns[SPID_HOSTILE_BARGE], FOREGROUND_INTENSITY, BH_HEAD_ON, TMPL_HOSTILE_BARGE, 1.0, 1.0, 1, weapons);
    gd->entityTemplates[TMPL_HOSTILE_BARGE] = hostileBarge;

    weapons[0] = HOSTILE_SCATTER_TMPL;
    Entity hostileDreadnaught = MakeEntityTemplate(gd->weaponTemplates, 15, 1, gd->patterns[SPID_HOSTILE_DREADNAUGHT], FOREGROUND_YELLOW, BH_HEAD_ON, TMPL_HOSTILE_DREADNAUGHT, 1.0, 1.0, 1, weapons);
    gd->entityTemplates[TMPL_HOSTILE_DREADNAUGHT] = hostileDreadnaught;
}

void InitPlayer(GameData *gd) {
    GTInit(&gd->player->moveTX, 0.04);
    GTInit(&gd->player->moveTY, 0.04);
    GTReset(&gd->player->moveTX);
    GTReset(&gd->player->moveTY);
    gd->player->sprite.attr = FOREGROUND_GREEN;
    gd->player->sprite.ptrn = gd->patterns[SPID_PLAYER];
    gd->player->sprite.x = FIELD_WIDTH / 2;
    gd->player->sprite.y = FIELD_HEIGHT / 2;
    gd->player->active = 1;
    gd->player->hp = 1;
    EquipWeapon(gd->player, gd->weaponTemplates[PL_LASER_TMPL]);
}

void AddEnemySpawn(GameData *gd, u64 spawnTemplate, u64 moveBehaviour, u64 actionBehaviour, f64 spawnDelay, i64 x, i64 y, i64 mdX, i64 mdY) {
    Spawn *spawn = gd->spawns + gd->spawnsL;
    spawn->x = x;
    spawn->y = y;
    spawn->mdX = mdX;
    spawn->mdY = mdY;
    spawn->specificSpawnData.entityData.movementBehaviour = moveBehaviour;
    spawn->specificSpawnData.entityData.actionBehaviour = actionBehaviour;
    spawn->tmplId = spawnTemplate;
    spawn->spawnType = SPAWN_TYPE_ENEMY;
    GTInit(&spawn->spawnTime, spawnDelay);
    gd->spawnsL += 1;
}

void AddSpawn(GameData *gd, u64 spawnTemplate, u64 spawnType, f64 spawnDelay, i64 x, i64 y, i64 mdX, i64 mdY) {
    Spawn *spawn = gd->spawns + gd->spawnsL;
    spawn->x = x;
    spawn->y = y;
    spawn->mdX = mdX;
    spawn->mdY = mdY;
    spawn->tmplId = spawnTemplate;
    spawn->spawnType = spawnType;
    GTInit(&spawn->spawnTime, spawnDelay);
    gd->spawnsL += 1;
}

void InitGame(GameData *gd, RenderTarget *rt) {
    InitPatterns(gd);
    InitProjectileTemplates(gd);
    InitWeaponTemplates(gd);
    InitPickupTemplates(gd);
    InitHostilesTemplates(gd);
    InitPlayer(gd);

    //SPAWNS

    gd->spawnsL = 0;
    gd->spawnCurrent = 0;

    usize midIsh = rt->w / 2;

    // BROAD WAVE
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, 5, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, midIsh - 3, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, rt->w - 8, 3, 0, 1);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 1.5, midIsh / 2, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, rt->w - (midIsh / 2) -5, 3, 0, 1);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 1.5, 5, 3, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 0.0, midIsh - 3, 3, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 0.0, rt->w - 8, 3, 1, 1);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 1.5, midIsh / 2, 3, -1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 0.0, rt->w - (midIsh / 2) -5, 3, -1, 1);
    //------------------

    //Sidewinders
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 4.0, 3, 10, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.5, 7, 8, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.5, 11, 6, 1, 1);

    AddSpawn(gd, PU_MISSILES, SPAWN_TYPE_PICKUP, 0.0, midIsh, 4, 0, 2);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.5, rt->w - 10, 10, -1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.5, rt->w - 14, 8, -1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.5, rt->w - 18, 6, -1, 1);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 1.0, 5, 3, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 0.0, midIsh - 3, 3, 1, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_ZIG_ZAG, ABH_FIRE_ALL_GUNS, 0.0, rt->w - 8, 3, 1, 1);
    //-----------

    //Barges
    AddEnemySpawn(gd, TMPL_HOSTILE_BARGE, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 1.5, 5, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_BARGE, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, midIsh - 3, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_BARGE, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, rt->w - 8, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_BARGE, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, midIsh / 2, 3, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_BARGE, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 0.0, rt->w - (midIsh / 2) -5, 3, 0, 1);
    //------------

    AddEnemySpawn(gd, TMPL_HOSTILE_DREADNAUGHT, BH_HEAD_ON, ABH_FIRE_ALL_GUNS, 4.0, 25, 4, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_DREADNAUGHT, BH_HEAD_ON, ABH_FIRE_ALL_GUNS, 0.0, 45, 4, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_DREADNAUGHT, BH_HEAD_ON, ABH_FIRE_ALL_GUNS, 1.0, 10, 4, 0, 1);
    AddEnemySpawn(gd, TMPL_HOSTILE_DREADNAUGHT, BH_HEAD_ON, ABH_FIRE_ALL_GUNS, 0.0, 60, 4, 0, 1);

    AddSpawn(gd, PU_SCATTER_GUN, SPAWN_TYPE_PICKUP, 3.0, midIsh - 5, 4, 0, 2);

    AddEnemySpawn(gd, TMPL_HOSTILE_STRIKER, BH_DIAGONAL, ABH_FIRE_ALL_GUNS, 5.0, 0, 0, 0, 1);
    GTReset(&gd->spawns[gd->spawnCurrent].spawnTime);

    gd->score = 0;

    gd->initialized = 1;
}

//TODO: Use pattern to see if unit is partially on field
BOOL IsOnField(Sprite s, u32 sX, u32 sY, u32 fw, u32 fh) {
    return s.y < fh && s.y > sY && s.x > sX && s.x < fw;
}

BOOL CellCollision(Sprite s1, Sprite s2) {
    for (usize k = 0; k < s1.ptrn.w * s1.ptrn.h; k++) {
        usize bCellXPos = s1.x + (k % s1.ptrn.w);
        usize bCellYPos = s1.y + (k / s1.ptrn.h);

        if (' ' == s1.ptrn.data[k]) {
            continue;
        }

        for (usize l = 0; l < s2.ptrn.w * s2.ptrn.h; l++) {
            usize hCellXPos = s2.x + (l % s2.ptrn.w);
            usize hCellYPos = s2.y + (l % s2.ptrn.h);

            if (' ' == s2.ptrn.data[l]) {
                continue;
            }

            if (bCellXPos == hCellXPos && bCellYPos == hCellYPos) {
                return 1;
            }
        }
    }
    
    return FALSE;
}

void OnPickup(GameData *gd, Pickup *pickup) {
    switch (pickup->type) {
        case PUT_HEALING: {
            gd->player->hp += pickup->data.hp;
        } break;
        case PUT_WEAPON: {
            if (MAX_EQUIPED_WEAPONS <= gd->player->equipedWeaponsCount) {
                return;
            }

            gd->player->equipedWeapons[gd->player->equipedWeaponsCount] = gd->weaponTemplates[pickup->data.weaponId];
            GTReset(&gd->player->equipedWeapons[gd->player->equipedWeaponsCount].actionT);
            gd->player->equipedWeaponsCount += 1;
        } break;
        default: {
            //TODO: Make an assert implementation
        }
    }
}

void OnHostileKill(GameData *gd, u64 hostileTemplateType) {
    switch (hostileTemplateType) {
        case TMPL_HOSTILE_DREADNAUGHT: {
            gd->score += 3;
        } break;
        case TMPL_HOSTILE_BARGE: {
            gd->score += 2;
        } break;
        case TMPL_HOSTILE_STRIKER: {
            gd->score += 1;
        } break;
    }
}

//Genereates 2 cols per collision, first for c1, second for c2
void HandleCollision(GameData *gd) {

    for (usize i = 0; i < gd->hostiles.l; i++) {
        Entity *hostile = ((Entity *)gd->hostiles.data) + i;

        if (FALSE == hostile->active) {
            continue;
        }

        //Player
        if (FALSE != CellCollision(gd->player->sprite, hostile->sprite)) {
            gd->player->hp -= hostile->dmg;
        }

        //Player bullets
        for (usize j = 0; j < gd->playerBullets.l; j++) {
            Projectile *playerProjectile = ((Projectile *)gd->playerBullets.data) + j;

            if (FALSE == playerProjectile->active) {
                continue;
            }

            if (1 == CellCollision(hostile->sprite, playerProjectile->sprite)) {
                hostile->hp -= playerProjectile->dmg;
                if (0 >= hostile->hp) {
                    OnHostileKill(gd, hostile->templateType);
                    hostile->active = FALSE;
                }
                playerProjectile->active = FALSE;
            }
        }
    }

    // Hostile projectiles <-> Player
    for (usize i = 0; i < gd->hostileBullets.l; i++) {
        Projectile *hostileProjectile = ((Projectile *)gd->hostileBullets.data) + i;

        if (FALSE == hostileProjectile->active) {
            continue;
        }

        if (FALSE != CellCollision(hostileProjectile->sprite, gd->player->sprite)) {
            gd->player->hp -= hostileProjectile->dmg;
            hostileProjectile->active = FALSE;
        }
    }

    // Pickups <-> Player
    for (usize i = 0; i < gd->pickupsActive.l; i++) {
        Pickup *pickup = ((Pickup *)gd->pickupsActive.data) + i;

        if (FALSE == pickup->active) {
            continue;
        }

        if (FALSE != CellCollision(pickup->sprite, gd->player->sprite)) {
            OnPickup(gd, pickup);
            pickup->active = FALSE;
        }
    }
}

#define OUT_OF_BOUNDS_OFFSET 1
void DisableOutOfBound(GameData *gd, RenderTarget *rt) {
    for (usize i = 0; i < gd->hostiles.l; i++) {
        Entity *ent = ((Entity *)gd->hostiles.data) + i;

        if (FALSE == ent->active) {
            continue;
        }

        if (ent->active) {
            ent->active = IsOnField(ent->sprite
                                   , OUT_OF_BOUNDS_OFFSET
                                   , OUT_OF_BOUNDS_OFFSET
                                   , rt->w -OUT_OF_BOUNDS_OFFSET
                                   , rt->h -OUT_OF_BOUNDS_OFFSET);
        }
    }

    for (usize i = 0; i < gd->hostileBullets.l; i++) {
        Projectile *p = ((Projectile *)gd->hostileBullets.data) + i;

        if (FALSE == p->active) {
            continue;
        }

        if (p->active) {
            p->active = IsOnField(p->sprite
                                   , OUT_OF_BOUNDS_OFFSET
                                   , OUT_OF_BOUNDS_OFFSET
                                   , rt->w -OUT_OF_BOUNDS_OFFSET
                                   , rt->h -OUT_OF_BOUNDS_OFFSET);
        }
    }

    for (usize i = 0; i < gd->playerBullets.l; i++) {
        Projectile *p = ((Projectile *)gd->playerBullets.data) + i;

        if (FALSE == p->active) {
            continue;
        }

        if (p->active) {
            p->active = IsOnField(p->sprite
                                   , OUT_OF_BOUNDS_OFFSET
                                   , OUT_OF_BOUNDS_OFFSET
                                   , rt->w -OUT_OF_BOUNDS_OFFSET
                                   , rt->h -OUT_OF_BOUNDS_OFFSET);
        }
    }

    for (usize i = 0; i < gd->pickupsActive.l; i++) {
        Pickup *pickup = ((Pickup *)gd->pickupsActive.data) + i;

        if (FALSE == pickup->active) {
            continue;
        }

        if (pickup->active) {
            pickup->active = IsOnField(pickup->sprite
                                   , OUT_OF_BOUNDS_OFFSET
                                   , OUT_OF_BOUNDS_OFFSET
                                   , rt->w -OUT_OF_BOUNDS_OFFSET
                                   , rt->h -OUT_OF_BOUNDS_OFFSET);
        }
    }
}
#undef OUT_OF_BOUNDS_OFFSET

void FireScatterGun(CBuff *bullets, Projectile *tmpl, u64 x, u64 y, int mdy) {
    SpawnProjectileFromTemplate(bullets, tmpl, x +1, y + mdy, 1, mdy);
    SpawnProjectileFromTemplate(bullets, tmpl, x -1, y + mdy, -1, mdy);
    SpawnProjectileFromTemplate(bullets, tmpl, x, y + mdy, 0, mdy);
}

void FireWeapon(CBuff *projectiles, Projectile *templates, Weapon *wp, i64 x, i64 y, i64 moveDirY) {
    if (FALSE == GTIsTriggeredReset(&wp->actionT)) {
        return;
    }

    switch(wp->type) {
        case WP_LAZER: {
            SpawnProjectileFromTemplate(projectiles, templates + TMPL_SINGLESHOT, x, y + moveDirY, 0, moveDirY);
        } break;
        case WP_ENERGY_BALL: {
            SpawnProjectileFromTemplate(projectiles, templates + TMPL_ENERGY_BALL, x, y + moveDirY, 0, moveDirY);
        } break;
        case WP_SCATTER_GUN: {
            FireScatterGun(projectiles, templates + TMPL_ENERGY_BALL, x, y, moveDirY);
        } break;
        case WP_MISSILES: {
            SpawnProjectileFromTemplate(projectiles, templates + TMPL_MISSILE_UP, x + 1, y + moveDirY, 0, moveDirY);
            SpawnProjectileFromTemplate(projectiles, templates + TMPL_MISSILE_UP, x - 1, y + moveDirY, 0, moveDirY);
        } break;
    }
}

void PlayerTick(GameData *gd, DigitalInput *keyboard) {
    if (GTIsTriggeredReset(&gd->player->moveTX)) {
        if (keyboard[INP_KEY_A].down) {
            gd->player->sprite.x -= 1;
        }
        if (keyboard[INP_KEY_D].down) {
            gd->player->sprite.x += 1;
        }
    }
    if (GTIsTriggeredReset(&gd->player->moveTY)) {
        if (keyboard[INP_KEY_W].down) {
            gd->player->sprite.y -= 1;
        }
        if (keyboard[INP_KEY_S].down) {
            gd->player->sprite.y += 1;
        }
    }

    if (keyboard[INP_SPACE].down) {
        for (usize i = 0; i < gd->player->equipedWeaponsCount; i++) {
            FireWeapon(&gd->playerBullets, gd->projectileTemplates, &gd->player->equipedWeapons[i], gd->player->sprite.x, gd->player->sprite.y, -1);
        }
    }
}

void AITick(GameData *gd, RenderTarget *rt) {

    for (usize i = 0; i < gd->hostiles.l; i++) {
        Entity *hostile = ((Entity *)gd->hostiles.data) + i;
        if (FALSE == hostile->active) {
            continue;
        }

        activeHostiles++;

        switch(hostile->actionBehaviourId) {
            case ABH_NO_SHOOT: {
            } break;
            case ABH_FIRE_ALL_GUNS: {
                for (usize i = 0; i < hostile->equipedWeaponsCount; i++) {
                    FireWeapon(&gd->hostileBullets, gd->projectileTemplates, &hostile->equipedWeapons[i], hostile->sprite.x, hostile->sprite.y, 1);
                }
            } break;
        }

        switch(hostile->moveBehaviourId) {
            case BH_STATIONARY: {
            } break;
            case BH_HEAD_ON: {
                if (GTIsTriggeredReset(&hostile->moveTY)) {
                    hostile->sprite.y += hostile->mdY;
                }
            } break;
            case BH_DIAGONAL: {
                if (GTIsTriggeredReset(&hostile->moveTX)) {
                    hostile->sprite.x += hostile->mdX;
                }
                if (GTIsTriggeredReset(&hostile->moveTY)) {
                    hostile->sprite.y += hostile->mdY;
                }
            } break;
            case BH_SIDE_TO_SIDE: {
                if (GTIsTriggeredReset(&hostile->moveTX)) {
                    hostile->sprite.x += hostile->mdX;
                }
            } break;
            case BH_ZIG_ZAG: {
                if (hostile->sprite.x >= rt->w -1 || hostile->sprite.x <= 1) {
                    hostile->mdX *= -1;
                }
                if (GTIsTriggeredReset(&hostile->moveTX)) {
                    hostile->sprite.x += hostile->mdX;
                }
                if (GTIsTriggeredReset(&hostile->moveTY)) {
                    hostile->sprite.y += hostile->mdY;
                }
            } break;
        }
    }
}


void GameUpdateAndRender(GameData *gd, DigitalInput *keyboard, RenderTarget *rt, f64 timeElapsed) {
    
    if (FALSE == gd->initialized) {
        InitGame(gd, rt);
    }

    #ifdef TUNE_MODE
        InitProjectileTemplates(gd);
        InitWeaponTemplates(gd);
        InitPickupTemplates(gd);
    #endif

    if (keyboard[INP_ESCAPE].down) {
        gd->running = FALSE;
    }


    GTTickAllTimers(gd, timeElapsed);

    //TODO: clean up spawn logic
    if (gd->spawnCurrent < gd->spawnsL && TS_TRIGGERED == gd->spawns[gd->spawnCurrent].spawnTime.state) {
        Spawn *spawn = gd->spawns + gd->spawnCurrent;
        switch(spawn->spawnType) {
            case SPAWN_TYPE_ENEMY: {
                SpawnEntityFromTemplates(&gd->hostiles, gd->entityTemplates + spawn->tmplId, spawn->specificSpawnData.entityData.movementBehaviour
                                         , spawn->specificSpawnData.entityData.actionBehaviour, spawn->x, spawn->y, spawn->mdX, spawn->mdY);
            } break;
            case SPAWN_TYPE_PICKUP: {
                SpawnPickupsFromTemplate(&gd->pickupsActive, gd->pickupTemplates + spawn->tmplId, spawn->x, spawn->y, spawn->mdX, spawn->mdY);
            } break;
        }

        gd->spawnCurrent += 1;
        if (gd->spawnCurrent < gd->spawnsL) {
            GTReset(&gd->spawns[gd->spawnCurrent].spawnTime);
        }
    }
    else if (gd->spawnCurrent >= gd->spawnsL) {
        gd->spawnCurrent = 0;
    }

    /*------------------
    * Gameplay logic
    * ------------------
    */

    if (gd->player->hp <= 0) {
        gd->running = FALSE;
        return;
    }

    PlayerTick(gd, keyboard);

    activeHostiles = 0;
    AITick(gd, rt);

    //TODO: Remove debug var
    u64 apb = 0;
    for (usize i = 0; i < gd->playerBullets.l; i++) {
        Projectile *bullet = ((Projectile *)gd->playerBullets.data) + i;

        if (FALSE == bullet->active) {
            continue;
        }

        apb += 1;

        if (GTIsTriggeredReset(&bullet->moveTX)) {
            bullet->sprite.x += bullet->mdX;
        }
        if (GTIsTriggeredReset(&bullet->moveTY)) {
            bullet->sprite.y += bullet->mdY;
        }
    }

    for (usize i = 0; i < gd->hostileBullets.l; i++) {
        Projectile *bullet = ((Projectile *)gd->hostileBullets.data) + i;

        if (FALSE == bullet->active) {
            continue;
        }
        
        if (GTIsTriggeredReset(&bullet->moveTX)) {
            bullet->sprite.x += bullet->mdX;
        }
        if (GTIsTriggeredReset(&bullet->moveTY)) {
            bullet->sprite.y += bullet->mdY;
        }
    }

    for (usize i = 0; i < gd->pickupsActive.l; i++) {
        Pickup *pickup = ((Pickup *)gd->pickupsActive.data) + i;

        if (FALSE == pickup->active) {
            continue;
        }

        if (GTIsTriggeredReset(&pickup->moveTX)) {
            pickup->sprite.x += pickup->mdX;
        }
        if (GTIsTriggeredReset(&pickup->moveTY)) {
            pickup->sprite.y += pickup->mdY;
        }
    }

    /*------------------
    * Collision checking
    * ------------------
    */

    DisableOutOfBound(gd, rt);
    HandleCollision(gd);

    /*------------------
    * Rendering
    * ------------------
    */

    //Clear to spaces
    for (int i = 0; i < FIELD_WIDTH * FIELD_HEIGHT; i++) {
        rt->chs[i] = ' ';
        rt->attrs[i] = FOREGROUND_BLUE;
    }

    RenderGameData(gd, rt);
    RenderBorder(rt);

    //TODO: Remove debug displays and setup full score and stats rendering
    char *activePlayerBulletsStr = FormatString(&gd->tempAlloc, "APB: %u64", apb);
    DebugRenderText(rt, activePlayerBulletsStr, 3, 3);

    char *activeHostilesStr = FormatString(&gd->tempAlloc, "AH: %u64", activeHostiles);
    DebugRenderText(rt, activeHostilesStr, 13, 3);

    char *scoreStr = FormatString(&gd->tempAlloc, "Score: %u64", gd->score);
    DebugRenderText(rt, scoreStr, 23, 3);
}

#endif
