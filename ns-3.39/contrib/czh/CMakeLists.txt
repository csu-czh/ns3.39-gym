check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
           test/czh-test-suite.cc
        )
endif()

build_lib(
    LIBNAME czh
    SOURCE_FILES model/ipv4-orca-routing.cc
                 model/ipv4-czh-routing.cc
                 model/udp-orca-client.cc
                 model/udp-orca-server.cc
                 model/entity/MNode.cc
                 model/entity/MPacket.cc
                 model/entity/BloomFilter.cc
                 model/entity/ElmoPacket.cc
                 model/entity/LIPSINPacket.cc
                 model/entity/Session.cc
                 model/entity/Topolopy.cc
                 helper/ipv4-orca-routing-helper.cc
                 helper/ipv4-czh-routing-helper.cc
                 helper/udp-orca-server-client-helper.cc

    HEADER_FILES model/ipv4-orca-routing.h
                 model/ipv4-czh-routing.h
                 model/udp-orca-client.h
                 model/udp-orca-server.h
                 model/entity/BloomFilter.h
                 model/entity/MNode.h
                 model/entity/MPacket.h
                 model/entity/ElmoPacket.h
                 model/entity/LIPSINPacket.h
                 model/entity/Session.h
                 model/entity/Topolopy.h
                 helper/ipv4-orca-routing-helper.h
                 helper/ipv4-czh-routing-helper.h
                 helper/udp-orca-server-client-helper.h
                
    LIBRARIES_TO_LINK ${libcore}
                      ${internet}
    TEST_SOURCES test/czh-test-suite.cc
                 ${examples_as_tests_sources}
)
