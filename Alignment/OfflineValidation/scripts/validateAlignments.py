#!/usr/bin/env python
#test execute: export CMSSW_BASE=/tmp/CMSSW && ./validateAlignments.py -c defaultCRAFTValidation.ini,test.ini -n -N test
from __future__ import print_function
from future.utils import lmap
import subprocess
import json
import yaml
import os
import argparse
import pprint
import sys

import Alignment.OfflineValidation.TkAlAllInOneTool.GCP as GCP
import Alignment.OfflineValidation.TkAlAllInOneTool.DMR as DMR

def parser():
    parser = argparse.ArgumentParser(description = "AllInOneTool for validation of the tracker alignment", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("config", metavar='config', type=str, action="store", help="Global AllInOneTool config (json/yaml format)")
    parser.add_argument("-d", "--dry", action = "store_true", help ="Set up everything, but don't run anything")
    parser.add_argument("-v", "--verbose", action = "store_true", help ="Enable standard output stream")
    parser.add_argument("-e", "--example", action = "store_true", help ="Print example of config in JSON format")
    parser.add_argument("-f", "--force", action = "store_true", help ="Force creation of enviroment, possible overwritten old configuration")

    return parser.parse_args()

def main():
    ##Read parser arguments
    args = parser()

    ##Print example config which is in Aligment/OfflineValidation/bin if wished
    if args.example:
        with open("{}/src/Alignment/OfflineValidation/bin/example.yaml".format(os.environ["CMSSW_BASE"]), "r") as exampleFile:
            config = yaml.load(exampleFile, Loader=yaml.Loader)
            pprint.pprint(config, width=30)
            sys.exit(0)    

    ##Read in AllInOne config dependent on what format you choose
    with open(args.config, "r") as configFile:
        if args.verbose:
            print("Read AllInOne config: '{}'".format(args.config))

        if args.config.split(".")[-1] == "json":
            config = json.load(configFile)

        elif args.config.split(".")[-1] == "yaml":
            config = yaml.load(configFile, Loader=yaml.Loader)

        else:
            raise Exception("Unknown config extension '{}'. Please use json/yaml format!".format(args.config.split(".")[-1])) 
        
    ##Create working directory
    if os.path.isdir(config["name"]) and not args.force:
	raise Exception("Validation directory '{}' already exists! Please choose another name for your directory.".format(config["name"]))	

    validationDir = os.path.abspath(config["name"])
    exeDir = "{}/executables".format(validationDir)

    subprocess.call(["mkdir", "-p", validationDir] + ((["-v"] if args.verbose else [])))
    subprocess.call(["mkdir", "-p", exeDir] + (["-v"] if args.verbose else []))

    ##Copy AllInOne config in working dir in json/yaml format
    subprocess.call(["cp", "-f", args.config, validationDir] + (["-v"] if args.verbose else []))

    ##List with all jobs
    jobs = []

    ##Check in config for all validation and create jobs
    for validation in config["validations"]:
        if validation == "GCP":
            jobs.extend(GCP.GCP(config, validationDir))

        elif validation == "DMR":
            jobs.extend(DMR.DMR(config, validationDir))

        else:
            raise Exception("Unknown validation method: {}".format(validation)) 
            
    ##Create dir for DAG file and loop over all jobs
    subprocess.call(["mkdir", "-p", "{}/DAG/".format(validationDir)] + (["-v"] if args.verbose else []))

    with open("{}/DAG/dagFile".format(validationDir), "w") as dag:
        for job in jobs:
            ##Create job dir, output dir
            subprocess.call(["mkdir", "-p", job["dir"]] + (["-v"] if args.verbose else []))
            subprocess.call(["mkdir", "-p", job["config"]["output"]] + (["-v"] if args.verbose else []))
            subprocess.call(["ln", "-fs", job["config"]["output"], "{}/output".format(job["dir"])] + (["-v"] if args.verbose else []))

            ##Create symlink for executable/python cms config if needed
            subprocess.call("cp -f $(which {}) {}".format(job["exe"], exeDir) + (" -v" if args.verbose else ""), shell = True)
            subprocess.call(["ln", "-fs", "{}/{}".format(exeDir, job["exe"]), job["dir"]] + (["-v"] if args.verbose else []))
            if "cms-config" in job:
                subprocess.call(["ln", "-fs", job["cms-config"], "{}/validation_cfg.py".format(job["dir"])] + (["-v"] if args.verbose else []))

            ##Write local config file 
            with open("{}/validation.json".format(job["dir"]), "w") as jsonFile:
                if args.verbose:
                    print("Write local json config: '{}'".format("{}/validation.json".format(job["dir"])))           

                json.dump(job["config"], jsonFile, indent=4)

            with open("{}/validation.yaml".format(job["dir"]), "w") as yamlFile:
                if args.verbose:
                    print("Write local yaml config: '{}'".format("{}/validation.yaml".format(job["dir"])))     

                yaml.dump(job["config"], yamlFile, default_flow_style=False, width=float("inf"), indent=4)

            ##Write shell executable use in condor job
            with open("{}/run.sh".format(job["dir"]), "w") as runFile:
                if args.verbose:
                    print("Write shell executable: '{}'".format("{}/run.sh".format(job["dir"])))

                runContent = [
                    "#!/bin/bash",
                    "cd $CMSSW_BASE/src",
                    "source /cvmfs/cms.cern.ch/cmsset_default.sh",
                    "eval `scram runtime -sh`",
                    "cd {}".format(job["dir"]),
                    "./{} {}validation.json".format(job["exe"], "validation_cfg.py config=" if "cms-config" in job else ""),
                ]

                for line in runContent:
                    runFile.write(line + "\n")

            subprocess.call(["chmod", "a+rx", "{}/run.sh".format(job["dir"])] + (["-v"] if args.verbose else []))

            ##Write condor submit file
            with open("{}/condor.sub".format(job["dir"]), "w") as subFile:
                if args.verbose:
                    print("Write condor submit: '{}'".format("{}/condor.sub".format(job["dir"])))

                subContent = [
                    "universe = vanilla",
                    "getenv = true",
                    "executable = run.sh",
                    "output = condor.out",
                    "error  = condor.err",
                    "log    = condor.log",
                    'requirements = (OpSysAndVer =?= "CentOS7")',
                   # '+JobFlavour = "espresso"',
                    '+AccountingGroup = "group_u_CMS.CAF.ALCA"',
                    "queue"
                ]

                for line in subContent:
                    subFile.write(line + "\n")

            ##Write command in dag file
            dag.write("JOB {} condor.sub DIR {}\n".format(job["name"], job["dir"]))

            if job["dependencies"]:
                dag.write("\n")
                dag.write("PARENT {} CHILD {}".format(" ".join(job["dependencies"]), job["name"]))

            dag.write("\n\n")

    if args.verbose:
        print("DAGman config has been written: '{}'".format("{}/DAG/dagFile".format(validationDir)))            

    ##Call submit command if not dry run
    if args.dry:
        print("Enviroment is set up. If you want to submit everything, call 'condor_submit_dag {}/DAG/dagFile'".format(validationDir))

    else:
        subprocess.call(["condor_submit_dag", "{}/DAG/dagFile".format(validationDir)])
        
if __name__ == "__main__":
    main()
