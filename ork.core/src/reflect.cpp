///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/reflect.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

std::map<std::string,Class*> _classes;

///////////////////////////////////////////////////////////////////////////////

Class* FindClass( const std::string& name )
{
    auto it = _classes.find(name);
    if( it == _classes.end() )
    {
        //printf( "cannot find class<%s>\n", name.c_str() );
        return nullptr;
    }

    assert(it!=_classes.end());
    return it->second;
}

static anno_t g_no_anno(nullptr);

const anno_t& AnnoMap::find(const std::string& key) const
{
    auto it = _annomap.find(key);
    if(it==_annomap.end())
        return g_no_anno;
    else
        return it->second;
}

///////////////////////////////////////////////////////////////////////////////

void Property::set( UnpackContext& ctx, Object* object, const propdec_t& inpdata ) const
{
    ctx._propstack.push(this);
    {
        doSet(ctx,object,inpdata);
    }
    ctx._propstack.pop();
}

///////////////////////////////////////////////////////////////////////////////

const Property* Class::findProperty(const std::string& name) const
{
    auto it_prop = _properties.find(name);
    if( it_prop != _properties.end())
        return it_prop->second;
    else if( _parent )
        return _parent->findProperty(name);

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void init()
{
    printf( "initializing reflection...\n");

    for( auto item : _classes )
    {
        auto name = item.first;
        auto clazz = item.second;

        printf( "class< %p %s >\n", clazz, name.c_str() );
    }
}

///////////////////////////////////////////////////////////////////////////////

void exit()
{

}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork { namespace reflect {
