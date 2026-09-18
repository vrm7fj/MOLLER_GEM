#ifndef MOLLER_PHASE_HISTOGRAMS_H
#define MOLLER_PHASE_HISTOGRAMS_H

#include "TProfile2D.h"
#include "TH2D.h"
#include "TString.h"

#include <map>

#include "moller_phase_fill_vectors.h"
#include "moller_phase_config.h"

struct PhaseHistograms {

  // outer key = module index (imod), inner key = apv = (mpd<<4 | adc_id).
  // x = time sample (0-5), y = trigger phase bin (0-5, Tfine_by_APV % 6).
  std::map<int, std::map<int, TProfile2D*> > h_pulseshape_vs_phase_U; // z = <ADC>
  std::map<int, std::map<int, TProfile2D*> > h_pulseshape_vs_phase_V;

  // occupancy per (sample, phase) bin -- sanity check that phase is being
  // sampled reasonably uniformly rather than clustered/biased.
  std::map<int, std::map<int, TH2D*> > h_nentries_vs_phase_U;
  std::map<int, std::map<int, TH2D*> > h_nentries_vs_phase_V;

};

PhaseHistograms CreateHistograms() {

  PhaseHistograms hist; // maps start empty; filled lazily in FillHistograms
  return hist;
}

void FillHistograms(const PhaseData &data, PhaseHistograms &hist) {

  for (size_t i = 0; i < data.adc.size(); i++) {

    int imod  = data.imod[i];
    int apv   = data.apv[i];
    int phase = data.phase[i];

    int mpd   = apv >> 4;   // mpd_id can exceed 15, so no mask here
    int adcid = apv & 0xF;  // adc_id is 0-15 (4 bits), matches decoder's effChan encoding

    bool isU = data.isU[i];

    std::map<int, std::map<int, TProfile2D*> > &shapemap = isU ? hist.h_pulseshape_vs_phase_U
                                                                 : hist.h_pulseshape_vs_phase_V;
    std::map<int, std::map<int, TH2D*> >       &entrymap = isU ? hist.h_nentries_vs_phase_U
                                                                 : hist.h_nentries_vs_phase_V;

    std::map<int, TProfile2D*> &shapetarget = shapemap[imod];
    std::map<int, TH2D*>       &entrytarget = entrymap[imod];

    if (shapetarget.find(apv) == shapetarget.end()) {

      const char *axisname = isU ? "U" : "V";

      shapetarget[apv] = new TProfile2D(
        Form("h_pulseshape_vs_phase_mod%d_apv%d_%s", imod, apv, axisname),
        Form("Module %d, APV mpd=%d adc_id=%d (%s);time sample;trigger phase (T_{fine} mod %d);<ADC>",
             imod, mpd, adcid, axisname, NPHASE),
        NSAMP, -0.5, NSAMP-0.5, NPHASE, -0.5, NPHASE-0.5
      );

      entrytarget[apv] = new TH2D(
        Form("h_nentries_vs_phase_mod%d_apv%d_%s", imod, apv, axisname),
        Form("Module %d, APV mpd=%d adc_id=%d (%s);time sample;trigger phase (T_{fine} mod %d);entries",
             imod, mpd, adcid, axisname, NPHASE),
        NSAMP, -0.5, NSAMP-0.5, NPHASE, -0.5, NPHASE-0.5
      );
    }

    shapetarget[apv]->Fill(data.isamp[i], phase, data.adc[i]);
    entrytarget[apv]->Fill(data.isamp[i], phase);
  }
}

#endif
