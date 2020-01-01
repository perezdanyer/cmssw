#include <cstdlib>
#include <iostream>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <boost/filesystem.hpp>

using namespace std;
using namespace AllInOneConfig;

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

int merge (int argc, char * argv[])
{
    // parse the command line
    Options options;
    options.helper(argc, argv);
    options.parser(argc, argv);

    pt::ptree main_tree;
    pt::read_info(options.config, main_tree);

    pt::ptree alignments = main_tree.get_child("alignments");
    
    pt::ptree validation = main_tree.get_child("validation");

    dump(validation);
    string single = validation.get<string>("single");
    for (auto& alignment: alignments) {
        string file = alignment.second.get<string>("files.DMR." + single);
        cout << alignment.first << '\t' << file << '\n';
    }
    cout << flush;

    return EXIT_SUCCESS; 
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<merge>(argc, argv);
}
#endif
