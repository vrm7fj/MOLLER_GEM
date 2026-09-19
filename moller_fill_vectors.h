#ifndef MOLLER_FILL_VECTORS_H
#define MOLLER_FILL_VECTORS_H

#include "TChain.h"
#include "TTreeFormula.h"
#include "TLeaf.h"
#include "TBranch.h"
#include "TString.h"

#include <vector>
#include <iostream>
#include <algorithm>

#include "moller_config.h"

// One entry per (module, strip, sample) triple.
struct ADCData {
  std::vector<int>    isamp; // 0-5
  std::vector<int>    isU;   // 1 = U/X, 0 = V/Y (strip.IsU)
  std::vector<int>    imod;  // module index (0..NMOD-1)
  std::vector<int>    apv;   // mpd<<4 | adc_id, same as decoder's "effChan"
  std::vector<double> adc;
};

// Looks up the auto-generated "Ndata.<branch>" leafcount branch instead
// of hardcoding its name.
TString GetLeafCountBranchName(TTree *T, const char *branchname) {

  TLeaf *leaf = T->GetLeaf(branchname);
  if (!leaf) {
    std::cerr << "Warning: branch/leaf not found: " << branchname << std::endl;
    return "";
  }

  TLeaf *count = leaf->GetLeafCount();
  return count ? count->GetBranch()->GetName() : "";
}

void FillVectors(TChain *C, ADCData &data) {

  C->LoadTree(0);

  TString br_isU[NMOD], br_mpd[NMOD], br_adcid[NMOD], br_adcsamp[NMOD];
  TString cnt_strip[NMOD], cnt_adcsamp[NMOD];

  for (int imod = 0; imod < NMOD; imod++) {

    TString pfx = Form("%s%d.", modbase, imod);
    br_isU[imod]     = pfx + "strip.IsU";
    br_mpd[imod]     = pfx + "strip.mpd";
    br_adcid[imod]   = pfx + "strip.adc_id";
    br_adcsamp[imod] = pfx + "strip.ADCsamples";

    cnt_strip[imod]   = GetLeafCountBranchName(C, br_isU[imod].Data());
    cnt_adcsamp[imod] = GetLeafCountBranchName(C, br_adcsamp[imod].Data());

    std::cout << "Module " << imod << ": " << br_isU[imod] << " -> " << cnt_strip[imod]
              << ", " << br_adcsamp[imod] << " -> " << cnt_adcsamp[imod] << std::endl;
  }

  // static: keeps NMOD*MAXADC doubles off the stack
  static Double_t strip_isU[NMOD][MAXSTRIP];
  static Double_t strip_mpd[NMOD][MAXSTRIP];
  static Double_t strip_adcid[NMOD][MAXSTRIP];
  Int_t n_strip[NMOD] = {0};

  static Double_t adcsamples[NMOD][MAXADC];
  Int_t n_adcsamp[NMOD] = {0};

  C->SetBranchStatus("*", 0);

  for (int imod = 0; imod < NMOD; imod++) {

    C->SetBranchStatus(br_isU[imod], 1);
    C->SetBranchStatus(br_mpd[imod], 1);
    C->SetBranchStatus(br_adcid[imod], 1);
    C->SetBranchStatus(br_adcsamp[imod], 1);
    if (cnt_strip[imod].Length())   C->SetBranchStatus(cnt_strip[imod], 1);
    if (cnt_adcsamp[imod].Length()) C->SetBranchStatus(cnt_adcsamp[imod], 1);

    C->SetBranchAddress(br_isU[imod], strip_isU[imod]);
    C->SetBranchAddress(br_mpd[imod], strip_mpd[imod]);
    C->SetBranchAddress(br_adcid[imod], strip_adcid[imod]);
    C->SetBranchAddress(br_adcsamp[imod], adcsamples[imod]);

    if (cnt_strip[imod].Length())   C->SetBranchAddress(cnt_strip[imod], &n_strip[imod]);
    if (cnt_adcsamp[imod].Length()) C->SetBranchAddress(cnt_adcsamp[imod], &n_adcsamp[imod]);
  }

  TTreeFormula *GlobalCut = nullptr;
  bool haveCut = (((TString)globalcut.GetTitle()).Length() > 0);
  if (haveCut) GlobalCut = new TTreeFormula("GlobalCut", globalcut, C);

  Long64_t nevent = 0;
  int treenum = -1, oldtreenum = -1;

  while (C->GetEntry(nevent)) {

    treenum = C->GetTreeNumber();
    if (treenum != oldtreenum) {
      oldtreenum = treenum;
      if (haveCut) GlobalCut->UpdateFormulaLeaves();
    }

    if (nevent % 10000 == 0) {
      std::cout << "Event " << nevent << ", file = " << C->GetFile()->GetName() << std::endl;
    }

    bool passedcut = !haveCut || (GlobalCut->EvalInstance(0) != 0);

    if (passedcut) {
      for (int imod = 0; imod < NMOD; imod++) {

        int nstrip = std::min(n_strip[imod], MAXSTRIP);

        for (int istrip = 0; istrip < nstrip; istrip++) {

          int isU   = (strip_isU[imod][istrip] != 0) ? 1 : 0;
          int mpd   = (int) strip_mpd[imod][istrip];
          int adcid = (int) strip_adcid[imod][istrip];
          int apv   = (mpd << 4) | adcid;

          for (int isamp = 0; isamp < NSAMP; isamp++) {
            data.isamp.push_back(isamp);
            data.isU.push_back(isU);
            data.imod.push_back(imod);
            data.apv.push_back(apv);
            data.adc.push_back(adcsamples[imod][isamp + NSAMP*istrip]);
          }
        }
      }
    }

    nevent++;
  }

  std::cout << std::endl << "Total (module,strip,sample) entries: " << data.adc.size() << std::endl;

  if (GlobalCut) delete GlobalCut;
}

#endif
