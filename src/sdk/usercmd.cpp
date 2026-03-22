// Subtick inject/restore — mutable state lives here (single TU)

#include "usercmd.h"

// Static Rep + steps (created once via fnCreateElement, reused forever)
static struct
{
    bool initialized;
    CSubtickMoveStep* step0;
    CSubtickMoveStep* step1;
    struct
    {
        int allocatedSize;
        MEM_PAD(0x04);
        void* elements[2];
    } rep;
    RepeatedPtrField saved;
    CBaseUserCmdPB* activePBase;
} s_state = {};

void Subtick::Inject(CBaseUserCmdPB* pBase, uint64_t button, float flWhenPress, float flWhenRelease)
{
    if (!fnCreateElement)
        return;

    if (!s_state.initialized)
    {
        s_state.step0 = static_cast<CSubtickMoveStep*>(fnCreateElement(nullptr));
        s_state.step1 = static_cast<CSubtickMoveStep*>(fnCreateElement(nullptr));
        if (!s_state.step0 || !s_state.step1)
            return;
        s_state.rep.allocatedSize = 2;
        s_state.rep.elements[0] = s_state.step0;
        s_state.rep.elements[1] = s_state.step1;
        s_state.initialized = true;
    }

    WriteStep(s_state.step0, button, true, flWhenPress);
    WriteStep(s_state.step1, button, false, flWhenRelease);

    // Save original field state
    s_state.saved = pBase->subtickMoves;

    // Inject our static Rep
    pBase->subtickMoves.nCurrentSize = 2;
    pBase->subtickMoves.nTotalSize = 2;
    pBase->subtickMoves.pRep = &s_state.rep;

    s_state.activePBase = pBase;
}

void Subtick::Restore(CBaseUserCmdPB* pBase)
{
    if (!pBase || pBase != s_state.activePBase)
        return;

    pBase->subtickMoves = s_state.saved;
    s_state.activePBase = nullptr;
}
