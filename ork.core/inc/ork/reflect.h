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

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace reflect {

    struct Object;
    struct Description;
    struct Class;

    typedef std::function<Object*()> factory_t;
    typedef std::function<void(Description&)> describe_t;
    typedef ork::svar64_t anno_t;

    typedef ork::svar64_t propdec_t;
    typedef std::pair<propdec_t,propdec_t> propkvpair_t;
    typedef std::vector<propdec_t> decarray_t;
    typedef std::vector<propkvpair_t> decdict_t;

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
        virtual void set( Object* object, const propdec_t& inpdata ) const = 0;
        AnnoMap _annotations;
    };

    //////////////////////////////////////////////////////////////

    template <typename clazz_t, typename map_type>
    struct MapProperty : public Property
    {
        typedef typename map_type::key_type key_t;
        typedef typename map_type::mapped_type val_t;
       
        MapProperty( map_type clazz_t::* m);

        void set( Object* object, const propdec_t& inpdata ) const final;

        map_type clazz_t::* _member;
    };

    //////////////////////////////////////////////////////////////

    template <typename clazz_t, typename value_type>
    struct DirectProperty : public Property
    {       
        DirectProperty( value_type clazz_t::* m);

        void set( Object* object, const propdec_t& inpdata ) const final;

        value_type clazz_t::* _member;
    };

    //////////////////////////////////////////////////////////////

    struct Object
    {
        virtual ~Object() {}

        static ::ork::reflect::Class* getClassStatic() { return nullptr; }
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

        Class* _c;
    };

    //////////////////////////////////////////////////////////////


    Object* fromJson(const std::string& jsondata );

    void init();
    void exit();

    reflect::Object* unpack(const decdict_t& dict, const AnnoMap& annos );

}} // namespace ork::reflect

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define AddMapProperty(clasnam,propname,the_map)\
    desc._addMapProperty<clasnam>(propname,&clasnam::the_map);

#define AddDirectProperty(clasnam,propname,the_val)\
    desc._addDirectProperty<clasnam>(propname,&clasnam::the_val);

#define BEGIN_REFLECTED_CLASS(name,basename) \
    struct name : public basename {\
    public: \
    static void Describe(::ork::reflect::Description& desc);\
    static ::ork::reflect::Class* getClassStatic() { static reflect::Class _clazz; return & _clazz; }\
    static ::ork::reflect::Class* getParentClassStatic() { return basename::getClassStatic(); }

#define END_REFLECTED_CLASS };

#define ImplementAbstractClass(cname) ::ork::reflect::RegisterClass<cname>( \
    #cname, \
    "",\
    nullptr,\
    [](::ork::reflect::Description& desc){\
        cname::Describe(desc);\
    });
    

#define ImplementConcreteClass(cname) ::ork::reflect::RegisterClass<cname>( \
    #cname, \
    "",\
    []()->::ork::reflect::Object*{\
        return new cname;\
    },\
    [](::ork::reflect::Description& desc){\
        cname::Describe(desc);\
    });

//#define ImplementChildClass(cname,pname) RegisterClass(#cname, #pname);
