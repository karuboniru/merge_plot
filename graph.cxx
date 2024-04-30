#include <TArrow.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <tools.h>
#include <unordered_map>
#include <vector>

int main(int argc, char **argv) {
  {
    gStyle->SetOptStat(0);
    gSystem->ResetSignal(kSigBus);
    gSystem->ResetSignal(kSigSegmentationViolation);
    gSystem->ResetSignal(kSigIllegalInstruction);
    TH1::AddDirectory(kFALSE);
    if (argc != 2) {
      std::cout << "Usage: merge_plot <config_file>" << std::endl;
      return 1;
    }
  }
  constexpr std::array<int, 10> col{kRed,   kBlue, kViolet, kYellow, kOrange,
                                    kGreen, kGray, kTeal,   kPink};

  nlohmann::json config;
  {
    std::ifstream config_file{argv[1]};
    if (!config_file.is_open()) {
      std::cout << "Error: Could not open config file" << std::endl;
      return 1;
    }
    config_file >> config;
  }
  std::string title = config.value("title", "");
  using entry_t = std::pair<std::string, std::unique_ptr<TGraph>>;
  std::vector<entry_t> mylist;

  for (auto &&entry : config["input"]) {
    std::string filename = entry["filename"];
    double scale = entry.value<double>("scale", 1.);
    std::string legend = entry["legend"];
    auto &&[_, plot] = mylist.emplace_back(
        std::make_pair(legend, std::make_unique<TGraph>(filename.c_str())));
    // plot->SetLineColor(col[mylist.size() - 1]);
    plot->Scale(scale);
  }

  // plot everything
  {
    auto leg = std::make_unique<TLegend>(0.5, 0.7, 0.9, 0.9);
    ResetStyle(leg);
    auto canvas = getCanvas();
    canvas->SetTitle(title.c_str());
    for (size_t i{}; i < mylist.size(); ++i) {
      auto &&[legend, plot] = mylist[i];
      plot->SetLineColor(col[i]);
      plot->SetTitle(title.c_str());
      plot->Draw(i == 0 ? "AL" : "L SAME");
      leg->AddEntry(plot.get(), legend.c_str(), "l");
    }
    leg->Draw();
    canvas->SaveAs(config["output"].get<std::string>().c_str());
  }
  return 0;
}