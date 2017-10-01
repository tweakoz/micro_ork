///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ork/svariant.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <stack>
#include <memory>

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace reflect {

    struct Object;
    struct Description;
    struct Class;
    struct UnpackContext;

    typedef ork::svar64_t propdec_t;
    typedef std::pair<propdec_t,propdec_t> propkvpair_t;
    typedef std::vector<propdec_t> decarray_t;
    typedef std::vector<propkvpair_t> decdict_t;

    typedef std::function<Object*(const std::string& clazzname)> factory_t;
    typedef std::function<void(Description&)> describe_t;
    typedef ork::svar64_t anno_t;
    typedef std::function<factory_t(const std::string&)> classhandler_t; 
    typedef std::function<bool(Object*,const std::string& propname,propdec_t)> prophandler_t;

    template <typename class_type>
    Class* RegisterClass(const std::string& cname, 
                         const std::string& pname,
                         factory_t fac,
                         describe_t desCB );

    //////////////////////////////////////////////////////////////

    struct AnnoMap
    {
        std::map<std::string,anno_t> _annomap;
        const anno_t& find(const std::string& key) const;
    };

    //////////////////////////////////////////////////////////////

    struct Property
    {

        void annotate(const std::string& k, anno_t v) { _annotations._annomap[k]=v; }
        void set( UnpackContext& ctx, Object* object, const propdec_t& inpdata ) const;
        virtual void doSet( UnpackContext& ctx, Object* object, const propdec_t& inpdata ) const = 0;
        AnnoMap _annotations;
    };

    //////////////////////////////////////////////////////////////

    template <typename clazz_t, typename map_type>
    struct MapProperty : public Property
    {
        typedef typename map_type::key_type key_t;
        typedef typename map_type::mapped_type val_t;
       
        MapProperty( map_type clazz_t::* m);

        void doSet( UnpackContext& ctx, Object* object, const propdec_t& inpdata ) const final;

        map_type clazz_t::* _member;
    };

    //////////////////////////////////////////////////////////////

    template <typename clazz_t, typename value_type>
    struct DirectProperty : public Property
    {       
        DirectProperty( value_type clazz_t::* m);

        void doSet( UnpackContext& ctx, Object* object, const propdec_t& inpdata ) const final;

        value_type clazz_t::* _member;
    };

    //////////////////////////////////////////////////////////////

    struct Object
    {
        virtual ~Object() {}

        static ::ork::reflect::Class* getClassStatic() { return nullptr; }
        virtual ::ork::reflect::Class* getClassDynamic() const = 0;

        std::string _guid;
    };

    //////////////////////////////////////////////////////////////

    struct Class
    {
        bool isAbstract() const { return _factory==nullptr; }
        bool isConcrete() const { return _factory!=nullptr; }

        const Property* findProperty(const std::string& name) const;

        std::map<std::string,Property*> _properties;
        factory_t _factory = nullptr;
        Class* _parent = nullptr;
        std::string _name;
        AnnoMap _annotations;
    };

    //////////////////////////////////////////////////////////////

    Class* FindClass( const std::string& name );

    //////////////////////////////////////////////////////////////

    struct Description
    {
        Description(Class* c) : _c(c) {}

        template <typename classtype, typename map_type>
        Property* _addMapProperty(const char* name, map_type classtype::* member);

        template <typename classtype, typename val_type>
        Property* _addDirectProperty(const char* name, val_type classtype::* member);

        void annotateClass(const std::string& k, anno_t v) { _c->_annotations._annomap[k]=v; }

        Class* _c;
    };

    //////////////////////////////////////////////////////////////


    Object* fromJson(const std::string& jsondata );

    void init();
    void exit();

    struct UnpackContext
    {
        UnpackContext() 
        {

        }

        void pushVar(const std::string& key, anno_t v)
        {
            _ctxvarstack[key].push(v);
        }
        void popVar(const std::string& key)
        {
            auto itv = _ctxvarstack.find(key);

            if(itv != _ctxvarstack.end() )
            {
                std::stack<anno_t>& varstack = itv->second;

                if( ! varstack.empty() )
                    varstack.pop();
            }
        }

        const anno_t& findAnno(const std::string& key)
        {
            auto itv = _ctxvarstack.find(key);

            if(itv != _ctxvarstack.end() )
            {
                std::stack<anno_t>& varstack = itv->second;

                if( ! varstack.empty() )
                    return varstack.top();
            }

            const Property* prop = _propstack.empty()
                                 ? nullptr
                                 : _propstack.top();

            if( prop )
            {
                return prop->_annotations.find(key);
            }

            static const anno_t g_no_anno(nullptr);
            return g_no_anno;

        }

        std::stack<const Property*> _propstack;
        std::map<std::string,std::stack<anno_t>> _ctxvarstack;
        std::map<std::string,std::shared_ptr<Object>> _refmap;
    };

    reflect::Object* unpack(UnpackContext& ctx,const decdict_t& dict);

}} // namespace ork::reflect

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define AddMapProperty(clasnam,propname,the_map)\
    desc._addMapProperty<clasnam>(propname,&clasnam::the_map);

#define AddDirectProperty(clasnam,propname,the_val)\
    desc._addDirectProperty<clasnam>(propname,&clasnam::the_val);

#define DeclareClass(name,basename)\
    public: \
    static void Describe(::ork::reflect::Description& desc);\
    static ::ork::reflect::Class* getClassStatic() { static reflect::Class _clazz; return & _clazz; }\
    static ::ork::reflect::Class* getParentClassStatic() { return basename::getClassStatic(); }\
    ::ork::reflect::Class* getClassDynamic() const override { return getClassStatic(); }

#define BEGIN_REFLECTED_CLASS(name,basename) \
    struct name : public basename {\
    DeclareClass(name,basename);

#define END_REFLECTED_CLASS };

#define ImplementAbstractClass(cname) ::ork::reflect::RegisterClass<cname>( \
    #cname, \
    "",\
    nullptr,\
    [](::ork::reflect::Description& desc){\
        cname::Describe(desc);\
    });
    
#define ImplementNamedAbstractClass(cname,ctype) ::ork::reflect::RegisterClass<ctype>( \
    cname, \
    "",\
    nullptr,\
    [](::ork::reflect::Description& desc){\
        ctype::Describe(desc);\
    });

#define ImplementConcreteClass(cname) ::ork::reflect::RegisterClass<cname>( \
    #cname, \
    "",\
    [](const std::string& classname)->::ork::reflect::Object*{\
        return new cname;\
    },\
    [](::ork::reflect::Description& desc){\
        cname::Describe(desc);\
    });

#define ImplementNamedConcreteClass(cname,ctype) ::ork::reflect::RegisterClass<ctype>( \
    cname, \
    "",\
    [](const std::string& classname)->::ork::reflect::Object*{\
        return new ctype;\
    },\
    [](::ork::reflect::Description& desc){\
        ctype::Describe(desc);\
    });

//#define ImplementChildClass(cname,pname) RegisterClass(#cname, #pname);
