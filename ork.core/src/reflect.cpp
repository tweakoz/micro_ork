#include <ork/reflect.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace reflect {
///////////////////////////////////////////////////////////////////////////////

std::map<std::string,Class*> _classes;

///////////////////////////////////////////////////////////////////////////////

Class* FindClass( const std::string& name )
{
    auto it = _classes.find(name);
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
