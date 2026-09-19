#ifndef MOLLER_CONFIG_H
#define MOLLER_CONFIG_H

#include "TCut.h"

const int NSAMP = 6;        // ADC time samples per strip per trigger
const int MAXSTRIP = 4000;  // max fired strips/event on one module
const int MAXADC = MAXSTRIP*NSAMP;

// Branches are "<modbase><i>.strip.*" for i = 0 .. NMOD-1, matching
// moller.uvagem.modules in db_moller.uvagem.dat.
const int NMOD = 4;
const char *modbase = "moller.uvagem.m";

const char *rootfile = "/path/to/your/replayed/moller_uvagem_replayed_*.root";

TCut globalcut = ""; // leave empty for no cut

#endif
