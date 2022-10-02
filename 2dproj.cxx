#include <TASImage.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TFile.h>
#include <TGaxis.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLegend.h>
#include <iostream>
#include <memory>
#include <tools.h>

void Draw2DProjection(TH2D *hraw, TCanvas *cc) {
  TH1D *hx = hraw->ProjectionX("hx", 0, -1, "oe");
  hx->Scale(1., "width");
  hx->SetLineColor(kBlack);
  hx->GetYaxis()->SetLabelSize(0);
  double xmax = 1.1 * hx->GetBinContent(hx->GetMaximumBin());
  hx->SetMaximum(xmax);

  // divided at p_pi = 0.28 GeV
  size_t bin_id = 70;
  TH1D *hx1 = hraw->ProjectionX("hx1", 0, bin_id, "oe");
  hx1->Scale(1., "width");
  hx1->SetLineColor(kBlue);
  TH1D *hx2 = hraw->ProjectionX("hx2", bin_id + 1, -1, "oe");
  hx2->Scale(1., "width");
  hx2->SetLineColor(kRed);

  TH1D *hy = hraw->ProjectionY("hy", 0, -1, "oe");
  hy->Scale(1., "width");
  hy->SetLineColor(kBlack);
  hy->GetXaxis()->SetLabelSize(0);
  hy->GetYaxis()->SetLabelSize(0);
  double ymax = 1.1 * hy->GetBinContent(hy->GetMaximumBin());
  hy->SetMaximum(ymax);

  // 	divided at p_pi = 0.28 GeV
  TH1D *htmp1 = (TH1D *)hy->Clone("htmp1");
  TH1D *htmp2 = (TH1D *)hy->Clone("htmp2");
  htmp1->GetXaxis()->SetRange(-1, bin_id);
  htmp2->GetXaxis()->SetRange(bin_id, htmp2->GetXaxis()->GetNbins());
  htmp1->SetLineColor(kBlue);
  htmp1->SetFillColorAlpha(kBlue, 0.5);
  htmp2->SetLineColor(kRed);
  htmp2->SetFillColorAlpha(kRed, 0.5);

  auto *ctmp = new TCanvas("ctmp", "ctmp", 1200, 800);
  hy->Draw("histL");
  htmp1->Draw("histLF2 same");
  htmp2->Draw("histLF2 same");
  hy->Draw("histL same");
  gPad->SetLeftMargin(0);
  gPad->SetRightMargin(1. / 6);
  gPad->SetTopMargin(0.25);
  gPad->SetBottomMargin(0);
  ctmp->Print("output.png");

  cc->cd();
  gPad->SetLeftMargin(0);
  gPad->SetRightMargin(0);
  gPad->SetTopMargin(0);
  gPad->SetBottomMargin(0);
  auto *pad1 = new TPad("pad1", "pad1", 0.4, 0.4, 1, 1);
  pad1->SetLeftMargin(0);
  pad1->SetRightMargin(1. / 6);
  pad1->SetTopMargin(1. / 6);
  pad1->SetBottomMargin(0);
  pad1->SetNumber(1);
  pad1->Draw();
  auto *pad2 = new TPad("pad2", "pad2", 0.4, 0, 1, 0.4);
  pad2->SetLeftMargin(0);
  pad2->SetRightMargin(1. / 6);
  pad2->SetTopMargin(0);
  pad2->SetBottomMargin(0.25);
  pad2->SetNumber(2);
  pad2->Draw();
  auto *pad3 = new TPad("pad3", "pad3", 0, 0.4, 0.4, 1);
  pad3->SetLeftMargin(0.25);
  pad3->SetRightMargin(0);
  pad3->SetTopMargin(1. / 6);
  pad3->SetBottomMargin(0);
  pad3->SetNumber(3);
  pad3->Draw();
  cc->Modified();
  cc->cd();

  cc->cd(1);
  hraw->Draw("COLZ");
  hraw->GetXaxis()->SetLabelSize(0);
  hraw->GetYaxis()->SetLabelSize(0);
  hraw->GetZaxis()->ChangeLabel(1, -1, -1, 100);
  hraw->GetZaxis()->ChangeLabel(2, -1, -1, 100);
  hraw->GetZaxis()->ChangeLabel(3, -1, -1, 100);
  hraw->GetZaxis()->ChangeLabel(4, -1, -1, 100);
  hraw->SetZTitle("Events");
  gPad->SetLogz(1);
  cc->cd(2);
  hx->Draw("histL");
  hx->SetLineColor(kBlack);
  hx->SetXTitle("#it{W}(#pi^{0}N_{leading}) (GeV/#it{c}^{2})");
  hx1->Draw("histLF2 same");
  hx2->Draw("histLF2 same");
  hx->Draw("histL same");
  cc->cd(3);
  TASImage *img = new TASImage("output.png");
  img->SetConstRatio(kFALSE);
  img->Flip(90);
  img->Draw("x");
  cc->cd();
  TGaxis *ax1 = new TGaxis(0.4, 0.1, 0.4, 0.4, 0, xmax, 410, "SI");
  ax1->SetTitle("Events (1/GeV/#it{c}^{2})");
  ax1->SetTickSize(0.);
  ax1->SetLabelFont(42);
  ax1->SetLabelSize(0.02);
  ax1->SetTitleFont(42);
  ax1->SetTitleSize(0.02);
  ax1->SetTitleOffset(1.25);
  ax1->Draw();
  TGaxis *ax2 = new TGaxis(0.1, 0.4, 0.1, 0.9, 0, 1, 510, "S");
  ax2->SetTitle("#it{p}_{#pi^{0}} (GeV/#it{c})");
  ax2->SetTickSize(0.);
  ax2->SetLabelFont(42);
  ax2->SetLabelSize(0.02);
  ax2->SetTitleFont(42);
  ax2->SetTitleSize(0.02);
  ax2->SetTitleOffset(1.25);
  ax2->Draw();
  TGaxis *ax3 = new TGaxis(0.4, 0.93, 0.1, 0.93, 0, ymax, 410, "SIB+");
  ax3->SetTitle("Events (1/GeV/#it{c})");
  ax3->SetTickSize(0);
  ax3->SetLabelFont(42);
  ax3->SetLabelSize(0.02);
  ax3->SetLabelOffset(-0.01);
  ax3->SetTitleFont(42);
  ax3->SetTitleSize(0.02);
  ax3->SetTitleOffset(1.15);
  ax3->Draw();
  auto *lg = new TLegend(0.1, 0.15, 0.25, 0.25);
  lg->SetHeader("NuWro 21.09 p#rightarrowe^{+}#pi^{0}");
  lg->AddEntry((TObject *)0, "C12, SF+hN, 1#pi^{0} w/ N", "");
  lg->AddEntry(hx, "All", "lf");
  lg->AddEntry(htmp1, "#it{p}_{#pi} #leq 0.28 GeV/#it{c}", "lfb");
  lg->AddEntry(htmp2, "#it{p}_{#pi} > 0.28 GeV/#it{c}", "lfb");
  lg->SetTextFont(42);
  lg->SetTextSize(0.02);
  lg->Draw();
  cc->Update();
}

int main(int argc, char **argv) {
  // <rootfile> <plotname>
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <rootfile> <plotname>" << std::endl;
    return 1;
  }
  auto file = std::make_unique<TFile>(argv[1]);
  if (!file->IsOpen())
    return 1;
  auto hraw = dynamic_cast<TH2D *>(file->Get(argv[2]));
  if (!hraw)
    return 1;
  auto c1 = std::make_unique<TCanvas>("c", "c", 800 * 4, 600 * 4);
  Draw2DProjection(hraw, c1.get());
  c1->SaveAs("test.png");
}