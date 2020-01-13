#include <cstdlib>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <boost/filesystem.hpp>

using namespace std;

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

int main (int argc, char * argv[])
{
    cout << "hi from " << __FILE__ << endl;
    return EXIT_SUCCESS;
}
