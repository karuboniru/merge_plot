#include <GuiTypes.h>
#include <Rtypes.h>
#include <TArrow.h>
#include <TAttLine.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <THStack.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TPad.h>
#include <TSpline.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <regex>
#include <string>
#include <tools.h>
constexpr double min = 0.2, max = 100;

std::string name_normalize(std::string str_in) {
  const std::vector<std::pair<std::string, std::string>> replace_list{
      {"nu_e_bar", "#bar{#nu_{e}}"},
      {"nu_mu_bar", "#bar{#nu_{#mu}}"},
      {"nu_e", "#nu_{e}"},
      {"nu_mu", "#nu_{#mu}"},
      {"_C12", "+^{12}C"}};
  for (auto &&[from, to] : replace_list) {
    std::regex reg{from};
    str_in = std::regex_replace(str_in, reg, to);
  }
  std::cout << "Normalized name: " << str_in << std::endl;
  return str_in;
}

auto get_spline_sum(TFile *file, const std::vector<std::string> &channels,
                    std::string interaction) {
  return TF1{
      interaction.c_str(),
      [spline_vec =
           channels | std::views::transform([&](const std::string &channel) {
             auto path = interaction + "/" + channel;
             auto spline = dynamic_cast<TGraph *>(file->Get(path.c_str()));
             if (!spline) {
               std::cerr << "Error: Cannot find spline " << path
                         << " this might be expect on QEL channel" << std::endl;
               return std::shared_ptr<TSpline3>{nullptr};
             }
             return std::make_shared<TSpline3>(path.c_str(), spline);
           }) |
           std::views::filter(
               [](auto &&spline) -> bool { return spline != nullptr; }) |
           std::ranges::to<std::vector>()](const double *x, const double *) {
        return std::ranges::fold_left(
            spline_vec | std::views::transform([&](auto &&spline) {
              return spline->Eval(x[0]) / x[0];
            }),
            0.0, std::plus<double>{});
      },
      min, max, 0};
}

void aaa(std::vector<TF1> &splines, std::string out_title,
         std::string extra_text) {
  const double titleoffset = 1.1;
  const double textfont = 42;
  const double textsize = 0.05;
  auto canvas = std::make_unique<TCanvas>("canvas", "canvas", 1000, 600);
  canvas->cd();
  // canvas->Divide()
  double fair_share = 0.65;
  auto pad1 =
      std::make_unique<TPad>("plot", "plot", 0.0, 1 - fair_share, 1.0, 1.0, 0);
  auto pad2 = std::make_unique<TPad>("ratio", "ratio", 0.0, 0.0, 1.0,
                                     1 - fair_share, 0);
  PadSetup(canvas);
  PadSetup(pad1);
  PadSetup(pad2);
  auto currentm = pad1->GetBottomMargin();

  pad1->SetBottomMargin(0);
  pad2->SetTopMargin(0);
  pad2->SetBottomMargin(currentm * 2.0);
  pad1->cd();
  pad1->SetLogx();
  pad1->SetGrid(1, 1);
  // pad1->SetLogy();
  auto legend = std::make_unique<TLegend>(0.75, 0.5, 0.95, 0.9);
  // auto legend = std::make_unique<TLegend>();
  ResetStyle(legend.get());
  legend->SetHeader(out_title.c_str());
  legend->SetTextSize(legend->GetTextSize() * 1.5);
  // const int colors[] = {kRed, kBlue, kBlue, kRed, kMagenta};
  const int colors[] = {kRed, kRed, kBlue, kBlue, kMagenta};
  const int style[] = {kSolid, kDashed, kSolid, kDashed, kDashed};
  for (auto &&[id, spline] : std::views::enumerate(splines)) {
    spline.SetMinimum(0);
    ResetStyle(&spline);
    spline.SetTitle(out_title.c_str());
    spline.SetMinimum(-0.3);
    // spline.GetXaxis()->SetTitle("E_{#nu} (GeV)");
    spline.GetYaxis()->SetTitle("#sigma / E_{#nu} (10^{-38} cm^{2}/GeV)");
    spline.SetLineStyle(style[id]);
    spline.SetLineColor(colors[id]);
    spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
    spline.Draw(id ? "same" : "");
    legend->AddEntry(
        &spline, name_normalize(std::string{spline.GetName()}).c_str(), "l");
  }
  legend->Draw();
  auto tlatex = std::make_unique<TLatex>();
  tlatex->SetNDC();
  tlatex->SetTextFont(textfont);
  tlatex->SetTextSize(textsize * 1.8);
  tlatex->DrawLatex(0.43, 0.8, extra_text.c_str());
  tlatex->Draw();
  canvas->cd();
  pad1->Draw();

  pad2->cd();
  pad2->SetGrid(1, 1);
  pad2->SetLogx();
  auto &&base_func = splines[0];
  auto spline_ratio =
      splines | std::views::transform([&](const TF1 &func_in) {
        return TF1{"",
                   [&](const double *x, const double *) {
                     return func_in.Eval(x[0]) / base_func.Eval(x[0]);
                   },
                   min, max, 0};
      }) |
      std::ranges::to<std::vector>();
  for (auto &&[id, spline] : std::views::enumerate(spline_ratio)) {
    spline.SetMaximum(1.1);
    spline.SetMinimum(0.);
    ResetStyle(&spline);
    spline.SetTitle(out_title.c_str());
    spline.GetXaxis()->SetTitle("Energy (GeV)");
    spline.GetYaxis()->SetTitle(
        ("Ratio over " + name_normalize(std::string{splines[0].GetName()}))
            .c_str());
    spline.SetLineStyle(style[id]);
    spline.SetLineColor(colors[id]);
    spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
    spline.GetXaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
    spline.GetXaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));

    spline.GetYaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
    spline.GetYaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));
    spline.GetYaxis()->SetTitleOffset(1.1 / (fair_share / (1 - fair_share)));
    spline.Draw(id ? "same" : "");
  }
  canvas->cd();
  pad2->Draw();

  canvas->SaveAs((out_title + ".pdf").c_str());
}

int main(int argc, char **argv) {
  const std::vector<std::string> version{"3.00.02", "3.02.04"};
  const std::vector<std::string> tune{"G18_02a_02_11a", "G18_10a_02_11a",
                                      "G21_11a_02_11b"};
  const std::vector<std::string> interactions{"nu_e_C12", "nu_e_bar_C12",
                                              "nu_mu_C12", "nu_mu_bar_C12"};
  const std::vector<std::tuple<std::string, std::vector<std::string>>> channels{
      {"Total CC", {"tot_cc"}},
      {"QEL CC", {"qel_cc_p", "qel_cc_n"}},
      {"MEC CC", {"mec_cc"}},
      {"RES CC", {"res_cc_p", "res_cc_n"}},
      {"DIS CC", {"dis_cc_p", "dis_cc_n"}},
  };
  if (argc < 2) {
    std::cout << "Usage: " << argv[0]
              << " <root_input> <out_title> <plot_name1> [<plot_name2> ... ]"
              << std::endl;
    return 1;
  }
  // std::string root_input = argv[1];
  // std::string out_title = argv[2];
  // std::string extra_text = argv[3];
  // std::vector<std::string> plot_names;
  // std::string pretty_str{};
  // for (int i = 4; i < argc; i++) {
  //   plot_names.push_back(argv[i]);
  //   pretty_str += argv[i];
  //   if (i < argc - 1) {
  //     pretty_str += "+";
  //   }
  // }
  // std::cout << "Plotting " << pretty_str << " from " << root_input << std::endl;
  // auto file = std::make_unique<TFile>(root_input.c_str());
  // if (file->IsZombie()) {
  //   std::cerr << "Error: Cannot open file " << root_input << std::endl;
  //   return 1;
  // }
  // auto splines = interactions |
  //                std::views::transform(std::bind(get_spline_sum, file.get(),
  //                                                std::cref(plot_names),
  //                                                std::placeholders::_1)) |
  //                std::ranges::to<std::vector>();

  // canvas->SaveAs((out_title + ".eps").c_str());
  // canvas->SaveAs((out_title + ".svg").c_str());
  return 0;
}