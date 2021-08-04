/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

// g++ spike-server.cpp -Lprecompiled -lopen62541 -o spike-server

#include "precompiled/open62541.h"
#include <signal.h>
#include <stdlib.h>
#include <vector>
#include <string>


using namespace std::string_literals;

constexpr auto VAR_COUNT = 1;

static const UA_NodeId baseDataVariableType = {
    0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEDATAVARIABLETYPE}};

char * leak_memory(std::string src) {
    auto ptr = new char[src.size()]{};
    mempcpy(ptr, src.c_str(), src.size());
    return ptr;
}

std::string get_var_name(int index) {
    return "server_var_"s + std::to_string(index);
}

std::string get_node_id_name(int index) {
    return "node_id_"s + std::to_string(index);
}

static void
addVariable(UA_Server *server, int index) {
    /* add a static variable node to the server */
    UA_VariableAttributes myVar = UA_VariableAttributes_default;
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", leak_memory(get_var_name(index)));
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    UA_Int32 myInteger = index;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, leak_memory(get_var_name(index)));
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, leak_memory(get_node_id_name(index)));
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName, baseDataVariableType,
                              myVar, NULL, NULL);
}

static volatile UA_Boolean running = true;
static void
stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int
main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    for(int i = 0; i < VAR_COUNT; i++) {
        addVariable(server, i);
    }

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
