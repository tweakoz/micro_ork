#include <ork/reflect.h>
#include <rapidjson/reader.h>
#include <rapidjson/document.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// _decodeJson - recursive function
//   which converts json object tree representation into
//   an svariant based object tree representation (aka propdec_t)
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
    // array
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
    // dictionary (object)
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
// unpack (unmarshal) propdec_t based object representation
//  into an actual object tree/graph
///////////////////////////////////////////////////////////////////////////////

reflect::Object* unpack(const decdict_t& dict, 
                        const AnnoMap& annos )
{
    reflect::Object* rval = nullptr;

    /////////////////////////////

    std::string clazz_name;
    Class* clazz = nullptr;

    auto createObject = [&](const std::string& classname){
        assert(rval==nullptr);
        assert(clazz_name=="");
        clazz_name = classname;
        clazz = FindClass(clazz_name);        
        assert(clazz->isConcrete());      
        rval = clazz->_factory();
    };

    /////////////////////////////
    // first try map val objclass (if annotation set)
    /////////////////////////////

    if( auto try_mvclass = annos.find("map.val.objclass")
                         . TryAs<Class*>() )
        createObject(try_mvclass.value()->_name);

    /////////////////////////////
    // loop through dict items
    /////////////////////////////

    for( const auto& item : dict )
    {   
        if( auto as_string = item.first.TryAs<std::string>() )
        {   
            const auto& name = as_string.value();
            
            /////////////////////////////
            // class in dict ?
            /////////////////////////////

            if(name=="class")
                createObject(item.second.Get<std::string>());

            /////////////////////////////

            else if( rval and clazz )
            {   const auto& value = item.second;
                auto prop = clazz->findProperty(name);
                if( prop )
                    prop->set(rval,value);
                else
                    printf( "property<%s> not found, skipping...\n", name.c_str() );
            }
        }
    }

    return rval;
}

///////////////////////////////////////////////////////////////////////////////
// unmarshall object graph from json
///////////////////////////////////////////////////////////////////////////////

reflect::Object* fromJson(const std::string& jsondata )
{
    rapidjson::Document document;
    document.Parse(jsondata.c_str());
    assert(document.IsObject());

    propdec_t decoded;
    _decodeJson(document,decoded);

    static AnnoMap g_no_annos;

    return unpack(decoded.Get<decdict_t>(),g_no_annos);
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
