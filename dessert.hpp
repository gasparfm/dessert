/* Lightweight and simple C++11 test framework. No dependencies.
 * Copyright (c) 2012,2013,2014 Mario 'rlyeh' Rodriguez

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 * - rlyeh ~~ listening to Led Zeppelin / No Quarter
 */

#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <deque>
#include <functional>
#include <string>

/* Public API */
#define throws(...)  ( [&](){ try { __VA_ARGS__; } catch( ... ) { return true; } return false; } () )
#define dessert(...) ( dessert::suite(#__VA_ARGS__,bool(__VA_ARGS__),__FILE__,__LINE__) < __VA_ARGS__ )
#define desserts(...) \
        static void dessert$line(dessert)(); \
        static const bool dessert$line(dsstSuite_) = dessert::suite::queue( [&](){ \
            dessert(1)<< "start of suite: " __VA_ARGS__; \
            dessert$line(dessert)(); \
            dessert(1)<< "end of suite: " __VA_ARGS__; \
            }, "" #__VA_ARGS__ ); \
        void dessert$line(dessert)()

/* API Details */
namespace dessert {
    using namespace std;
    class suite {
        using timer = chrono::high_resolution_clock;
        chrono::high_resolution_clock::time_point start = timer::now();
        deque< string > xpr;
        int ok = false, has_bp = false;
        enum { BREAKPOINT, BREAKPOINTS, PASSED, FAILED, TESTNO };
        template<int VAR> static unsigned &get() { static unsigned var = 0; return var; }
        static string time( chrono::high_resolution_clock::time_point start ) {
            return to_string( double((timer::now() - start).count()) * timer::period::num / timer::period::den );
        }
    public:
        static bool queue( const function<void()> &fn, const string &text ) {
            static auto start = timer::now();
            static struct install {
                deque< function<void()> > all;
                install() {
                    get<BREAKPOINT>() = stoul( getenv("BREAKON") ? getenv("BREAKON") : "0" );
                }
                ~install() {
                    for( auto &fn : all ) fn();
                    string ss, run = to_string( get<PASSED>()+get<FAILED>() ), res = get<FAILED>() ? "[FAIL]  " : "[ OK ]  ";
                    if( get<FAILED>() ) ss += res + "Failure! " + to_string(get<FAILED>()) + '/'+ run + " tests failed :(\n";
                    else                ss += res + "Success: " + run + " tests passed :)\n";
                    ss += "        Breakpoints: " + to_string( get<BREAKPOINTS>() ) + " (*)\n";
                    ss += "        Total time: " + time(start) + " seconds.\n";
                    fprintf( stderr, "\n%s", ss.c_str() );
                }
            } queue;
            return text.find("before main()") == string::npos ? ( queue.all.push_back( fn ), false ) : ( fn(), true );
        }
        suite( const char *const text, bool result, const char *const file, int line )
        :   ok(result), xpr( {string(file) + ':' + to_string(line), " - ", text, "" } ) {

            xpr[0] = "Test " + to_string(++get<TESTNO>()) + " at " + xpr[0];
            queue( [&](){ if( result ) get<PASSED>()++; else get<FAILED>()++; }, "before main()" );

            if( 0 != ( has_bp = ( get<TESTNO>() == get<BREAKPOINT>() )) ) {
                get<BREAKPOINTS>()++;
                fprintf( stderr, "<dessert/dessert.hpp> says: breaking on test #%d\n\t", get<TESTNO>() );
                    assert(! "<dessert/dessert.hpp> says: breakpoint requested" );
                fprintf( stderr, "%s", "\n<dessert/dessert.hpp> says: breakpoint failed!\n" );
            };
        }
        ~suite() {
            string res[] = { "[FAIL]", "[ OK ]" }, bp[] = { "  ", " *" }, tab[] = { "        ", "" };
            xpr[0] = res[ok] + bp[has_bp] + xpr[0] + " (" + time(start) + " s)" + (xpr[1].size() > 3 ? xpr[1] : tab[1]);
            xpr.erase( xpr.begin() + 1 );
            if( !ok ) {
                xpr[2] = xpr[2].substr( xpr[2][2] == ' ' ? 3 : 4 );
                xpr[1].resize( (xpr[1] != xpr[2]) * xpr[1].size() );
                xpr.push_back( "(unexpected)" );
            } else xpr = { xpr[0] };
            for( unsigned it = 0; it < xpr.size(); ++it ) {
                fprintf( stderr, xpr[it].size() ? "%s%s\n" : "", tab[ !it ].c_str(), xpr[it].c_str() );
            }
        }

#       define dessert$join(str, num) str##num
#       define dessert$glue(str, num) dessert$join(str, num)
#       define dessert$line(str)      dessert$glue(str, __LINE__)
#       define dessert$impl(OP) \
        template<typename T> suite &operator OP( const T &rhs         ) { return xpr[3] += " "#OP" " + to_string(rhs), *this; } \
        template<          > suite &operator OP( const string &rhs    ) { return xpr[3] += " "#OP" " + rhs,            *this; } \
        template< size_t N > suite &operator OP( const char (&rhs)[N] ) { return xpr[3] += " "#OP" " + string(rhs),    *this; }
        template<typename T> suite &operator <<( const T &t           ) { return xpr[1] += to_string(t),               *this; }
        template<          > suite &operator <<( const string &str    ) { return xpr[1] += str,                        *this; }
        template< size_t N > suite &operator <<( const char (&str)[N] ) { return xpr[1] += str,                        *this; }
        operator bool() const { return ok; }
        dessert$impl( <); dessert$impl(<=);
        dessert$impl( >); dessert$impl(>=);
        dessert$impl(!=); dessert$impl(==);
        dessert$impl(&&); dessert$impl(||);
    };
}