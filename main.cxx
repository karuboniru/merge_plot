#include <TArrow.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <tools.h>
int main(int argc, char const *argv[])
{
    gStyle->SetOptStat(0);
    gSystem->ResetSignal(kSigBus);
    gSystem->ResetSignal(kSigSegmentationViolation);
    gSystem->ResetSignal(kSigIllegalInstruction);
    TH1::AddDirectory(kFALSE);
    if (argc != 2)
    {
        std::cout << "Usage: merge_plot <config_file>" << std::endl;
        return 1;
    }
    std::string plot_title{};
    using entry_t = std::pair<std::string, std::unique_ptr<TH1>>;
    double max{};
    std::vector<entry_t> entries{};
    std::vector<std::unique_ptr<TObject>> objects{};
    nlohmann::json config;
    auto canvas = getCanvas();

    {
        std::ifstream config_file{argv[1]};
        if (!config_file.is_open())
        {
            std::cout << "Error: Could not open config file" << std::endl;
            return 1;
        }
        config_file >> config;
    }
    {
        plot_title = config["plot_title"];
        auto uplimit = config["max_x_value"];
        for (const auto &entry : config["hists"])
        {
            std::string legend = entry["legend"];
            std::string file_path = entry["file_path"];
            std::string plot_name = entry["hist"];
            double scale = entry.value("scale", 1.);
            size_t rebin_factor = entry.value<size_t>("rebin", 1);
            TFile file{file_path.c_str(), "READ"};
            if (!file.IsOpen())
            {
                std::cout << "Error: Could not open file " << file_path << std::endl;
                return 1;
            }
            std::unique_ptr<TH1> hist{dynamic_cast<TH1 *>(file.Get(plot_name.c_str()))};
            if (!hist)
            {
                std::cout << "Error: Could not get histogram " << plot_name << " from file " << file_path << std::endl;
                return 1;
            }
            hist->Scale(scale / rebin_factor);
            if (rebin_factor != 1)
                hist = std::unique_ptr<TH1>{dynamic_cast<TH1 *>(hist->Rebin(rebin_factor)->Clone())};
            hist->GetXaxis()->SetRangeUser(0, uplimit);
            std::cout << "plotting " << plot_name << " from file " << file_path << " scale " << scale << " rebin with " << rebin_factor << std::endl;
            max = std::max(max, hist->GetMaximum());
            ResetStyle(hist, canvas->GetPad(0));
            entries.emplace_back(legend, std::move(hist));
        }
        if (config.contains("misc"))
        {
            const std::map<std::string, std::function<std::unique_ptr<TObject>(const nlohmann::json &)>> misc_objects_maker{
                {"TLine", [](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto line = std::make_unique<TLine>(config["x1"], config["y1"], config["x2"], config["y2"]);
                     line->SetLineColor(config.value("color", kBlack));
                     line->SetLineWidth(config.value("width", 1));
                     line->SetLineStyle(config.value("style", line->GetLineStyle()));
                     return line;
                 }},
                {"VLine", [&max](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto line = std::make_unique<TLine>(config["x"], 0, config["x"], max * 1.1);
                     line->SetLineColor(config.value("color", kBlack));
                     line->SetLineWidth(config.value("width", 1));
                     line->SetLineStyle(config.value("style", line->GetLineStyle()));
                     return line;
                 }},
                {"HLine", [&uplimit](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto line = std::make_unique<TLine>(0, config["y"], uplimit, config["y"]);
                     line->SetLineColor(config.value("color", kBlack));
                     line->SetLineWidth(config.value("width", 1));
                     line->SetLineStyle(config.value("style", line->GetLineStyle()));
                     return line;
                 }},
                {"TMarker", [](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto marker = std::make_unique<TMarker>(config["x"], config["y"], config.value("marker", 20));
                     marker->SetMarkerColor(config.value("color", kBlack));
                     marker->SetMarkerSize(config.value("size", 1));
                     return marker;
                 }},
                {"TText", [](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto text = std::make_unique<TText>(config["x"], config["y"], config["text"].get<std::string>().c_str());
                     text->SetTextSize(config.value("size", 0.05));
                     text->SetNDC(true);
                     text->SetTextColor(config.value("color", kBlack));
                     text->SetTextFont(config.value("font", 42));
                     text->SetTextAlign(config.value("align", 22));
                     return text;
                 }},
                {"TLatex", [](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto latex = std::make_unique<TLatex>(config["x"], config["y"], config["text"].get<std::string>().c_str());
                     latex->SetTextSize(config.value("size", 0.05));
                     latex->SetTextColor(config.value("color", kBlack));
                     latex->SetTextFont(config.value("font", 42));
                     latex->SetTextAlign(config.value("align", 22));
                     return latex;
                 }},
                {"TArrow", [](const nlohmann::json &config) -> std::unique_ptr<TObject>
                 {
                     auto arrow = std::make_unique<TArrow>(config["x1"], config["y1"], config["x2"], config["y2"], config.value("arrowsize", 0.1), config.value("arrowstyle", ">").c_str());
                     arrow->SetLineColor(config.value("color", kBlack));
                     arrow->SetLineWidth(config.value("width", 1));
                     arrow->SetLineStyle(config.value("style", arrow->GetLineStyle()));
                     return arrow;
                 }}};

            for (const auto &entry : config["misc"])
            {
                objects.emplace_back(misc_objects_maker.at(entry["type"])(entry));
            }
        }
    }

    std::array<int, 10> col{kRed, kGreen, kBlue, kMagenta, kCyan,
                            kOrange, kViolet, kGray, kYellow, kBlack};
    auto leg = config.contains("legend_place") ? std::make_unique<TLegend>(config["legend_place"]["x1"], config["legend_place"]["y1"], config["legend_place"]["x2"], config["legend_place"]["y2"]) : std::make_unique<TLegend>(.7, .7, .9, .9);
    ResetStyle(leg);
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        auto &[legend_title, hist] = entries[i];
        hist->SetLineColor(col[i]);
        hist->SetLineWidth(1);
        leg->AddEntry(hist.get(), legend_title.c_str(), "l");
        const auto draw_opt = config.value("draw_opt", "hist C");
        const auto opt = i == 0 ? draw_opt : draw_opt + " same";
        hist->SetTitle(plot_title.c_str());
        hist->SetMaximum(max * 1.1);
        hist->SetMinimum(0);
        hist->Draw(opt.c_str());
    }
    for (const auto &object : objects)
        object->Draw("same");
    leg->Draw();
    for (const std::string &output_name : config["output_names"])
    {
        canvas->SaveAs(output_name.c_str());
    }

    return 0;
}
