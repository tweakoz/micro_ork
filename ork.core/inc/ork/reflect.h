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

    struct Property
    {

        void annotate(const std::string& k, anno_t v) { _annotations[k]=v; }
        virtual void set( Object* object, const propdec_t& inpdata ) = 0;
        std::map<std::string,anno_t> _annotations;
    };

    //////////////////////////////////////////////////////////////

    template <typename clazz_t, typename map_type>
    struct MapProperty : public Property
    {
        typedef typename map_type::key_type key_t;
        typedef typename map_type::mapped_type val_t;
       
        MapProperty( map_type clazz_t::* m);

        void set( Object* object, const propdec_t& inpdata ) final;

        map_type clazz_t::* _member;
    };

    //////////////////////////////////////////////////////////////

    struct Object
    {
        virtual ~Object() {}
    };

    //////////////////////////////////////////////////////////////

    struct Class
    {
        bool isAbstract() const { return _factory==nullptr; }
        bool isConcrete() const { return _factory!=nullptr; }

        std::map<std::string,Property*> _properties;
        factory_t _factory = nullptr;
    };

    //////////////////////////////////////////////////////////////

    Class* FindClass( const std::string& name );

    //////////////////////////////////////////////////////////////

    struct Description
    {
        Description(Class* c) : _c(c) {}

        template <typename classtype, typename map_type>
        Property* _addMapProperty(const char* name, map_type classtype::* member);

        Class* _c;
    };

    //////////////////////////////////////////////////////////////


    Object* fromJson(const std::string& jsondata );

    void init();
    void exit();

    reflect::Object* unpack(const decdict_t& dict );

}} // namespace ork::reflect

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define AddMapProperty(clasnam,propname,the_map)\
    desc._addMapProperty<clasnam>(propname,&clasnam::the_map);


#define ReflectClass() \
    static void Describe(::ork::reflect::Description& desc);\
    static ::ork::reflect::Class* getClassStatic() { static reflect::Class _clazz; return & _clazz; }


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
