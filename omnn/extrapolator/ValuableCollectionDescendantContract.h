//
// Created by Сергей Кривонос on 01.09.17.
//
#pragma once
#include "Exponentiation.h"

namespace omnn{
namespace extrapolator {

    template <class Chld, class ContT>
    class ValuableCollectionDescendantContract
        : public ValuableDescendantContract<Chld>
    {
        using base = ValuableDescendantContract<Chld>;
        
    protected:
        using cont = ContT;
        virtual cont& GetCont() = 0;
        
    public:
        virtual const cont& GetConstCont() const = 0;

//        template<class... T>
//        ValuableCollectionDescendantContract(const T&... vals)
//        {
//            for (const Valuable& arg : { Valuable(vals)... })
//            {
//                Add(arg);
//            }
//        }
        
        auto begin()
        {
            return GetCont().begin();
        }

        auto end()
        {
            return GetCont().end();
        }

        auto begin() const
        {
            return GetConstCont().begin();
        }

        auto end() const
        {
            return GetConstCont().end();
        }
        
        size_t size() const
        {
            return GetConstCont().size();
        }

        virtual void Add(typename ContT::const_reference item)
        {
            GetCont().insert(item);
            Valuable::hash ^= item.Hash();
        }

        template<class T>
        auto GetFirstOccurence() const
        {
            auto& c = GetConstCont();
            auto e = c.end();
            for(auto i = c.begin(); i != e; ++i)
            {
                auto v = T::cast(*i);
                if(v)
                    return i;
            }
            return e;
        }
        
        bool HasValueType(const std::type_info& type) const
        {
            for(const auto& a : GetConstCont())
            {
                if(typeid(a) == type)
                    return true;
            }
            return false;
        }
        
        bool Has(const Valuable& v) const
        {
            for(const auto& a : GetConstCont())
            {
                if(a==v) return true;
            }
            return false;
        }
        
        const Variable* FindVa() const override
        {
            for (auto& i : GetConstCont())
            {
                auto va = i.FindVa();
                if (va)
                    return va;
            }
            return nullptr;
        }
        
        void CollectVa(std::set<Variable>& s) const override
        {
            for (auto& i : GetConstCont())
                i.CollectVa(s);
        }
        
        void Eval(const Variable& va, const Valuable& v) override
        {
            auto& c = GetCont();
            auto e = c.end();
            bool updated;
            do
            {
                updated = {};
                for(auto i = c.begin(); i != e;)
                {
                    auto co = *i;
                    co.Eval(va,v);
                    if (!i->Same(co))
                    {
                        Update(i,co);
                        updated = true;
                    }
                    else
                    {
                        ++i;
                    }
                }
            } while (updated);
//            this->optimize();
        }

        virtual void Update(typename cont::iterator& it, const Valuable& v)
        {
            auto& c = GetCont();
            Delete(it);
            it = c.insert(it, v);
            Valuable::hash ^= v.Hash();
        }

        virtual void Delete(typename cont::iterator& it)
        {
            Valuable::hash ^= it->Hash();
            auto& c = GetCont();
            c.erase(it++);
        }

        Valuable& operator^=(const Valuable& v) override
        {
            return base::Become(Exponentiation(*this,v));
        }
        
        bool operator ==(const Valuable& v) const override
        {
            auto c = Chld::cast(v);
            return c
                && Valuable::hash == v.Hash()
                && GetConstCont()==c->GetConstCont();
        }
        
        template<class T>
        bool Visit(const std::function<void(Valuable&)>& f)
        {
            bool wasVisits = {};
            auto& c = GetCont();
            auto e = c.end();
            for(auto i = c.begin(); i != e; ++i)
            {
                auto v = T::cast(*i);
                if(v)
                {
                    f(CollectionAccessor(*v, *this));
                    wasVisits = true;
                }
            }
            return wasVisits;
        }
    };
}}