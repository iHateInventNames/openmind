//
// Created by Сергей Кривонос on 01.09.17.
//

#pragma once
#include "OpenOps.h"
#include <memory>
#include <set>
#include <typeindex>
#include <boost/shared_ptr.hpp>

namespace omnn{
namespace extrapolator {
    
    namespace ptrs = ::std;

    class Accessor;
    class Variable;
    struct ValuableDescendantMarker {};
    
class Valuable
        : public OpenOps<Valuable>
{
#ifdef BOOST_TEST_MODULE
public:
#endif
	using self = Valuable;
    using encapsulated_instance = ptrs::shared_ptr<Valuable>;
    encapsulated_instance exp = nullptr;
    /// gets valuable under accessor if accessor passed
    static const Valuable* accessor_cast(const Valuable& mayBeAccessor);

protected:
    virtual Valuable* Clone() const;
    virtual int getTypeSize() const;

    template<class T>
    static const T* cast(const Valuable& v)
    {
        auto e = v.exp;
        while (e && e->exp) e = e->exp;
        auto t = dynamic_cast<const T*>(e ? e.get() : &v);
        if (!t)
        {
            auto a = accessor_cast(v);
            if (a)
                t = cast<T>(*a);
        }
        return t;
    }
    
    template<class T> Valuable() {}
    
    Valuable(ValuableDescendantMarker)
    {}
    
    Valuable(const Valuable& v, ValuableDescendantMarker)
    : hash(v.hash)
    {}
    
    virtual std::ostream& print(std::ostream& out) const;
    virtual Valuable& Become(Valuable&& i);
    
    size_t hash = 0;
    int maxVaExp = 0; // ordering weight: vars max exponentiation in this valuable
    
public:
    static thread_local bool optimizations;
    virtual int getMaxVaExp() const;

    explicit Valuable(Valuable* v);
    operator std::type_index() const;
    
    Valuable& operator =(const Valuable& v);
    Valuable& operator =(const Valuable&& v);
    Valuable(const Valuable& v);
    Valuable(Valuable&& v);
    Valuable(int i = 0);

    virtual ~Valuable();
    virtual Valuable operator -() const;
    virtual Valuable& operator +=(const Valuable&);
    virtual Valuable& operator +=(int);
    virtual Valuable& operator *=(const Valuable&);
    virtual Valuable& operator /=(const Valuable&);
    virtual Valuable& operator %=(const Valuable&);
    virtual Valuable& operator--();
    virtual Valuable& operator++();
    virtual Valuable& operator^=(const Valuable&);
    virtual bool operator<(const Valuable& smarter) const;
    virtual bool operator==(const Valuable& smarter) const;
    virtual Valuable abs() const;
    virtual void optimize(); /// if it simplifies than it should become the type
    virtual Valuable sqrt() const;
    friend std::ostream& operator<<(std::ostream& out, const Valuable& obj);
    
    virtual const Variable* FindVa() const;
    virtual void CollectVa(std::set<Variable>& s) const;
    virtual void Eval(const Variable& va, const Valuable& v);
    virtual bool IsComesBefore(const Valuable& v) const;
    
    bool OfSameType(const Valuable& v) const;
    bool Same(const Valuable& v) const;
    explicit operator bool() const;
    virtual explicit operator int() const;
    virtual explicit operator size_t() const;
    virtual explicit operator double() const;
    virtual explicit operator long double() const;
    
    size_t Hash() const;
    std::string str() const;
};

    template<>
    const Accessor* Valuable::cast(const Valuable& v);

    struct HashCompare
    {
        bool operator()(const Valuable&, const Valuable&) const;
    };
    
    size_t hash_value(const omnn::extrapolator::Valuable& v);
    
    void implement();
}}

namespace std
{
	omnn::extrapolator::Valuable abs(const omnn::extrapolator::Valuable& v);
	omnn::extrapolator::Valuable sqrt(const omnn::extrapolator::Valuable& v);
    
    template<>
    struct hash<omnn::extrapolator::Valuable> {
        size_t operator()(const omnn::extrapolator::Valuable& v) const {
            return v.Hash();
        }
    };
}

::omnn::extrapolator::Valuable operator"" _v(unsigned long long v);
::omnn::extrapolator::Valuable operator"" _v(long double v);

#define IMPLEMENT { implement(); throw; }

