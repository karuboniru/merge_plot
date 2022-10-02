#include <TFile.h>
#include <fstream>
#include <memory>
#include <tools.h>

void plot_normalize(TH2 *ptr) {
  auto nbinsx = ptr->GetNbinsX();
  auto nbinsy = ptr->GetNbinsY();
  for (int i = 1; i <= nbinsx; i++) {
    for (int j = 1; j <= nbinsy; j++) {
      ptr->SetBinContent(i, j,
                         ptr->GetBinContent(i, j) /
                             std::pow(ptr->GetXaxis()->GetBinCenter(i), 2));
    }
  }
}

int main(int argc, const char **argv) {
  gStyle->SetPalette(60);
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <input_root_file_path>" << std::endl;
    return 1;
  }
  TFile file{argv[1], "READ"};
  std::unique_ptr<TH2> hist{
      dynamic_cast<TH2 *>(file.Get("2d_init_proton_momentum_deltaE"))};
  hist->SetDirectory(nullptr);
  plot_normalize(hist.get());
  ResetStyle(hist.get());
  {
    auto c = getCanvas();
    hist->Draw("colz");
    c->SaveAs("2d_init_proton_momentum_deltaE.png");
    c->SaveAs("2d_init_proton_momentum_deltaE.pdf");
  }
}