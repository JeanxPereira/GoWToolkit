#pragma once

// EventManager â€” Type-safe Pub/Sub event system
// Adapted from ImHex's EventManager (WerWolv, LGPLv2.1)
// Simplified for GOWToolkit: no plugin DLL support, no localization

#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <map>
#include <string_view>
#include <memory>
#include <cstdint>
#include <typeinfo>

#include <Onyx/Services/Logger.h>

// â”€â”€ Event definition macros â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Usage:
//   EVENT_DEF(EventWadOpened, AssetContainer*);
//   EventWadOpened::subscribe([](AssetContainer* wad) { ... });
//   EventWadOpened::post(wadPtr);

#define EVENT_DEF_IMPL(event_name, event_name_string, should_log, ...)                                                  \
    struct event_name final : public Onyx::Services::impl::Event<__VA_ARGS__> {                                          \
        constexpr static auto Id = [] { return Onyx::Services::impl::EventId(event_name_string); }();                   \
        constexpr static auto ShouldLog = (should_log);                                                                 \
        explicit event_name(Callback func) noexcept : Event(std::move(func)) { }                                        \
                                                                                                                        \
        static Onyx::Services::EventManager::EventList::iterator subscribe(Event::Callback function) {                  \
            return Onyx::Services::EventManager::subscribe<event_name>(std::move(function));                             \
        }                                                                                                               \
        template<typename = void>                                                                                       \
        static Onyx::Services::EventManager::EventList::iterator subscribe(Event::BaseCallback function)                 \
        requires (!std::same_as<Event::Callback, Event::BaseCallback>) {                                                \
            return Onyx::Services::EventManager::subscribe<event_name>([function = std::move(function)](auto && ...) {   \
                function();                                                                                             \
            });                                                                                                         \
        }                                                                                                               \
        static void subscribe(void *token, Event::Callback function) {                                                  \
            Onyx::Services::EventManager::subscribe<event_name>(token, std::move(function));                             \
        }                                                                                                               \
        template<typename = void>                                                                                       \
        static void subscribe(void *token, Event::BaseCallback function)                                                \
        requires (!std::same_as<Event::Callback, Event::BaseCallback>) {                                                \
            return Onyx::Services::EventManager::subscribe<event_name>(token,                                            \
                [function = std::move(function)](auto && ...) { function(); });                                          \
        }                                                                                                               \
        static void unsubscribe(const Onyx::Services::EventManager::EventList::iterator &token) noexcept {              \
            Onyx::Services::EventManager::unsubscribe(token);                                                            \
        }                                                                                                               \
        static void unsubscribe(void *token) noexcept {                                                                 \
            Onyx::Services::EventManager::unsubscribe<event_name>(token);                                                \
        }                                                                                                               \
        static void post(auto &&...args) {                                                                              \
            Onyx::Services::EventManager::post<event_name>(std::forward<decltype(args)>(args)...);                       \
        }                                                                                                               \
    }

#define EVENT_DEF(event_name, ...)          EVENT_DEF_IMPL(event_name, #event_name, true, __VA_ARGS__)
#define EVENT_DEF_NO_LOG(event_name, ...)   EVENT_DEF_IMPL(event_name, #event_name, false, __VA_ARGS__)


namespace Onyx::Services {

    namespace impl {

        // Compile-time event ID via FNV-1a-style hash
        class EventId {
        public:
            explicit constexpr EventId(const char *eventName) {
                m_hash = 0x811C'9DC5;
                for (const char c : std::string_view(eventName)) {
                    m_hash = (m_hash >> 5) | (m_hash << 27);
                    m_hash ^= c;
                }
            }

            constexpr bool operator==(const EventId &other) const { return m_hash == other.m_hash; }
            constexpr auto operator<=>(const EventId &other) const { return m_hash <=> other.m_hash; }

        private:
            uint32_t m_hash;
        };

        struct EventBase {
            EventBase() noexcept = default;
            virtual ~EventBase() = default;
        };

        template<typename... Params>
        struct Event : EventBase {
            using Callback     = std::function<void(Params...)>;
            using BaseCallback = std::function<void()>;

            explicit Event(Callback func) noexcept : m_func(std::move(func)) { }

            template<typename E>
            void call(auto&& ... params) const {
                try {
                    m_func(std::forward<decltype(params)>(params)...);
                } catch (const std::exception &e) {
                    GOW_LOG_ERROR("eventmanager", "Exception in event handler: {}", e.what());
                    throw;
                }
            }

        private:
            Callback m_func;
        };

        template<typename T>
        concept EventType = std::derived_from<T, EventBase>;

    } // namespace impl


    /// Thread-safe global event bus (Pub/Sub).
    /// Adapted from ImHex's EventManager â€” see hex/api/event_manager.hpp
    class EventManager {
    public:
        using EventList = std::multimap<impl::EventId, std::unique_ptr<impl::EventBase>>;

        /// Subscribe to an event, returns an iterator token for unsubscription.
        template<impl::EventType E>
        static EventList::iterator subscribe(typename E::Callback function) {
            std::lock_guard lock(getEventMutex());
            auto &events = getEvents();
            return events.insert({ E::Id, std::make_unique<E>(function) });
        }

        /// Subscribe with a void* token (useful for object lifetime management).
        template<impl::EventType E>
        static void subscribe(void *token, typename E::Callback function) {
            std::lock_guard lock(getEventMutex());

            if (isAlreadyRegistered(token, E::Id)) {
                GOW_LOG_WARN("eventmanager", "Token {} already registered for this event",
                             static_cast<const void*>(token));
                return;
            }
            getTokenStore().insert({ token, subscribe<E>(std::move(function)) });
        }

        /// Unsubscribe by iterator token.
        static void unsubscribe(const EventList::iterator &token) noexcept {
            std::lock_guard lock(getEventMutex());
            getEvents().erase(token);
        }

        /// Unsubscribe by void* token for a specific event type.
        template<impl::EventType E>
        static void unsubscribe(void *token) noexcept {
            std::lock_guard lock(getEventMutex());
            unsubscribe(token, E::Id);
        }

        /// Post an event to all subscribers.
        template<impl::EventType E>
        static void post(auto && ...args) {
            std::lock_guard lock(getEventMutex());

            const auto &[begin, end] = getEvents().equal_range(E::Id);
            for (auto it = begin; it != end; ++it) {
                const auto &[id, event] = *it;
                (*static_cast<E *const>(event.get())).template call<E>(std::forward<decltype(args)>(args)...);
            }

            #if defined(DEBUG) || defined(_DEBUG)
                if constexpr (E::ShouldLog)
                    GOW_LOG_DEBUG("eventmanager", "Event posted: '{}'", typeid(E).name());
            #endif
        }

        /// Unsubscribe all subscribers from all events.
        static void clear() noexcept {
            std::lock_guard lock(getEventMutex());
            getEvents().clear();
            getTokenStore().clear();
        }

    private:
        static std::multimap<void *, EventList::iterator>& getTokenStore() {
            static std::multimap<void *, EventList::iterator> tokenStore;
            return tokenStore;
        }

        static EventList& getEvents() {
            static EventList events;
            return events;
        }

        static std::recursive_mutex& getEventMutex() {
            static std::recursive_mutex mutex;
            return mutex;
        }

        static bool isAlreadyRegistered(void *token, impl::EventId id) {
            auto& store = getTokenStore();
            if (store.contains(token)) {
                auto&& [begin, end] = store.equal_range(token);
                return std::any_of(begin, end, [&](auto &item) {
                    return item.second->first == id;
                });
            }
            return false;
        }

        static void unsubscribe(void *token, impl::EventId id) {
            auto &tokenStore = getTokenStore();
            auto iter = std::ranges::find_if(tokenStore, [&](auto &item) {
                return item.first == token && item.second->first == id;
            });

            if (iter != tokenStore.end()) {
                getEvents().erase(iter->second);
                tokenStore.erase(iter);
            }
        }
    };

} // namespace Onyx::Services
