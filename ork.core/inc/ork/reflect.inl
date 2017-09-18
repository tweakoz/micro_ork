#pragma once

#include "reflect.h"
#include <ork/CVector2.h>
#include <ork/CVector3.h>
#include <ork/CVector4.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

template <typename clazz_t, typename map_type>
MapProperty<clazz_t,map_type>::MapProperty(map_type clazz_t::* m)
    : _member(m)
{
}

template <typename clazz_t, typename val_type>
DirectProperty<clazz_t,val_type>::DirectProperty(val_type clazz_t::* m)
    : _member(m)
{
}

///////////////////////////////////////////////////////////////////////////////
template <typename to_type> to_type refl_convert( const Property* prop, const propdec_t& from );
///////////////////////////////////////////////////////////////////////////////

template<>
std::string refl_convert<std::string>(const Property* prop, const propdec_t& from)
{   if(auto try_string = from.TryAs<std::string>() )
        return try_string.value();
    assert(false); // from's value type not convertible to string
    return "";
}

///////////////////////////////////////////////////////////////////////////////

template<>
int32_t refl_convert<int32_t>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (int32_t) try_number.value();
    else if(auto try_string = from.TryAs<std::string>() )
        return (int32_t) atoi(try_string.value().c_str());
    assert(false); // from's value type not convertible to int32_t
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

template<>
uint32_t refl_convert<uint32_t>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (uint32_t) try_number.value();
    assert(false); // from's value type not convertible to uint32_t
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

template<>
int64_t refl_convert<int64_t>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (int64_t) try_number.value();
    assert(false); // from's value type not convertible to int64_t
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

template<>
uint64_t refl_convert<uint64_t>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (uint64_t) try_number.value();
    assert(false); // from's value type not convertible to uint64_t
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

template<>
float refl_convert<float>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return (float) try_number.value();
    assert(false); // from's value type not convertible to float
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

template<>
double refl_convert<double>(const Property* prop, const propdec_t& from)
{   if(auto try_number = from.TryAs<double>() )
        return try_number.value();
    assert(false); // from's value type not convertible to float
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

template<>
Object* refl_convert<Object*>(const Property* prop, const propdec_t& from)
{   if(auto try_dict = from.TryAs<decdict_t>() )
        return unpack(try_dict.value(),prop->_annotations);
    assert(false); // from's value type not convertible to Object*
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

template<>
fvec2 refl_convert<fvec2>(const Property* prop, const propdec_t& from)
{   if(auto try_array = from.TryAs<decarray_t>() )
    {   const auto& array = try_array.value();
        assert(array.size()==2);
        float x = array[0].Get<double>();
        float y = array[1].Get<double>();
        return fvec2(x,y);
    }
    assert(false); // from's value type not convertible to Object*
    return fvec2();
}

///////////////////////////////////////////////////////////////////////////////

template<>
fvec3 refl_convert<fvec3>(const Property* prop, const propdec_t& from)
{   if(auto try_array = from.TryAs<decarray_t>() )
    {   const auto& array = try_array.value();
        assert(array.size()==3);
        float x = array[0].Get<double>();
        float y = array[1].Get<double>();
        float z = array[2].Get<double>();
        return fvec3(x,y,z);
    }
    assert(false); // from's value type not convertible to Object*
    return fvec3();
}

///////////////////////////////////////////////////////////////////////////////

template<>
fvec4 refl_convert<fvec4>(const Property* prop, const propdec_t& from)
{   if(auto try_array = from.TryAs<decarray_t>() )
    {   const auto& array = try_array.value();
        assert(array.size()==4);
        float x = array[0].Get<double>();
        float y = array[1].Get<double>();
        float z = array[2].Get<double>();
        float w = array[3].Get<double>();
        return fvec4(x,y,z,w);
    }
    assert(false); // from's value type not convertible to Object*
    return fvec4();
}

///////////////////////////////////////////////////////////////////////////////

template<>
std::shared_ptr<Object> refl_convert<std::shared_ptr<Object>>(const Property* prop, const propdec_t& from)
{   if(auto try_dict = from.TryAs<decdict_t>() )
        return std::shared_ptr<Object>(unpack(try_dict.value(),prop->_annotations));
    assert(false); // from's value type not convertible to Object*
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

template <typename clazz_t, typename map_type>
void MapProperty<clazz_t,map_type>::set( Object* object, const propdec_t& inpdata ) const //final 
{   if( auto try_as_dict = inpdata.TryAs<decdict_t>() )
    {   auto& as_dict = try_as_dict.value();
        for( const auto& item : as_dict )
        {   const auto& K = item.first;
            const auto& V = item.second;
            key_t out_k = refl_convert<key_t>(this,K);
            val_t out_v = refl_convert<val_t>(this,V);
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

template <typename clazz_t, typename val_type>
void DirectProperty<clazz_t,val_type>::set( Object* object, const propdec_t& inpdata ) const //final 
{   auto instance = (clazz_t*) object;
    val_type& destination = instance->*(this->_member);    
    if( auto try_as_dict = inpdata.TryAs<decdict_t>() )
    {   val_type value = refl_convert<val_type>(this,try_as_dict.value());
        destination = value;
    }
    else if( auto try_as_ary = inpdata.TryAs<decarray_t>() )
    {   val_type value = refl_convert<val_type>(this,try_as_ary.value());
        destination = value;
    }
    else if( auto try_as_number = inpdata.TryAs<double>() )
    {   val_type value = refl_convert<val_type>(this,try_as_number.value());
        destination = value;
    }
    else if( auto try_as_string = inpdata.TryAs<std::string>() )
    {   val_type value = refl_convert<val_type>(this,try_as_string.value());
        destination = value;
    }
    else
    {   
        assert(false);
    }
}

///////////////////////////////////////////////////////////////////////////////

template <typename classtype, typename map_type>
Property* Description::_addMapProperty(const char* name, map_type classtype::* m)
{   Property* prop = new MapProperty<classtype,map_type>(m);
    auto clazz = classtype::getClassStatic();
    clazz->_properties[name] = prop;
    return prop;
}

///////////////////////////////////////////////////////////////////////////////

template <typename classtype, typename val_type>
Property* Description::_addDirectProperty(const char* name, val_type classtype::* m)
{   Property* prop = new DirectProperty<classtype,val_type>(m);
    auto clazz = classtype::getClassStatic();
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
    clazz->_name = cname;
    clazz->_parent = class_type::getParentClassStatic();

    _classes[cname]=clazz;
    clazz->_factory = fac;
    
    Description desc(clazz);

    if(desCB)
        desCB(desc);


    return clazz;
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
