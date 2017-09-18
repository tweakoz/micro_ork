#pragma once

#include "reflect.h"

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

template <typename clazz_t, typename map_type>
MapProperty<clazz_t,map_type>::MapProperty(map_type clazz_t::* m)
    : _member(m)
{

}

template <typename clazz_t, typename map_type>
void MapProperty<clazz_t,map_type>::set( Object* object, const propdec_t& inpdata ) //final 
{
    if( auto try_as_dict = inpdata.TryAs<decdict_t>() )
    {
        auto& as_dict = try_as_dict.value();

        for( const auto& item : as_dict )
        {
            const auto& K = item.first;
            const auto& V = item.second;

            key_t out_k;
            val_t out_v;

            assert(false);
            // this should be fun...
            //if( auto try_int32 = K.TryAs<int32_t>() )
            //    out_k = (key_t) try_int32.value();
        }        
    }
    else
    {
        assert(false);
    }
}


///////////////////////////////////////////////////////////////////////////////

template <typename classtype, typename map_type>
Property* Description::_addMapProperty(const char* name, map_type classtype::* m)
{

    Property* prop = new MapProperty<classtype,map_type>(m);
    auto clazz = classtype::getClassStatic();
    printf( "addmapprop clazz<%p> name<%s> prop<%p>\n", clazz, name, prop );
    clazz->_properties[name] = prop;
    return prop;
}

///////////////////////////////////////////////////////////////////////////////

extern std::map<std::string,Class*> _classes;

template <typename class_type>
Class* RegisterClass(const std::string& cname, 
                     const std::string& pname,
                     factory_t fac,
                     describe_t desCB )
{
    auto it = _classes.find(cname);
    assert(it==_classes.end());
    auto clazz = class_type::getClassStatic();
    _classes[cname]=clazz;
    clazz->_factory = fac;
    
    Description desc(clazz);

    if(desCB)
        desCB(desc);

    return clazz;
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
