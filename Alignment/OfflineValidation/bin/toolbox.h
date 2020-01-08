#include <string>
#include <vector>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// just a "translator" to get the IOVs from a space-separated string into a vector of integers
template<typename T> class Tokenise {

    T Cast (string s) const;

public:

    typedef string internal_type; // mandatory to define translator
    typedef vector<T> external_type; // id.

    boost::optional<external_type> get_value (internal_type s) const // name is mandatory for translator
    {
        // pre-treatment :
        // - replace all '\n' by ' '
        // - remove spaces in front or at the end
        replace(s.begin(), s.end(), '\n', ' ');
        boost::trim(s);

        // tokenise
        boost::char_separator<char> sep{" "};
        boost::tokenizer<boost::char_separator<char>> tok{s,sep};
        // note: multiple spaces should (in principle...) be considered as a single space by boost...

        // put into a vector
        external_type results; 
        for (string t: tok) {
            T tt = boost::lexical_cast<T>(t);
            results.push_back(tt);
        }
        return results;
    }
};
#endif
