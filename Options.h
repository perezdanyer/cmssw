#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>

namespace AllInOneConfig {

class Options {

    boost::program_options::options_description help, desc, hide;
    boost::program_options::positional_options_description pos_hide;

public:
    // TODO: use optional<const T>?
    // -> pb: how to choose between std::optional & boost::optional...
    std::string config;
    bool dry;

    Options ();
    void helper (int argc, char * argv[]);
    void parser (int argc, char * argv[]);
};

}
