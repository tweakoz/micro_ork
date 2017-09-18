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

    std::map<std::string,int>     _intmap;
    std::map<std::string,Object*> _objmap;
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
    // todo - prop inheritance

    AddMapProperty(C,"intmap",_intmap);
    AddMapProperty(C,"objmap",_objmap);
}

///////////////////////////////////////////////////////////////////////////////
} // namespace refl_test_1

TEST(Reflect1)
{
    ImplementAbstractClass(refl_test_1::A);
    ImplementConcreteClass(refl_test_1::B);
    ImplementConcreteClass(refl_test_1::C);

    //////////////////////////////

    auto reflstream = R"XXX(

    {
        "class": "refl_test_1::C",
        "intmap": {
            "yo": 1,
            "what": 2,
            "up": 7
        },
        "objmap": {
            "what": {
                "class": "refl_test_1::C",
                "intmap": {
                    "one": 1,
                    "two": 2,
                    "four": 4
                }
            },
            "the": {
                "class": "refl_test_1::C",
                "intmap": {
                    "six": 6,
                    "seven": 7
                }
            }
        }
    }

    )XXX";

    //////////////////////////////
    
    auto deser = reflect::fromJson(reflstream);    

    assert(deser!=nullptr);

    auto as_c = dynamic_cast<refl_test_1::C*>(deser);

    assert(as_c!=nullptr);

    for( auto item : as_c->_intmap )
    {
        const auto& K = item.first;
        const auto& V = item.second;

        printf( "intmap k<%s> -> v<%d>\n", K.c_str(), V );
    }

    auto what = dynamic_cast<refl_test_1::C*>(as_c->_objmap["what"]);

    assert(what!=nullptr);

    CHECK_EQUAL(what->_intmap["one"],1);
    CHECK_EQUAL(what->_intmap["two"],2);
    CHECK_EQUAL(what->_intmap["four"],4);
}

