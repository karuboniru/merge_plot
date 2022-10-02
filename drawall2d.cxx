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

    if (name.substr(0, 2) == "2d") {
      auto hist = dynamic_cast<TH2 *>(file->Get(name.c_str()));
      clear_overflow(*hist);
      if (hist->Integral() < 1) {
        std::cout << "found empty one " << name << " int " << hist->Integral()
                  << std::endl;
        continue;
      }
      std::cout << name << std::endl;

      ResetStyle(hist);
      // hist->GetXaxis()->SetRangeUser(0., 1.2);
      // hist->GetYaxis()->SetRangeUser(0., 1.2);
      {
        auto canvas = getCanvas();
        hist->Draw("colz");
        canvas->SaveAs((name + ".png").c_str());
        canvas->SaveAs((name + ".pdf").c_str());
      }
      {
        auto canvas = getCanvas();
        canvas->SetLogz();
        hist->Draw("colz");
        canvas->SaveAs((name + ".logz.png").c_str());
        canvas->SaveAs((name + ".logz.pdf").c_str());
      }
      {
        auto canvas = getCanvas();
        auto hist_normalized = normalize_slice(hist, true);
        hist_normalized->Draw("colz");
        canvas->SaveAs((name + "_normalizedx.png").c_str());
        canvas->SaveAs((name + "_normalizedx.pdf").c_str());
      }
      {
        auto canvas = getCanvas();
        auto hist_normalized = normalize_slice(hist, false);
        hist_normalized->Draw("colz");
        canvas->SaveAs((name + "_normalizedy.png").c_str());
        canvas->SaveAs((name + "_normalizedy.pdf").c_str());
      }
    }
  }
}