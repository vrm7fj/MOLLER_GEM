#ifndef MOLLER_HISTOGRAMS_H
#define MOLLER_HISTOGRAMS_H

#include "TProfile.h"
#include "TProfile2D.h"
#include "TString.h"

#include <map>

#include "moller_fill_vectors.h"
#include "moller_config.h"

struct ADCHistograms {

  // outer key = module index (imod), inner key = apv = (mpd<<4 | adc_id).

  // <ADC> vs. time sample (0-5): one U profile + one V profile per (module, APV)
  std::map<int, std::map<int, TProfile*> > h_ADC_vs_sample_U;
  std::map<int, std::map<int, TProfile*> > h_ADC_vs_sample_V;

  // <ADC> vs. (time sample, trigger phase): only filled for entries with a
  // valid strip.iAPV/time.Tfine_by_APV match (data.phase[i] >= 0)
  std::map<int, std::map<int, TProfile2D*> > h_pulseshape_vs_phase_U;
  std::map<int, std::map<int, TProfile2D*> > h_pulseshape_vs_phase_V;

};

ADCHistograms CreateHistograms() {

  ADCHistograms hist; // all maps start empty; filled lazily in FillHistograms
  return hist;
}

void FillHistograms(const ADCData &data, ADCHistograms &hist) {

  for (size_t i = 0; i < data.adc.size(); i++) {

    int imod = data.imod[i];
    int apv  = data.apv[i];

    int mpd   = apv >> 4;   // mpd_id can exceed 15, so no mask here
    int adcid = apv & 0xF;  // adc_id is 0-15 (4 bits), matches decoder's effChan encoding

    bool isU = data.isU[i];
    const char *axisname = isU ? "U" : "V";

    // ---- ADC vs sample (always filled) ----

    std::map<int, std::map<int, TProfile*> > &shapemap = isU ? hist.h_ADC_vs_sample_U
                                                               : hist.h_ADC_vs_sample_V;
    std::map<int, TProfile*> &shapetarget = shapemap[imod];

    if (shapetarget.find(apv) == shapetarget.end()) {
      shapetarget[apv] = new TProfile(
        Form("h_ADC_vs_sample_mod%d_apv%d_%s", imod, apv, axisname),
        Form("Module %d, APV mpd=%d adc_id=%d (%s);time sample;<ADC>", imod, mpd, adcid, axisname),
        NSAMP, -0.5, NSAMP-0.5
      );
    }
    shapetarget[apv]->Fill(data.isamp[i], data.adc[i]);

    // ---- ADC vs (sample, trigger phase) -- only when phase is valid ----

    int phase = data.phase[i];
    if (phase < 0) continue;

    std::map<int, std::map<int, TProfile2D*> > &phasemap = isU ? hist.h_pulseshape_vs_phase_U
                                                                 : hist.h_pulseshape_vs_phase_V;
    std::map<int, TProfile2D*> &phasetarget = phasemap[imod];

    if (phasetarget.find(apv) == phasetarget.end()) {
      phasetarget[apv] = new TProfile2D(
        Form("h_pulseshape_vs_phase_mod%d_apv%d_%s", imod, apv, axisname),
        Form("Module %d, APV mpd=%d adc_id=%d (%s);time sample;trigger phase (T_{fine} mod %d);<ADC>",
             imod, mpd, adcid, axisname, NPHASE),
        NSAMP, -0.5, NSAMP-0.5, NPHASE, -0.5, NPHASE-0.5
      );
    }
    phasetarget[apv]->Fill(data.isamp[i], phase, data.adc[i]);
  }
}

#endif
