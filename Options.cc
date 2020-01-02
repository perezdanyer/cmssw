#include <cstdlib>

#include <iostream>
#include <iomanip>

//#include <experimental/filesystem>
#include <boost/filesystem.hpp>

#include <boost/version.hpp>
#include <boost/program_options.hpp>

#include "Options.h"

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options; // used to read the options from command line

namespace AllInOneConfig {

////////////////////////////////////////////////////////////////////////////////
/// Function used by Boost::PO to check if the file exist
void check_file (string file)
{
    // check that the config file exists
    if (!fs::exists(file)) {
        cerr << "Didn't find configuration file " << file << '\n';
        exit(EXIT_FAILURE);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Function used by Boost::PO to enable standard output
void set_verbose (bool flag)
{
    if (!flag) cout.setstate(ios_base::failbit);
}

////////////////////////////////////////////////////////////////////////////////
/// Function used by Boost::PO to disable standard error
void set_silent (bool flag)
{
    if ( flag) cerr.setstate(ios_base::failbit); 
}

////////////////////////////////////////////////////////////////////////////////
/// Constructor for PO:
/// - contains a parser for the help itself
/// - contains a parser for the options (unless help was displayed)
/// - and contains a hidden/position option to get the INFO config file
Options::Options (bool getter) :
    help{"Helper"}, desc{"Options"}, hide{"Hidden"}
{
    // define all options

    // first the helper
    help.add_options()
        ("help,h", "Help screen")
        ("example,e", "Print example of config")
        ("tutorial,t", "Explain how to use the command");

    // then (except for getINFO) the running options
    if (!getter) 
        desc.add_options()
            ("dry,d", po::value<bool>(&dry)->default_value(false), "Set up everything, but don't run anything")
            ("verbose,v", po::bool_switch()->default_value(false)->notifier(set_verbose), "Enable standard output stream")
            ("silent,s" , po::bool_switch()->default_value(false)->notifier(set_silent), "Disable standard error stream");

    // and finally / most importantly, the config file as apositional argument
    hide.add_options()
        ("config,c", po::value<string>(&config)->required()->notifier(check_file), "INFO config file");
    pos_hide.add("config", 1); // 1 means one (positional) argument

    // in case of getIMFO, adding a second positional argument for the key
    if (!getter) return;

    hide.add_options()
        ("key,k", po::value<string>(&key)->required(), "key");
    pos_hide.add("key", 1);
}

////////////////////////////////////////////////////////////////////////////////
/// Parser for help
void Options::helper (int argc, char * argv[])
{
    po::options_description options;
    options.add(help).add(desc); 

    po::command_line_parser parser_helper{argc, argv};
    parser_helper.options(help) // don't parse required options before parsing help
                 .allow_unregistered(); // ignore unregistered options,

    // defines actions
    po::variables_map vm;
    po::store(parser_helper.run(), vm);
    po::notify(vm); // necessary for config to be given the value from the cmd line

    if (vm.count("help")) {
        cout << "Basic syntax:\n  " << argv[0] << " config.info\n"
             << options << '\n'
             << "Boost " << BOOST_LIB_VERSION << endl;
        exit(EXIT_SUCCESS);
    }

    if (vm.count("tutorial") || vm.count("example")) { // TODO
        cout << "   ______________________\n"
             << "  < Oops! not ready yet! >\n"
             << "   ----------------------\n"
             <<"          \\   ^__^\n"
             <<"           \\  (oo)\\_______\n"
             << "              (__)\\       )\\/\\\n"
             << "                  ||----w |\n"
             << "                  ||     ||\n"
             << flush;
        exit(EXIT_SUCCESS);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Parser for options
void Options::parser (int argc, char * argv[])
{
    try {
        po::options_description cmd_line;
        cmd_line.add(desc).add(hide);

        po::command_line_parser parser{argc, argv};
        parser.options(cmd_line)
              .positional(pos_hide);

        po::variables_map vm;
        po::store(parser.run(), vm);
        po::notify(vm); // necessary for config to be given the value from the cmd line
    }
    catch (const po::required_option &e) {
        if (e.get_option_name() == "--config")
            cerr << "Missing config" << '\n';
        else 
            cerr << "Program Options Required Option: " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }

    // TODO
    // Boost.ProgramOptions also defines the function boost::program_options::parse_environment(),
    // which can be used to load options from environment variables.
    // The class boost::environment_iterator lets you iterate over environment variables.

    // TODO
    // error handling for program options entirely here
}

} // end of namespace AllInOneConfig
