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
#include <optional>
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
      {"_C12", " ^{12}C"}};
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
             auto xsec_graph = dynamic_cast<TGraph *>(file->Get(path.c_str()));
             if (!xsec_graph) {
               std::cerr << "Error: Cannot find spline " << path
                         << " this might be expect on QEL channel" << std::endl;
               return std::optional<TSpline3>{std::nullopt};
             }
             return std::optional<TSpline3>{std::in_place, path.c_str(),
                                            xsec_graph};
           }) |
           std::views::filter(
               [](auto &&spline) -> bool { return spline.has_value(); }) |
           std::ranges::to<std::vector>()](const double *x, const double *) {
        return std::ranges::fold_left(
            spline_vec | std::views::transform([&](auto &&spline) {
              return spline.value().Eval(x[0]) / x[0];
            }),
            0.0, std::plus<double>{});
      },
      min, max, 0};
}

auto get_spline_nuwro(const std::vector<std::string> &channels,
                      std::string interaction) {
  auto file =
      TFile::Open(("~/neutrino/nuwro_xsec/plots/" +
                   interaction.substr(0, interaction.length() - 4) + ".root")
                      .c_str(),
                  "READONLY");
  return TF1{
      interaction.c_str(),
      [=, spline_vec =
              channels | std::views::transform([&](const std::string &channel) {
                auto path = channel;
                auto xsec_graph = dynamic_cast<TH1 *>(file->Get(path.c_str()));
                if (!xsec_graph) {
                  std::cerr << "Error: Cannot find spline " << path
                            << " this might be expect on QEL channel"
                            << std::endl;
                  return std::optional<TSpline3>{std::nullopt};
                }
                return std::optional<TSpline3>{std::in_place, xsec_graph};
              }) |
              std::views::filter(
                  [](auto &&spline) -> bool { return spline.has_value(); }) |
              std::ranges::to<std::vector>()](const double *x, const double *) {
        return std::ranges::fold_left(
            spline_vec | std::views::transform([&](auto &&spline) {
              return spline.value().Eval(x[0]) / x[0] * 12.;
            }),
            0.0, std::plus<double>{});
      },
      min, max, 0};
}

int main(int argc, char **argv) {
  const Double_t titleoffset = 0.5;
  const Double_t textfont = 42;
  const Double_t textsize = 0.05;

  const std::vector<std::string> interactions{"nu_e_C12", "nu_e_bar_C12",
                                              "nu_mu_C12", "nu_mu_bar_C12"};
  if (argc < 2) {
    std::cout << "Usage: " << argv[0]
              << " <root_input> <out_title> <plot_name1> [<plot_name2> ... ]"
              << std::endl;
    return 1;
  }
  double x1 = std::stod(argv[1]);
  double y1 = std::stod(argv[2]);
  double x2 = std::stod(argv[3]);
  double y2 = std::stod(argv[4]);
  double factor = std::stod(argv[5]);
  int leg_off = std::stod(argv[6]);
  std::string root_input = argv[7];
  std::string out_title = argv[8];
  std::string extra_text = argv[9];
  std::vector<std::string> plot_names;
  std::string pretty_str{};
  for (int i = 10; i < argc; i++) {
    plot_names.push_back(argv[i]);
    pretty_str += argv[i];
    if (i < argc - 1) {
      pretty_str += "+";
    }
  }
  std::cout << "Plotting " << pretty_str << " from " << root_input << std::endl;
  auto file = std::make_unique<TFile>(root_input.c_str());
  if (file->IsZombie()) {
    std::cerr << "Error: Cannot open file " << root_input << std::endl;
    return 1;
  }
  auto splines = interactions |
                 std::views::transform(std::bind(get_spline_sum, file.get(),
                                                 std::cref(plot_names),
                                                 std::placeholders::_1)) |
                 std::ranges::to<std::vector>();
  auto splines_nuwro =
      interactions |
      std::views::transform(std::bind(get_spline_nuwro, std::cref(plot_names),
                                      std::placeholders::_1)) |
      std::ranges::to<std::vector>();
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
  auto currentr = canvas->GetRightMargin();
  auto currentl = canvas->GetLeftMargin();
  auto currentt = canvas->GetTopMargin();
  // canvas->SetRightMargin(0.5);
  pad1->SetRightMargin(currentr / 2);
  pad2->SetRightMargin(currentr / 2);

  pad1->SetLeftMargin(currentl / 2);
  pad2->SetLeftMargin(currentl / 2);

  pad1->SetTopMargin(currentt / 4);

  pad1->SetBottomMargin(0);
  pad2->SetTopMargin(0);
  pad2->SetBottomMargin(currentm * 2.0);
  pad1->cd();
  pad1->SetLogx();
  pad1->SetGrid(1, 1);
  // pad1->SetLogy();
  auto legend = std::make_unique<TLegend>(x1, y1, x2, y2);
  auto legend_NuWro =
      leg_off ? std::make_unique<TLegend>(x1, y1 - (y2 - y1), x2, y1)
              : std::make_unique<TLegend>(x1 - (x2 - x1), y1, x1, y2);

  // auto legend = std::make_unique<TLegend>();
  ResetStyle(legend.get());
  ResetStyle(legend_NuWro.get());
  legend->SetNColumns(2);
  legend_NuWro->SetNColumns(2);
  legend->SetHeader((extra_text).c_str());
  legend->SetTextSize(legend->GetTextSize() * 1.5);
  legend_NuWro->SetTextSize(legend_NuWro->GetTextSize() * 1.5);
  legend_NuWro->SetHeader("NuWro 21.09.2 ESF");

  // const int colors[] = {kRed, kBlue, kBlue, kRed, kMagenta};
  const int colors[] = {kRed, kRed, kOrange, kOrange};
  const int style[] = {kSolid, kDashed, kSolid, kDashed};
  const int width[] = {3, 3, 2, 2};
  auto max_val1 =
      std::ranges::max(splines | std::views::transform([](const TF1 &func) {
                         return func.GetMaximum();
                       }));
  auto max_val2 = std::ranges::max(
      splines_nuwro |
      std::views::transform([](const TF1 &func) { return func.GetMaximum(); }));
  double max_val = std::max(max_val1, max_val2);
  for (auto &&[id, spline] : std::views::enumerate(splines)) {
    // spline.SetMinimum(0);
    ResetStyle(&spline);
    spline.SetTitle(out_title.c_str());
    spline.SetMinimum(-0.3);
    spline.SetMaximum(max_val * factor);
    // spline.GetXaxis()->SetTitle("E_{#nu} (GeV)");
    spline.GetYaxis()->SetTitle("#sigma / #it{E}_{#nu} (10^{-38} cm^{2}/GeV)");
    spline.SetLineStyle(style[id]);
    spline.SetLineColor(colors[id]);
    spline.SetLineWidth(width[id]);
    spline.GetYaxis()->SetTitleOffset(titleoffset);
    spline.Draw(id ? "same" : "");
    legend->AddEntry(
        &spline, name_normalize(std::string{spline.GetName()}).c_str(), "l");
  }
  {
    const int colors[] = {kBlue+2, kBlue+2, kBlack, kBlack};

    for (auto &&[id, spline] : std::views::enumerate(splines_nuwro)) {
      // spline.SetMinimum(0);
      ResetStyle(&spline);
      spline.SetTitle(out_title.c_str());
      spline.SetMinimum(-0.3);
      spline.SetMaximum(max_val * factor);
      // spline.GetXaxis()->SetTitle("E_{#nu} (GeV)");
      spline.GetYaxis()->SetTitle(
          "#sigma / #it{E}_{#nu} (10^{-38} cm^{2}/GeV)");
      spline.SetLineStyle(style[id]);
      spline.SetLineColor(colors[id]);
      spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
      spline.Draw("same");
      legend_NuWro->AddEntry(
          &spline, name_normalize(std::string{spline.GetName()}).c_str(), "l");
    }
  }

  legend->Draw();
  legend_NuWro->Draw();
  canvas->cd();
  pad1->Draw();

  pad2->cd();
  pad2->SetGrid(1, 1);
  pad2->SetLogx();
  auto &&base_func = splines[0];
  auto spline2ratio =
      std::views::transform([&](const TF1 &func_in) {
        return TF1{"",
                   [&](const double *x, const double *) {
                     return func_in.Eval(x[0]) / base_func.Eval(x[0]);
                   },
                   min, max, 0};
      }) |
      std::ranges::to<std::vector>();
  auto spline_ratio = splines | spline2ratio;
  auto spline_nuwro_ratio = splines_nuwro | spline2ratio;

  for (auto &&[id, spline] : std::views::enumerate(spline_ratio)) {
    spline.SetMaximum(1.2);
    spline.SetMinimum(-0.1);
    ResetStyle(&spline);
    spline.SetTitle(out_title.c_str());
    spline.GetXaxis()->SetTitle("#it{E}_{#nu} (GeV)");
    spline.GetYaxis()->SetTitle(
        ("Ratio to " + name_normalize(std::string{splines[0].GetName()}))
            .c_str());
    spline.SetLineStyle(style[id]);
    spline.SetLineColor(colors[id]);
    spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
    spline.GetXaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
    spline.GetXaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));

    spline.GetYaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
    spline.GetYaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));
    spline.GetYaxis()->SetTitleOffset(titleoffset /
                                      (fair_share / (1 - fair_share)));
    spline.Draw(id ? "same" : "");
  }

  {
    const int colors[] = {kGray, kGray, kBlack, kBlack};
    for (auto &&[id, spline] : std::views::enumerate(spline_nuwro_ratio)) {
      spline.SetMaximum(1.2);
      spline.SetMinimum(-0.1);
      ResetStyle(&spline);
      spline.SetTitle(out_title.c_str());
      spline.GetXaxis()->SetTitle("#it{E}_{#nu} (GeV)");
      spline.GetYaxis()->SetTitle(
          ("Ratio to " + name_normalize(std::string{splines[0].GetName()}))
              .c_str());
      spline.SetLineStyle(style[id]);
      spline.SetLineColor(colors[id]);
      spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
      spline.GetXaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
      spline.GetXaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));

      spline.GetYaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
      spline.GetYaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));
      spline.GetYaxis()->SetTitleOffset(titleoffset /
                                        (fair_share / (1 - fair_share)));
      spline.Draw("same");
    }
  }

  canvas->cd();
  pad2->Draw();

  canvas->SaveAs((out_title + ".pdf").c_str());
  canvas->SaveAs((out_title + ".eps").c_str());
  // canvas->SaveAs((out_title + ".svg").c_str());
  return 0;
}