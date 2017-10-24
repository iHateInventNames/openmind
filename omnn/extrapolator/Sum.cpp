//
// Created by Сергей Кривонос on 25.09.17.
//
#include "Sum.h"

#include "Exponentiation.h"
#include "Formula.h"
#include "Fraction.h"
#include "Product.h"
#include "Variable.h"

#include <cmath>
#include <map>

namespace omnn{
namespace extrapolator {
    
    void Sum::Add(const Valuable& item)
    {
        members.insert(item);
    }

	Valuable Sum::operator -() const
	{
		Sum s;
		for (auto& a : members) 
			s.Add(-a);
		return s;
	}

    void Sum::optimize()
    {
        if (isOptimizing)
            return;
        isOptimizing = true;

        Valuable w(0);
        do
        {
            w = *this;
            std::cout << "optimizing sum " << w << std::endl;
            if (members.size() == 1) {
                cont::iterator b = members.begin();
                Become(std::move(const_cast<Valuable&>(*b)));
                isOptimizing = false;
                return;
            }

            for (auto it = members.begin(); it != members.end();)
            {
                // optimize member
                auto copy = *it;
                copy.optimize();
                if (copy != *it) {
                    members.erase(it++);
                    it = members.insert(it, copy);
                    continue;
                }
                
                if (*it == 0)
                {
                    members.erase(it++);
                    continue;
                }
                auto s = cast(*it);
                if (s) {
                    for (auto& m : s->members)
                    {
                        Add(std::move(m));
                    }
                    members.erase(it++);
                    continue;
                }
                auto t = it;
                for (auto it2 = ++t; it2 != members.end();)
                {
                    if ((it2->OfSameType(*it)
                        && !Variable::cast(*it)
                        && (!Product::cast(*it) || *it == -*it2))
                        || (Integer::cast(*it) && Fraction::cast(*it2))
                        || (Integer::cast(*it2) && Fraction::cast(*it))
                        )
                    {
                        const_cast<Valuable&>(*it) += *it2;
                        members.erase(it2++);
                    }
                    else
                        ++it2;
                }

                ++it;
            }

            // commonize by vars
            using K = std::multiset<Variable>;
            using V = Product*;
            using KV = std::pair<K, V>;
            std::map<K, V> kv;
            for (auto it = members.begin(); it != members.end();)
            {
                auto p = Product::cast(*it);
                if (p)
                {
                    auto k = p->getCommonVars();
                    if (k.size())
                    {
                        std::map<K, V>::iterator v = kv.find(k);
                        if (v == kv.end()) {
                            kv[k] = const_cast<Product*>(p);
                        }
                        else
                        {
                            *v->second += *p;
                            members.erase(it++);
                            continue;
                        }
                    }
                }
                ++it;
            }

            // optimize members
            for (auto it = members.begin(); it != members.end();)
            {
                auto copy = *it;
                copy.optimize();
                if (copy != *it) {
                    members.erase(it++);
                    it = members.insert(it, copy);
                }
                else
                    ++it;
            }
            
            if (w!=*this) {
                std::cout << "Sum optimized from \n\t" << w << "\n \t to " << *this << std::endl;
            }
        } while (w != *this);
        isOptimizing = false;
    }

	Valuable& Sum::operator +=(const Valuable& v)
	{
		auto i = cast(v);
		if (i) {
			for (auto& a : i->members) {
				Add(a);
			}
		}
		else
		{
            if(!Product::cast(v))
            {
                for (auto& a : members)
                {
                    if(a.OfSameType(v))
                    {
                        const_cast<Valuable&>(a) += v;
                        optimize();
                        return *this;
                    }
                }
            }
            
            // add new member
			Add(v);
		}

        optimize();
		return *this;
	}

	Valuable& Sum::operator +=(int v)
	{
		return *this+=(Integer(v));
	}

	Valuable& Sum::operator *=(const Valuable& v)
	{
        Sum s;
        auto f = cast(v);
		if (f)
		{
			for (auto& a : members) {
				for (auto& b : f->members) {
					s.Add(a*b);
				}
			}
		}
		else
        {
            for (auto& a : members) {
                s.Add(a*v);
            }
		}

        s.optimize();
        members = s.members;

		return *this;
	}

	Valuable& Sum::operator /=(const Valuable& v)
	{
        Sum s;
		auto i = cast(v);
		if (i)
		{
			for (auto& a : members) {
				for (auto& b : i->members) {
					s.Add(a/b);
				}
			}
		}
		else
		{
            for (auto& a : members) {
                s.Add(a/v);
            }
		}
        s.optimize();
        members = s.members;
		return *this;
	}

	Valuable& Sum::operator %=(const Valuable& v)
	{
		return base::operator %=(v);
	}

	Valuable& Sum::operator --()
	{
		return *this += -1;
	}

	Valuable& Sum::operator ++()
	{
		return *this += 1;
	}

	bool Sum::operator <(const Valuable& v) const
	{

		// not implemented comparison to this Valuable descent
		return base::operator <(v);
	}

	bool Sum::operator ==(const Valuable& v) const
	{
        auto s = cast(v);
        bool eq = s && members==s->members;
        return eq;
	}

	std::ostream& Sum::print(std::ostream& out) const
	{
        std::stringstream s;
        s << '(';
        constexpr char sep[] = " + ";
		for (auto& b : members) 
            s << b << sep;
        auto str = s.str();
        auto cstr = const_cast<char*>(str.c_str());
        cstr[str.size() - sizeof(sep) + 1] = 0;
        out << cstr << ')';
		return out;
	}
    
    Valuable Sum::sqrt() const
    {
        return Exponentiation(*this, 1_v/2);
    }

    /** fast linear equation formula deduction */
	Formula Sum::FormulaOfVa(const Variable& v) const
	{
        Valuable fo(0);
        std::vector<Valuable> coefficients(4);
        auto grade = 0;
        for (auto& m : members)
        {
            auto p = Product::cast(m);
            if(p)
            {
                auto vcnt = 0; // exponentation of va
                for(auto& pv : p->getCommonVars())
                    if(pv==v)
                        ++vcnt;
                
                if (vcnt > grade) {
                    grade = vcnt;
                    if (vcnt >= coefficients.size()) {
                        coefficients.resize(vcnt+1);
                    }
                }
                
                coefficients[vcnt] += m / (v^vcnt);
            }
            else
            {
                coefficients[0] += m;
            }
        }

        switch (grade) {
            case 2: {
                // square equation axx+bx+c=0
                // root formula: x=((b*b-4*a*c)^(1/2)-b)/(2*a)
                auto& a = coefficients[2];
                auto& b = coefficients[1];
                auto& c = coefficients[0];
                fo = ((b*b-4*a*c).sqrt()-b)/(2*a);
                break;
            }
            case 4: {
                // four grade equation ax^4+bx^3+cx^2+dx+e=0
                break;
            }
            default: {
                throw "Implement!";
                break;
            }
        }
        
        return Formula::DeclareFormula(v, fo);
	}
    

}}
