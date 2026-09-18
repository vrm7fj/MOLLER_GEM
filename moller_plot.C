#include "TChain.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TColor.h"

#include "moller_config.h"
#include "moller_fill_vectors.h"
#include "moller_histograms.h"


void moller_plot() {

  gStyle->SetOptStat(0);

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

  // ==========================================================
  // ROOT output
  // ==========================================================

  TFile *fout = new TFile("moller_adc_uv_output.root", "RECREATE");

  hist.h_ADC_vs_sample_U->Write();
  hist.h_ADC_vs_sample_V->Write();

  fout->Close();

  // ==========================================================
  // PDF output: U and V overlaid on one pad, different colors
  // ==========================================================

  TCanvas *c1 = new TCanvas("c1", "ADC vs time sample, U/V overlay", 700, 600);

  hist.h_ADC_vs_sample_U->SetLineColor(kBlue+1);
  hist.h_ADC_vs_sample_U->SetMarkerColor(kBlue+1);
  hist.h_ADC_vs_sample_U->SetMarkerStyle(20);
  hist.h_ADC_vs_sample_U->SetLineWidth(2);

  hist.h_ADC_vs_sample_V->SetLineColor(kRed+1);
  hist.h_ADC_vs_sample_V->SetMarkerColor(kRed+1);
  hist.h_ADC_vs_sample_V->SetMarkerStyle(21);
  hist.h_ADC_vs_sample_V->SetLineWidth(2);

  double maxU = hist.h_ADC_vs_sample_U->GetMaximum();
  double maxV = hist.h_ADC_vs_sample_V->GetMaximum();
  double ymax = 1.15 * (maxU > maxV ? maxU : maxV);

  hist.h_ADC_vs_sample_U->SetMinimum(0);
  hist.h_ADC_vs_sample_U->SetMaximum(ymax);

  hist.h_ADC_vs_sample_U->Draw("E1");
  hist.h_ADC_vs_sample_V->Draw("E1 SAME");

  TLegend *leg = new TLegend(0.65, 0.75, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->AddEntry(hist.h_ADC_vs_sample_U, "U/X strips", "lep");
  leg->AddEntry(hist.h_ADC_vs_sample_V, "V/Y strips", "lep");
  leg->Draw();

  c1->Print("moller_adc_uv_output.pdf");

  std::cout
    << "Output written to:" << std::endl
    << "  moller_adc_uv_output.root" << std::endl
    << "  moller_adc_uv_output.pdf" << std::endl;
}
