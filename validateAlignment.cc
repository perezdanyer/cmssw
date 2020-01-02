#include <cstdlib>
#include <iostream>

#include "exceptions.h"
#include "Options.h"
#include "DAG.h"

using namespace std;
using namespace AllInOneConfig;

int validateAlignment (int argc, char * argv[])
{
    // parse the command line
    Options options;
    options.helper(argc, argv);
    options.parser(argc, argv);

    // validate, and catch exception if relevant
    // example how to produce all the config files and the DAGMAN
    DAG dag(options.config);
    dag.GCP();
    dag.DMR();
    dag.close();

    if (options.dry) {
        cout << "Dry run, exiting now" << endl;
        return EXIT_SUCCESS;
    }

    return true; //dag.submit();
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<validateAlignment>(argc, argv);
}
#endif
