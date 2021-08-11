/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

// g++ -O3 spike-client.cpp -Lprecompiled -lopen62541 -o spike-client

#include "precompiled/open62541.h"
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include <string>


using namespace std::string_literals;

constexpr auto VAR_COUNT = 2000;

static long long
current_timestamp_in_us(void) {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long microseconds = te.tv_sec * 1000 * 1000 + te.tv_usec;
    return microseconds;
}

char *leak_memory(std::string src)
{
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

std::vector<UA_NodeId> NODE_IDS{};
void init_node_ids() {
    for (auto i = 0; i < VAR_COUNT; i++) {
        NODE_IDS.push_back(UA_NODEID_STRING(1, leak_memory(get_node_id_name(i))));
    }
}

std::vector<UA_ReadValueId> generate_values_to_read() {
    auto result = std::vector<UA_ReadValueId>();
    for (auto i = 0; i < VAR_COUNT; i++) {
        UA_ReadValueId value;
        UA_ReadValueId_init(&value);
        value.nodeId = NODE_IDS[i];
        value.attributeId = UA_ATTRIBUTEID_VALUE;
        result.push_back(value);
    }

    return result;
}

static unsigned
read_variables(UA_Client *client, void *out) {
    auto values_to_read = generate_values_to_read();
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = values_to_read.data();
    request.nodesToReadSize = values_to_read.size();
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize == values_to_read.size()) {
            auto first_item = response.results[0];
            retval = first_item.status;
        } else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_clear(&response);
        return retval;
    }

    UA_DataValue *res = response.results;
    if(res->hasStatus)
        retval = res->status;

    if(!res->hasValue) {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        UA_ReadResponse_clear(&response);
        return retval;
    }

    memcpy(out, &res->value, sizeof(UA_Variant));
    UA_Variant_init(&res->value);
    
    UA_ReadResponse_clear(&response);
    return retval;
}

int
main(void) {
    init_node_ids();

    auto program_start = current_timestamp_in_us();
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    constexpr auto ONE_SECOND = 1'000'000;
    constexpr auto pumpFuelLevelONDS = 10 * ONE_SECOND;
    const long long EXECUTION_TIME = ONE_SECOND / 10;
    long long requests_start = current_timestamp_in_us();
    long long processed_requests = 0;
    auto ts_before_requests = current_timestamp_in_us();
    while(true) {
        if((current_timestamp_in_us() - requests_start) > EXECUTION_TIME) {
            break;
        }
        long long before_ts = current_timestamp_in_us();

        UA_Variant value;
        UA_Variant_init(&value);
        retval = read_variables(client, &value);

        if(retval == UA_STATUSCODE_GOOD) {
            processed_requests += 1;
            // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
            //             "Array len: %ld\n", value.arrayLength);
        }

        // UA_sleep_ms(100);

        // printf("Request took %lldus\n", current_timestamp_in_us() - before_ts);
    }
    printf("TS: %lldms; Processed %lld requests and %lld Data Points in %lldms\n", current_timestamp_in_us() / 1000, processed_requests, processed_requests * VAR_COUNT, (current_timestamp_in_us() - ts_before_requests) / 1000);
    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
