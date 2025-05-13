#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "z64save.h"
#include "z64play.h"
#include "z64cutscene.h"
#include "z64interface.h"
#include "z_file_select.h"

void TriggerMoonCrashCutsceneIfNoHearts(PlayState* play) {
    if ((gSaveContext.save.saveInfo.playerData.healthCapacity == 16) && (gSaveContext.save.saveInfo.playerData.health == 0)) {
        if (play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON) {
            SEQCMD_DISABLE_PLAY_SEQUENCES(false);
        }
    
        gSaveContext.save.saveInfo.playerData.health = 48;
        gSaveContext.save.day = 4;
        gSaveContext.save.eventDayCount = 4;
        gSaveContext.save.time = CLOCK_TIME(6, 0) + 10;
        play->nextEntrance = ENTRANCE(TERMINA_FIELD, 12);
        gSaveContext.nextCutsceneIndex = 0;
        play->transitionTrigger = TRANS_TRIGGER_START;
        play->transitionType = TRANS_TYPE_FADE_WHITE;
    }
}

static bool hasRunThisDeath = false;
static bool hasRunThisRespawn = false;

RECOMP_HOOK("GameOver_Update") void DecreaseMaxHeartsOnDeath(PlayState* play) {
    if (hasRunThisDeath) {
        return;
    }
    hasRunThisDeath = true;

    // Decrement defense hearts if greater than 0
    if (gSaveContext.save.saveInfo.inventory.defenseHearts > 0) {
        gSaveContext.save.saveInfo.inventory.defenseHearts -= 1;
        recomp_printf("Defense hearts decreased to %d\n", gSaveContext.save.saveInfo.inventory.defenseHearts);
    } else if (gSaveContext.save.saveInfo.inventory.defenseHearts == 0) {
        // If no defense hearts remain, decrement total health capacity
        gSaveContext.save.saveInfo.playerData.healthCapacity -= 16;
        recomp_printf("Health capacity decreased to %d\n", gSaveContext.save.saveInfo.playerData.healthCapacity / 16);
    }

    // Ensure defense hearts do not exceed the number of normal hearts
    int maxDefenseHearts = gSaveContext.save.saveInfo.playerData.healthCapacity / 16;
    if (gSaveContext.save.saveInfo.inventory.defenseHearts > maxDefenseHearts) {
        gSaveContext.save.saveInfo.inventory.defenseHearts = maxDefenseHearts;
    }
}

// Reset death flag when player has control so hearts can be decreased again, also play moon crash if losing final heart
// GameOver_Update sucks btw
RECOMP_HOOK("Player_Update") void ResetDeathFlagOnRespawn(Player* this, PlayState* play, FileSelectState* fileSelect2, SramContext* sramCtx) {
    if (play->gameOverCtx.state == GAMEOVER_INACTIVE) {
        if (!hasRunThisRespawn) {
            hasRunThisRespawn = true;
            hasRunThisDeath = false;

            // Check if max health is less than 48 (3 hearts) and adjust current health
            if (gSaveContext.save.saveInfo.playerData.healthCapacity < 48) {
                if (gSaveContext.save.saveInfo.playerData.health != gSaveContext.save.saveInfo.playerData.healthCapacity) {
                    gSaveContext.save.saveInfo.playerData.health = gSaveContext.save.saveInfo.playerData.healthCapacity;
                    recomp_printf("Health adjusted to match max health: %d\n", gSaveContext.save.saveInfo.playerData.health / 16);
                }
            }
        }

        TriggerMoonCrashCutsceneIfNoHearts(play);    
    } else {
        hasRunThisRespawn = false;
    }
}

// This function is called when a heart piece or container is collected; if statement checks for if healthCapacity was just incremented
RECOMP_HOOK("func_8082ECE0") void onGetNewHeart(PlayState* play) {
    if (gSaveContext.save.saveInfo.playerData.healthCapacity % 0x10 == 0) {
        if (gSaveContext.save.saveInfo.inventory.defenseHearts > 0) {
            gSaveContext.save.saveInfo.inventory.defenseHearts += 1;
        }
    }
}