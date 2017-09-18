#include <unittest++/UnitTest++.h>
#include <string.h>
#include <ork/atomic.h>
#include <ork/reflect.inl>
///////////////////////////////////////////////////////////////////////////////
using namespace ork;
///////////////////////////////////////////////////////////////////////////////
namespace refl_test_1 {
///////////////////////////////////////////////////////////////////////////////

struct A : public reflect::Object
{
    ReflectClass(); 
};

struct B : public A
{
    ReflectClass(); 
};

struct C : public B
{
    ReflectClass(); 

    std::map<std::string,int> _intmap;
    std::map<std::string,A*> _objmap;
};

///////////////////////////////////////////////////////////////////////////////

void A::Describe( reflect::Description& desc )
{
}

void B::Describe( reflect::Description& desc )
{

}

void C::Describe( reflect::Description& desc )
{
    AddMapProperty(C,"intmap",_intmap);
    AddMapProperty(C,"objmap",_objmap);
}

///////////////////////////////////////////////////////////////////////////////
} // namespace refl_test_1

TEST(Reflect1)
{
    ImplementAbstractClass(refl_test_1::A);
    ImplementAbstractClass(refl_test_1::B);
    ImplementConcreteClass(refl_test_1::C);

    //////////////////////////////

    auto reflstream = R"XXX(

    {
        'class': 'C',
        'intmap': {

        }.
        'objmap': {
            
        }
    }

    )XXX";

    //////////////////////////////
    
    auto deser = reflect::fromJson(reflstream);    

    assert(deser!=nullptr);

    auto as_c = dynamic_cast<refl_test_1::C*>(deser);

    assert(as_c!=nullptr);

}

