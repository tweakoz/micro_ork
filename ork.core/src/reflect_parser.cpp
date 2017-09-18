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
    ///////////////////////////////
    if( jsonvalue.IsNumber() )
    {   if( jsonvalue.IsInt() )
            decoded.Set<int32_t>(jsonvalue.GetInt());
        else if( jsonvalue.IsUint() )
            decoded.Set<uint32_t>(jsonvalue.GetUint());
        else if( jsonvalue.IsInt64() )
            decoded.Set<int64_t>(jsonvalue.GetInt64());
        else if( jsonvalue.IsUint64() )
            decoded.Set<uint64_t>(jsonvalue.GetUint64());
        else if( jsonvalue.IsDouble() )
            decoded.Set<double>(jsonvalue.GetDouble());
        else if( jsonvalue.IsFloat() )
            decoded.Set<float>(jsonvalue.GetFloat());
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

void _setProperty(reflect::Object* obj, Property* prop, const rapidjson::Value& jsonvalue )
{
    propdec_t decoded;
    _decodeJson(jsonvalue,decoded);
    prop->set(obj,decoded);
}

///////////////////////////////////////////////////////////////////////////////

reflect::Object* fromJson(const rapidjson::Value* jsonobj )
{
    reflect::Object* rval = nullptr;

    bool has_class = (jsonobj->HasMember("class"));

    /////////////////////////////
    // was class pre-specified ?
    /////////////////////////////

    std::string clazz_name;
    Class* clazz = nullptr;

    if( has_class )
    {
        clazz_name = (*jsonobj)["class"].GetString();
        clazz = FindClass(clazz_name);

        rval = clazz->_factory();

        printf( "clazz<%s> object<%p>\n", clazz_name.c_str(), rval );        
    }

    for (auto itr = jsonobj->MemberBegin();
              itr != jsonobj->MemberEnd();
            ++itr )
    {
        std::string name = itr->name.GetString();
        if( name == "class" ) continue; // skip class (we already grabbed it)

        const auto& val = itr->value;
        auto val_t = val.GetType();

        printf( "clazz<%p:%s> prop<%s>\n", clazz, clazz_name.c_str(), name.c_str() );

        auto it_prop = clazz->_properties.find(name);
        assert(it_prop != clazz->_properties.end());

        auto prop = it_prop->second;

        _setProperty(rval,prop,val);
        //printf("Type of member %s is %s\n",
        //    itr->name.GetString(), kTypeNames[itr->value.GetType()]);
    }

    return rval;
}

///////////////////////////////////////////////////////////////////////////////

reflect::Object* fromJson(const std::string& jsondata )
{
    rapidjson::Document document;
    document.Parse(jsondata.c_str());

    assert(document.IsObject());

    return fromJson(&document);

}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
