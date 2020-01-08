#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <experimental/filesystem>

#include <boost/version.hpp>
#include <boost/program_options.hpp>

#include "validateAlignmentProgramOptions.h"

using namespace std;
namespace fs = experimental::filesystem;
namespace po = boost::program_options; // used to read the options from command line

namespace AllInOneConfig {

void check_file (string file)
{
    // check that the config file exists
    if (!fs::exists(file)) {
        cerr << "Didn't find configuration file " << file << '\n';
        exit(EXIT_FAILURE);
    }
}

void set_verbose (bool flag)
{
    if (!flag) cout.setstate(ios_base::failbit);
}

void set_silent (bool flag)
{
    if ( flag) cerr.setstate(ios_base::failbit); 
}

ValidationProgramOptions::ValidationProgramOptions () :
    help{"Helper"}, desc{"Options"}, hide{"Hidden"}
{
    // define all options
    help.add_options()
        ("help,h", "Help screen")
        ("example,e", "Print example of config")
        ("tutorial,t", "Explain how to use the command");
    desc.add_options()
        ("dry,d", po::value<bool>(&dry)->default_value(false), "Set up all configs and jobs, but don't submit")
        ("verbose,v", po::bool_switch()->default_value(false)->notifier(set_verbose), "Enable standard output stream")
        ("silent,s" , po::bool_switch()->default_value(false)->notifier(set_silent), "Disable standard error stream");
    hide.add_options()
        ("config,c", po::value<string>(&config)->required()->notifier(check_file), "INFO config file");

    // positional argument
    pos_hide.add("config", 1);
}

void ValidationProgramOptions::helper (int argc, char * argv[])
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

void ValidationProgramOptions::parser (int argc, char * argv[])
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
}

} // end of namespace
