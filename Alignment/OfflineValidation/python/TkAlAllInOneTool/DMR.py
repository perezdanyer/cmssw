import copy

def DMR(config, validationDir):
    ##List with all jobs
    jobs = []

    for dmrType in config["validations"]["DMR"]:
        if dmrType == "single":
            for validation in config["validations"]["DMR"][dmrType]:
                for IOV in config["validations"]["DMR"][dmrType][validation]["IOV"]:
                    for alignment in config["validations"]["DMR"][dmrType][validation]["alignments"]:
                        ##Work directory for each IOV
                        workDir = "{}/DMR/{}/{}/{}/{}".format(validationDir, dmrType, validation, alignment, IOV)

                        ##Write local config
                        local = {}
                        local["output"] = "{}/{}".format(config["LFS"], workDir)
                        local["alignment"] = copy.deepcopy(config["alignments"][alignment])
                        local["validation"] = copy.deepcopy(config["validations"]["DMR"][dmrType][validation])
                        local["validation"].pop("alignments")
                        local["validation"]["IOV"] = IOV

                        ##Write job info
                        job = {
                            "name": "DMR_{}_{}_{}_{}".format(dmrType, alignment, validation, IOV),
                            "dir": workDir,
                            "exe": "DMRsingle",
                            "run-mode": "Condor",
                            "dependencies": [],
                            "config": local, 
                        }

                        jobs.append(job)

    return jobs
