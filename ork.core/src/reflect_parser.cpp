#include <ork/reflect.h>
#include <rapidjson/reader.h>
#include <rapidjson/document.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

void _decodeJson(const rapidjson::Value& jsonvalue,propdec_t& decoded)
{
    ///////////////////////////////
    // numeric types
    //  lets not pretend
    //  JSON uses doubles
    //  it will make parsing downstream easier
    ///////////////////////////////
    if( jsonvalue.IsNumber() )
    {   if( jsonvalue.IsInt() )
            decoded.Set<double>(jsonvalue.GetInt());
        else if( jsonvalue.IsUint() )
            decoded.Set<double>(jsonvalue.GetUint());
        else if( jsonvalue.IsInt64() )
            decoded.Set<double>(jsonvalue.GetInt64());
        else if( jsonvalue.IsUint64() )
            decoded.Set<double>(jsonvalue.GetUint64());
        else if( jsonvalue.IsDouble() )
            decoded.Set<double>(jsonvalue.GetDouble());
        else if( jsonvalue.IsFloat() )
            decoded.Set<double>(jsonvalue.GetFloat());
        else {
            assert(false);
        }
    }
    ///////////////////////////////
    // boolean
    ///////////////////////////////
    else if( jsonvalue.IsBool() )
        decoded.Set<bool>(jsonvalue.GetBool());
    ///////////////////////////////
    // string
    ///////////////////////////////
    else if( jsonvalue.IsString() )
        decoded.Set<std::string>(jsonvalue.GetString());
    ///////////////////////////////
    // arrays
    ///////////////////////////////
    else if( jsonvalue.IsArray() ) {
        
        auto& array = decoded.Make<decarray_t>();
        
        for (auto itr = jsonvalue.Begin(); 
                  itr != jsonvalue.End(); 
                ++itr)
        {
            const auto& val = (*itr);
            propdec_t item;
            _decodeJson(val,item);
            array.push_back(item);
        }
    }
    ///////////////////////////////
    // dictionarys (objects)
    ///////////////////////////////
    else if( jsonvalue.IsObject() )
    {
        auto& out_dict = decoded.Make<decdict_t>();

        for (auto itr = jsonvalue.MemberBegin();
                  itr != jsonvalue.MemberEnd();
                ++itr )
        {
            std::string name = itr->name.GetString();
            const auto& val = itr->value;

            propkvpair_t out_pair;
            out_pair.first = name;
            _decodeJson(val,out_pair.second);
            out_dict.push_back(out_pair);
        }
    }
    ///////////////////////////////
    else {
        assert(false);
    }
}

///////////////////////////////////////////////////////////////////////////////

reflect::Object* unpack(const decdict_t& dict )
{
    printf( "unpack dict size<%zu>\n", dict.size() );

    reflect::Object* rval = nullptr;

    /////////////////////////////

    std::string clazz_name;
    Class* clazz = nullptr;

    for( const auto& item : dict )
    {   
        if( auto as_string = item.first.TryAs<std::string>() )
        {   
            const auto& name = as_string.value();
            
            if(name=="class")
            {
                clazz_name = item.second.Get<std::string>();
                clazz = FindClass(clazz_name);
                printf( "clazz<%s:%p>\n", clazz_name.c_str(), clazz );
                assert(clazz->isConcrete());      
                rval = clazz->_factory();
                printf( "clazz<%s> object<%p>\n", clazz_name.c_str(), rval );        
            }
            else if( rval and clazz )
            {
                const auto& value = item.second;
                //auto val_t = val.GetType();

                printf( "clazz<%p:%s> prop<%s>\n", clazz, clazz_name.c_str(), name.c_str() );

                auto it_prop = clazz->_properties.find(name);
                assert(it_prop != clazz->_properties.end());

                auto prop = it_prop->second;

                prop->set(rval,value);
     
            }
        }
    }

    printf( "unpack rval<%p>\n", rval );

    return rval;
}

///////////////////////////////////////////////////////////////////////////////

reflect::Object* fromJson(const std::string& jsondata )
{
    rapidjson::Document document;
    document.Parse(jsondata.c_str());
    assert(document.IsObject());

    propdec_t decoded;
    _decodeJson(document,decoded);

    return unpack(decoded.Get<decdict_t>());

}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
