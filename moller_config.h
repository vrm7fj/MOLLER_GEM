#ifndef MOLLER_CONFIG_H
#define MOLLER_CONFIG_H

#include "TCut.h"

// ============================================================
// Constants
// ============================================================

// GEM readout: 6 ADC time samples per strip per trigger
const int NSAMP = 6;

// Trigger phase bins: time.Tfine_by_APV is in 4 ns units, and
// Tfine_by_APV % NPHASE gives which of the six 4 ns slices within one
// 24 ns coarse MPD clock tick the trigger landed in for that event.
const int NPHASE = 6;

// Generous upper bounds for fixed-size branch buffers.
const int MAXSTRIP = 4000;            // max fired strips/event on one module
const int MAXADC   = MAXSTRIP*NSAMP;  // max flattened ADC-sample entries
const int MAXAPV   = 64;              // max APV25 cards on one module (size of time.Tfine_by_APV)

// Number of GEM modules to read out. Branches are named
// "<modbase><i>.strip.*" / "<modbase><i>.time.*" for i = 0 .. NMOD-1,
// e.g. with the default modbase below: moller.uvagem.m0., moller.uvagem.m1.,
// moller.uvagem.m2., moller.uvagem.m3. -- must match the module names in
// moller.uvagem.modules in the DB (db_moller.uvagem.dat).
const int NMOD = 4;
const char *modbase = "moller.uvagem.m";

// strip.ADCsamples and strip.IsU are in the output tree by default. The
// trigger-phase histograms additionally need strip.iAPV (decoder patch,
// already applied) and time.Tfine_by_APV (needs the
// "block moller.uvagem.*.time.*" lines uncommented in
// replay_moller_uvagem.odef) -- both now on.

// ============================================================
// Input ROOT file(s)
// ============================================================

const char *rootfile = "/path/to/your/replayed/moller_uvagem_replayed_*.root";

// ============================================================
// Optional event-level cut (leave empty TCut("") for no cut)
// ============================================================

TCut globalcut = "";

#endif
