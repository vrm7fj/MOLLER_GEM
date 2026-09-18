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
// One entry per (module, strip, sample) triple.
// ============================================================

struct ADCData {

  std::vector<int>    isamp; // which of the 6 raw ADC samples (0-5)
  std::vector<int>    isU;   // 1 = U/X strip, 0 = V/Y strip (from strip.IsU)
  std::vector<int>    imod;  // GEM module index (0 .. NMOD-1)
  std::vector<int>    apv;   // APV card id = (mpd<<4 | adc_id) within that
                              // module, same convention as "effChan" in
                              // MOLLERGEMModule.cxx
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

  // ----------------------------------------------------------
  // Build per-module branch names and look up their leafcounts
  // ----------------------------------------------------------

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

    std::cout << "Module " << imod << " leafcount branches found:" << std::endl
              << "  " << br_isU[imod]     << " -> " << cnt_strip[imod]   << std::endl
              << "  " << br_adcsamp[imod] << " -> " << cnt_adcsamp[imod] << std::endl;
  }

  // ----------------------------------------------------------
  // Branch variables (one set of buffers per module).
  // static so these live off the stack -- NMOD*MAXADC doubles adds up
  // fast once you have more than one module.
  // ----------------------------------------------------------

  static Double_t strip_isU[NMOD][MAXSTRIP];
  static Double_t strip_mpd[NMOD][MAXSTRIP];
  static Double_t strip_adcid[NMOD][MAXSTRIP];
  Int_t n_strip[NMOD] = {0};

  static Double_t adcsamples[NMOD][MAXADC];
  Int_t n_adcsamp[NMOD] = {0};

  // ----------------------------------------------------------
  // Enable branches / set addresses, module by module
  // ----------------------------------------------------------

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

      for (int imod = 0; imod < NMOD; imod++) {

        // Guard against the buffer being smaller than what's actually in
        // this event (increase MAXSTRIP in moller_config.h if this
        // clamp is ever hit for real data):
        int nstrip = std::min(n_strip[imod], MAXSTRIP);

        for (int istrip = 0; istrip < nstrip; istrip++) {

          int isU   = (strip_isU[imod][istrip] != 0) ? 1 : 0;
          int mpd   = (int) strip_mpd[imod][istrip];
          int adcid = (int) strip_adcid[imod][istrip];
          int apv   = (mpd << 4) | adcid; // same convention as "effChan" in the decoder

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

  std::cout << std::endl;
  std::cout << "Total (module,strip,sample) entries collected: "
            << data.adc.size() << std::endl;

  if (GlobalCut) delete GlobalCut;
}

#endif
