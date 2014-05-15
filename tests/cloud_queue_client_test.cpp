// -----------------------------------------------------------------------------------------
// <copyright file="cloud_queue_client_test.cpp" company="Microsoft">
//    Copyright 2013 Microsoft Corporation
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//      http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// </copyright>
// -----------------------------------------------------------------------------------------

#include "stdafx.h"
#include "test_helper.h"
#include "was/queue.h"

SUITE(QueueClient)
{
    TEST(QueueClient_Empty)
    {
        azure::storage::cloud_queue_client client;

        CHECK(client.base_uri().primary_uri().is_empty());
        CHECK(client.base_uri().secondary_uri().is_empty());
        CHECK(client.credentials().is_anonymous());
    }

    TEST(QueueClient_BaseUri)
    {
        azure::storage::storage_uri base_uri(web::http::uri(U("https://myaccount.queue.core.windows.net")), web::http::uri(U("https://myaccount-secondary.queue.core.windows.net")));

        azure::storage::cloud_queue_client client(base_uri);

        CHECK(client.base_uri().primary_uri() == base_uri.primary_uri());
        CHECK(client.base_uri().secondary_uri() == base_uri.secondary_uri());
        CHECK(client.credentials().is_anonymous());
    }

    TEST(QueueClient_BaseUriAndCredentials)
    {
        azure::storage::storage_uri base_uri(web::http::uri(U("https://myaccount.queue.core.windows.net")), web::http::uri(U("https://myaccount-secondary.queue.core.windows.net")));
        azure::storage::storage_credentials credentials(U("devstoreaccount1"), U("Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw=="));
        azure::storage::queue_request_options default_request_options;

        azure::storage::cloud_queue_client client(base_uri, credentials);

        CHECK(client.base_uri().primary_uri() == base_uri.primary_uri());
        CHECK(client.base_uri().secondary_uri() == base_uri.secondary_uri());
        CHECK(client.credentials().is_shared_key());
    }

    TEST(QueueClient_BaseUriAndCredentialsAndDefaultRequestOptions)
    {
        azure::storage::storage_uri base_uri(web::http::uri(U("https://myaccount.queue.core.windows.net")), web::http::uri(U("https://myaccount-secondary.queue.core.windows.net")));
        azure::storage::storage_credentials credentials(U("devstoreaccount1"), U("Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw=="));
        azure::storage::queue_request_options default_request_options;

        azure::storage::cloud_queue_client client(base_uri, credentials, default_request_options);

        CHECK(client.base_uri().primary_uri() == base_uri.primary_uri());
        CHECK(client.base_uri().secondary_uri() == base_uri.secondary_uri());
        CHECK(client.credentials().is_shared_key());

        default_request_options.set_location_mode(azure::storage::location_mode::secondary_only);

        client = azure::storage::cloud_queue_client(base_uri, credentials, default_request_options);

        CHECK(client.default_request_options().location_mode() == azure::storage::location_mode::secondary_only);
    }

    TEST(ListQueues_Normal)
    {
        const int QUEUE_COUNT = 5;

        azure::storage::cloud_queue queues[QUEUE_COUNT];
        bool is_found[QUEUE_COUNT];
        for (int i = 0; i < QUEUE_COUNT; ++i)
        {
            azure::storage::cloud_queue queue = get_queue();

            queue.metadata().insert(std::make_pair(U("aaa"), U("111")));
            queue.metadata().insert(std::make_pair(U("bbb"), U("222")));
            queue.upload_metadata();

            queues[i] = queue;
            is_found[i] = false;
        }

        azure::storage::cloud_queue_client client = get_queue_client();

        {
            utility::string_t prefix;
            bool get_metadata;
            azure::storage::queue_request_options options;
            azure::storage::operation_context context;

            prefix = object_name_prefix;
            get_metadata = false;

            std::vector<azure::storage::cloud_queue> results = client.list_queues(prefix, get_metadata, options, context);

            CHECK(results.size() >= QUEUE_COUNT);

            for (std::vector<azure::storage::cloud_queue>::const_iterator itr = results.cbegin(); itr != results.cend(); ++itr)
            {
                azure::storage::cloud_queue queue = *itr;

                for (int i = 0; i < QUEUE_COUNT; ++i)
                {
                    if (queue.name().compare(queues[i].name()) == 0)
                    {
                        CHECK(!queue.service_client().base_uri().primary_uri().is_empty());
                        CHECK(queue.service_client().credentials().is_shared_key());
                        CHECK(!queue.name().empty());
                        CHECK(!queue.uri().primary_uri().is_empty());

                        CHECK(queue.metadata().empty());

                        is_found[i] = true;
                    }
                }
            }

            CHECK(!context.client_request_id().empty());
            CHECK(context.start_time().is_initialized());
            CHECK(context.end_time().is_initialized());
            CHECK_EQUAL(1, context.request_results().size());
            CHECK(context.request_results()[0].is_response_available());
            CHECK(context.request_results()[0].start_time().is_initialized());
            CHECK(context.request_results()[0].end_time().is_initialized());
            CHECK(context.request_results()[0].target_location() != azure::storage::storage_location::unspecified);
            CHECK_EQUAL(web::http::status_codes::OK, context.request_results()[0].http_status_code());
            CHECK(!context.request_results()[0].service_request_id().empty());
            CHECK(context.request_results()[0].request_date().is_initialized());
            CHECK(context.request_results()[0].content_md5().empty());
            CHECK(context.request_results()[0].etag().empty());
            CHECK(context.request_results()[0].extended_error().code().empty());
            CHECK(context.request_results()[0].extended_error().message().empty());
            CHECK(context.request_results()[0].extended_error().details().empty());
        }

        {
            utility::string_t prefix;
            bool get_metadata;
            azure::storage::queue_request_options options;
            azure::storage::operation_context context;

            prefix = object_name_prefix;
            get_metadata = true;

            std::vector<azure::storage::cloud_queue> results = client.list_queues(prefix, get_metadata, options, context);

            CHECK(results.size() >= QUEUE_COUNT);

            for (std::vector<azure::storage::cloud_queue>::const_iterator itr = results.cbegin(); itr != results.cend(); ++itr)
            {
                azure::storage::cloud_queue queue = *itr;

                for (int i = 0; i < QUEUE_COUNT; ++i)
                {
                    if (queue.name().compare(queues[i].name()) == 0)
                    {
                        CHECK(!queue.service_client().base_uri().primary_uri().is_empty());
                        CHECK(queue.service_client().credentials().is_shared_key());
                        CHECK(!queue.name().empty());
                        CHECK(!queue.uri().primary_uri().is_empty());

                        CHECK_EQUAL(2U, queue.metadata().size());
                        CHECK(queue.metadata()[U("aaa")].compare(U("111")) == 0);
                        CHECK(queue.metadata()[U("bbb")].compare(U("222")) == 0);

                        is_found[i] = true;
                    }
                }
            }

            CHECK(!context.client_request_id().empty());
            CHECK(context.start_time().is_initialized());
            CHECK(context.end_time().is_initialized());
            CHECK_EQUAL(1, context.request_results().size());
            CHECK(context.request_results()[0].is_response_available());
            CHECK(context.request_results()[0].start_time().is_initialized());
            CHECK(context.request_results()[0].end_time().is_initialized());
            CHECK(context.request_results()[0].target_location() != azure::storage::storage_location::unspecified);
            CHECK_EQUAL(web::http::status_codes::OK, context.request_results()[0].http_status_code());
            CHECK(!context.request_results()[0].service_request_id().empty());
            CHECK(context.request_results()[0].request_date().is_initialized());
            CHECK(context.request_results()[0].content_md5().empty());
            CHECK(context.request_results()[0].etag().empty());
            CHECK(context.request_results()[0].extended_error().code().empty());
            CHECK(context.request_results()[0].extended_error().message().empty());
            CHECK(context.request_results()[0].extended_error().details().empty());
        }

        for (int i = 0; i < QUEUE_COUNT; ++i)
        {
            queues[i].delete_queue();
        }
    }

    TEST(ListQueues_Segmented)
    {
        const int QUEUE_COUNT = 5;

        azure::storage::cloud_queue queues[QUEUE_COUNT];
        bool is_found[QUEUE_COUNT];
        for (int i = 0; i < QUEUE_COUNT; ++i)
        {
            queues[i] = get_queue();
            is_found[i] = false;
        }

        azure::storage::cloud_queue_client client = get_queue_client();

        utility::string_t prefix;
        bool get_metadata;
        int max_results;
        azure::storage::continuation_token token;
        azure::storage::queue_request_options options;
        azure::storage::operation_context context;

        prefix = object_name_prefix;
        get_metadata = false;
        max_results = 3;

        int segment_count = 0;
        azure::storage::queue_result_segment result_segment;
        do
        {
            result_segment = client.list_queues_segmented(prefix, get_metadata, max_results, token, options, context);
            std::vector<azure::storage::cloud_queue> results = result_segment.results();

            CHECK(results.size() >= 0);
            CHECK((int)results.size() <= max_results);

            for (std::vector<azure::storage::cloud_queue>::const_iterator itr = results.cbegin(); itr != results.cend(); ++itr)
            {
                azure::storage::cloud_queue queue = *itr;

                for (int i = 0; i < QUEUE_COUNT; ++i)
                {
                    if (queue.name().compare(queues[i].name()) == 0)
                    {
                        CHECK(!queue.service_client().base_uri().primary_uri().is_empty());
                        CHECK(queue.service_client().credentials().is_shared_key());
                        CHECK(!queue.name().empty());
                        CHECK(!queue.uri().primary_uri().is_empty());

                        is_found[i] = true;
                    }
                }
            }

            CHECK(!context.client_request_id().empty());
            CHECK(context.start_time().is_initialized());
            CHECK(context.end_time().is_initialized());
            CHECK_EQUAL(segment_count + 1, context.request_results().size());
            CHECK(context.request_results()[segment_count].is_response_available());
            CHECK(context.request_results()[segment_count].start_time().is_initialized());
            CHECK(context.request_results()[segment_count].end_time().is_initialized());
            CHECK(context.request_results()[segment_count].target_location() != azure::storage::storage_location::unspecified);
            CHECK_EQUAL(web::http::status_codes::OK, context.request_results()[segment_count].http_status_code());
            CHECK(!context.request_results()[segment_count].service_request_id().empty());
            CHECK(context.request_results()[segment_count].request_date().is_initialized());
            CHECK(context.request_results()[segment_count].content_md5().empty());
            CHECK(context.request_results()[segment_count].etag().empty());
            CHECK(context.request_results()[segment_count].extended_error().code().empty());
            CHECK(context.request_results()[segment_count].extended_error().message().empty());
            CHECK(context.request_results()[segment_count].extended_error().details().empty());

            ++segment_count;
            token = result_segment.continuation_token();
        }
        while (!token.empty());

        CHECK(segment_count > 1);

        for (int i = 0; i < QUEUE_COUNT; ++i)
        {
            queues[i].delete_queue();
        }
    }
}
