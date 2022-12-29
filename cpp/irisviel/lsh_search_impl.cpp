#include "lsh_search_impl.hpp"
#include "irisviel_types.hpp"
#include "feature_searcher_factory.hpp"
#include "memory_resource_adapter.hpp"

#include "distance.hpp"
#include <cstring>
#include <string.h>
#include <string>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"
#include <ctime>

const int group_sum = 16;
const int one_group_sum = 256;
const int normal_vec_sum = 8;
const int dimension_max = 512;

namespace glasssix::irisviel
{

    class lsh_search_impl::impl
    {
    public:
        impl(int dimension, std::string path) : dimension_{ dimension }, current_data_{}, pool_{ make_synchronized_pool_resource_workaround() }, float_allocator_{ pool_.get() }, size_t_allocator_{ pool_.get() }
        {
            LSH_init();
            clock_t be, ed;
            be = clock();
            LSH_load(path);
            ed = clock();
            std::cout << "load time = " << ed - be << "\n";
        }
        ~impl()
        {
            LSH_clear();
        }

        int dimension() const noexcept
        {
            return dimension_;
        }

        void current_data(const std::vector<database_feature_observer::feature>& data) noexcept
        {
            current_data_ = &data;
        }

        std::vector<std::vector<database_search_result>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k)
        {
            std::vector<std::vector<database_search_result>> result;
            for (auto&& item : query_data)
            {
                result.emplace_back(search_single_vector(item, min_similarity, top_k));
            }
            return result;
        }

        bool add(database_record& record)
        {
            nlohmann::json temp_feature;
            temp_feature.clear();

            exposing::param_span<const float> x = record.feature();

            for (int i = 0; i < dimension_; i++)
            {
                temp_feature["feature"][i] = x[i];
            }
            std::string _key(record.key());
            temp_feature["key"] = _key;
            
            return LSH_addRecord(temp_feature, dimension_);
        }

        std::vector<bool> remove(std::vector<std::string>& keys)
        {
            std::size_t index{};
            std::vector<bool> result(keys.size());

            for (int i = 0; i < keys.size(); i++)
            {
                result[index++] = LSH_removeRecord(keys[i]);
            }
            for (int i = 0; i < test.bucket_sum; i++)
            {
                LSH_updataActive(i);
            }

            return result;
        }

        std::vector<bool> update(const std::vector<std::shared_ptr<database_record>>& records)
        {
            std::size_t index{};
            std::vector<bool> result(records.size());

            for (auto& record : records)
            {
                std::string _key(record->key());
                result[index++] = LSH_removeRecord(_key);
            }
            for (int i = 0; i < test.bucket_sum; i++)
                LSH_updataActive(i);

            index = 0;
            for (auto& record : records)
            {
                nlohmann::json temp_feature;
                std::string _key(record->key());
                temp_feature["key"] = _key;
                exposing::param_span<const float> x = record->feature();
                for (int i = 0; i < dimension_; i++)
                {
                    temp_feature["feature"][i] = x[i];
                }
                result[index++] = LSH_addRecord(temp_feature, dimension_);
            }

            return result;
        }

    private:
        bool LSH_removeRecord(const std::string& _key)
        {
            if (test.key_id.count(_key) == 0)
            {
                return false;
            }

            struct node now = test.key_id[_key];
            for (int i = 0; i < now.bucket_id.size(); i++)
            {
                int x, y;
                x = now.bucket_id[i];
                y = now.key_id[i];
                if (x < one_group_sum)
                    test.set[x].feature[y].clear();
                test.set[x].key[y].clear();
                test.set[x].active = true;
            }
            test.key_id.erase(_key);

            return true;
        }

        bool LSH_addRecord(nlohmann::json &temp_feature, int dimension)
        {
            std::vector<float>feature_float;
            for (int i = 0; i < dimension; i++)
                feature_float.push_back(temp_feature["feature"][i].get<float>());
            std::string _key = temp_feature["key"].get<std::string>();
            nlohmann::json mini_feature;
            mini_feature["key"] = temp_feature["key"];

            if (test.key_id.count(_key))
            {
                return false;
            }
            for (int i = 0; i < group_sum; i++)
            {
                int id = get_bucket_id(feature_float, dimension, i);
                LSH_addRecordTobucket(id, _key, feature_float);

                std::string name = test.path + "/" + std::to_string(id) + ".json";
                if (id < one_group_sum)
                    WriteJsonFile(name, temp_feature);
                else
                    WriteJsonFile(name, mini_feature);
            }

            return true;
        }

        void LSH_load(std::string path)
        {
            std::string BucketListName = path + "/normal_vector.json";
            test.path = path;
            std::ofstream fp;
            fp.open(BucketListName, std::ios::in);
            if (!fp)////  no normal_vector.json file , clear
            {
                nlohmann::json temp;
                for (int k = 0; k < group_sum; k++)
                {
                    for (int i = 0; i < normal_vec_sum; i++)
                    {
                        for (int j = 0; j < dimension_max; j++)
                        {
                            temp[k][i][j] = test.normal_vector[k][i][j];
                        }
                    }
                }
                WriteJsonFile(BucketListName, temp);
                temp.clear();
                for (int i = 0; i < test.bucket_sum; i++)
                {
                    std::string name = path + "/" + std::to_string(i) + ".json";
                    WriteNullFile(name);    //create null file;
                }
            }
            else
            {
                nlohmann::json temp;
                ReadJsonFile_NormalVector(BucketListName, temp);

                for (int k = 0; k < group_sum; k++)
                {
                    for (int i = 0; i < normal_vec_sum; i++)
                    {
                        test.normal_vector[k][i].clear();
                        for (int j = 0; j < dimension_max; j++)
                        {
                            test.normal_vector[k][i].push_back(temp[k][i][j].get<float>());
                        }
                    }
                }
                for (int i = 0; i < test.bucket_sum; i++)
                {
                    temp.clear();
                    std::string name = path + "/" + std::to_string(i) + ".json";
                    ReadJsonFile(i, name);
                }
            }
        }


        void LSH_updataActive(int bucket_id)  //Delay updating to disk
        {
            if (test.set[bucket_id].active == false)
                return;

            std::ifstream fp;
            fp.open(test.path + "/" + std::to_string(bucket_id) + ".json", std::ios::binary);
            std::string result_str;
            nlohmann::json result;
            int i = 0;
            std::vector<nlohmann::json>write_data;

            while (std::getline(fp, result_str))
            {
                if (result_str.length() == 0)
                    break;
                result.clear();
                result = nlohmann::json::parse(result_str);

                std::string _key = result["key"].get<std::string>();

                while (i < test.set[bucket_id].key.size())
                {
                    if (test.set[bucket_id].key[i].length() > 0)
                        break;
                    i++;
                }
                if (i >= test.set[bucket_id].key.size())
                    break;

                if (_key == test.set[bucket_id].key[i])
                {
                    write_data.push_back(result);
                    i++;
                }
            }
            fp.close();

            std::ofstream fpw;
            fpw.open(test.path + "/" + std::to_string(bucket_id) + ".json", std::ios::ate | std::ios::out);
            fpw.close();

            std::ofstream fpo(test.path + "/" + std::to_string(bucket_id) + ".json", std::fstream::out | std::ios::app);
            for (int i = 0; i < write_data.size(); i++)
                fpo << write_data[i].dump()<< "\n";
            fpo.close();
            test.set[bucket_id].active = false;
        }

        void  ReadJsonFile_NormalVector(std::string path, nlohmann::json &result)// Read Norma lVector
        {
            std::ifstream fp;
            fp.open(path, std::ios::binary);
            std::string result_str;
            std::getline(fp, result_str);

            result.clear();
            result= nlohmann::json::parse(result_str);
            fp.close();
        }

        void LSH_addRecordTobucket(int id, const std::string& _key, const std::vector<float>& _feature)
        {
            test.set[id].key.push_back(_key);
            if (id < one_group_sum)
                test.set[id].feature.push_back(_feature);

            test.key_id[_key].bucket_id.push_back(id);
            test.key_id[_key].key_id.push_back(test.set[id].key.size() - 1);
        }

        void  ReadJsonFile(int id, std::string path)// Read  feature and key
        {
            std::ifstream fp;
            fp.open(path, std::ios::binary);
            std::string result_str;
            nlohmann::json result;
            while (std::getline(fp, result_str))
            {
                if (result_str.length() == 0)
                {
                    break;
                }
                result.clear();
                result=nlohmann::json::parse(result_str);

                int len = result["feature"].size();
                std::vector<float> _feature;
                for (int k = 0; k < len; k++)
                    _feature.push_back(result["feature"][k].get<float>());

                std::string _key = result["key"].get<std::string>();
                LSH_addRecordTobucket(id, _key, _feature);
            }
            fp.close();
        }

        void WriteNullFile(std::string path)
        {
            std::ofstream fp(path, std::fstream::out | std::ios::ate);
            fp << "";
            fp.close();
        }

        void WriteJsonFile(std::string path, nlohmann::json &data)// app write
        {
            std::ofstream fp(path, std::fstream::out | std::ios::app);

            if (!data.empty())
                fp << data.dump()<<"\n";
            else
                fp << "";
            fp.close();
        }

        void LSH_clear()  
        {
            for (int i = 0; i < test.bucket_sum; i++)
            {
                LSH_updataActive(i);
            }
            LSH_init();
        }

        void BuildNormalVector()
        {
            srand((unsigned)time(NULL));
            for (int k = 0; k < group_sum; k++)
            {
                for (int i = 0; i < normal_vec_sum; i++)
                {
                    test.normal_vector[k][i].clear();
                    for (int j = 0; j < dimension_max; j++)
                    {
                        test.normal_vector[k][i].push_back((rand() % 1000) * 2 / 1000.0 - 1.0);
                    }
                }
            }
        }

        void LSH_init()
        {
            test.bucket_sum = group_sum * one_group_sum;
            for (int i = 0; i < test.bucket_sum; i++)
            {
                for (int j = 0; j < test.set[i].feature.size(); j++)
                {
                    test.set[i].feature[j].clear();
                    test.set[i].key[j].clear();
                }
                test.set[i].active = false;
                test.set[i].feature.clear();
                test.set[i].key.clear();
            }
            test.path.clear();
            BuildNormalVector();
            for (auto& it : test.key_id)
            {
                it.second.bucket_id.clear();
                it.second.key_id.clear();
            }
            test.key_id.clear();
        }


        inline float Cosine_distance_AVX256(std::vector<float>& x, std::vector<float>& y)  
        {
            float sum, a, b;
            int len = x.size();
            float xx[1024],yy[1024];
            for (int i = 0; i < x.size(); i++)
                xx[i] = x[i], yy[i] = y[i];
            a = distance_inner_product::compare(xx, xx, x.size());
            b = distance_inner_product::compare(yy, yy, y.size());
            if (a == 0 || b == 0)
                return 0;
            sum = distance_inner_product::compare(xx, yy, x.size());
            float ans = sum / (sqrt(a) * sqrt(b));
            ans = std::min(1.0f, static_cast<float>(fabs(ans)));
            return  ans;
        }

        inline float Cosine_distance(std::vector<float>& x, std::vector<float>& y) 
        {
            double len_x = 0, len_y = 0;
            double cnt = 0;
            int len = x.size();
            for (int i = 0; i < len; ++i)
            {
                len_x += x[i] * x[i];
                len_y += y[i] * y[i];
                cnt += x[i] * y[i];
            }
            if (len_x * len_y == 0)
                return 0;
            //Prevent loss of accuracy
            float ans = cnt / (sqrt(len_x) * sqrt(len_y));
            return std::min(1.0f, static_cast<float>(fabs(ans)));
        }

        int get_bucket_id(const std::vector<float>& temp, int dimension, int group_id)
        {
            int ans = group_id;
            for (int i = 0; i < normal_vec_sum; i++)
            {
                ans <<= 1;
                float sum = 0;
                float xx[1024], yy[1024];
                for (int j = 0; j < temp.size(); j++)
                    xx[j] = temp[j], yy[j] = test.normal_vector[group_id][i][j];
                sum = distance_inner_product::compare(xx, yy, dimension);

                if (sum >= 0)
                    ans++;
            }
            return ans;
        }

        std::vector<database_search_result> LSH_searchTobucket(int dimension, int top, std::vector<float>& _feature, double similarity)
        {
            nlohmann::json temp;
            std::vector<database_search_result> result;

            std::priority_queue<nlohmann::json, std::vector<nlohmann::json>, JsonCmp >q;
            std::unordered_map<std::string, bool >symbol;
            symbol.clear();
            result.reserve(top);
            for (int k = 0; k < group_sum; k++)
            {
                int id = get_bucket_id(_feature, dimension, k);

                for (int i = 0; i < test.set[id].key.size(); i++)
                {
                    std::string temp_string = test.set[id].key[i];
                    if (temp_string == "")
                        continue;
                    if (symbol[temp_string] != true)
                        symbol[temp_string] = true;
                    else
                        continue;

                    int x, y;
                    x = test.key_id[temp_string].bucket_id[0];
                    y = test.key_id[temp_string].key_id[0];
                    double P = Cosine_distance_AVX256(_feature, test.set[x].feature[y]);

                    if (P > similarity || fabs(similarity - 1) < 0.001)
                    {
                        int len = test.set[x].feature[y].size();
                        temp.clear();
                        temp["data"]["key"] = temp_string;
                        for (int j = 0; j < len; j++)
                        {
                            temp["data"]["feature"][j] = test.set[x].feature[y][j];
                        }
                        temp["similarity"] = P;
                        q.push(temp);
                        if (q.size() > top)
                            q.pop();
                    }
                }
            }

            symbol.clear();


            while (!q.empty())
            {
                temp.clear();
                temp = q.top();

                auto temp_record = database_record::create(dimension);

                std::vector<float>val;
                for (int i = 0; i < dimension; i++)
                    val.push_back(temp["data"]["feature"][i].get<float>());

                temp_record->feature(val);
                temp_record->key(temp["data"]["key"].get<std::string>());
                result.emplace_back(database_search_result{ temp_record, temp["similarity"].get<float>()});
                q.pop();
            }
            std::reverse(result.begin(), result.end());
            return result;
        }


        std::vector<database_search_result> search_single_vector(const float* query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k)
        {
            std::vector<database_search_result> result;
            std::vector<float>feature_float;
            double similarity = min_similarity ? *min_similarity : 0.0;
            int top = top_k ? std::min<std::size_t>(3e5, *top_k) : 3e5;
            for (int i = 0; i < dimension_; i++)
                feature_float.push_back(query_data[i]);

            result = LSH_searchTobucket(dimension_, top, feature_float, similarity);
            return result;
        }
        struct node
        {
            std::vector<int> bucket_id;
            std::vector<int> key_id;
        };

        struct bucket
        {
            bool active;                            // DelayDeleteFlag
            std::vector < std::vector<float> >feature;
            std::vector < std::string >key;
        };

        struct LSH
        {
            struct bucket set[one_group_sum * group_sum];
            std::string path;               
            int bucket_sum;
            std::vector<float>normal_vector[group_sum][normal_vec_sum];   // 2^8=256    16group  one goup 8 normal_vec  512 
            std::unordered_map<std::string, struct node >key_id;   //The "key" can be used to quickly find the original data
        }test;

        struct JsonCmp
        {
            //The small probability is in the top
            bool operator()(const nlohmann::json &x, const nlohmann::json &y)
            {
                return x["similarity"].get<float>() > y["similarity"].get<float>();
            }
        };


        int dimension_;
        std::shared_ptr<hide_exp::pmr::memory_resource> pool_;
        hide_exp::pmr::polymorphic_allocator<float> float_allocator_;
        hide_exp::pmr::polymorphic_allocator<std::size_t> size_t_allocator_;
        const std::vector<database_feature_observer::feature>* current_data_;
    };


    lsh_search_impl::lsh_search_impl(int dimension, std::string path) : impl_{ std::make_unique<impl>(dimension,path) }
    {
    }

    lsh_search_impl::~lsh_search_impl()
    {
    }

    int lsh_search_impl::dimension() const noexcept
    {
        return impl_->dimension();
    }

    void lsh_search_impl::build_cache() const
    {
    }

    void lsh_search_impl::save_cache(std::string_view path) const
    {
    }

    void lsh_search_impl::load_cache(std::string_view path) const
    {
    }

    void lsh_search_impl::current_data(const std::vector<database_feature_observer::feature>& data) noexcept
    {
        impl_->current_data(data);
    }

    std::vector<std::vector<database_search_result>>lsh_search_impl::search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const
    {
        return impl_->search_vector(query_data, min_similarity, top_k);
    }

    bool lsh_search_impl::add(database_record& record)
    {
        return impl_->add(record);
    }

    std::vector<bool> lsh_search_impl::remove(std::vector<std::string>& keys)
    {
        return impl_->remove(keys);
    }
    std::vector<bool> lsh_search_impl::update(const std::vector<std::shared_ptr<database_record>>& records) const
    {
        return impl_->update(records);
    }

    namespace
    {
        int register_hint = (register_feature_searcher(face_service_implemention::lsh_algorithm, [](int dimension, std::string path) { return std::make_shared<lsh_search_impl>(dimension, path); }), int{});
    }
}
