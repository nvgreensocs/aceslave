#ifndef ACE_SLAVE_WRAPPER_HPP_
#define ACE_SLAVE_WRAPPER_HPP_

#include "ace_slave.hpp"

sc_core::sc_object* create_ace_slave (const char * instance_name)
{
    ACE_slave<128>* ace_slave_instance = new ACE_slave<128>(instance_name,"debug_port","ace_slave");
    return (sc_core::sc_object*)(ace_slave_instance);
};

void destroy_ace_slave(sc_core::sc_object* instance){
    delete ((ACE_slave<128>*)instance);
};

#endif /*ACE_SLAVE_WRAPPER_HPP_*/
