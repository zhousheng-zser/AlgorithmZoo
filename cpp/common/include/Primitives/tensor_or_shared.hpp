#pragma once

#include <memory>

#include "tensor.hpp"

namespace glasssix::memory
{
	template<typename UnderlyingType, bool Shared>
	struct tensor_or_shared {};

	template<typename UnderlyingType>
	struct tensor_or_shared<UnderlyingType, false>
	{
		std::reference_wrapper<memory::tensor<UnderlyingType>> data;

		tensor_or_shared(const memory::tensor<UnderlyingType>& tensor) : data{ const_cast<memory::tensor<UnderlyingType>&>(tensor) }
		{
		}

		auto operator->() const
		{
			return &data.get();
		}

		auto& access() const
		{
			return data.get();
		}
	};

	template<typename UnderlyingType>
	struct tensor_or_shared<UnderlyingType, true>
	{
		std::shared_ptr<memory::tensor<UnderlyingType>> data;

		tensor_or_shared(const std::shared_ptr<tensor<UnderlyingType>>& tensor) : data{ tensor }
		{
		}

		auto operator->() const
		{
			return data.get();
		}

		auto& access() const
		{
			return *data;
		}
	};
}
