#include "TChain.h"
#include "TFile.h"
#include "TString.h"

#include "moller_config.h"
#include "moller_fill_vectors.h"
#include "moller_histograms.h"

// Fills the per-(module,APV) ADC-vs-sample profiles for one replayed
// file in a DB-parameter scan and writes them to moller_scan_hist_<tag>.root.
// moller_scan_overlay.C later opens every scan point's file and overlays
// them. Doesn't use moller_config.h's "rootfile" -- infile is whatever
// moller_scan_corrcoeff.sh passes for this iteration.
void moller_scan_fill(const char *infile, const char *tag) {

  TChain *C = new TChain("T");
  C->Add(infile);

  ADCData data;
  FillVectors(C, data);

  ADCHistograms hist = CreateHistograms();
  FillHistograms(data, hist);

  TString outname = Form("moller_scan_hist_%s.root", tag);
  TFile *fout = new TFile(outname, "RECREATE");

  for (auto &modkv : hist.h_ADC_vs_sample_U) for (auto &kv : modkv.second) kv.second->Write();
  for (auto &modkv : hist.h_ADC_vs_sample_V) for (auto &kv : modkv.second) kv.second->Write();

  fout->Close();

  std::cout << "Scan point \"" << tag << "\": wrote " << outname << std::endl;
}
