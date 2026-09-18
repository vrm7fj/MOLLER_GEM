#ifndef MOLLER_PHASE_CONFIG_H
#define MOLLER_PHASE_CONFIG_H

#include "TCut.h"

// ============================================================
// Constants
// ============================================================

// GEM readout: 6 ADC time samples per strip per trigger
const int NSAMP = 6;

// Trigger phase bins: Tfine_by_APV is in 4 ns units, and Tfine_by_APV % 6
// gives which of the six 4 ns slices within one 24 ns coarse MPD clock
// tick the trigger landed in for that event.
const int NPHASE = 6;

// Generous upper bounds for fixed-size branch buffers.
const int MAXSTRIP = 4000;            // max fired strips/event on one module
const int MAXADC   = MAXSTRIP*NSAMP;  // max flattened ADC-sample entries
const int MAXAPV   = 64;              // max APV25 cards on one module (size of time.Tfine_by_APV)

// Number of GEM modules to read out (same convention as the moller_* ADC/UV
// macros): branches are "<modbase><i>.strip.*" / "<modbase><i>.time.*" for
// i = 0 .. NMOD-1.
const int NMOD = 4;
const char *modbase = "moller.uvagem.m";

// Requires:
//  1) the strip.iAPV decoder patch (ties each strip back to its slot in
//     time.Tfine_by_APV) -- already applied.
//  2) the "block moller.uvagem.*.time.*" lines uncommented in
//     replay_moller_uvagem.odef, so time.Tfine_by_APV is actually in the
//     tree -- done.

// ============================================================
// Input ROOT file(s)
// ============================================================

const char *rootfile = "/path/to/your/replayed/moller_uvagem_replayed_*.root";

// ============================================================
// Optional event-level cut (leave empty TCut("") for no cut)
// ============================================================

TCut globalcut = "";

#endif
