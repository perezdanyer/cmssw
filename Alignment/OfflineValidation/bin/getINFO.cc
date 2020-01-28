#include <cstdlib>
#include <iostream>
#include <string>

#include "exceptions.h"
#include "Options.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace AllInOneConfig;

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

int getINFO (int argc, char * argv[])
{
    // parse the command line
    Options options(/* getter = */ true);
    options.helper(argc, argv);
    options.parser(argc, argv);

    pt::ptree tree;
    pt::read_json(options.config, tree);

    // TODO: use regex on options.key
    //       -> if several entries are possible, return them all?

    string value = tree.get<string>(options.key);

    cout << value << endl;

    return EXIT_SUCCESS; 
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<getINFO>(argc, argv);
}
#endif

