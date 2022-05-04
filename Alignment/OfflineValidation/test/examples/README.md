# Examples

## Full JetHT analysis example using semi-automatic CRAB implementation

There is a possibility to run the analysis using CRAB. Currently the implementation for this is semi-automatic, meaning that the All-In-One tool provides you all the necessary configuration, but you will need to manually run the jobs and make sure all the jobs are finished successfully.

Before starting, make sure your have voms proxy available:

```
voms-proxy-init --voms cms
```

To begin, create the configuration using the All-In-One tool. It is important that you will do a dry-run:

```
validateAlignments.py -d jetHtAnalysis_fullExampleConfiguration.json
```

Move to the created directory with the configuration files:

```
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/examples/example_json_jetHT/JetHT/single/fullExample/prompt
```

Check that you have write access to the default output directory used by the CRAB. By default the shared EOS space of the tracker validation group at CERN is used.

```
crab checkwrite --site=T2_CH_CERN --lfn=/store/group/alca_trackeralign/`whoami`
```

At this point, you should also check the running time and memory usage. Running over one file can take up to 2 hours and you will need about 1000 MB of RAM for that. So you should set the corresponding variables to values slightly above these.

```
vim crabConfiguration.py
...
config.Data.outLFNDirBase = '/store/group/alca_trackeralign/username/' + config.General.requestName
...
config.JobType.maxMemoryMB = 1200
config.JobType.maxJobRuntimeMin = 200
...
config.Data.unitsPerJob = 1
...
config.Site.storageSite = 'T2_CH_CERN'
```

After checking the configuration, submit the jobs.

```
crab submit -c crabConfiguration.py
```

Do the same for ReReco and UltraLegacy folders:

```
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/examples/example_json_jetHT/JetHT/single/fullExample/rereco
crab submit -c crabConfiguration.py
...
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/examples/example_json_jetHT/JetHT/single/fullExample/ultralegacy
crab submit -c crabConfiguration.py
```

Now you need to wait for the crab jobs to finish. It should take around two hours times the value you set for unitsPerJob variable for these example runs. After the jobs are finished, you will need to merge the output files and transfer the merged files to the correct output folders. One way to do this is as follows:

```
cd $CMSSW_BASE/src/JetHtExample/example_json_jetHT/JetHT/merge/fullExample/prompt
hadd -ff JetHTAnalysis_merged.root `xrdfs root://eoscms.cern.ch ls -u /store/group/alca_trackeralign/username/path/to/prompt/files | grep '\.root'`
cd $CMSSW_BASE/src/JetHtExample/example_json_jetHT/JetHT/merge/fullExample/rereco
hadd -ff JetHTAnalysis_merged.root `xrdfs root://eoscms.cern.ch ls -u /store/group/alca_trackeralign/username/path/to/rereco/files | grep '\.root'`
cd $CMSSW_BASE/src/JetHtExample/example_json_jetHT/JetHT/merge/fullExample/ultralegacy
hadd -ff JetHTAnalysis_merged.root `xrdfs root://eoscms.cern.ch ls -u /store/group/alca_trackeralign/username/path/to/ultralegacy/files | grep '\.root'`
```

For 100 files, the merging will take about one to two minutes. Now all the files are merged, the only thing that remains is to plot the results. To do this, navigate to the folder where the plotting configuration is located and run it:

```
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/examples/example_json_jetHT/JetHT/plot/fullExample
jetHtPlotter validation.json
```

The final validation plots appear to the output folder. If you want to change the style of the plots or the histograms plotted, you can edit the validation.json file here and rerun the plotter. No need to redo the time-consuming analysis part.

## Full example using condor

The CRAB running is recommended for large datasets, but smaller tests can also be readily done with condor. For condor running, the same exmaple configuration file is set up to analyze 1000 events from each file. You can run everything with the command:

```
validateAlignments.py -j espresso jetHtAnalysis_fullExampleConfiguration.json
```

Then you just wait for your jobs to get submitted, and soon afterwards the plots will appear in the folder

```
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/examples/example_json_jetHT/JetHT/plot/fullExample/output
```

## jetHt_multiYearTrendPlot.json

This configuration shows how to plot multi-year trend plots using previously merged jetHT validation files. It uses the jetHT plotting macro standalone. You can run this example using

```
jetHtPlotter jetHt_multiYearTrendPlot.json
```

## jetHt_ptHatWeightForMCPlot.json

This configuration shows how to apply ptHat weight for MC files produced with different ptHat cuts. What you need to do is to collect the file names and lower boundaries of the ptHat bins into a file, which is this case is ptHatFiles_MC2018_PFJet320.txt. For a file list like this, the ptHat weight is automatically applied by the code. The weights are correct for run2. The plotting can be done using the jetHT plotter standalone:

```
jetHtPlotter jetHt_ptHatWeightForMCPlot.json
```
