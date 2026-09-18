#ifndef MOLLER_HISTOGRAMS_H
#define MOLLER_HISTOGRAMS_H

#include "TProfile.h"
#include "TString.h"

#include "moller_fill_vectors.h"
#include "moller_config.h"

struct ADCHistograms {

  TProfile *h_ADC_vs_sample_U; // <ADC> vs time sample, U/X strips
  TProfile *h_ADC_vs_sample_V; // <ADC> vs time sample, V/Y strips

};

ADCHistograms CreateHistograms() {

  ADCHistograms hist;

  hist.h_ADC_vs_sample_U = new TProfile(
    "h_ADC_vs_sample_U",
    "ADC vs time sample;time sample;<ADC>",
    NSAMP, -0.5, NSAMP-0.5
  );

  hist.h_ADC_vs_sample_V = new TProfile(
    "h_ADC_vs_sample_V",
    "ADC vs time sample;time sample;<ADC>",
    NSAMP, -0.5, NSAMP-0.5
  );

  return hist;
}

void FillHistograms(const ADCData &data, ADCHistograms &hist) {

  for (size_t i = 0; i < data.adc.size(); i++) {

    if (data.isU[i]) {
      hist.h_ADC_vs_sample_U->Fill(data.isamp[i], data.adc[i]);
    } else {
      hist.h_ADC_vs_sample_V->Fill(data.isamp[i], data.adc[i]);
    }
  }
}

#endif
