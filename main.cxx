#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TSystem.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <tools.h>
int main(int argc, char const *argv[])
{
    gStyle->SetOptStat(0);
    gSystem->ResetSignal(kSigBus);
    gSystem->ResetSignal(kSigSegmentationViolation);
    gSystem->ResetSignal(kSigIllegalInstruction);
    TH1::AddDirectory(kFALSE);
    if (argc != 3)
    {
        std::cout << "Usage: ./main <config_file> <output_prefix>" << std::endl;
        return 1;
    }
    std::string plot_title{};
    // std::vector<std::string> legends{}, plot_names{};
    // std::vector<std::unique_ptr<TH1D>> histos{};
    using entry_t = std::pair<std::string, std::unique_ptr<TH1D>>;
    std::string output_prefix = argv[2];
    double max{};
    std::vector<entry_t> entries{};
    {
        std::ifstream config_file{argv[1]};
        if (!config_file.is_open())
        {
            std::cout << "Error: Could not open config file" << std::endl;
            return 1;
        }
        auto getline = [&config_file]() -> std::string
        {
            std::string line{};
            while (true)
            {
                if (!std::getline(config_file, line))
                {
                    break;
                }

                if (line.empty())
                    continue;
                if (line[0] == '#')
                    continue;
                if (line[0] == ' ')
                    continue;
                break;
            }
            return line;
        };
        auto get_input = [&getline]() -> std::vector<std::string>
        {
            auto line = getline();
            std::vector<std::string> input{};
            if (line.empty())
                return input;
            for (auto pos = line.find_first_of('|'); pos != std::string::npos; pos = line.find_first_of('|'))
            {
                input.push_back(line.substr(0, pos));
                line = line.substr(pos + 1);
            }
            input.push_back(line);
            return input;
        };
        plot_title = getline();
        auto uplimit = std::stod(getline());
        while (config_file.eof() == false)
        {
            auto input = get_input();
            if (input.empty())
                continue;
            if (input.size() < 3)
            {
                std::cout << "Error: Invalid input, for line:" << std::endl;
                std::cout << input[0] << std::endl;
                return 1;
            }
            auto &legend = input[0];
            auto &file_path = input[1];
            auto &plot_name = input[2];
            double scale = 1.;
            if (input.size() > 3)
                scale = std::stod(input[3]);
            TFile file{file_path.c_str(), "READ"};
            if (!file.IsOpen())
            {
                std::cout << "Error: Could not open file " << file_path << std::endl;
                return 1;
            }
            std::unique_ptr<TH1D> hist{dynamic_cast<TH1D *>(file.Get(plot_name.c_str()))};
            if (!hist)
            {
                std::cout << "Error: Could not get histogram " << plot_name << " from file " << file_path << std::endl;
                return 1;
            }
            hist->Scale(scale);
            hist->GetXaxis()->SetRangeUser(0, uplimit);
            max = std::max(max, hist->GetMaximum());
            entries.emplace_back(legend, std::move(hist));
            file.Close();
        }
        {
            std::array<int, 10> col{kRed, kGreen, kBlue, kMagenta, kCyan,
                                    kOrange, kViolet, kGray, kYellow, kBlack};
            auto canvas = getCanvas();
            auto leg = std::make_unique<TLegend>();
            for (std::size_t i = 0; i < entries.size(); ++i)
            {
                auto &[legend_title, hist] = entries[i];
                ResetStyle(hist, canvas->GetPad(0));
                hist->SetLineColor(col[i]);
                hist->SetLineWidth(1);
                leg->AddEntry(hist.get(), legend_title.c_str(), "l");
                auto opt = i == 0 ? "hist" : "hist same";
                hist->SetTitle(plot_title.c_str());
                hist->SetMaximum(max * 1.1);
                hist->Draw(opt);
            }
            leg->Draw();
            canvas->SaveAs((output_prefix + ".pdf").c_str());
            canvas->SaveAs((output_prefix + ".png").c_str());
        }
    }
    return 0;
}
