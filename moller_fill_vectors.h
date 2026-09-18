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

// ============================================================
// Structure containing quantities needed after event selection
// One entry per (strip, sample) pair.
// ============================================================

struct ADCData {

  std::vector<int>    isamp; // which of the 6 raw ADC samples (0-5)
  std::vector<int>    isU;   // 1 = U/X strip, 0 = V/Y strip (from strip.IsU)
  std::vector<int>    apv;   // APV card id = (mpd<<4 | adc_id), same convention
                              // as "effChan" in MOLLERGEMModule.cxx
  std::vector<double> adc;   // ADC value at that sample (strip.ADCsamples)

};


// ============================================================
// Helper: look up the auto-generated "Ndata.<branch>" leafcount
// branch name for a variable-size Podd/THaAnalysisObject branch,
// instead of guessing/hardcoding it.
// ============================================================

TString GetLeafCountBranchName(TTree *T, const char *branchname) {

  TLeaf *leaf = T->GetLeaf(branchname);

  if (!leaf) {
    std::cerr << "Warning: branch/leaf not found: " << branchname << std::endl;
    return "";
  }

  TLeaf *count = leaf->GetLeafCount();

  if (!count) {
    // fixed-size array (or scalar): no separate counter branch
    return "";
  }

  return count->GetBranch()->GetName();
}


// ============================================================
// Fill vectors
// ============================================================

void FillVectors(TChain *C, ADCData &data) {

  // Make sure at least one tree is loaded so GetLeaf() below works:
  C->LoadTree(0);

  TString pfx = modprefix;

  TString br_isU     = pfx + "strip.IsU";
  TString br_mpd     = pfx + "strip.mpd";
  TString br_adcid   = pfx + "strip.adc_id";
  TString br_adcsamp = pfx + "strip.ADCsamples";

  TString cnt_strip   = GetLeafCountBranchName(C, br_isU.Data());
  TString cnt_adcsamp = GetLeafCountBranchName(C, br_adcsamp.Data());

  std::cout << "Leafcount branches found:" << std::endl
            << "  " << br_isU     << " -> " << cnt_strip   << std::endl
            << "  " << br_adcsamp << " -> " << cnt_adcsamp << std::endl;

  // ----------------------------------------------------------
  // Branch variables
  // ----------------------------------------------------------

  Double_t strip_isU[MAXSTRIP];
  Double_t strip_mpd[MAXSTRIP];
  Double_t strip_adcid[MAXSTRIP];
  Int_t    n_strip = 0;

  Double_t adcsamples[MAXADC];
  Int_t    n_adcsamp = 0;

  // ----------------------------------------------------------
  // Enable branches
  // ----------------------------------------------------------

  C->SetBranchStatus("*", 0);

  C->SetBranchStatus(br_isU, 1);
  C->SetBranchStatus(br_mpd, 1);
  C->SetBranchStatus(br_adcid, 1);
  C->SetBranchStatus(br_adcsamp, 1);
  if (cnt_strip.Length())   C->SetBranchStatus(cnt_strip, 1);
  if (cnt_adcsamp.Length()) C->SetBranchStatus(cnt_adcsamp, 1);

  // ----------------------------------------------------------
  // Set branch addresses
  // ----------------------------------------------------------

  C->SetBranchAddress(br_isU, strip_isU);
  C->SetBranchAddress(br_mpd, strip_mpd);
  C->SetBranchAddress(br_adcid, strip_adcid);
  C->SetBranchAddress(br_adcsamp, adcsamples);

  if (cnt_strip.Length())   C->SetBranchAddress(cnt_strip, &n_strip);
  if (cnt_adcsamp.Length()) C->SetBranchAddress(cnt_adcsamp, &n_adcsamp);

  // ----------------------------------------------------------
  // Optional global cut
  // ----------------------------------------------------------

  TTreeFormula *GlobalCut = nullptr;
  bool haveCut = (((TString)globalcut.GetTitle()).Length() > 0);
  if (haveCut) {
    GlobalCut = new TTreeFormula("GlobalCut", globalcut, C);
  }

  // ----------------------------------------------------------
  // Event loop
  // ----------------------------------------------------------

  Long64_t nevent = 0;

  int treenum = -1;
  int oldtreenum = -1;

  while (C->GetEntry(nevent)) {

    treenum = C->GetTreeNumber();

    if (treenum != oldtreenum) {
      oldtreenum = treenum;
      if (haveCut) GlobalCut->UpdateFormulaLeaves();
    }

    if (nevent % 10000 == 0) {
      std::cout << "Event " << nevent
                << ", file = " << C->GetFile()->GetName()
                << std::endl;
    }

    bool passedcut = !haveCut || (GlobalCut->EvalInstance(0) != 0);

    if (passedcut) {

      // Guard against the buffer being smaller than what's actually in
      // this event (increase MAXSTRIP in moller_config.h if this
      // clamp is ever hit for real data):
      int nstrip = std::min(n_strip, MAXSTRIP);

      for (int istrip = 0; istrip < nstrip; istrip++) {

        int isU  = (strip_isU[istrip] != 0) ? 1 : 0;
        int mpd  = (int) strip_mpd[istrip];
        int adcid = (int) strip_adcid[istrip];
        int apv  = (mpd << 4) | adcid; // same convention as "effChan" in the decoder

        for (int isamp = 0; isamp < NSAMP; isamp++) {

          data.isamp.push_back(isamp);
          data.isU.push_back(isU);
          data.apv.push_back(apv);
          data.adc.push_back(adcsamples[isamp + NSAMP*istrip]);

        }
      }
    }

    nevent++;
  }

  std::cout << std::endl;
  std::cout << "Total (strip,sample) entries collected: "
            << data.adc.size() << std::endl;

  if (GlobalCut) delete GlobalCut;
}

#endif
