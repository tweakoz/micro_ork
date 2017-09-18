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

///////////////////////////////////////////////////////////////////////////////
template <typename to_type> to_type refl_convert( const propdec_t& from );
///////////////////////////////////////////////////////////////////////////////

template<>
std::string refl_convert<std::string>(const propdec_t& from)
{   if(auto try_string = from.TryAs<std::string>() )
        return try_string.value();
    assert(false); // from's value type not convertible to string
    return "";
}

///////////////////////////////////////////////////////////////////////////////

template<>
int refl_convert<int>(const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (int) try_number.value();
    assert(false); // from's value type not convertible to int
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

template<>
Object* refl_convert<Object*>(const propdec_t& from)
{   if(auto try_dict = from.TryAs<decdict_t>() )
        return unpack(try_dict.value());
    assert(false); // from's value type not convertible to Object*
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

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

            key_t out_k = refl_convert<key_t>(K);
            val_t out_v = refl_convert<val_t>(V);

            auto instance = (clazz_t*) object;

            map_type& out_map = instance->*(this->_member);

            out_map[out_k] = out_v;
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
