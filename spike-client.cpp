/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

// g++ spike-client.cpp -Lprecompiled -lopen62541 -o spike-client

#include "precompiled/open62541.h"
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include <string>


using namespace std::string_literals;

constexpr auto VAR_COUNT = 1;

static long long
current_timestamp(void) {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long microseconds = te.tv_sec * 1000 * 1000 + te.tv_usec;
    return microseconds;
}

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

std::vector<UA_NodeId> NODE_IDS{};
void init_node_ids() {
    for (auto i = 0; i < VAR_COUNT; i++) {
        NODE_IDS.push_back(UA_NODEID_STRING(0, leak_memory(get_var_name(i))));
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

    /* Set the StatusCode */
    UA_DataValue *res = response.results;
    if(res->hasStatus)
        retval = res->status;

    /* Return early of no value is given */
    if(!res->hasValue) {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        UA_ReadResponse_clear(&response);
        return retval;
    }

    /* Copy value into out */
    memcpy(out, &res->value, sizeof(UA_Variant));
    UA_Variant_init(&res->value);
    
    UA_ReadResponse_clear(&response);
    return retval;
}

int
main(void) {
    init_node_ids();

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    constexpr auto TEN_SECONDS = 10'000'000;
    const long long EXECUTION_TIME = TEN_SECONDS;
    long long one_sec_start = current_timestamp();
    long long processed_requests = 0;
    while(true) {
        if((current_timestamp() - one_sec_start) > EXECUTION_TIME) {
            break;
        }

        long long before_ts = current_timestamp();
        /* Read the value attribute of the node. UA_Client_readValueAttribute is a
         * wrapper for the raw read service available as UA_Client_Service_read. */
        UA_Variant value; /* Variants can hold scalar values and arrays of any type */
        UA_Variant_init(&value);

        retval = read_variables(client, &value);

        // if(retval == UA_STATUSCODE_GOOD &&
        //    UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        //     UA_DateTime raw_date = *(UA_DateTime *)value.data;
        //     UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        //     (void)dts;
        //     processed_requests += 1;
        //     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        //                 "date is: %u-%u-%u %u:%u:%u.%03u\n", dts.day, dts.month,
        //                 dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        // }

        UA_Variant_clear(&value);

        // UA_sleep_ms(100);

        printf("Request took about %lldus\n", current_timestamp() - before_ts);
    }
    printf("Processed %lld requests in %lldus\n", processed_requests, EXECUTION_TIME);
    printf("Speed: %lld req/second\n",
           processed_requests / (EXECUTION_TIME / 1000));

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
