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

// Which module to look at. Change "m0" to "m1"/"m2"/"m3" etc as needed --
// must match one of the names in moller.uvagem.modules in the DB
// (db_moller.uvagem.dat).
const char *modprefix = "moller.uvagem.m0.";

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
