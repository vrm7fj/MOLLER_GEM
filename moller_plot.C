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
  gStyle->SetPalette(kRainBow);

  // ==========================================================
  // Create chain
  // ==========================================================

  TChain *C = new TChain("T");

  C->Add(rootfile);

  // ==========================================================
  // Fill vectors
  // ==========================================================

  ADCData data;

  FillVectors(C, data);

  // ==========================================================
  // Create and fill histograms
  // ==========================================================

  ADCHistograms hist = CreateHistograms();

  FillHistograms(data, hist);

  // Distinct modules seen (should normally just be 0 .. NMOD-1):
  std::set<int> mods;
  for (auto &kv : hist.h_ADC_vs_sample_U) mods.insert(kv.first);
  for (auto &kv : hist.h_ADC_vs_sample_V) mods.insert(kv.first);

  std::cout << "Number of distinct modules found: " << mods.size() << std::endl;

  // ==========================================================
  // ROOT output -- both the ADC-vs-sample profiles and the
  // pulse-shape-vs-phase profiles go in the same file.
  // ==========================================================

  TFile *fout = new TFile("moller_adc_uv_output.root", "RECREATE");

  for (auto &modkv : hist.h_ADC_vs_sample_U) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_ADC_vs_sample_V) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_pulseshape_vs_phase_U) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_pulseshape_vs_phase_V) for (auto &kv : modkv.second) kv.second->Write();

  fout->Close();

  // ==========================================================
  // PDF output: for each (module, MPD), two consecutive pages --
  //   page 1: <ADC> vs time sample (5x2 grid, U top/V bottom, as before)
  //   page 2: <ADC> vs (time sample, trigger phase) for the same 10 APVs,
  //           same grid position, drawn COLZ. A flat, horizontal-band
  //           pattern means the pulse shape doesn't depend on trigger
  //           phase; a pattern that tilts/shifts across phase bins means
  //           there's a real sub-sample timing offset for that APV.
  // Each MPD normally serves 10 APVs (5 U, 5 V); the grid widens if a
  // given MPD has more than 5 on one axis.
  // ==========================================================

  TCanvas *c1 = new TCanvas("c1", "ADC vs time sample / trigger phase, by module/MPD", 1300, 500);

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

      std::sort(uApvs.begin(), uApvs.end()); // sorts by adc_id since mpd is fixed here
      std::sort(vApvs.begin(), vApvs.end());

      int ncols = (int) std::max((size_t)5, std::max(uApvs.size(), vApvs.size()));

      // ---- page 1: ADC vs sample ----

      c1->Clear();
      c1->Divide(ncols, 2);

      for (size_t i = 0; i < uApvs.size(); i++) {
        c1->cd(i + 1); // pads 1..ncols = top row (U)
        TProfile *h = hist.h_ADC_vs_sample_U[imod][uApvs[i]];
        h->SetLineColor(kBlue+1);
        h->SetMarkerColor(kBlue+1);
        h->SetMarkerStyle(20);
        h->SetMinimum(0);
        h->Draw("E1");
      }

      for (size_t i = 0; i < vApvs.size(); i++) {
        c1->cd(ncols + i + 1); // pads ncols+1..2*ncols = bottom row (V)
        TProfile *h = hist.h_ADC_vs_sample_V[imod][vApvs[i]];
        h->SetLineColor(kRed+1);
        h->SetMarkerColor(kRed+1);
        h->SetMarkerStyle(21);
        h->SetMinimum(0);
        h->Draw("E1");
      }

      c1->Print("moller_adc_uv_output.pdf", Form("Title:Module %d, MPD %d -- ADC vs sample", imod, mpd));

      // ---- page 2: ADC vs (sample, trigger phase), same layout ----

      bool havePhaseU = hist.h_pulseshape_vs_phase_U.count(imod) > 0;
      bool havePhaseV = hist.h_pulseshape_vs_phase_V.count(imod) > 0;

      if (havePhaseU || havePhaseV) {

        c1->Clear();
        c1->Divide(ncols, 2);

        for (size_t i = 0; i < uApvs.size(); i++) {
          if (!havePhaseU || !hist.h_pulseshape_vs_phase_U[imod].count(uApvs[i])) continue;
          c1->cd(i + 1);
          hist.h_pulseshape_vs_phase_U[imod][uApvs[i]]->Draw("COLZ");
        }

        for (size_t i = 0; i < vApvs.size(); i++) {
          if (!havePhaseV || !hist.h_pulseshape_vs_phase_V[imod].count(vApvs[i])) continue;
          c1->cd(ncols + i + 1);
          hist.h_pulseshape_vs_phase_V[imod][vApvs[i]]->Draw("COLZ");
        }

        c1->Print("moller_adc_uv_output.pdf", Form("Title:Module %d, MPD %d -- ADC vs sample/phase", imod, mpd));
      }
    }
  }

  c1->Print("moller_adc_uv_output.pdf]");

  std::cout
    << "Output written to:" << std::endl
    << "  moller_adc_uv_output.root" << std::endl
    << "  moller_adc_uv_output.pdf" << std::endl;
}
