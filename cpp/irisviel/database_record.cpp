#include "database_record.hpp"

#include <cstring>
#include <cstdint>
#include <algorithm>

namespace glasssix
{
	namespace irisviel
	{
		template<int Dimension>
		struct basic_database_record
		{
			float feature[Dimension];
			char key[33];
			bool active;
		};

		template<int Dimension>
		class database_record_ref_impl : public database_record
		{
		public:
			using record_type = basic_database_record<Dimension>;

			database_record_ref_impl(record_type* record) : record_{ record }
			{
			}

			virtual std::size_t size() const noexcept override
			{
				return Dimension;
			}

			virtual std::uint8_t* data() noexcept override
			{
				return reinterpret_cast<uint8_t*>(record_);
			}

			virtual const std::uint8_t* data() const noexcept override
			{
				return reinterpret_cast<const uint8_t*>(record_);
			}

			virtual int dimension() const noexcept override
			{
				return Dimension;
			}

			virtual bool active() const noexcept override
			{
				return record_->active;
			}

			virtual void active(bool value) noexcept override
			{
				record_->active = value;
			}

			virtual std::string_view key() const noexcept override
			{
				return record_->key;
			}

			virtual void key(std::string_view value) noexcept override
			{
				std::copy_n(value.begin(), std::min(value.size(), std::size(record_->key) - 1), std::begin(record_->key));
				record_->key[std::size(record_->key)] = '\0';
			}

			virtual exposing::param_span<const float> feature() const noexcept override
			{
				return exposing::param_span<const float>{ record_->feature, std::size(record_->feature) };
			}

			virtual void feature(exposing::param_span<const float> value) noexcept override
			{
				std::copy_n(value.begin(), std::min(value.size(), std::size(record_->feature)), std::begin(record_->feature));
			}

			virtual std::size_t feature_offset() const noexcept override
			{
				return offsetof(basic_database_record<Dimension>, feature);
			}
		private:
			record_type* record_;
		};

		template<int Dimension>
		class database_record_impl : public database_record
		{
		public:
			using record_type = basic_database_record<Dimension>;

			database_record_impl() : record_{}
			{
			}

			database_record_impl(const record_type& record) : record_{ record }
			{
			}

			database_record_impl(record_type&& data) : record_{ data }
			{
			}

			virtual std::size_t size() const noexcept override
			{
				return sizeof(record_);
			}

			virtual std::uint8_t* data() noexcept override
			{
				return reinterpret_cast<uint8_t*>(&record_);
			}

			virtual const std::uint8_t* data() const noexcept override
			{
				return reinterpret_cast<const uint8_t*>(&record_);
			}

			virtual int dimension() const noexcept override
			{
				return Dimension;
			}

			virtual bool active() const noexcept override
			{
				return record_.active;
			}

			virtual void active(bool value) noexcept override
			{
				record_.active = value;
			}

			virtual std::string_view key() const noexcept override
			{
				return record_.key;
			}

			virtual void key(std::string_view value) noexcept override
			{
				std::copy_n(value.begin(), std::min(value.size(), std::size(record_.key) - 1), std::begin(record_.key));
				record_.key[std::size(record_.key)] = '\0';
			}

			virtual exposing::param_span<const float> feature() const noexcept override
			{
				return exposing::param_span<const float>{ record_.feature, std::size(record_.feature) };
			}

			virtual void feature(exposing::param_span<const float> value) noexcept override
			{
				std::copy_n(value.begin(), std::min(value.size(), std::size(record_.feature)), std::begin(record_.feature));
			}

			virtual std::size_t feature_offset() const noexcept override
			{
				return offsetof(record_type, feature);
			}
		private:
			record_type record_;
		};

		std::shared_ptr<database_record> database_record::shared()
		{
			return shared_from_this();
		}

		std::size_t database_record::record_size(int dimension) noexcept
		{
			switch (dimension)
			{
			case 128:
				return sizeof(database_record_impl<128>::record_type);
			case 512:
				return sizeof(database_record_impl<512>::record_type);
			default:
				return sizeof(database_record_impl<128>::record_type);
			}
		}

		std::size_t database_record::feature_offset(int dimension) noexcept
		{
			switch (dimension)
			{
			case 128:
				return offsetof(basic_database_record<128>, feature);
			case 512:
				return offsetof(basic_database_record<512>, feature);
			default:
				return offsetof(basic_database_record<128>, feature);
			}
		}

		std::shared_ptr<database_record> database_record::create(int dimension)
		{
			switch (dimension)
			{
			case 128:
				return std::make_shared<database_record_impl<128>>();
			case 512:
				return std::make_shared<database_record_impl<512>>();
			default:
				return std::make_shared<database_record_impl<128>>();
			}
		}

		std::shared_ptr<database_record> database_record::create(int dimension, std::uint8_t* ptr)
		{
			switch (dimension)
			{
			case 128:
				return std::make_shared<database_record_impl<128>>(*reinterpret_cast<basic_database_record<128>*>(ptr));
			case 512:
				return std::make_shared<database_record_impl<512>>(*reinterpret_cast<basic_database_record<512>*>(ptr));
			default:
				return std::make_shared<database_record_impl<128>>(*reinterpret_cast<basic_database_record<128>*>(ptr));
			}
		}

		std::shared_ptr<database_record> database_record::create_ref(int dimension, std::uint8_t* ptr)
		{
			switch (dimension)
			{
			case 128:
				return std::make_shared<database_record_ref_impl<128>>(reinterpret_cast<basic_database_record<128>*>(ptr));
			case 512:
				return std::make_shared<database_record_ref_impl<512>>(reinterpret_cast<basic_database_record<512>*>(ptr));
			default:
				return std::make_shared<database_record_ref_impl<128>>(reinterpret_cast<basic_database_record<128>*>(ptr));
			}
		}

		bool database_record::key_equals(const char* left, const char* right)
		{
#ifdef _MSC_VER
			return _stricmp(left, right) == 0;
#else
			return strcasecmp(left, right) == 0;
#endif
		}

		bool database_record::key_equals(const database_record& left, const database_record& right)
		{
			return key_equals(left.key().data(), right.key().data());
		}
	}
}
