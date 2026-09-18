#ifndef MOLLER_CONFIG_H
#define MOLLER_CONFIG_H

#include "TCut.h"

// ============================================================
// Constants
// ============================================================

// GEM readout: 6 ADC time samples per strip per trigger
const int NSAMP = 6;

// Generous upper bounds for fixed-size branch buffers.
const int MAXSTRIP = 4000;            // max fired strips/event on one module
const int MAXADC   = MAXSTRIP*NSAMP;  // max flattened ADC-sample entries

// Number of GEM modules to read out. Branches are named
// "<modbase><i>.strip.*" for i = 0 .. NMOD-1, e.g. with the default
// modbase below: moller.uvagem.m0., moller.uvagem.m1., moller.uvagem.m2.,
// moller.uvagem.m3. -- must match the module names in
// moller.uvagem.modules in the DB (db_moller.uvagem.dat).
const int NMOD = 4;
const char *modbase = "moller.uvagem.m";

// No decoder changes or odef edits needed for this one -- strip.ADCsamples
// and strip.IsU are both already in the output tree.

// ============================================================
// Input ROOT file(s)
// ============================================================

const char *rootfile = "/path/to/your/replayed/moller_uvagem_replayed_*.root";

// ============================================================
// Optional event-level cut (leave empty TCut("") for no cut)
// ============================================================

TCut globalcut = "";

#endif
