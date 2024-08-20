#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TSystem.h>
#include <iostream>
#include <memory>
#include <tools.h>

int main(int argc, const char **argv) {
  TH1::AddDirectory(false);

  if (argc != 4) {
    std::cout << "Usage: " << argv[0]
              << " <input1.root> <input2.root> <output.root>" << std::endl;
    return 1;
  }

  auto input1 = std::make_unique<TFile>(argv[1]);
  auto input2 = std::make_unique<TFile>(argv[2]);
  auto output = std::make_unique<TFile>(argv[3], "RECREATE");

  for (auto key : *input1->GetListOfKeys()) {
    auto legend = std::make_unique<TLegend>();
    auto name = key->GetName();
    auto hist1 = dynamic_cast<TH1 *>(input1->Get(name));
    if (!hist1)
      continue;
    auto sum1 = hist1->Integral();
    if (!sum1)
      continue;
    auto hist2 = dynamic_cast<TH1 *>(input2->Get(name));
    if (!hist2) {
      std::cerr << "Cannot find " << name << " in the second file" << std::endl;
      continue;
    }

    auto hist3 = dynamic_cast<TH1 *>(hist2->Clone());
    hist3->Divide(hist1);
    hist3->SetTitle((std::string(hist3->GetTitle()) + " (divided)").c_str());
    output->Add(hist3);
  }
  output->Write();
  output->Close();
  return 0;
}