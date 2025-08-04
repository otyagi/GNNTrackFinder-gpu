# QA-checker framework

## Introduction 

The framework provides tools to compare histograms in ROOT-files, which were created under different conditions
(code, parameter or geometry versions). Within a single routine, multiple data sets (e.g., different detector 
setups), multiple ROOT-files and multiple versions can be processed. The output ROOT-file comprises the histograms 
from all required variants and optionally the comparison canvases.

## Comparison Methods

The framework proposes three different comparison methods: exact equality check, ratio check and $\chi^2$-test. 
The exact equality check provides a value and error equality check for each bin of the two histograms (including
under- and overflows). The ratio check estimates the minimal and maximal ratio of the two values among the histogram
bins and tests, if they stay in the defined acceptable range. The $\chi^2$-test runs a standard
routine for ROOT histograms: `TH1::Chi2TestX()` and returns the p-value, which is then compared with a threshold.

A user can combine these methods for achieving a final inference. If the exact equality check is applied, and if
according to it the two histograms are equal, other checks are not executed, even if they are required by the 
configuration. 

## Interface and Configuration

An interaction between a user and the framework is carried out using the interface of the `cbm::qa::checker::Core`
class. A recommended structure of the ROOT-macro is the following:

```
/* clang-format off */
int check_my_histograms(
  const char* configName = "./my_config.yaml",
  const char* outputName = "./checks_output.root"
)
/* clang-format on */
{
  // Creating an instance of the checker
  auto pChecker = std::make_unique<cbm::qa::checker::Core>();

  // Configuring the instance of the checker
  pChecker->RegisterOutFile(outputName);
  pChecker->SetFromYaml(configName);

  // Additional configuration functions such as pChecker->AddVersion() or pChecker->SetPvalThreshold() can be
  // called here.

  // Executes the comparison routine
  int res = pChecker->Process("ESR");  // process exact comparison and chi2-test, draw the ratio on canvas
  return res;
}
```

The selection of the comparison methods as well as of canvas handling options is performed via an option string
parameter of the `Core::Process()` function. This works in a similar manner as for many ROOT classes. The following
options are supported at the moment:

| Option | Meaning                                                         |
|--------|-----------------------------------------------------------------|
| 'E'    | enable exact comparison                                         |
| 'S'    | enable $\chi^2$-test                                            |
| 'U'    | enable ratio comparison                                         |
| 'B'    | suppress comparison canvas creation                             |
| 'F'    | force canvas creation (even if the two histograms are the same) |
| 'D'    | draw difference on canvas (only for 1D-histograms)              |
| 'R'    | draw ratio on canvas (only for 1D-histograms)                   |


The configuration of the routine requires:

-  definition of version list (at least two versions);
-  definition of dataset list (at least one dataset);
-  definition of ROOT-files list (at least one file).

This can be done using the YAML-configuration file and partially using the `Core` class functions. 

### Version list configuration

Each version requires a label and a path. The label is used to distinguish histograms from different code versions
inside the output file as well as for legends in the comparison canvases. The path is used for the definition of the
input file. The list of versions can be provided either via `Core::AddVersion(label, path)` function, or via the 
node `versions` in the YAML. For example,

```
checker:
  versions:
  - label: "master"
    path: "master_output"
  - label: "feature"
    path: "feature_output"
```

At least two versions must be provided. The default version can be assigned explicitly via the 
`Core::SetDefaultVersion(label)` or in the configuration file:

```
checker:
  default_label: "master"
```

If the default version was not defined explicitly, the first version in the list is assigned to be the default one.

### Dataset list configuration

Each dataset provides a label only. The label is used to distinguish input files and the histograms in the output. 
One can provide a list of dataset labels either via the `Core::AddDataset(label)`, or in the configuration file. For example:

```
checker:
  datasets:
    - mcbm_beam_2022_05_23_nickel
    - mcbm_beam_2024_05_08_nickel
```

At least one dataset must be provided. 

### File and object list configuration

The file and object lists can be set only via the YAML configuration. For each file its name and path in the
filesystem are required. The name of the file is used as name of the directory in the output ROOT-file. To
distinguish between different versions and datasets, one can use specifiers %v and %d, which will be replaced a
version path and a dataset label respectively. 

Let's consider an example. We want to compare QA-output for two different mCBM setups (mcbm_beam_2022_05_23_nickel
and mcbm_beam_2024_05_08_nickel) for two different CbmRoot branches (let's say "master" and "feature"). Let the
QA-files being stored on disk under the following names:

| version | setup                       | path                                                                                         |
|---------|-----------------------------|----------------------------------------------------------------------------------------------|
| master  | mcbm_beam_2022_05_23_nickel | /path/to/master_output/mcbm_beam_2022_05_23_nickel/data/mcbm_beam_2022_05_23_nickel.qa.root  |
| master  | mcbm_beam_2024_05_08_nickel | /path/to/master_output/mcbm_beam_2024_05_23_nickel/data/mcbm_beam_2024_05_08_nickel.qa.root  |
| feature | mcbm_beam_2022_05_23_nickel | /path/to/feature_output/mcbm_beam_2022_05_23_nickel/data/mcbm_beam_2022_05_23_nickel.qa.root |
| feature | mcbm_beam_2024_05_08_nickel | /path/to/feature_output/mcbm_beam_2024_05_08_nickel/data/mcbm_beam_2024_05_08_nickel.qa.root |

According to the pattern of the qa.root files, the filename should be provided as follows:
"/path/to/**%v**/**%d**/data/**%d**.qa.root". Please note, that the file name can contain multiple version and
dataset specifiers.

A user can also provide a list of histograms, which should be compared. It is useful, if only particular histograms
from a heavy ROOT-file are of interest. If the histogram list for a particular input file is not provided, the 
framework will scan all the versions of this file in all the defined datasets and collect a full list of
histograms. If the list of histograms diverges for different datasets, the absent ones will be just skipped.

Below is an example of the configuration file:

```
checker:
  files:
    - name: "/path/to/%v/%d/data/%d.qa.root"
      label: "qa"
      objects:  # optional node
        - CbmCaInputQaSts/efficiencies/casts_reco_eff_vs_r_st0
        - CbmCaInputQaSts/efficiencies/casts_reco_eff_vs_xy_st0
        - CbmCaInputQaSts/histograms/casts_pull_t_st2
        - CbmCaInputQaSts/histograms/casts_res_x_vs_x_st2   
```

### Comparison settings

At the moment, all the comparison settings are global for all processed objects. The settings include the range
of accepted histogram ratio and a p-value threshold for the chi2-test. These parameters can be specified either via 
`Core` class interface (methods `Core::SetRatioRange(min, max)` and `Core::SetPvalThreshold(pValMin)`), or in the
configuration file:

```
checker:
  settings:
    ratio_min: 0.9
    ratio_max: 1.1
    pval_threshold: 0.01
```

If these values are not defined, the default ones will be used (ratio_min = 0.95, ratio_max = 1.05, 
pval_threshold = 0.05).

### Full YAML-config example

The full configuration file will be as follows:

```
checker:
  settings:
    ratio_min: 0.9
    ratio_max: 1.1
    pval_threshold: 0.01
  versions:
    - label: "master"
      path: "master_output"
    - label: "feature"
      path: "feature_output"
  default_label: "master"
  datasets:
    - mcbm_beam_2022_05_23_nickel
    - mcbm_beam_2024_05_08_nickel
  files:
    - name: "/path/to/%v/%d/data/%d.qa.root"
      label: "qa"
      objects:  # optional node
        - CbmCaInputQaSts/efficiencies/casts_reco_eff_vs_r_st0
        - CbmCaInputQaSts/efficiencies/casts_reco_eff_vs_xy_st0
        - CbmCaInputQaSts/histograms/casts_pull_t_st2
        - CbmCaInputQaSts/histograms/casts_res_x_vs_x_st2   

```

## Framework Inference

For each particular compared object withing two different versions an inference on the equality is produced. The 
inference can have one of three following values:

| Inference | Label      | Meaning                                                                      |
|-----------|------------|------------------------------------------------------------------------------|
| 0         | same       | All the comparison methods showed equality                                   |
| 1         | consistent | At least one of the comparison methods showed equality, but at least one did |
| 2         | different  | No of the comparison methods showed equality                                 |

The final inference of the routine returns the maximum inference value among all the objects for all the processed
files.


## Notes

Please note, that the framework accepts **only** the ROOT-files, which have an a directory system built on the
`TDirectory` classes. The files with the `TFolder`-based directory system is not supported. 


