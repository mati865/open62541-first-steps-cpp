/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

// g++ -O3 spike-server.cpp -Lprecompiled -lopen62541 -o spike-server

#include "precompiled/open62541.h"
#include <signal.h>
#include <stdlib.h>
#include <vector>
#include <string>

using namespace std::string_literals;

constexpr auto VAR_COUNT = 2000;

char *leak_memory(std::string src) {
    auto ptr = new char[src.size() + 1]{};
    mempcpy(ptr, src.c_str(), src.size());
    return ptr;
}

std::string get_var_name(int index) {
    return "servervar"s + std::to_string(index);
}

std::string get_node_id_name(int index) {
    return "nodeid"s + std::to_string(index);
}

static void
addVariable(UA_Server *server, int index) {
    /* add a static variable node to the server */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = index;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", leak_memory(get_var_name(index)));
    attr.description = UA_LOCALIZEDTEXT("en-US", leak_memory(get_var_name(index)));
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    const UA_NodeId currentNodeId = UA_NODEID_STRING(1, leak_memory(get_node_id_name(index)));
    const UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, leak_memory(get_var_name(index)));
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, currentNodeId, parentNodeId,
                              parentReferenceNodeId, currentName, variableTypeNodeId,
                              attr, NULL, NULL);
}

// // TODO: implement this
// static int32_t data_source = 0;

static UA_StatusCode
readCurrentTime(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
    UA_DateTime now = UA_DateTime_now();
    UA_Variant_setScalarCopy(&dataValue->value, &now,
                             &UA_TYPES[UA_TYPES_DATETIME]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static void
addVariableFromExample(UA_Server *server, int index) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", leak_memory(get_var_name(index)));
    attr.description = UA_LOCALIZEDTEXT("en-US", leak_memory(get_var_name(index)));
    // attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    const UA_NodeId currentNodeId = UA_NODEID_STRING(1, leak_memory(get_node_id_name(index)));
    const UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, leak_memory(get_var_name(index)));
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES); 
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource timeDataSource;
    timeDataSource.read = readCurrentTime;
    timeDataSource.write = nullptr;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        timeDataSource, NULL, NULL);
}

static volatile UA_Boolean running = true;
static void
stopHandler(int sig)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    for (int i = 0; i < VAR_COUNT; i++) {
        // addVariable(server, i);
        addVariableFromExample(server, i);
    }

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
