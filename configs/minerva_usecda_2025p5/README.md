# MINERvA useCdA comparison

These configurations compare the GiBUU 2025p5 `useCdA=F` and `useCdA=T`
predictions with the MINERvA data histograms written by the TKI analysis.
The absolute covariance-matrix chi-square is included in each legend.

On the CC cluster, build `merge_plot`, then run all four configurations with:

```sh
for config in configs/minerva_usecda_2025p5/*.json; do
  build/merge_plot "$config"
done
```

Plots are written as PNG, PDF and SVG under the TKI production comparison
directory:

```text
/sps/juno/yqiyu/GiBUUGEN/minerva_useCdA_2025p5/minerva_usecda_2025p5/comparison/merge_plot/
```
