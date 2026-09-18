#include "TChain.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TColor.h"

#include <set>
#include <vector>
#include <algorithm>

#include "moller_phase_config.h"
#include "moller_phase_fill_vectors.h"
#include "moller_phase_histograms.h"


void moller_phase_plot() {

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

  PhaseData data;

  FillVectors(C, data);

  // ==========================================================
  // Create and fill histograms
  // ==========================================================

  PhaseHistograms hist = CreateHistograms();

  FillHistograms(data, hist);

  // Distinct modules seen (should normally just be 0 .. NMOD-1):
  std::set<int> mods;
  for (auto &kv : hist.h_pulseshape_vs_phase_U) mods.insert(kv.first);
  for (auto &kv : hist.h_pulseshape_vs_phase_V) mods.insert(kv.first);

  std::cout << "Number of distinct modules found: " << mods.size() << std::endl;

  // ==========================================================
  // ROOT output -- both the pulse-shape-vs-phase profiles AND the
  // occupancy-vs-phase histograms are saved here, even though only the
  // former gets its own PDF pages below.
  // ==========================================================

  TFile *fout = new TFile("moller_phase_output.root", "RECREATE");

  for (auto &modkv : hist.h_pulseshape_vs_phase_U) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_pulseshape_vs_phase_V) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_nentries_vs_phase_U)   for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_nentries_vs_phase_V)   for (auto &kv : modkv.second) kv.second->Write();

  fout->Close();

  // ==========================================================
  // PDF output: one page per (module, MPD) -- same 5x2 U(top)/V(bottom)
  // grid as the ADC-vs-sample macro, but each pad now shows a 2D
  // "<ADC> vs (sample, trigger phase)" plot instead of a 1D profile.
  // A flat, horizontal-band pattern means the pulse shape doesn't
  // depend on trigger phase (no meaningful sub-sample timing offset);
  // a pattern that tilts/shifts across phase bins means it does.
  // The occupancy-vs-phase histograms (in the ROOT file only) tell you
  // whether phase is being sampled uniformly across events.
  // ==========================================================

  TCanvas *c1 = new TCanvas("c1", "Pulse shape vs trigger phase by module/MPD", 1300, 500);

  c1->Print("moller_phase_output.pdf[");

  for (int imod : mods) {

    std::set<int> mpds;

    if (hist.h_pulseshape_vs_phase_U.count(imod))
      for (auto &kv : hist.h_pulseshape_vs_phase_U[imod]) mpds.insert(kv.first >> 4);
    if (hist.h_pulseshape_vs_phase_V.count(imod))
      for (auto &kv : hist.h_pulseshape_vs_phase_V[imod]) mpds.insert(kv.first >> 4);

    for (int mpd : mpds) {

      std::vector<int> uApvs, vApvs;

      if (hist.h_pulseshape_vs_phase_U.count(imod))
        for (auto &kv : hist.h_pulseshape_vs_phase_U[imod])
          if ((kv.first >> 4) == mpd) uApvs.push_back(kv.first);

      if (hist.h_pulseshape_vs_phase_V.count(imod))
        for (auto &kv : hist.h_pulseshape_vs_phase_V[imod])
          if ((kv.first >> 4) == mpd) vApvs.push_back(kv.first);

      std::sort(uApvs.begin(), uApvs.end());
      std::sort(vApvs.begin(), vApvs.end());

      int ncols = (int) std::max((size_t)5, std::max(uApvs.size(), vApvs.size()));

      c1->Clear();
      c1->Divide(ncols, 2);

      for (size_t i = 0; i < uApvs.size(); i++) {
        c1->cd(i + 1); // pads 1..ncols = top row (U)
        hist.h_pulseshape_vs_phase_U[imod][uApvs[i]]->Draw("COLZ");
      }

      for (size_t i = 0; i < vApvs.size(); i++) {
        c1->cd(ncols + i + 1); // pads ncols+1..2*ncols = bottom row (V)
        hist.h_pulseshape_vs_phase_V[imod][vApvs[i]]->Draw("COLZ");
      }

      c1->Print("moller_phase_output.pdf", Form("Title:Module %d, MPD %d", imod, mpd));
    }
  }

  c1->Print("moller_phase_output.pdf]");

  std::cout
    << "Output written to:" << std::endl
    << "  moller_phase_output.root" << std::endl
    << "  moller_phase_output.pdf" << std::endl;
}
