#include <cstdlib>
#include <iostream>
#include <exception>
#include <string>

#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp> 

#include <boost/property_tree/exceptions.hpp>
#include <boost/program_options/errors.hpp>

// TODO: filesystem error

using std::cerr;

template<int FUNC(int, char **)>
int exceptions (int argc, char * argv[])
{
    try { return FUNC(argc, argv); }
    catch (const boost::exception & e) { cerr << "Boost exception: " << boost::diagnostic_information(e); throw; }
    catch (const boost::program_options::invalid_syntax & e) { cerr << "Program Options Invalid Syntax: " << e.what() << '\n'; }
    catch (const boost::program_options::error          & e) { cerr << "Program Options Error: " << e.what() << '\n'; }
    catch (const boost::property_tree::ptree_bad_data & e) { cerr << "Property Tree Bad Data Error: " << e.data<std::string>() << '\n'; }
    catch (const boost::property_tree::ptree_bad_path & e) { cerr << "Property Tree Bad Path Error: " << e.path<std::string>() << '\n'; }
    catch (const boost::property_tree::ptree_error    & e) { cerr << "Property Tree Error: " << e.what() << '\n'; }
    catch (const std::logic_error & e) { cerr << "Logic Error: "    << e.what() << '\n'; }
    catch (const std::exception   & e) { cerr << "Standard Error: " << e.what() << '\n'; }
    catch (...) { cerr << "Unkown failure\n"; }
    return EXIT_FAILURE;
}

