#ifndef G_TIMER
#define G_TIMER

#define TS_STOPED       0
#define TS_PAUSED       1
#define TS_RUNNING      2
#define TS_TRIGGERED    3

#include "cutils/UtTypes.h"

struct Entity;
struct GameData;

typedef struct GTimer {
    f64 trigger;
    f64 acm;
    u64 state;
} GTimer;

void GTInit(GTimer *gt, f64 trigger);
void GTReset(GTimer *gt);
BOOL GTIsTriggeredReset(GTimer *gt);
void GTTick(GTimer *gt, f64 tickL);
void GTTickAllTimers(struct GameData *gd, f64 tickL);

#endif
