#include "face_service_impl.hpp"
#include "face_service_internal.hpp"

#include "record_impl.hpp"
#include "search_result_impl.hpp"
#include <json.h>

#include <algorithm>
#include <vector>
#include <utility>

namespace glasssix::irisviel
{
    namespace
    {
        exposing::param_vector<bool> from_bool_vector(const std::vector<bool>& source)
        {
            auto result = exposing::make_param_vector<bool>();

            result.resize(source.size());

            for (std::size_t i = 0; i < source.size(); i++)
            {
                result.set_at(i, source[i]);
            }

            return result;
        }

        record create_record(const database_record& internal_record)
        {
            auto record = exposing::make_as_first<record_impl>();

            record.init(internal_record.dimension());
            record.key(exposing::to_param_string(internal_record.key()));
            record.feature(internal_record.feature());

            return record;
        }

        std::shared_ptr<database_record> create_internal_record(const record& record)
        {
            auto feature = record.feature();
            auto internal_record = database_record::create(record.dimension());
            std::vector<float> internal_feature(exposing::begin(feature), exposing::end(feature));

            internal_record->key(exposing::to_narrow_string(record.key()));
            internal_record->feature(internal_feature);

            return internal_record;
        }

        exposing::param_vector<search_result> create_search_result(const std::vector<database_search_result>& internal_result)
        {
            auto result = exposing::make_param_vector<search_result>();

            for (const auto& item : internal_result)
            {
                result.push_back(exposing::make_as_first<search_result_impl>(item));
            }

            return result;
        }

        template<typename Container>
        void check_dimension(Container&& feature, int dimension)
        {
            if (std::forward<Container>(feature).size() != static_cast<std::size_t>(dimension))
            {
                throw exposing::abi_invalid_argument{ exposing::format(u8"The feature size {} is different from {}.", std::forward<Container>(feature).size(), dimension) };
            }
        }
    }

    face_service_impl::face_service_impl()
    {
    }

    face_service_impl::~face_service_impl()
    {
    }
    void face_service_impl::init(const exposing::param_string& str_params)
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        if (!reader.parse(exposing::to_narrow_string(str_params), root))
            throw Json::Exception("parse json failed");
        std::string working_directory = root["working_directory"].asString();
        int implementation = root.get("implementation_type", Json::Int(0)).asInt();
        int single_database_capacity = root.get("single_database_capacity", Json::Int(1000)).asInt();
        int dimension = root["dimension"].asInt();

        impl_ = std::make_unique<face_service_internal>(static_cast<face_service_implemention>(implementation), single_database_capacity, dimension, exposing::to_narrow_string(working_directory));
    }
    exposing::param_string face_service_impl::version() const
    {
        return u8"1.0.0";
    }

    exposing::param_string face_service_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
    {
        if (!impl_)
            throw exposing::abi_invalid_operation(u8"irisviel internal object not initialized");

        Json::Reader reader(Json::Features::strictMode());
        Json::FastWriter writer;
        Json::Value root, value;
        if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
            throw Json::Exception("parse json failed");

        int command = root["command"].asInt();
        switch (command)
        {
        case 0://load_databases
            impl_->load_databases();
            break;
        case 1://search
            {
                auto assuming_top = root.get("top", Json::nullValue);
                auto assuming_min_similarity = root.get("min_similarity", Json::nullValue);
                bool has_top = assuming_top.isIntegral();
                bool has_min_similarity = assuming_min_similarity.isNumeric();

                std::vector<float> feature;
                for (auto& i : root["feature"])
                    feature.push_back(i.asFloat());

                if (impl_->dimension() * sizeof(float) != feature.size())
                    throw exposing::abi_invalid_argument(u8"impl_->dimension() * sizeof(float) != input_data.size()");

                std::vector<database_search_result> result;
                if (has_top && has_min_similarity)
                {
                    result = impl_->search(reinterpret_cast<float*>(feature.data()), assuming_min_similarity.asFloat(), std::optional<int>(assuming_top.asInt()));
                }

                if (has_top)
                {
                    result = impl_->search(reinterpret_cast<float*>(feature.data()), assuming_top.asInt());
                }

                if (has_min_similarity)
                {
                    result = impl_->search(reinterpret_cast<float*>(feature.data()), assuming_min_similarity.asFloat(), std::nullopt);
                }

                Json::Value record_array = Json::Value(Json::arrayValue);
                for (size_t i = 0; i < result.size(); i++)
                {
                    Json::Value record;
                    record["similarity"] = Json::Value(result[i].similarity);
                    record["data"]["key"] = std::string(result[i].data->key());
                    auto feature = result[i].data->feature();
                    Json::Value feature_array = Json::Value(Json::arrayValue);
                    for (size_t j = 0; j < feature.size(); j++)
                        feature_array.append(Json::Value(feature[j]));
                    record["data"]["feature"] = feature_array;

                    record_array.append(record);
                }
                value["search_result"] = record_array;
            }
            break;
        case 2://search_nf
            {
                auto assuming_top = root.get("top", Json::nullValue);
                auto assuming_min_similarity = root.get("min_similarity", Json::nullValue);
                bool has_top = assuming_top.isIntegral();
                bool has_min_similarity = assuming_min_similarity.isNumeric();

                std::vector<float> feature;
                for (auto& i : root["feature"])
                    feature.push_back(i.asFloat());

                if (impl_->dimension() * sizeof(float) != feature.size())
                    throw exposing::abi_invalid_argument(u8"impl_->dimension() * sizeof(float) != input_data.size()");

                std::vector<database_search_result> result;
                if (has_top && has_min_similarity)
                {
                    result = impl_->search_nf(reinterpret_cast<float*>(feature.data()), assuming_min_similarity.asFloat(), std::optional<int>(assuming_top.asInt()));
                }
                else
                    throw exposing::abi_invalid_argument(u8"has_top && has_min_similarity");

                Json::Value record_array = Json::Value(Json::arrayValue);
                for (size_t i = 0; i < result.size(); i++)
                {
                    Json::Value record;
                    record["similarity"] = Json::Value(result[i].similarity);
                    record["data"]["key"] = std::string(result[i].data->key());

                    record_array.append(record);
                }
                value["search_result"] = record_array;
            }
            break;
        case 3://contains_key
            {
                auto key = root["key"].asString();
                value["contains_key_result"] = Json::Value(impl_->contains_key(key));
            }
        break;
        case 4://clear
            impl_->clear();
        break;
        case 5://remove_all
            impl_->remove_all();
            break;
        case 6://remove_records
            {
                std::vector<std::string> keys;
                for (auto& i : root["keys"])
                    keys.push_back(i.asString());
                std::vector<bool> ret = impl_->remove(keys);
                Json::Value ret_array = Json::Value(Json::arrayValue);
                for (auto& i : ret)
                {
                    Json::Value ret_i;
                    ret_i["success"] = Json::Value(i);
                    ret_i["reason"] = Json::Value(i ? u8"OK" : u8"Could not find the key.");
                    ret_array.append(ret_i);
                }
                value["remove_records_result"] = ret_array;
            }
            break;
        case 7://add_records
            {
                std::vector<std::shared_ptr<database_record>> internal_records;
                for (const auto& item : root["data"])
                {
                    std::vector<float> feature;
                    for (const auto& i : item["feature"])
                        feature.push_back(i.asFloat());

                    auto internal_record = database_record::create(feature.size());

                    internal_record->key(exposing::to_narrow_string(item["key"].asString()));
                    internal_record->feature(feature);

                    internal_records.emplace_back(internal_record);
                }
                auto ret = impl_->add(internal_records);

                Json::Value ret_array = Json::Value(Json::arrayValue);
                for (auto& i : ret)
                {
                    Json::Value ret_i;
                    ret_i["success"] = Json::Value(i);
                    ret_i["reason"] = Json::Value(i ? u8"OK" : u8"The key already exists.");
                    ret_array.append(ret_i);
                }
                value["add_records_result"] = ret_array;
            }
            break;
        case 8://update_records
            {
                std::vector<std::shared_ptr<database_record>> internal_records;
                for (const auto& item : root["data"])
                {
                    std::vector<float> feature;
                    for (const auto& i : item["feature"])
                        feature.push_back(i.asFloat());

                    auto internal_record = database_record::create(feature.size());

                    internal_record->key(exposing::to_narrow_string(item["key"].asString()));
                    internal_record->feature(feature);

                    internal_records.emplace_back(internal_record);
                }
                auto ret = impl_->update(internal_records);

                Json::Value ret_array = Json::Value(Json::arrayValue);
                for (auto& i : ret)
                {
                    Json::Value ret_i;
                    ret_i["success"] = Json::Value(i);
                    ret_i["reason"] = Json::Value(i ? u8"OK" : u8"Could not find the key.");
                    ret_array.append(ret_i);
                }
                value["update_records_result"] = ret_array;
            }
            break;
        default:
            break;
        }
        value["command"] = root["command"];
        return exposing::to_param_string(writer.write(value));
    }

    void face_service_impl::clear() const
    {
        impl_->clear();
    }

    void face_service_impl::remove_all() const
    {
        impl_->remove_all();
    }

    std::int32_t face_service_impl::dimension() const
    {
        return impl_->dimension();
    }

    exposing::param_string face_service_impl::database_directory() const
    {
        return exposing::to_param_string(impl_->database_directory());
    }

    exposing::param_string face_service_impl::cache_directory() const
    {
        return exposing::to_param_string(impl_->cache_directory());
    }

    exposing::param_string face_service_impl::lsh_directory() const
    {
        return exposing::to_param_string(impl_->lsh_directory());
    }

    void face_service_impl::load_databases() const
    {
        impl_->load_databases();
    }

    std::uint64_t face_service_impl::record_count() const
    {
        return impl_->record_count();
    }

    bool face_service_impl::contains_key(const exposing::param_string& key) const
    {
        return impl_->contains_key(key);
    }

    record face_service_impl::try_get_record(const exposing::param_string& key) const
    {
        if (auto inner_record = impl_->try_get_record(exposing::to_narrow_string(key)))
        {
            return create_record(*inner_record);
        }

        return nullptr;
    }

    exposing::param_vector<bool> face_service_impl::add_records(const exposing::param_vector<record>& records) const
    {
        std::vector<std::shared_ptr<database_record>> internal_records;

        for (const auto& item : records)
        {
            internal_records.emplace_back(create_internal_record(item));
        }

        return from_bool_vector(impl_->add(internal_records));
    }

    exposing::param_vector<bool> face_service_impl::remove_records(const exposing::param_vector<exposing::param_string>& keys) const
    {
        std::vector<std::string> internal_keys;

        for (const auto& item : keys)
        {
            internal_keys.emplace_back(exposing::to_narrow_string(item));
        }

        return from_bool_vector(impl_->remove(internal_keys));
    }

    exposing::param_vector<bool> face_service_impl::update_records(const exposing::param_vector<record>& records) const
    {
        std::vector<std::shared_ptr<database_record>> internal_records;

        for (const auto& item : records)
        {
            internal_records.emplace_back(create_internal_record(item));
        }

        return from_bool_vector(impl_->update(internal_records));
    }

    exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, std::uint32_t top_count_to_retrieve) const
    {
        check_dimension(feature, dimension());

        auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), top_count_to_retrieve);

        return create_search_result(internal_result);
    }

    exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, float min_similarity) const
    {
        check_dimension(feature, dimension());

        auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), min_similarity, std::nullopt);

        return create_search_result(internal_result);
    }

    exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
    {
        check_dimension(feature, dimension());

        auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), min_similarity, top_count_to_retrieve);

        return create_search_result(internal_result);
    }

    exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, std::uint32_t top_count_to_retrieve) const
    {

        check_dimension(feature, dimension());

        auto internal_result = impl_->search(feature.data(), top_count_to_retrieve);

        return create_search_result(internal_result);
    }

    exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, float min_similarity) const
    {
        check_dimension(feature, dimension());

        auto internal_result = impl_->search(feature.data(), min_similarity, std::nullopt);

        return create_search_result(internal_result);
    }

    exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
    {
        check_dimension(feature, dimension());
        
        auto internal_result = impl_->search(feature.data(), min_similarity, top_count_to_retrieve);

        return create_search_result(internal_result);
    }
    exposing::param_vector<search_result> face_service_impl::search_nf(const exposing::param_vector<float>& feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
    {
        check_dimension(feature, dimension());

        auto internal_result = impl_->search_nf(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), min_similarity, top_count_to_retrieve);

        return create_search_result(internal_result);
    }
}
