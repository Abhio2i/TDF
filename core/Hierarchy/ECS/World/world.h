#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <cstdint>
#include <utility>
#include <limits>

namespace ECS {

using Entity = uint32_t;

// 1. Abstract Base Interface for Component Storage
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity e) = 0;
};

// 2. Concrete Template Storage (Upgraded to Sparse Set Architecture)
template <typename T>
class ComponentArray : public IComponentArray {
public:
    void InsertData(Entity e, T component) {
        if (HasData(e)) {
            // Agar pehle se hai, toh sirf update karo
            m_components[m_entityToIndex[e]] = component;
            return;
        }

        // Agar entity ID index array se bada hai, toh usko bada karo
        if (e >= m_entityToIndex.size()) {
            // Unused slots ko 'max()' se fill kar do
            m_entityToIndex.resize(e + 1, std::numeric_limits<size_t>::max());
        }

        // Dense Arrays (Packed) mein data last mein push karo
        size_t newIndex = m_activeEntities.size();
        m_entityToIndex[e] = newIndex;
        m_activeEntities.push_back(e);
        m_components.push_back(component);
    }

    void RemoveData(Entity e) {
        if (!HasData(e)) return;

        // O(1) Swop-and-Pop Deletion!
        size_t indexOfRemovedEntity = m_entityToIndex[e];
        size_t indexOfLastElement = m_activeEntities.size() - 1;

        // Agar remove hone wali entity last nahi hai, toh last wale ko iski jagah swap kar do
        if (indexOfRemovedEntity != indexOfLastElement) {
            Entity lastEntity = m_activeEntities[indexOfLastElement];

            // Move data
            m_activeEntities[indexOfRemovedEntity] = lastEntity;
            m_components[indexOfRemovedEntity] = m_components[indexOfLastElement];

            // Update the mapping for the swapped entity
            m_entityToIndex[lastEntity] = indexOfRemovedEntity;
        }

        // Purani entity ka index delete karo aur arrays ko shrink karo
        m_entityToIndex[e] = std::numeric_limits<size_t>::max();
        m_activeEntities.pop_back();
        m_components.pop_back();
    }

    T* GetData(Entity e) {
        if (HasData(e)) {
            return &m_components[m_entityToIndex[e]];
        }
        return nullptr;
    }

    bool HasData(Entity e) const {
        return e < m_entityToIndex.size() && m_entityToIndex[e] != std::numeric_limits<size_t>::max();
    }

    void EntityDestroyed(Entity e) override {
        RemoveData(e);
    }

    // Fast loop iterations ke liye
    const std::vector<Entity>& GetActiveEntities() const { return m_activeEntities; }
    std::vector<T>& GetComponents() { return m_components; }

private:
    std::vector<T> m_components;          // Packed Array: Sirf valid components
    std::vector<Entity> m_activeEntities; // Packed Array: Sirf valid Entities
    std::vector<size_t> m_entityToIndex;  // Sparse Array: Entity ID -> Packed Array Index
};

// 3. World Class with Fast Query System
class World
{
public:
    World() = default;

    Entity CreateEntity() {
        Entity id;
        if (!m_freeEntities.empty()) {
            id = m_freeEntities.back();
            m_freeEntities.pop_back();
        } else {
            id = m_nextEntityId++;
        }
        return id;
    }

    void DestroyEntity(Entity e) {
        for (auto const& pair : m_componentArrays) {
            pair.second->EntityDestroyed(e);
        }
        m_freeEntities.push_back(e);
    }

    template <typename T, typename... Args>
    T& AddComponent(Entity e, Args&&... args) {
        GetComponentArray<T>()->InsertData(e, T{std::forward<Args>(args)...});
        return *GetComponent<T>(e);
    }

    template <typename T>
    T* GetComponent(Entity e) {
        return GetComponentArray<T>()->GetData(e);
    }

    template <typename... Components>
    std::tuple<Components*...> GetComponents(Entity e) {
        return std::make_tuple(GetComponent<Components>(e)...);
    }

    template <typename T>
    void RemoveComponent(Entity e) {
        GetComponentArray<T>()->RemoveData(e);
    }

    template <typename T>
    bool HasComponent(Entity e) {
        return GetComponentArray<T>()->HasData(e);
    }

    // -------------------------------------------------------------
    // Option 1: Multi-Has Check (Variadic Template)
    // Check karta hai ki kya entity ke paas SAARI list kiye gaye components HAIN.
    // Usage: if (world.HasComponents<Transform, Velocity>(entity)) { ... }
    // -------------------------------------------------------------
    template <typename... Components>
    bool HasComponents(Entity e) {
        return (HasComponent<Components>(e) && ...);
    }

    // -------------------------------------------------------------
    // Option 2: Any-Has Check
    // Check karta hai ki kya entity ke paas kam se kam EK (at least one) component hai.
    // Usage: if (world.HasAnyComponent<Transform, Velocity>(e)) { ... }
    // -------------------------------------------------------------
    template <typename... Components>
    bool HasAnyComponent(Entity e) {
        return (HasComponent<Components>(e) || ...);
    }

    // -------------------------------------------------------------
    // Option 3: Array Level Check
    // Check karta hai ki kya us Component Type ka Array exist karta hai aur usme data hai.
    // Usage: if (world.HasComponentArray<Transform>()) { ... }
    // -------------------------------------------------------------
    template <typename T>
    bool HasComponentArray() {
        std::type_index typeName = std::type_index(typeid(T));
        if (m_componentArrays.find(typeName) != m_componentArrays.end()) {
            return !GetComponentArray<T>()->GetActiveEntities().empty();
        }
        return false;
    }

    template <typename T>
    std::vector<T>& GetComponentsArray() {
        return GetComponentArray<T>()->GetComponents();
    }

    // 2. Ek sath multiple types ke component vectors tuple me reference ke roop me lene ke liye
    template <typename... Components>
    auto GetPackedComponents() {
        return std::tie(GetComponentsArray<Components>()...);
    }

    // ─── SUPER FAST QUERY SYSTEM ───
    // Istemal: auto entities = world.Query<TransformComponent, VelocityComponent>();
    template <typename FirstComponent, typename... RestComponents>
    std::vector<Entity> Query() {
        std::vector<Entity> matchingEntities;

        // 1. Sabse pehle FirstComponent ki PACKED list nikalo
        auto firstArray = GetComponentArray<FirstComponent>();
        const auto& activeEntities = firstArray->GetActiveEntities();

        // 2. Ab ZERO (0) se m_nextEntityId tak loop NAHI chalega!
        // Loop sirf unhi par chalega jinke paas FirstComponent hai (bohot choti list).
        for (Entity e : activeEntities) {

            // 3. Check karo ki kya is entity ke paas baaki ke bache hue (RestComponents) bhi hain
            // C++17 Fold Expression magic
            if ((HasComponent<RestComponents>(e) && ...)) {
                matchingEntities.push_back(e);
            }
        }

        return matchingEntities;
    }

    template <typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        std::type_index typeName = std::type_index(typeid(T));

        if (m_componentArrays.find(typeName) == m_componentArrays.end()) {
            m_componentArrays[typeName] = std::make_shared<ComponentArray<T>>();
        }
        return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]);
    }

private:


    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_componentArrays;
    Entity m_nextEntityId{0};
    std::vector<Entity> m_freeEntities;
};

} // namespace ECS
#endif // WORLD_H
