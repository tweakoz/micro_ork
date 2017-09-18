///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

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

struct TypeConverterBase
{
    TypeConverterBase(const Property* prop) : _prop(prop) {}
    const Property* _prop;
};

template<typename T,typename SFINAE = void> struct TypeConverter : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<std::string> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    std::string convert(const propdec_t& from)
    {   if(auto try_string = from.TryAs<std::string>() )
            return try_string.value();
        assert(false); // from's value type not convertible to string
        return "";
    }
};

///////////////////////////////////////////////////////////////////////////////
template<> struct TypeConverter<int32_t> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    int32_t convert(const propdec_t& from)
    {   if(auto try_number = from.TryAs<double>() )
            return (int32_t) try_number.value();
        else if(auto try_string = from.TryAs<std::string>() )
            return (int32_t) atoi(try_string.value().c_str());
        assert(false); // from's value type not convertible to int32_t
        return 0;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<uint32_t> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    uint32_t convert(const propdec_t& from)
    {   if(auto try_number = from.TryAs<double>() )
            return (uint32_t) try_number.value();
        assert(false); // from's value type not convertible to uint32_t
        return 0;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<int64_t> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    int64_t convert(const propdec_t& from)
    {   if(auto try_number = from.TryAs<double>() )
            return (int64_t) try_number.value();
        assert(false); // from's value type not convertible to int64_t
        return 0;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<uint64_t> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    uint64_t convert(const propdec_t& from)
    {   if(auto try_number = from.TryAs<double>() )
            return (uint64_t) try_number.value();
        assert(false); // from's value type not convertible to uint64_t
        return 0;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<float> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    float convert(const propdec_t& from)
    {   if(auto try_number = from.TryAs<double>() )
            return (float) try_number.value();
        assert(false); // from's value type not convertible to float
        return 0.0f;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<double> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    double convert(const propdec_t& from)
    {   
        if(auto try_number = from.TryAs<double>() )
            return try_number.value();
        assert(false); // from's value type not convertible to float
        return 0.0f;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TypeConverter<T*> : public TypeConverterBase {

    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    T* convert(const propdec_t& from)
    {   
        static_assert( std::is_base_of<::ork::reflect::Object, T>::value, "yo" );

        if(auto try_dict = from.TryAs<decdict_t>() )
        {
            auto obj = unpack(try_dict.value(),_prop->_annotations);
            auto as_t = dynamic_cast<T*>(obj);
            return as_t;
        }
        assert(false); // from's value type not convertible to Object*
        return nullptr;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TypeConverter<std::shared_ptr<T>> : public TypeConverterBase {

    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    std::shared_ptr<T> convert(const propdec_t& from)
    {   
        static_assert( std::is_base_of<::ork::reflect::Object, T>::value, "yo" );

        if(auto try_dict = from.TryAs<decdict_t>() )
        {
            auto obj = unpack(try_dict.value(),_prop->_annotations);
            auto as_t = dynamic_cast<T*>(obj);
            return std::shared_ptr<T>(as_t);
        }
        assert(false); // from's value type not convertible to Object*
        return nullptr;
    }
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<fvec2> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    fvec2 convert(const propdec_t& from)
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
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<fvec3> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    fvec3 convert(const propdec_t& from)
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
};

///////////////////////////////////////////////////////////////////////////////

template<> struct TypeConverter<fvec4> : public TypeConverterBase
{
    TypeConverter(const Property* prop) : TypeConverterBase(prop) {}

    fvec4 convert(const propdec_t& from)
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
};

///////////////////////////////////////////////////////////////////////////////

template <typename clazz_t, typename map_type>
void MapProperty<clazz_t,map_type>::set( Object* object, const propdec_t& inpdata ) const //final 
{   if( auto try_as_dict = inpdata.TryAs<decdict_t>() )
    {   auto& as_dict = try_as_dict.value();
        for( const auto& item : as_dict )
        {   const auto& K = item.first;
            const auto& V = item.second;
            TypeConverter<key_t> keycon(this);
            TypeConverter<val_t> valcon(this);
            key_t out_k = keycon.convert(K);
            val_t out_v = valcon.convert(V);
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
    {   TypeConverter<val_type> typecon(this);
        val_type value = typecon.convert(try_as_dict.value());
        destination = value;
    }
    else if( auto try_as_ary = inpdata.TryAs<decarray_t>() )
    {   TypeConverter<val_type> typecon(this);
        val_type value = typecon.convert(try_as_ary.value());
        destination = value;
    }
    else if( auto try_as_number = inpdata.TryAs<double>() )
    {   TypeConverter<val_type> typecon(this);
        val_type value = typecon.convert(try_as_number.value());
        destination = value;
    }
    else if( auto try_as_string = inpdata.TryAs<std::string>() )
    {   TypeConverter<val_type> typecon(this);
        val_type value = typecon.convert(try_as_string.value());
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
