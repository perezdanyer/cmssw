#!/usr/bin/env python
#test execute: export CMSSW_BASE=/tmp/CMSSW && ./validateAlignments.py -c defaultCRAFTValidation.ini,test.ini -n -N test
from __future__ import print_function
from future.utils import lmap
import subprocess
import json
import yaml
import os
import argparse
import Alignment.OfflineValidation.TkAlAllInOneTool.GCP as GCP
import Alignment.OfflineValidation.TkAlAllInOneTool.DMR as DMR

def parser():
    parser = argparse.ArgumentParser(description = "AllInOneTool for validation of the tracker alignment", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("config", metavar='config', type=str, action="store", help="Global AllInOneTool config (json/yaml format)")

    return parser.parse_args()

def main():
    ##Read parser arguments
    args = parser()

    ##Read in AllInOne config dependent on what format you choose
    with open(args.config, "r") as configFile:
        if args.config.split(".")[-1] == "json":
            config = json.load(configFile)

        elif args.config.split(".")[-1] == "yaml":
            config = yaml.load(configFile, Loader=yaml.Loader)

        else:
            raise Exception("Unknown config extension '{}'. Please use json/yaml format!".format(args.config.split(".")[-1])) 
        
    ##Create working directory
    validationDir = "{}/src/{}".format(os.environ["CMSSW_BASE"], config["name"])
    exeDir = "{}/executables".format(validationDir)

    binDir = "{}/bin/{}".format(os.environ["CMSSW_BASE"], os.environ["SCRAM_ARCH"])
    subprocess.call(["mkdir", "-p", validationDir])
    subprocess.call(["mkdir", "-p", exeDir])

    ##List with all jobs
    jobs = []

    ##Check in config for all validation and create jobs
    for validation in config["validations"]:
        if validation == "GCP":
            jobs.extend(GCP.GCP(config, validationDir))
            subprocess.call(["cp", "-f", "{}/GCP".format(binDir), exeDir])

        elif validation == "DMR":
            jobs.extend(DMR.DMR(config, validationDir))
            subprocess.call(["cp", "-f", "{}/DMRsingle".format(binDir), exeDir])
            subprocess.call(["cp", "-f", "{}/DMRmerge".format(binDir), exeDir])

        else:
            raise Exception("Unknown validation method: {}".format(validation)) 
            
    ##Create dir for DAG file and loop over all jobs
    subprocess.call(["mkdir", "-p", "{}/DAG/".format(validationDir)])

    with open("{}/DAG/dagFile".format(validationDir), "w") as dag:
        for job in jobs:
            ##Create job dir and create symlink for executable
            subprocess.call(["mkdir", "-p", job["dir"]])
            subprocess.call(["ln", "-sf", "{}/{}".format(exeDir, job["exe"]), job["dir"]])

            ##Write local config file
            with open("{}/validation.json".format(job["dir"]), "w") as jsonFile:
                json.dump(job["config"], jsonFile, indent=4)

            ##Copy condor.sub into job directory
            defaultSub = "{}/src/Alignment/OfflineValidation/bin/.default.sub".format(os.environ["CMSSW_BASE"])
            subprocess.call(["cp", "-f", defaultSub, "{}/condor.sub".format(job["dir"])])

            ##Write command in dag file
            dag.write("JOB {} condor.sub\n".format(job["name"]))
            dag.write("DIR {} \n".format(job["dir"]))
            dag.write('VARS {} exec="{}"\n'.format(job["name"], job["exe"]))

            if job["dependencies"]:
                dag.write("PARENT {} CHILD {} \n".format(job["name"], " ".join(job["dependencies"])))

            dag.write("\n")

if __name__ == "__main__":
    main()
