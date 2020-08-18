#ifndef _NSG_CALCULATE_ERROR_HPP_
#define _NSG_CALCULATE_ERROR_HPP_
#include <stdexcept>
#include <string>

namespace glasssix
{
	namespace irisviel
	{
		class nsg_calculate_error : public std::exception
		{	// base of all runtime-error exceptions

			std::string message_;
		public:
			typedef std::exception _Mybase;

			explicit nsg_calculate_error(const std::string& _Message)
				: _Mybase(), message_(_Message)
			{	// construct from message string				
			}

			explicit nsg_calculate_error(const char* _Message)
				: _Mybase(), message_(_Message)
			{	// construct from message string
			}

			virtual char const* what() const noexcept
			{
				return message_.c_str();
			}
		};
	}
}

#endif // _NSG_CALCULATE_ERROR_HPP_