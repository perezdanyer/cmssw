def GCP(config, validationDir):
    ##List with all jobs
    jobs = []

    for comparison in config["validations"]["GCP"]:
        for IOV in config["validations"]["GCP"][comparison]["IOV"]:
            ##Create directory for each IOV and comparison you want to make
            workDir = "{}/GCP/{}/{}".format(validationDir, comparison, IOV)

            ##Write local config
            local = {}
            local["output"] = "{}/{}".format(config["LFS"], workDir)
            local["aligments"] = config["alignments"]
            local["validation"] = config["validations"]["GCP"][comparison]
            local["validation"]["IOV"] = IOV

            ##Write job info
            job = {
                "name": "GCP_{}_{}".format(comparison, IOV),
                "dir": workDir,
                "exe": "GCP",
                "run-mode": "Condor",
                "dependencies": [],
                "config": local, 
            }

            jobs.append(job)

    return jobs
