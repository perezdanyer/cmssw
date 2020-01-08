# Validation

We use the Boost library (Program Options, Filesystem & Property Trees) to deal with the treatment of the config file.
Basic idea:
 - a generic config file is "projected" for each validation (*e.g.* the geometry is changed, together with the plotting style);
 - for each config file, a new condor config file is produced;
 - a DAGMAN file is also produced in order to submit the whole validation at once.

In principle, the `validateAlignment` command is enough to submit everything.
However, for local testing, one may want to make a dry run: all files will be produced, but the condor jobs will not be submitted;
then one can just test locally any step, or modify any parameter before simply submitting the DAGMAN.

## TODO list

 - exceptions handling (filesystem + own)
   - check inconsistencies in config file
 - other executables
   - start with GCP and DMRs
   - soft links to root files
 - documentation
   - tutorial
   - instructions for developers
 - port to CMSSW

## Tutorial


## Instructions for developers

