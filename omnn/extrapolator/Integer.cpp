//
// Created by Сергей Кривонос on 01.09.17.
//

#include "Integer.h"
#include "Exponentiation.h"
#include "Fraction.h"
#include "Sum.h"
#include "Product.h"

namespace omnn{
namespace extrapolator {

    Valuable Integer::operator -() const
    {
        return Integer(-arbitrary);
    }
    
    Valuable& Integer::operator +=(const Valuable& v)
    {
        auto i = cast(v);
        if (i)
            arbitrary += i->arbitrary;
        else
        {
            auto f = Fraction::cast(v);
            if (f) {
                return Become(v + *this);
            }
            else
                return Become(Sum(*this, v));
        }
        return *this;
    }

    Valuable& Integer::operator +=(int v)
    {
        arbitrary += v;
        return *this;
    }

    Valuable& Integer::operator *=(const Valuable& v)
    {
        auto i = cast(v);
        if (i)
            arbitrary *= i->arbitrary;
        else
        {
            // all other types should handle multiplication by Integer
            Become(v**this);
        }
        return *this;
    }

    Valuable& Integer::operator /=(const Valuable& v)
    {
        auto i = cast(v);
        if (i)
        {
            auto div = arbitrary/i->arbitrary;
            if(div*i->arbitrary==arbitrary)
                arbitrary=div;
            else
                Become(Fraction(*this,*i));
        }
        else
        {
			Become(Fraction(*this, v));
        }
        return *this;
    }

    Valuable& Integer::operator %=(const Valuable& v)
    {
        auto i = cast(v);
        if (i)
            arbitrary %= i->arbitrary;
        else
        {
            // try other type
            // no type matched
            base::operator %=(v);
        }
        return *this;
    }

    Valuable& Integer::operator --()
    {
        arbitrary--;
        return *this;
    }

    Valuable& Integer::operator ++()
    {
        arbitrary++;
        return *this;
    }

    Valuable& Integer::operator^=(const Valuable& v)
    {
        auto i = cast(v);
        if(i)
        {
            if (*i != 0_v) {
                if (*i > 1) {
                    auto a = arbitrary;
                    for (base_int n = *i; n > 1; --n) {
                        arbitrary *= a;
                    }
                } else {
                    // negative
                    Become(Exponentiation(*this, v));
                }
            }
            else { // zero
                if (arbitrary == 0)
                    throw "NaN"; // feel free to handle this properly
                else
                    arbitrary = 1;
            }
        }
        else
        {
            Become(Exponentiation(*this, v));
        }
        
        optimize();
        return *this;
    }
    
    bool Integer::operator <(const Valuable& v) const
    {
        auto i = cast(v);
        if (i)
            return arbitrary < i->arbitrary;
        else
        {
            // try other type
            auto i = Fraction::cast(v);
            if (i)
            {
                return v >= *this;
            }
            else
            {
                // try other type
                // no type matched
            }
        }
        // not implemented comparison to this Valuable descent
        return base::operator <(v);
    }

    bool Integer::operator ==(const Valuable& v) const
    {
        auto i = cast(v);
        if (i)
            return arbitrary == i->arbitrary;
        else if(v.FindVa())
            return false;
        else if(Fraction::cast(v))
            return v == *this;

        // no type matched
        return base::operator ==(v);
    }

    std::ostream& Integer::print(std::ostream& out) const
    {
        return out << arbitrary;
    }
    
    std::ostream& operator <<(std::ostream& out, const Integer& obj)
    {
        return obj.print(out);
    }

    size_t Integer::Hash() const
    {
        return std::hash<base_int>()(arbitrary);
    }
}}

