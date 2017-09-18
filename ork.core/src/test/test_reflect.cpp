#include <unittest++/UnitTest++.h>
#include <string.h>
#include <ork/atomic.h>
#include <ork/reflect.inl>
///////////////////////////////////////////////////////////////////////////////
using namespace ork;
///////////////////////////////////////////////////////////////////////////////
namespace refl_test_1 {
///////////////////////////////////////////////////////////////////////////////

BEGIN_REFLECTED_CLASS( A, reflect::Object )

END_REFLECTED_CLASS; 

///////////////////////////////////////////////////////////////////////////////

BEGIN_REFLECTED_CLASS( B, A )

    std::map<std::string,int>     _intmap;

END_REFLECTED_CLASS;

///////////////////////////////////////////////////////////////////////////////

BEGIN_REFLECTED_CLASS( C, B ) 

    C(){}

    ~C() final
    {
        for( auto item : _objmap )
            delete item.second;
    }

    std::map<std::string,Object*> _objmap;

END_REFLECTED_CLASS;

///////////////////////////////////////////////////////////////////////////////

void A::Describe( reflect::Description& desc )
{
}

void B::Describe( reflect::Description& desc )
{
    auto imapprop = AddMapProperty(B,"intmap",_intmap);
}

void C::Describe( reflect::Description& desc )
{
    auto omapprop = AddMapProperty(C,"objmap",_objmap);

    omapprop->annotate("map.val.objclass",C::getClassStatic());
}

///////////////////////////////////////////////////////////////////////////////
} // namespace refl_test_1

TEST(Reflect1)
{
    //////////////////////////////
    // init reflection data
    //////////////////////////////

    ImplementAbstractClass(refl_test_1::A);
    ImplementConcreteClass(refl_test_1::B);
    ImplementConcreteClass(refl_test_1::C);

    reflect::init();

    //////////////////////////////
    // test Object Graph JSON
    //////////////////////////////

    auto reflstream = R"JSON(

    {
        "class": "refl_test_1::C",
        "intmap": {
            "yo": 1,
            "what": 2,
            "up": 7
        },
        "objmap": {
            "what": {
                "intmap": {
                    "one": 1,
                    "two": 2,
                    "four": 4
                }
            },
            "the": {
                "intmap": {
                    "six": 6,
                    "seven": 7
                }
            }
        }
    }

    )JSON";

    //////////////////////////////
    // deserialize objects from JSON stream
    //////////////////////////////

    auto deser = reflect::fromJson(reflstream);    
    assert(deser!=nullptr);
    auto root_as_c = dynamic_cast<refl_test_1::C*>(deser);
    assert(root_as_c!=nullptr);

    //////////////////////////////
    // inspect "what" child
    //////////////////////////////

    auto child_what = root_as_c->_objmap["what"];
    auto what_as_c = dynamic_cast<refl_test_1::C*>(child_what);

    assert(what_as_c!=nullptr);

    CHECK_EQUAL(1,what_as_c->_intmap["one"]);
    CHECK_EQUAL(2,what_as_c->_intmap["two"]);
    CHECK_EQUAL(4,what_as_c->_intmap["four"]);

    //////////////////////////////
    // inspect "the" child
    //////////////////////////////

    auto child_the = root_as_c->_objmap["the"];
    auto the_as_c = dynamic_cast<refl_test_1::C*>(child_the);

    assert(the_as_c!=nullptr);

    CHECK_EQUAL(6,the_as_c->_intmap["six"]);
    CHECK_EQUAL(7,the_as_c->_intmap["seven"]);

    //////////////////////////////
    // clean up
    //////////////////////////////

    delete deser;
}

