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
    DAG dag(options.config);
    dag.GCP();
    dag.DMR();
    dag.close();

    return dag.submit(options.dry);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<validateAlignment>(argc, argv);
}
#endif
