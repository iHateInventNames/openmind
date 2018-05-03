//
// Created by Sergej Krivonos on 4/19/18.
//
#define BOOST_TEST_MODULE Sh test
#include <boost/test/unit_test.hpp>

#include "Variable.h"

using namespace omnn::math;
using namespace boost::unit_test;
using namespace std;

BOOST_AUTO_TEST_CASE(And_test)
{
    Variable v;
    auto _1 = v+10;
    auto _2 = v+11;
    Valuable::optimizations = {};
    auto _ = _1.And(5, _2);
    auto t = _;
    Valuable::optimizations = true;
    t.Eval(v,0);
    t.optimize();
    BOOST_TEST(t == 10);
    t = _;
    t.Eval(v,1);
    t.optimize();
    BOOST_TEST(t == 8);
}

BOOST_AUTO_TEST_CASE(Or_test, *disabled())
{
    Variable v;
    auto _1 = v+10;
    auto _2 = v+11;
    Valuable::optimizations = {};
    auto _ = _1.Or(5, _2);
    auto t = _;
    Valuable::optimizations = true;
    t.Eval(v,0);
    t.optimize();
    BOOST_TEST(t == 11);
    t = _;
    t.Eval(v,1);
    t.optimize();
    BOOST_TEST(t == 15);
}

BOOST_AUTO_TEST_CASE(XOr_test, *disabled())
{
    Variable v;
    auto _1 = v+10;
    auto _2 = v+11;
    Valuable::optimizations = {};
    auto _ = _1.Xor(5, _2);
    auto t = _;
    Valuable::optimizations = true;
    t.Eval(v,0);
    t.optimize();
    BOOST_TEST(t == 1);
    t = _;
    t.Eval(v,1);
    t.optimize();
    BOOST_TEST(t == 7);
}