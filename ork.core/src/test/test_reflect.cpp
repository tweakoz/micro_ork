///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <unittest++/UnitTest++.h>
#include <string.h>
#include <ork/atomic.h>
#include <ork/reflect.inl>
#include <ork/CVector3.h>

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

    C() : _directInt(0)
    {

    }

    ~C() final
    {
        for( auto item : _objmap )
            delete item.second;
    }

    std::map<std::string,A*> _objmap;
    std::shared_ptr<Object> _shobj;
    std::map<int,fvec2> _vmap;

    int _directInt;
    std::string _directString;
    float _directF32;
    double _directF64;
    fvec2 _directVec2;
    fvec3 _directVec3;
    fvec4 _directVec4;

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
    auto vmapprop = AddMapProperty(C,"vmap",_vmap);
    auto shobjprop = AddDirectProperty(C,"shobj",_shobj);
    auto dintprop = AddDirectProperty(C,"dint",_directInt);
    auto dstrprop = AddDirectProperty(C,"dstr",_directString);
    auto df32prop = AddDirectProperty(C,"df32",_directF32);
    auto df64prop = AddDirectProperty(C,"df64",_directF64);
    auto dvec2prop = AddDirectProperty(C,"dvec2",_directVec2);
    auto dvec3prop = AddDirectProperty(C,"dvec3",_directVec3);
    auto dvec4prop = AddDirectProperty(C,"dvec4",_directVec4);

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
                },
                "dstr": "lulz",
                "shobj": {
                    "class": "refl_test_1::C",
                    "intmap": {
                        "nine": 9,
                        "ten": 10
                    },
                    "dint": 99,
                    "dstr": "it_works",
                    "df32": 3.14,
                    "df64": 1e9,
                    "dvec2": [0.5,0.5],
                    "dvec3": [0,1,0],
                    "dvec4": [1,0,1,0.5],
                    "vmap": {
                        "0": [0,0],
                        "1": [1,0],
                        "2": [0,1],
                        "3": [1,1]
                    }
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
    CHECK_EQUAL("lulz",the_as_c->_directString);

    //////////////////////////////
    // inspect "the"'s shobj child
    //////////////////////////////

    auto shobj = the_as_c->_shobj;

    assert(shobj!=nullptr);

    auto sh_as_c = std::dynamic_pointer_cast<refl_test_1::C>(shobj);

    assert(sh_as_c!=nullptr);

    CHECK_EQUAL(9,sh_as_c->_intmap["nine"]);
    CHECK_EQUAL(10,sh_as_c->_intmap["ten"]);
    CHECK_EQUAL(99,sh_as_c->_directInt);
    CHECK_EQUAL("it_works",sh_as_c->_directString);
    CHECK_EQUAL(3.14f,sh_as_c->_directF32);
    CHECK_EQUAL(1e9,sh_as_c->_directF64);

    CHECK_EQUAL(0.5f,sh_as_c->_directVec2.x);
    CHECK_EQUAL(0.5f,sh_as_c->_directVec2.y);

    CHECK_EQUAL(0,sh_as_c->_directVec3.x);
    CHECK_EQUAL(1,sh_as_c->_directVec3.y);
    CHECK_EQUAL(0,sh_as_c->_directVec3.z);

    CHECK_EQUAL(1.0f,sh_as_c->_directVec4.x);
    CHECK_EQUAL(0.0f,sh_as_c->_directVec4.y);
    CHECK_EQUAL(1.0f,sh_as_c->_directVec4.z);
    CHECK_EQUAL(0.5f,sh_as_c->_directVec4.w);

    auto& VMAP = sh_as_c->_vmap;
    for( int i=0; i<3; i++ )
    {
        float bit0 = float(i&1);
        float bit1 = float(i>>1);
        auto& V = VMAP[i];  
        CHECK_EQUAL(bit0,V.x);
        CHECK_EQUAL(bit1,V.y);
    }


    //////////////////////////////
    // clean up
    //////////////////////////////

    delete deser;
}

