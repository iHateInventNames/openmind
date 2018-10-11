//
// Created by Sergej Krivonos on 25.02.18.
//
#define BOOST_TEST_MODULE System test
#include <boost/test/unit_test.hpp>

#include "System.h"
#include "VarHost.h"
#include "e.h"
#include "i.h"
#include "π.h"

#include <array>
#include <future>
#include <iostream>     // cout, endl
#include <fstream>      // fstream
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#endif

#include <boost/lexical_cast.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/tokenizer.hpp>

using namespace omnn::math;
using namespace boost::unit_test;
using namespace std;

BOOST_AUTO_TEST_CASE(System_tests)
{
    {
        System sys;
        Variable a,b;
        Valuable t;
        t.SetView(Valuable::View::Equation);
        t = a - 8 - b;
        sys << t;
        sys << a + b - 100;
        auto s = sys.Solve(a);
        auto haveOneSolution = s.size()==1;
        BOOST_TEST(haveOneSolution);
        if(haveOneSolution)
        {
            auto _ = *s.begin();
            BOOST_TEST(_ == 54);
        }
    }
    
    {
        System s;
        Variable a,b;
        s << a - 8 - b;
        s << a + b - 21;
        auto _ = s.Solve(a);
        BOOST_TEST(_.size()==1);
    }
}

BOOST_AUTO_TEST_CASE(sq_System_test, *disabled())
{
    System s;
    Variable a,b,x;
	s << a - b - 3
		<< a + b - x
		<< a * b * 4 - (49 - 9)
		//<< x * x - 49
        << x*b + 9 +2*a*b + 3*b - 49
        ;

    auto _ = s.Solve(x);
    BOOST_TEST(_.size()==1);
	auto sqx = *_.begin();
	BOOST_TEST(sqx == 7);
}

BOOST_AUTO_TEST_CASE(ComplexSystem_test, *disabled()) // TODO :
{
    // https://github.com/ohhmm/openmind/issues/8
    // In a farm there are 100 animals consisting of cows,goats,and buffalos.each goat gives 250g of milk,each Buffalo gives 6kg of milk and if each cow gives 4kg of milk.if in total 100 animals produce 40 kg of milk how many animals of each type are present?
    // https://www.quora.com/Can-this-math-problem-be-solved
    System s;
    Variable c,g,b;
    s   << c+g+b-100
        << g*250+b*6000+c*4000-40000;
    
    // c,g,b are integers, see https://math.stackexchange.com/a/1598552/118612
    using namespace constant;
    s   << (e^(2*π*i*c))-1
        << (e^(2*π*i*g))-1
        << (e^(2*π*i*b))-1
    ;
    auto _ = s.Solve(c);
//    auto cc = s.SolveSingleInteger(c);
//    auto gc = s.SolveSingleInteger(g);
//    auto bc = s.SolveSingleInteger(b);
//    BOOST_TEST(values);
}

BOOST_AUTO_TEST_CASE(Sudoku_simple_test) // solve sudoku through a system of equations though
{
    System s;
    Variable x,y,v;
    std::array<std::array<int, 9>, 9> data {{
        {0, 9, 8, 0, 4, 0, 0, 0, 0},
        {4, 2, 0, 0, 9, 0, 0, 8, 0},
        {0, 0, 0, 3, 0, 1, 0, 0, 0},
        {6, 3, 9, 0, 0, 8, 7, 0, 0},
        {2, 0, 4, 9, 0, 7, 3, 0, 8},
        {0, 0, 7, 5, 0, 0, 9, 2, 6},
        {0, 0, 0, 4, 0, 3, 0, 0, 0},
        {0, 6, 0, 0, 1, 0, 0, 4, 9},
        {0, 0, 0, 0, 5, 0, 8, 3, 0}
    }};
    
    // at
    auto at = [&](auto& xx, auto& yy, auto& vv){
        return x.Equals(xx).LogicAnd(y.Equals(yy)).LogicAnd(v.Equals(vv));
    };
    
    // define known data
    for(auto rowIdx = data.size(); rowIdx--;){
        for(auto colIdx = data[rowIdx].size(); colIdx--;){
            auto& i = data[rowIdx][colIdx];
            if(i)
                s << at(colIdx,rowIdx,i);
        }
    }
    
    // define possible values
    auto abet = [](const Variable& x) {
        auto v = 1_v;
        for(int i=1; i<10; ++i){
            v.logic_or(x.Equals(i));
        }
        return v;
    };
    
    // define the universe
    // for simplicity check only row/col/square sum equalities to 45
    Variable value;
    for(int xx=0; xx<=8; ++xx){
        auto sumInRow = -45_v;
        auto sumInCol = -45_v;
        auto sumInSq = -45_v;
        for(int yy=0; yy<=8; ++yy){
//            sumInRow += at(xx,yy,value)(value);
//            sumInCol += at(yy,xx,value)(value);
            auto sqn = xx;
            auto sqx = sqn%3;
            auto sqy = (sqn - sqx) / 3;
            auto insqn = yy;
            auto insqx = insqn % 3;
            auto insqy = (insqn - insqx) / 3;
//            sumInSq += at(sqx*3+insqx,sqy*3+insqy,value)(value);
        }
        s << sumInRow << sumInCol << sumInSq;
    }

//    std::atomic<int> MaxTasks = boost::thread::hardware_concurrency();
//    std::deque<std::function<void()>> tasks;
//    auto ChooseNextTask = [&tasks](){
//
//    };
//    auto AddPoolTask = [](std::function<
    // solving
    auto sysMutex = std::make_shared<boost::shared_mutex>();
    for(auto rowIdx = data.size(); rowIdx--;){
        for(auto colIdx = data[rowIdx].size(); colIdx--;){
            auto& i = data[rowIdx][colIdx];
            if(!i){
                std::function<void()> addThisTask;
                addThisTask = [colIdx,rowIdx,sysMutex,&s,&at,addThisTask](){
                    auto t = std::async([colIdx,rowIdx, sysMutex, &s, &at, addThisTask](){
                        Variable find;
                        sysMutex->lock_shared();
                        decltype(s) localSystem = s;
                        sysMutex->unlock_shared();
                        localSystem << at(colIdx,rowIdx, find);
                        auto solutions = localSystem.Solve(find);
                        if (solutions.size()) {
                            if (solutions.size()==1) {
                                auto solution = *solutions.begin();
                                if (!solution.IsInt()) {
                                    IMPLEMENT
                                }
                                else
                                {
                                    auto item = at(colIdx,rowIdx,solution);
                                    sysMutex->lock();
                                    s << item;
                                    sysMutex->unlock();
                                }
                            }
                            else
                            {
                                // todo : intersect into possible solutions array for this x,y
                                for(auto& solution: solutions){
                                    
                                }
                            }
                        }
                    });
                };
            }
        }
    }
    

}

BOOST_AUTO_TEST_CASE(Sudoku_test, *disabled()) // solve sudoku through a system of equations though
{
    System s;
    Variable x,y,v;
    std::array<std::array<int, 9>, 9> data {{
        {0, 9, 8, 0, 4, 0, 0, 0, 0},
        {4, 2, 0, 0, 9, 0, 0, 8, 0},
        {0, 0, 0, 3, 0, 1, 0, 0, 0},
        {6, 3, 9, 0, 0, 8, 7, 0, 0},
        {2, 0, 4, 9, 0, 7, 3, 0, 8},
        {0, 0, 7, 5, 0, 0, 9, 2, 6},
        {0, 0, 0, 4, 0, 3, 0, 0, 0},
        {0, 6, 0, 0, 1, 0, 0, 4, 9},
        {0, 0, 0, 0, 5, 0, 8, 3, 0}
    }};

    // at
    auto at = [&](auto& xx, auto& yy, auto& vv){
        return x.Equals(xx).LogicAnd(y.Equals(yy)).LogicAnd(v.Equals(vv));
    };

    // define known data
    for(auto rowIdx = data.size(); rowIdx--;){
        for(auto colIdx = data[rowIdx].size(); colIdx--;){
            auto& i = data[rowIdx][colIdx];
            if(i)
                s << at(colIdx,rowIdx,i);
        }
    }

    // define possible values
    auto abet = [](const Variable& x) {
        auto v = 1_v;
        for(int i=1; i<10; ++i){
            v.logic_or(x.Equals(i));
        }
        return v;
    };

    // define the universe
     // each item
    for(int xx=0; xx<=8; ++xx){
        for(int yy=0; yy<=8; ++yy){
            for(int val=0; val<=8; ++val)
            {
                auto valsExceptVal = abet(v)/v.Equals(val);
                auto valAt = at(x,y,val);

                // inequality in row
                for(auto leastCorowers = xx + 1; leastCorowers <= 8; ++leastCorowers){
                    auto siblingSameVal = at(leastCorowers,y,val);
                    auto siblingExceptVal = 1_v;
                    for(int exceptVal = 0; exceptVal<=8; ++exceptVal){
                        if(exceptVal!=val)
                            siblingExceptVal.logic_or(at(leastCorowers,y,exceptVal));
                    }
                    s << valAt.Ifz(siblingExceptVal,siblingExceptVal.LogicOr(siblingSameVal));
                }

                // inequality in column
                for(auto leastCocolumners = yy + 1; leastCocolumners <= 8; ++leastCocolumners){
                    auto siblingSameVal = at(leastCocolumners,y,val);
                    auto siblingExceptVal = 1_v;
                    for(int exceptVal = 0; exceptVal<=8; ++exceptVal){
                        if(exceptVal!=val)
                            siblingExceptVal.logic_or(at(leastCocolumners,y,exceptVal));
                    }
                    s << valAt.Ifz(siblingExceptVal,siblingExceptVal.LogicOr(siblingSameVal));
                }

                // inequality in square
                auto sqxStart = xx-(xx%3);
                for(int sqx = sqxStart; sqx < sqxStart+3; ++sqx){
                    auto sqyStart = yy-(yy%3);
                    for(int sqy = sqyStart; sqy < sqyStart+3; ++sqy){
                        if(!(sqx==xx && sqy==yy)){
                            auto siblingSameVal = at(sqx,sqy,val);
                            auto siblingExceptVal = 1_v;
                            for(int exceptVal = 0; exceptVal<=8; ++exceptVal){
                                if(exceptVal!=val)
                                    siblingExceptVal.logic_or(at(sqx,sqy,exceptVal));
                            }
                            s << valAt.Ifz(siblingExceptVal,siblingExceptVal.LogicOr(siblingSameVal));
                        }
                    }
                }
            }
        }
    }

    // x,y,v are integers, see https://math.stackexchange.com/a/1598552/118612
    //    using namespace constant;
    //    s   << (e^(2*π*i*x))-1
    //        << (e^(2*π*i*y))-1
    //        << (e^(2*π*i*v))-1
    //    ;
    
    // inequality in squares
// todo: try general using coordinatesMultiplicityTo3
//    Variable gaugeX, gaugeY;
//    auto coordinatesMultiplicityTo3 = logicAnd(gaugeX % 3, gaugeY % 3);
//    auto notEqual = [](auto& e1, auto&e2){
//IMPLEMENT
//    };
//    s << logicAnd(coordinatesMultiplicityTo3,
//            logicOr({
//                logicAnd(equality(gaugeX,x), equality(gaugeY,y))
//            })
//            )
     // inequality in squares
//first attept
//    for(int sqareStartX = 0; sqareStartX < 9; sqareStartX += 3){
//        for(int sqareStartY = 0; sqareStartY < 9; sqareStartY += 3){
//            for(int i=1; i<=9; ++i) {
//                auto valsExcept = [&](auto& va, auto& exceptValue){
//                    Valuable allItems=1_v;
//                    for(int xi = sqareStartX; xi < sqareStartX+3; ++xi){
//                        for(int yj = sqareStartY; yj < sqareStartY+3; ++yj){
//                            for(int ii = 1; ii<=9; ++ii)
//                            {
//                                if(ii!=i){
//                                    allItems = allItems.LogicOr(at(xi,yj,));
//                                } else {
//
//                                }
//                            }
//                            auto item = equals(x,xi)
//                                    .LogicAnd(equals(y,yj))
//                                    .LogicAnd(equals(v,i));
//
//                        }
//                    }
//                    return abet(va) / va.Equals(exceptValue);
//                };
//                auto valsExceptI = valsExcept(v,i);
//                s << (x.Equals(sqareStartX).LogicAnd(y.Equals(sqareStartY)))
//                    .LogicAnd();
//            }
//        }
//    }

    // solving
    for(auto rowIdx = data.size(); rowIdx--;){
        for(auto colIdx = data[rowIdx].size(); colIdx--;){
            auto& i = data[rowIdx][colIdx];
            if(!i){
                auto sysMutex = std::make_shared<boost::shared_mutex>();
                std::function<void()> addThisTask = [colIdx,rowIdx,sysMutex,&s,&at,addThisTask](){
                    std::async([colIdx,rowIdx, sysMutex, &s, &at, addThisTask](){
                        Variable find;
                        sysMutex->lock_shared();
                        decltype(s) localSystem = s;
                        sysMutex->unlock_shared();
                        localSystem << at(colIdx,rowIdx, find);
                        auto solutions = localSystem.Solve(find);
                        if (solutions.size()) {
                            if (solutions.size()==1) {
                                auto solution = *solutions.begin();
                                if (!solution.IsInt()) {
                                    IMPLEMENT
                                }
                                else
                                {
                                    auto item = at(colIdx,rowIdx,solution);
                                    sysMutex->lock();
                                    s << item;
                                    sysMutex->unlock();
                                }
                            }
                            else
                            {
                                // todo : intersect into possible solutions array for this x,y
                                for(auto& solution: solutions){
                                    
                                }
                            }
                        }
                    });
                };
            }
        }
    }


//    auto _ = s.Solve(c);
//    auto cc = s.SolveSingleInteger(c);
//    auto gc = s.SolveSingleInteger(g);
//    auto bc = s.SolveSingleInteger(b);
//    BOOST_TEST(values);
}

BOOST_AUTO_TEST_CASE(kaggle_test, *disabled())
{
    boost::basic_thread_pool tp;

    //TypedVarHost<std::string> vh;

    std::atomic<int> cntLines = 0;
    std::map<int, std::string> ids;
    std::string line;
    {
    ifstream in(TEST_SRC_DIR"train.csv", ifstream::in);
    if (!in.is_open())
    {
        cout << "cannot open file\n";
        return;
    }

    std::vector<Variable> v;
    System sys;
    std::mutex systemMutex;
    using namespace boost;

    if (!in.eof()) {
        in >> line; // headers
        tokenizer<escaped_list_separator<char> > tk(
            line, escaped_list_separator<char>('\\', ',', '\"'));
        for (tokenizer<escaped_list_separator<char> >::iterator i(tk.begin());
            i != tk.end(); ++i)
            v.push_back(Variable());

#ifdef _WIN32
        EnableMenuItem(
            GetSystemMenu(GetConsoleWindow(), {}),
            SC_CLOSE,
            MF_GRAYED);
#endif // _WIN32
    }

    if (!in.eof())
        ++cntLines;

    int skip = 0;
    {
        ifstream system(TEST_SRC_DIR"sys.csv", fstream::in);
        if (system.is_open())
        {
            while (!system.eof()) {
                constexpr auto Sz = 1 << 18;
                char s[Sz];
                system.getline(s, Sz);
                line = s;
                if (!line.empty())
                {
                    ++skip;
                    auto c = memchr(s, ';', Sz);
                    *(char*)c = 0;
                    ids[skip] = s;
                }
            }
            system.close();
        }
        else
        {
            cout << "cannot open file, creating new";
            ofstream(TEST_SRC_DIR"sys.csv", fstream::out).close();
            return;
        }
    }

    std::atomic<int> completedLines = skip;
    while (!in.eof()) {
        in >> line;
        if (!in.eof())
            ++cntLines;
        if (completedLines + 1 >= cntLines)
            continue;
        tp.submit([&, line]()
        {
            auto sum = std::make_shared<Sum>();
            sum->SetView(Valuable::View::Equation);
            auto sumMutex = std::make_shared<std::mutex>();

            auto cntWords = std::make_shared<std::atomic<int>>(0);
            tokenizer<escaped_list_separator<char> > tk(
                line, escaped_list_separator<char>('\\', ',', '\"'));
            int vi = 0;
            for (tokenizer<escaped_list_separator<char> >::iterator i(tk.begin());
                i != tk.end(); ++i)
            {
                tp.submit([&, sum, sumMutex, cntWords, w = *i, vi](){
                    auto tid = std::this_thread::get_id();
                    std::cout << tid << " start " << vi << std::endl;
                    const Variable& va = v[vi];
                    {
                        Valuable val;
                        if (vi) {
                            val = boost::lexical_cast<double>(w);
                        }
                        else {
                            long long l;
                            std::stringstream ss(w);
                            ss >> hex >> l;
                            val = l;
                        }
                        val = std::move((va - val).sq());

                        std::lock_guard g(*sumMutex);
                        sum->Add(val);
                    }
                    if (cntWords->fetch_add(1) + 1 == v.size())
                    {
                        sum->optimize();

                        {
                            std::lock_guard g(systemMutex);
                            sys << *sum;
                            ofstream system(TEST_SRC_DIR"sys.csv", fstream::app);
                            system << completedLines << ';' << *sum << endl;
                        }
                        std::cout << tid << " line complete " << completedLines << std::endl;
                        if (completedLines.fetch_add(1) + 1 == cntLines)
                        {
                            // save sys and start processing test lines file
                            std::cout << tid << " sys complete " << std::endl;
                        }
                    }
                    std::cout << tid << " complete " << vi << std::endl;
                });
                ++vi;
            }
        });
    }

    in.close();
    }
    tp.join();

    {
        ifstream in(TEST_SRC_DIR"train.csv", ifstream::in);
        ifstream system(TEST_SRC_DIR"sys.csv", fstream::in);
        ifstream test(TEST_SRC_DIR"test.csv", ifstream::in);

        if (!in.is_open() || !system.is_open())
        {
            cout << "cannot open file\n";
            return;
        }
        auto h = VarHost::make<std::string>();
        while (!system.eof()) {
            constexpr auto Sz = 1 << 18;
            char s[Sz];
            system.getline(s, Sz);
            line = std::string(s);
            if (!line.empty())
            {
                std::size_t sc = line.find(';');
                line = line.substr(sc + 1, line.length() - sc);
                Valuable v(line, h);
                BOOST_TEST(v.str() == line);
            }
        }
    }

    tp.join();
}
