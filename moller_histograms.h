#ifndef MOLLER_HISTOGRAMS_H
#define MOLLER_HISTOGRAMS_H

#include "TProfile.h"
#include "TString.h"

#include <map>

#include "moller_fill_vectors.h"
#include "moller_config.h"

struct ADCHistograms {

  // outer key = module index (imod), inner key = apv = (mpd<<4 | adc_id);
  // one U profile + one V profile per (module, APV)
  std::map<int, std::map<int, TProfile*> > h_ADC_vs_sample_U;
  std::map<int, std::map<int, TProfile*> > h_ADC_vs_sample_V;

};

ADCHistograms CreateHistograms() {

  ADCHistograms hist; // both maps start empty; filled lazily in FillHistograms
  return hist;
}

void FillHistograms(const ADCData &data, ADCHistograms &hist) {

  for (size_t i = 0; i < data.adc.size(); i++) {

    int imod = data.imod[i];
    int apv  = data.apv[i];

    int mpd   = apv >> 4;   // mpd_id can exceed 15, so no mask here
    int adcid = apv & 0xF;  // adc_id is 0-15 (4 bits), matches decoder's effChan encoding

    std::map<int, std::map<int, TProfile*> > &axismap =
      data.isU[i] ? hist.h_ADC_vs_sample_U : hist.h_ADC_vs_sample_V;

    std::map<int, TProfile*> &target = axismap[imod];

    if (target.find(apv) == target.end()) {

      const char *axisname = data.isU[i] ? "U" : "V";

      target[apv] = new TProfile(
        Form("h_ADC_vs_sample_mod%d_apv%d_%s", imod, apv, axisname),
        Form("Module %d, APV mpd=%d adc_id=%d (%s);time sample;<ADC>", imod, mpd, adcid, axisname),
        NSAMP, -0.5, NSAMP-0.5
      );
    }

    target[apv]->Fill(data.isamp[i], data.adc[i]);
  }
}

#endif
