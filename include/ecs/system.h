#ifndef SYSTEM_H
#define SYSTEM_H

#include <vector>
#include <algorithm>
#include <type_traits>

#include <ecs/types.h>

// #include "tbb/parallel_for.h"
// #include "tbb/blocked_range.h"
// #include "tbb/concurrent_unordered_set.h"
// #include "tbb/concurrent_vector.h"

namespace ecs {

class system {
public:
    virtual ~system () noexcept = default;

    virtual void run (registry_type& registry) = 0;
};

enum class EntityNotification {
    ADDED,
    REMOVED
};

namespace detail {
    template<typename T> struct has_method__pre {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;
        template<typename U> static auto test(int) -> decltype(std::declval<U>().pre(), yes());
        template<typename> static no test(...);
    public:
        static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
    };
    template<typename T> typename std::enable_if<has_method__pre<T>::value, void>::type call_if_declared__pre(T* self) {self->pre();}
    void call_if_declared__pre(...) {}

    template<typename T> struct has_method__post {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;
        template<typename U> static auto test(int) -> decltype(std::declval<U>().post(), yes());
        template<typename> static no test(...);
    public:
        static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
    };
    template<typename T> typename std::enable_if<has_method__post<T>::value, void>::type call_if_declared__post(T* self) {self->post();}
    void call_if_declared__post(...) {}

    template<typename T> struct has_method__notify {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;
        template<typename U> static auto test(int) -> decltype(std::declval<U>().notify(std::declval<registry_type&>(), EntityNotification::ADDED, std::declval<const std::vector<entity>&>()), yes());
        template<typename> static no test(...);
    public:
        static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
    };
    template<typename T> typename std::enable_if<has_method__notify<T>::value, void>::type call_if_declared__notify(T* self, registry_type& r, EntityNotification n, const std::vector<entity>& e) {self->notify(r, n, e);}
    void call_if_declared__notify(...) {}
}

template <class This, typename... Components>
class base_system : public system {
public:
    base_system()
        : parallel(false)
        , notificationsEnabled(false) {

    }
    virtual ~base_system() noexcept = default;

    void run (registry_type& registry) {
        std::vector<entity> added;
        std::vector<entity> removed;
        detail::call_if_declared__pre(static_cast<This*>(this));
        if (parallel) {
            // auto view = registry.view<Components...>(entt::persistent_t{});
            // tbb::concurrent_vector<entity> updatedEntities;
            // tbb::parallel_for(tbb::blocked_range<std::size_t>(0, view.size()), [this,view,&updatedEntities](const tbb::blocked_range<size_t>& range){
            //     auto iter = view.begin() + range.begin();
            //     for (auto i = range.begin(); i != range.end(); ++i) {
            //         auto entity = *iter++;
            //         addLiveEntity(updatedEntities, entity);
            //         static_cast<This*>(this)->update(entity, (view.template get<Components>(entity))...);
            //     }
            // });
            // findAddedAndRemovedEntities(std::move(updatedEntities), added, removed);
        } else {
            std::vector<entity> updatedEntities;
            registry.view<Components...>().each([this,&updatedEntities](auto entity, Components&... args){
                addLiveEntity(updatedEntities, entity);
                static_cast<This*>(this)->update(entity, args...);
            });
            findAddedAndRemovedEntities(std::move(updatedEntities), added, removed);
        }
        // Compiled away if This::notify(r,n,e) is not defined
        if constexpr (detail::has_method__notify<This>::value) {
            if (notificationsEnabled) {
                detail::call_if_declared__notify(static_cast<This*>(this), registry, EntityNotification::REMOVED, removed);
                detail::call_if_declared__notify(static_cast<This*>(this), registry, EntityNotification::ADDED, added);
            }
        }
        detail::call_if_declared__post(static_cast<This*>(this));
    }

protected:
    bool notificationsEnabled;

private:
    bool parallel;
    std::vector<entity> liveEntities;

    template <typename T>
    inline void addLiveEntity (T& current, entity e) {
        // Compiled away if This::notify(n,e) is not defined
        if constexpr (detail::has_method__notify<This>::value) {
            if (notificationsEnabled) {
                current.push_back(e);
            }
        }
    }

    template <typename T>
    inline void findAddedAndRemovedEntities (T&& current, std::vector<entity>& added, std::vector<entity>& removed) {
        // Compiled away if This::notify(n,e) is not defined
        if constexpr (detail::has_method__notify<This>::value) {
            if (notificationsEnabled) {
                std::sort(current.begin(), current.end());
                std::set_difference(current.begin(), current.end(), liveEntities.begin(), liveEntities.end(), std::back_inserter(added));
                std::set_difference(liveEntities.begin(), liveEntities.end(), current.begin(), current.end(), std::back_inserter(removed));
                liveEntities.clear();
                liveEntities.insert(liveEntities.end(), std::make_move_iterator(current.begin()), std::make_move_iterator(current.end()));
            }
        }
    }
};

}

#endif // SYSTEM_H