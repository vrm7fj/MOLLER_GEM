#include "TChain.h"
#include "TFile.h"
#include "TString.h"

#include "moller_config.h"
#include "moller_fill_vectors.h"
#include "moller_histograms.h"

// ============================================================
// Run once per replayed file in a database-parameter scan (see
// moller_scan_corrcoeff.sh). Fills the same per-(module,APV)
// ADC-vs-sample TProfiles as moller_plot.C, but for a single
// specified input file, and writes them to a small standalone ROOT
// file named after "tag" instead of drawing a PDF.
//
// moller_scan_overlay.C later opens every scan point's output file
// and overlays the same-named histograms on top of each other.
//
//   infile : path to one replayed ROOT file (one scan point)
//   tag    : label for this scan point, e.g. "corrcoeff_cut_0p50"
//            (used only in the output filename -- histogram names
//            inside stay identical across scan points on purpose,
//            since each scan point lives in its own file)
//
// Note: this intentionally does NOT use the "rootfile" constant from
// moller_config.h -- the input file is whatever moller_scan_corrcoeff.sh
// passes in for this iteration.
// ============================================================

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
