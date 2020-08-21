#include <utility>
#include <functional>

namespace glasssix
{
    class scope_guard
    {
    public:
        template<typename Callable>
        scope_guard(Callable&& handler) : handler_{ std::forward<Callable>(handler) }
        {
        }

        ~scope_guard()
        {
            if (handler_)
            {
                handler_();
            }
        }
    private:
        std::function<void()> handler_;
    };
}
