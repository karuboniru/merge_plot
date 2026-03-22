#include "tools.h"
#include <TCanvas.h>
#include <TFile.h>
#include <TObject.h>
#include <TStyle.h>
#include <iostream>
#include <memory>
#include <string_view>

void clear_overflow(TH2 &h) {
  auto binx = h.GetNbinsX();
  auto biny = h.GetNbinsY();
  for (int i = 1; i <= h.GetNbinsX(); i++) {
    for (int j = 1; j <= h.GetNbinsY(); j++) {
      h.SetBinContent(i, biny + 1, 0);
      h.SetBinContent(binx + 1, j, 0);
    }
  }
  h.SetBinContent(binx + 1, biny + 1, 0);
}

int main(int argc, char const *argv[]) {
  gStyle->SetPalette(60);
  if (argc < 2) {
    std::cout << "Usage: merge_plot <input_root_file_path>" << std::endl;
    return 1;
  }
  auto file = std::make_unique<TFile>(argv[1]);
  auto list = file->GetListOfKeys();
  TIter next(list);
  TObject *obj{};
  while ((obj = next())) {
    auto name = std::string{obj->GetName()};

    auto hist = dynamic_cast<TH2 *>(file->Get(name.c_str()));
    if (!hist) {
      std::cout << "not TH2 " << name << std::endl;
      continue;
    }
    clear_overflow(*hist);
    if (hist->Integral() < 1) {
      std::cout << "found empty one " << name << " int " << hist->Integral()
                << std::endl;
      continue;
    }
    std::cout << name << std::endl;

    ResetStyle(hist);
    hist->GetYaxis()->SetTitleOffset(0.8);
    // hist->GetXaxis()->SetRangeUser(0., 1.2);
    // hist->GetYaxis()->SetRangeUser(0., 1.2);
    auto mcanvas = []() {
      auto canvas = getCanvas();
      canvas->SetRightMargin(3.0 * canvas->GetRightMargin());
      canvas->SetLeftMargin(0.8 * canvas->GetLeftMargin());
      return canvas;
    };
    {
      auto canvas = mcanvas();
      hist->Draw("colz");
      canvas->SaveAs((name + ".eps").c_str());
      canvas->SaveAs((name + ".pdf").c_str());
    }
    {
      auto canvas = mcanvas();
      canvas->SetLogz();
      hist->Draw("colz");
      canvas->SaveAs((name + ".logz.eps").c_str());
      canvas->SaveAs((name + ".logz.pdf").c_str());
    }
    {
      auto canvas = mcanvas();
      auto hist_normalized = normalize_slice(hist, true);
      hist_normalized->Draw("colz");
      canvas->SaveAs((name + "_normalizedx.eps").c_str());
      canvas->SaveAs((name + "_normalizedx.pdf").c_str());
    }
    {
      auto canvas = mcanvas();
      auto hist_normalized = normalize_slice(hist, false);
      hist_normalized->Draw("colz");
      canvas->SaveAs((name + "_normalizedy.eps").c_str());
      canvas->SaveAs((name + "_normalizedy.pdf").c_str());
    }
  }
}
