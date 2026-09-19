#include "TChain.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TColor.h"

#include <set>
#include <vector>
#include <algorithm>

#include "moller_config.h"
#include "moller_fill_vectors.h"
#include "moller_histograms.h"

void moller_plot() {

  gStyle->SetOptStat(0);
  gStyle->SetTitleFontSize(0.09);

  TChain *C = new TChain("T");
  C->Add(rootfile);

  ADCData data;
  FillVectors(C, data);

  ADCHistograms hist = CreateHistograms();
  FillHistograms(data, hist);

  std::set<int> mods;
  for (auto &kv : hist.h_ADC_vs_sample_U) mods.insert(kv.first);
  for (auto &kv : hist.h_ADC_vs_sample_V) mods.insert(kv.first);

  std::cout << "Number of distinct modules found: " << mods.size() << std::endl;

  TFile *fout = new TFile("moller_adc_uv_output.root", "RECREATE");
  for (auto &modkv : hist.h_ADC_vs_sample_U) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_ADC_vs_sample_V) for (auto &kv : modkv.second) kv.second->Write();
  fout->Close();

  // One page per (module, MPD): top row = U APVs, bottom row = V APVs,
  // sorted by adc_id. Grid widens if a module/MPD has more than 5 on
  // one axis.
  TCanvas *c1 = new TCanvas("c1", "ADC vs time sample by module/MPD", 1300, 500);
  c1->Print("moller_adc_uv_output.pdf[");

  for (int imod : mods) {

    std::set<int> mpds;
    if (hist.h_ADC_vs_sample_U.count(imod))
      for (auto &kv : hist.h_ADC_vs_sample_U[imod]) mpds.insert(kv.first >> 4);
    if (hist.h_ADC_vs_sample_V.count(imod))
      for (auto &kv : hist.h_ADC_vs_sample_V[imod]) mpds.insert(kv.first >> 4);

    for (int mpd : mpds) {

      std::vector<int> uApvs, vApvs;
      if (hist.h_ADC_vs_sample_U.count(imod))
        for (auto &kv : hist.h_ADC_vs_sample_U[imod])
          if ((kv.first >> 4) == mpd) uApvs.push_back(kv.first);
      if (hist.h_ADC_vs_sample_V.count(imod))
        for (auto &kv : hist.h_ADC_vs_sample_V[imod])
          if ((kv.first >> 4) == mpd) vApvs.push_back(kv.first);

      std::sort(uApvs.begin(), uApvs.end());
      std::sort(vApvs.begin(), vApvs.end());

      int ncols = (int) std::max((size_t)5, std::max(uApvs.size(), vApvs.size()));

      c1->Clear();
      c1->Divide(ncols, 2);

      for (size_t i = 0; i < uApvs.size(); i++) {
        c1->cd(i + 1);
        TProfile *h = hist.h_ADC_vs_sample_U[imod][uApvs[i]];
        h->SetLineColor(kBlue+1);
        h->SetMarkerColor(kBlue+1);
        h->SetMarkerStyle(20);
        h->SetMinimum(0);
        h->Draw("E1");
      }

      for (size_t i = 0; i < vApvs.size(); i++) {
        c1->cd(ncols + i + 1);
        TProfile *h = hist.h_ADC_vs_sample_V[imod][vApvs[i]];
        h->SetLineColor(kRed+1);
        h->SetMarkerColor(kRed+1);
        h->SetMarkerStyle(21);
        h->SetMinimum(0);
        h->Draw("E1");
      }

      c1->Print("moller_adc_uv_output.pdf", Form("Title:Module %d, MPD %d", imod, mpd));
    }
  }

  c1->Print("moller_adc_uv_output.pdf]");

  std::cout << "Output written to moller_adc_uv_output.root / .pdf" << std::endl;
}
