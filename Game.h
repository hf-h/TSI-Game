#ifndef GAME_H
#define GAME_H

#include "cutils/UtAlloc.h"
#include "GameInput.h"
#include "GTimer.h"
#include "CoreTypeDefs.h"

#define FIELD_WIDTH  80
#define FIELD_HEIGHT 40

#define KEYBOARD_KEY_AMOUNT 256

#define MAX_SPRITES 20
#define MAX_HOSTILES  1000
#define MAX_HOSTILE_BULLETS 1000
#define MAX_PLAYER_BULLETS 100
#define MAX_PICKUPS_ACTIVE 100

#define MAX_SPAWNS 100
#define SPAWN_TYPE_ENEMY 0
#define SPAWN_TYPE_PICKUP 1

#define MAX_PATTERNS 20
#define PATTERN_CHAR_LEN 20
#define SPID_PLAYER                     0
#define SPID_SINGLESHOT                 1
#define SPID_ENEMY_ONE                  2
#define SPID_ENERGY_BALL                3
#define SPID_MISSILE_UP                 4
#define SPID_HOSTILE_STRIKER            5
#define SPID_HOSTILE_BARGE              6
#define SPID_HOSTILE_DREADNAUGHT        7
#define SPID_PICKUP                     8

#define MAX_ENTITY_TEMPLATES 10
#define TMPL_EMENY_MEDIUM       0
#define TMPL_HOSTILE_STRIKER    1
#define TMPL_HOSTILE_BARGE      2
#define TMPL_HOSTILE_DREADNAUGHT 3

#define MAX_PROJECTILE_TEMPLATES 10
#define TMPL_SINGLESHOT         0
#define TMPL_ENERGY_BALL        1
#define TMPL_MISSILE_UP         2

//Pickup types
#define PUT_WEAPON      0
#define PUT_HEALING     1

#define MAX_PICKUP_TEMPLATES     10
#define PU_LASAER       0
#define PU_ENERGY_BALL  1
#define PU_SCATTER_GUN  2
#define PU_MISSILES     3
#define PU_HP_SMALL     4
#define PU_HP_MEDIUM    5
#define PU_HP_LARGE     6

//AI behaviours
#define BH_STATIONARY   0
#define BH_HEAD_ON      1
#define BH_DIAGONAL     2
#define BH_ZIG_ZAG      3
#define BH_SIDE_TO_SIDE 4

#define ABH_NO_SHOOT 0
#define ABH_FIRE_ALL_GUNS 1

#define MAX_WEAPONS_TEMPLATES   20
#define PL_LASER_TMPL                   0
#define PL_ENERGY_BALL_TMPL             1
#define PL_SCATTER_GUN_TMPL             2
#define PL_MISSILES_TMPL                3
#define HOSTILE_ENERGY_BALL_TMPL        4
#define HOSTILE_LASER_TMPL              5
#define HOSTILE_SCATTER_TMPL            6

//Weapon types
#define WP_LAZER        1
#define WP_ENERGY_BALL  2
#define WP_SCATTER_GUN  3
#define WP_MISSILES     4

#define MAX_EQUIPED_WEAPONS 4


typedef struct Pattern {
    u8 *data;
    u32 w, h;
} Pattern;

typedef struct Sprite {
    Pattern ptrn;
    u16 attr;
    i64 x, y;
} Sprite;

typedef struct Weapon {
    GTimer actionT;
    usize type;
} Weapon;

typedef struct Pickup {
    union {
        usize weaponId;
        int hp;
    } data;
    usize type;

    BOOL active;

    Sprite sprite;
    i64 mdX, mdY;
    GTimer moveTX, moveTY;
} Pickup;

typedef struct Projectile {
    BOOL active;

    Sprite sprite;

    i64 dmg;
    i64 mdX, mdY;
    GTimer moveTX, moveTY;
} Projectile;

typedef struct Entity {
    BOOL active;

    Sprite sprite;

    usize templateType;
    usize moveBehaviourId;
    usize actionBehaviourId;

    Weapon equipedWeapons[MAX_EQUIPED_WEAPONS];
    usize equipedWeaponsCount;

    i64 mdX, mdY;
    GTimer moveTX, moveTY;

    u64 behaviour;

    i64 hp, dmg;
} Entity;

typedef struct EntitySpawn {
    u64 movementBehaviour;
    u64 actionBehaviour;
} EntitySpawn;

typedef struct Spawn {
    union {
        EntitySpawn entityData;
    } specificSpawnData;

    usize tmplId;
    usize spawnType;
    GTimer spawnTime;
    i64 x, y;
    i64 mdX, mdY;
} Spawn;

typedef struct CBuff {
    usize wp;
    usize l;
    usize dataSize;
    void *data;
} CBuff;

typedef struct RenderTarget {
    u8 *chs;
    u16 *attrs;
    usize w;
    usize h;
} RenderTarget;

typedef struct GameData {
    BOOL running;
    BOOL initialized;

    AL sysAlloc;
    AL tempAlloc;

    u64 score;

    CBuff playerBullets;
    CBuff hostileBullets;
    CBuff hostiles;
    CBuff pickupsActive;

    Entity *player;

    Spawn *spawns;
    usize spawnCurrent;
    usize spawnsL;

    Entity *entityTemplates;
    Projectile *projectileTemplates;
    Weapon *weaponTemplates;
    Pickup *pickupTemplates;
    Pattern *patterns;
    u8 *patternsData;
} GameData;

void GameUpdateAndRender(GameData *gd, DigitalInput *keyboard, RenderTarget *rt, f64 timeElapsed);

#endif
