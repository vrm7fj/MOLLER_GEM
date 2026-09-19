#include "TFile.h"
#include "TKey.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TProfile.h"
#include "TColor.h"

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdio>

// Overlays the per-(module,APV) ADC-vs-sample curves from every scan
// point (see moller_scan_corrcoeff.sh / moller_scan_fill.C), one color
// per value, same 5x2 module/MPD layout as moller_plot.C. Edit
// scanFiles/scanLabels to match your scan -- the shell script prints
// the list to paste in.
void moller_scan_overlay() {

  std::vector<TString> scanFiles = {
    "moller_scan_hist_corrcoeff_cut_0p30.root",
    "moller_scan_hist_corrcoeff_cut_0p40.root",
    "moller_scan_hist_corrcoeff_cut_0p50.root",
    "moller_scan_hist_corrcoeff_cut_0p60.root",
    "moller_scan_hist_corrcoeff_cut_0p70.root",
  };
  std::vector<TString> scanLabels = {
    "corrcoeff_cut = 0.30",
    "corrcoeff_cut = 0.40",
    "corrcoeff_cut = 0.50",
    "corrcoeff_cut = 0.60",
    "corrcoeff_cut = 0.70",
  };

  gStyle->SetOptStat(0);
  gStyle->SetTitleFontSize(0.09);

  int npts = (int) scanFiles.size();
  std::vector<TFile*> files(npts, nullptr);
  for (int i = 0; i < npts; i++) {
    files[i] = TFile::Open(scanFiles[i], "READ");
    if (!files[i] || files[i]->IsZombie()) {
      std::cerr << "Warning: could not open " << scanFiles[i] << std::endl;
      files[i] = nullptr;
    }
  }

  const int palette[] = { kBlue+1, kRed+1, kGreen+2, kMagenta+1, kOrange+1, kCyan+2, kBlack, kGray+2 };
  const int npalette = sizeof(palette)/sizeof(palette[0]);

  TFile *reffile = nullptr;
  for (auto *f : files) if (f) { reffile = f; break; }
  if (!reffile) {
    std::cerr << "None of the scan files could be opened." << std::endl;
    return;
  }

  std::set<int> mods;
  std::map<int, std::vector<int> > uApvsByMod, vApvsByMod;

  TIter next(reffile->GetListOfKeys());
  TKey *key;
  while ((key = (TKey*) next())) {
    TString name = key->GetName();
    if (!name.BeginsWith("h_ADC_vs_sample_mod")) continue;

    int imod = -1, apv = -1;
    char axis = 0;
    if (sscanf(name.Data(), "h_ADC_vs_sample_mod%d_apv%d_%c", &imod, &apv, &axis) == 3) {
      mods.insert(imod);
      if (axis == 'U') uApvsByMod[imod].push_back(apv);
      else if (axis == 'V') vApvsByMod[imod].push_back(apv);
    }
  }

  for (auto &kv : uApvsByMod) std::sort(kv.second.begin(), kv.second.end());
  for (auto &kv : vApvsByMod) std::sort(kv.second.begin(), kv.second.end());

  TCanvas *c1 = new TCanvas("c1", "ADC vs sample, DB-parameter scan overlay", 1300, 500);
  c1->Print("moller_scan_overlay.pdf[");

  for (int imod : mods) {

    std::set<int> mpds;
    for (int apv : uApvsByMod[imod]) mpds.insert(apv >> 4);
    for (int apv : vApvsByMod[imod]) mpds.insert(apv >> 4);

    for (int mpd : mpds) {

      std::vector<int> uApvs, vApvs;
      for (int apv : uApvsByMod[imod]) if ((apv >> 4) == mpd) uApvs.push_back(apv);
      for (int apv : vApvsByMod[imod]) if ((apv >> 4) == mpd) vApvs.push_back(apv);

      int ncols = (int) std::max((size_t)5, std::max(uApvs.size(), vApvs.size()));

      c1->Clear();
      c1->Divide(ncols, 2);

      for (size_t i = 0; i < uApvs.size(); i++) {
        c1->cd(i + 1);
        TLegend *leg = nullptr;
        if (i == 0) {
          leg = new TLegend(0.15, 0.60, 0.60, 0.88);
          leg->SetBorderSize(0);
          leg->SetTextSize(0.06);
        }
        bool first = true;
        for (int ip = 0; ip < npts; ip++) {
          if (!files[ip]) continue;
          TProfile *h = (TProfile*) files[ip]->Get(Form("h_ADC_vs_sample_mod%d_apv%d_U", imod, uApvs[i]));
          if (!h) continue;
          h->SetLineColor(palette[ip % npalette]);
          h->SetMarkerColor(palette[ip % npalette]);
          h->SetMarkerStyle(20);
          h->SetMinimum(0);
          h->SetTitle(Form("Module %d, APV mpd=%d adc_id=%d (U)", imod, mpd, uApvs[i] & 0xF));
          h->Draw(first ? "E1" : "E1 SAME");
          if (leg) leg->AddEntry(h, ip < (int)scanLabels.size() ? scanLabels[ip] : scanFiles[ip], "lep");
          first = false;
        }
        if (leg) leg->Draw();
      }

      for (size_t i = 0; i < vApvs.size(); i++) {
        c1->cd(ncols + i + 1);
        bool first = true;
        for (int ip = 0; ip < npts; ip++) {
          if (!files[ip]) continue;
          TProfile *h = (TProfile*) files[ip]->Get(Form("h_ADC_vs_sample_mod%d_apv%d_V", imod, vApvs[i]));
          if (!h) continue;
          h->SetLineColor(palette[ip % npalette]);
          h->SetMarkerColor(palette[ip % npalette]);
          h->SetMarkerStyle(21);
          h->SetMinimum(0);
          h->SetTitle(Form("Module %d, APV mpd=%d adc_id=%d (V)", imod, mpd, vApvs[i] & 0xF));
          h->Draw(first ? "E1" : "E1 SAME");
          first = false;
        }
      }

      c1->Print("moller_scan_overlay.pdf", Form("Title:Module %d, MPD %d -- DB-parameter scan", imod, mpd));
    }
  }

  c1->Print("moller_scan_overlay.pdf]");
  std::cout << "Overlay written to moller_scan_overlay.pdf" << std::endl;
}
