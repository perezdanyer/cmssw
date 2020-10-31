import copy
import os

def PV(config, validationDir):
    ##List with all jobs
    jobs = []
    PVType = "single"

    ##List with all wished IOVs
    IOVs = []

    ##Start with single PV jobs
    if not PVType in config["validations"]["PV"]: 
        raise Exception("No 'single' key word in config for PV") 

    for datasetName in config["validations"]["PV"][PVType]:
        for IOV in config["validations"]["PV"][PVType][datasetName]["IOV"]:
            ##Save IOV to loop later for merge jobs
            if not IOV in IOVs:
                IOVs.append(IOV)

            for alignment in config["validations"]["PV"][PVType][datasetName]["alignments"]:
                ##Work directory for each IOV
                workDir = "{}/PV/{}/{}/{}/{}".format(validationDir, PVType, datasetName, alignment, IOV)

                ##Write local config
                local = {}
                local["output"] = "{}/{}/{}/{}/{}/{}".format(config["LFS"], config["name"], PVType, alignment, datasetName, IOV)
                local["alignment"] = copy.deepcopy(config["alignments"][alignment])
                local["validation"] = copy.deepcopy(config["validations"]["PV"][PVType][datasetName])
                local["validation"].pop("alignments")
                local["validation"]["IOV"] = IOV
                if "goodlumi" in local["validation"]:
                    local["validation"]["goodlumi"] = local["validation"]["goodlumi"].format(IOV)

                ##Write job info
                job = {
                    "name": "PV_{}_{}_{}_{}".format(PVType, alignment, datasetName, IOV),
                    "dir": workDir,
                    "exe": "cmsRun",
                    "cms-config": "{}/src/Alignment/OfflineValidation/python/TkAlAllInOneTool/PV_cfg.py".format(os.environ["CMSSW_BASE"]),
                    "run-mode": "Condor",
                    "dependencies": [],
                    "config": local, 
                }

                jobs.append(job)

    ##Do merge PV if wished
    if "merge" in config["validations"]["PV"]:
        ##List with merge jobs, will be expanded to jobs after looping
        mergeJobs = []
        PVType = "merge"

        ##Loop over all merge jobs/IOVs which are wished
        for mergeName in config["validations"]["PV"][PVType]:
            for IOV in IOVs:
                ##Work directory for each IOV
                workDir = "{}/PV/{}/{}/{}".format(validationDir, PVType, mergeName, IOV)

                ##Write job info
                local = {}

                job = {
                    "name": "PV_{}_{}_{}".format(PVType, mergeName, IOV),
                    "dir": workDir,
                    "exe": "PVmerge",
                    "run-mode": "Condor",
                    "dependencies": [],
                    "config": local, 
                }

                for alignment in config["alignments"]:
                    ##Deep copy necessary things from global config
                    local.setdefault("alignments", {})
                    local["alignments"][alignment] = copy.deepcopy(config["alignments"][alignment])
                    local["validation"] = copy.deepcopy(config["validations"]["PV"][PVType][mergeName])
                    local["output"] = "{}/{}/{}/{}/{}".format(config["LFS"], config["name"], PVType, mergeName, IOV)

                ##Loop over all single jobs
                for singleJob in jobs:
                    ##Get single job info and append to merge job if requirements fullfilled
                    alignment, datasetName, singleIOV = singleJob["name"].split("_")[2:]    

                    if int(singleIOV) == IOV and datasetName in config["validations"]["PV"][PVType][mergeName]["singles"]:
                        local["alignments"][alignment]["file"] = singleJob["config"]["output"]
                        job["dependencies"].append(singleJob["name"])
                        
                mergeJobs.append(job)

        jobs.extend(mergeJobs)

    return jobs
