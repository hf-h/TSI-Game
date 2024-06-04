
#include "Windows.h"
#define _Win32

#include "CoreTypeDefs.h"
#include "cutils/UtMath.c"
#include "cutils/UtAlloc.c"
#include "cutils/UtString.c"

#include "Game.h"

#include "GTimer.c"
#include "Game.c"
#include "GameInput.h"

#include "stdio.h"


LRESULT ProcessMSG(INPUT_RECORD *record, DigitalInput *keyboard) {
    LRESULT res = 1;

    switch (record->EventType) {
        case KEY_EVENT:
            KEY_EVENT_RECORD ke = record->Event.KeyEvent;
            if (FALSE == keyboard[ke.wVirtualKeyCode].down
                && ke.bKeyDown) {

                keyboard[ke.wVirtualKeyCode].pressed = TRUE;
            }

            keyboard[ke.wVirtualKeyCode].down = ke.bKeyDown;
            break;
    }

    return res;
}

void ZeroPressed(DigitalInput *keyboard) {
    for (usize i = 0; i < KEYBOARD_KEY_AMOUNT; i++) {
        keyboard[i].pressed = FALSE;
    }
}


#define PARTITION_MEM(mem, size) mem; mem += size


int main(void) {
    
    HANDLE stdIn = GetStdHandle (STD_INPUT_HANDLE);
    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE cb = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (INVALID_HANDLE_VALUE == stdOut
        || INVALID_HANDLE_VALUE == stdIn
        || INVALID_HANDLE_VALUE == cb) {
        return 1;
    }
    if (FALSE == SetConsoleActiveScreenBuffer(cb)) {
        return 1;
    }

    const i64 targetFps = 120;

    AL sysAlloc = AlMakeWin32Alloc();
    AL scratchBuffer = AlMakeScratchBuffer(&sysAlloc, 40000);

    u8 *cbuffsData = VirtualAlloc(0, (sizeof(Entity) * MAX_HOSTILES)
                                  + (sizeof(Projectile) * MAX_HOSTILE_BULLETS)
                                  + (sizeof(Projectile) * MAX_PLAYER_BULLETS)
                                  + (sizeof(Pickup) * MAX_PICKUPS_ACTIVE)
                                  + (sizeof(Spawn) * MAX_SPAWNS)
                                  , MEM_COMMIT, PAGE_READWRITE);

    u8 *assignPoint = cbuffsData;

    CBuff hostiles = {0};
    hostiles.l = MAX_HOSTILES;
    hostiles.dataSize = sizeof(Entity);
    hostiles.data = PARTITION_MEM(assignPoint, sizeof(Entity) * MAX_HOSTILES);
    ZeroMem(hostiles.data, hostiles.dataSize * MAX_HOSTILES);

    CBuff hostileBullets = {0};
    hostileBullets.l = MAX_HOSTILE_BULLETS;
    hostileBullets.dataSize = sizeof(Projectile);
    hostileBullets.data = PARTITION_MEM(assignPoint, sizeof(Projectile) * MAX_HOSTILE_BULLETS);
    ZeroMem(hostileBullets.data, hostileBullets.dataSize * MAX_HOSTILE_BULLETS);

    CBuff playerBullets = {0};
    playerBullets.l = MAX_PLAYER_BULLETS;
    playerBullets.dataSize = sizeof(Projectile);
    playerBullets.data = PARTITION_MEM(assignPoint, sizeof(Projectile) * MAX_PLAYER_BULLETS);
    ZeroMem(playerBullets.data, playerBullets.dataSize * MAX_PLAYER_BULLETS);

    CBuff pickupsActive = {0};
    pickupsActive.l = MAX_PICKUPS_ACTIVE;
    pickupsActive.dataSize = sizeof(Pickup);
    pickupsActive.data = PARTITION_MEM(assignPoint, sizeof(Pickup) * MAX_PICKUPS_ACTIVE);
    ZeroMem(pickupsActive.data, pickupsActive.dataSize * MAX_PICKUPS_ACTIVE);

    Spawn *spawnData = (Spawn *)PARTITION_MEM(assignPoint, sizeof(Spawn) * MAX_SPAWNS);

    GameData gd = {0};
    gd.running = TRUE;

    gd.hostiles = hostiles;
    gd.hostileBullets = hostileBullets;
    gd.playerBullets = playerBullets;
    gd.pickupsActive = pickupsActive;

    gd.spawns = spawnData;
    gd.spawnsL = 0;
    gd.spawnCurrent = 0;

    gd.sysAlloc = sysAlloc;
    gd.tempAlloc = scratchBuffer;

    u64 staticDataSize = sizeof(Entity)
                             + (sizeof(Sprite) * MAX_SPRITES) 
                             + (PATTERN_CHAR_LEN * MAX_PATTERNS) 
                             + (sizeof(Weapon) * MAX_WEAPONS_TEMPLATES)
                             + (sizeof(Entity) * MAX_ENTITY_TEMPLATES)
                             + (sizeof(Projectile) * MAX_PROJECTILE_TEMPLATES)
                             + (sizeof(Pickup) * MAX_PICKUP_TEMPLATES);

    u8 *staticData = Alloc(&gd.sysAlloc, staticDataSize);
    ZeroMem(staticData, staticDataSize);

    assignPoint = staticData;

    gd.player = (Entity *)PARTITION_MEM(assignPoint, sizeof(Entity));

    gd.entityTemplates = (Entity *)PARTITION_MEM(assignPoint, sizeof(Entity) * MAX_ENTITY_TEMPLATES);
    gd.projectileTemplates = (Projectile *)PARTITION_MEM(assignPoint, sizeof(Projectile) * MAX_PROJECTILE_TEMPLATES);
    gd.weaponTemplates = (Weapon *)PARTITION_MEM(assignPoint, sizeof(Weapon) * MAX_WEAPONS_TEMPLATES);
    gd.pickupTemplates = (Pickup *)PARTITION_MEM(assignPoint, sizeof(Pickup) * MAX_PICKUP_TEMPLATES);
    gd.patterns = (Pattern *)PARTITION_MEM(assignPoint, sizeof(Pattern) * MAX_PATTERNS);
    gd.patternsData = PARTITION_MEM(assignPoint, sizeof(u8) * MAX_PATTERNS * PATTERN_CHAR_LEN);

    //Tick limit
    LARGE_INTEGER perfFreq;
    QueryPerformanceFrequency(&perfFreq);
    LARGE_INTEGER startTS = {0};
    LARGE_INTEGER endTS = {0};
    LARGE_INTEGER elapsedTime = {0};

    //Field setup
    COORD cellsCount;
    COORD topLeftInChiBuff;
    SMALL_RECT extents;
    CHAR_INFO *chiBuff = VirtualAlloc(0, sizeof(CHAR_INFO) * FIELD_WIDTH * FIELD_HEIGHT, MEM_COMMIT, PAGE_READWRITE);

    topLeftInChiBuff.X = 0;
    topLeftInChiBuff.Y = 0;

    cellsCount.X = FIELD_WIDTH;
    cellsCount.Y = FIELD_HEIGHT;

    extents.Top = 0;
    extents.Left = 0;
    extents.Bottom = FIELD_HEIGHT -1;
    extents.Right =  FIELD_WIDTH -1;

    RenderTarget rt = {0};
    rt.chs = VirtualAlloc(0, sizeof(u8) * FIELD_WIDTH * FIELD_HEIGHT, MEM_COMMIT, PAGE_READWRITE);
    rt.attrs = VirtualAlloc(0, sizeof(u16) * FIELD_WIDTH * FIELD_HEIGHT, MEM_COMMIT, PAGE_READWRITE);
    rt.w = FIELD_WIDTH;
    rt.h = FIELD_HEIGHT;

    DigitalInput keyboard[KEYBOARD_KEY_AMOUNT] = {0};

    DWORD eventCount;
    INPUT_RECORD inpBuff[1] = {0};
    DWORD eventsRead;

    u64 frameCount = 0;
    while (gd.running) {

        QueryPerformanceCounter(&startTS);

        ZeroPressed(keyboard);
        GetNumberOfConsoleInputEvents(stdIn, &eventCount);
        while (eventCount > 0) {
            ReadConsoleInputA(stdIn, inpBuff, 1, &eventsRead); 
            ProcessMSG(inpBuff, keyboard);
            GetNumberOfConsoleInputEvents(stdIn, &eventCount);
        }

        f64 timerElapsed = (f128)elapsedTime.QuadPart / 1000000.0;
        GameUpdateAndRender(&gd, keyboard, &rt, timerElapsed);

        for (usize i = 0; i < FIELD_WIDTH * FIELD_HEIGHT; i++) {
            chiBuff[i].Char.AsciiChar = rt.chs[i];
            chiBuff[i].Attributes = rt.attrs[i];
        }

        if (FALSE == WriteConsoleOutputA(cb, chiBuff, cellsCount, topLeftInChiBuff, &extents)) {
            return 1;
        }

        QueryPerformanceCounter(&endTS);
        elapsedTime.QuadPart = endTS.QuadPart - startTS.QuadPart;
        elapsedTime.QuadPart *= 1000000;
        elapsedTime.QuadPart /= perfFreq.QuadPart;
        frameCount = (frameCount + 1) % targetFps;
        i64 tickLen = 1000000 / targetFps;
        i64 sleepTimeMicro = (tickLen - elapsedTime.QuadPart);
        i64 sleepTimeMilli = sleepTimeMicro / 1000;

        if (sleepTimeMilli > 0) {
            elapsedTime.QuadPart += sleepTimeMicro;
            Sleep(sleepTimeMilli);
        }
    }

    if (FALSE == SetConsoleActiveScreenBuffer(stdOut)) {
        return 1;
    }

    return 0;
}
