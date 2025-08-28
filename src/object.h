#pragma once

#include "asset.h"

#include <array>

struct IObjectComponent {
    static inline uint32_t nextId = 0;
};

template<typename T>
class ObjectComponent : public IObjectComponent {
public:
    virtual ~ObjectComponent() = default;

    static uint32_t GetID() {
        if (id == UINT32_MAX) {
            id = nextId++;
        }

        return id;
    }

private:
    static inline uint32_t id = UINT32_MAX;
};

struct StaticMeshComponent : public ObjectComponent<StaticMeshComponent> {
    bool drawOutline;
    Ref<Mesh> mesh;
};

struct SpriteComponent : public ObjectComponent<SpriteComponent> {
    Ref<Texture2D> texture;
};

struct Object {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    std::array<Ref<IObjectComponent>, 32> components;

    template<typename T>
    Ref<T> AddComponent() {
        static_assert(std::is_base_of<IObjectComponent, T>::value, "T must be derived from IObjectComponent");

        Ref<T> component = CreateRef<T>();
        components[T::GetID()] = component;

        return component;
    }

    template<typename T>
    bool HasComponent() const {
        static_assert(std::is_base_of<IObjectComponent, T>::value, "T must be derived from IObjectComponent");

        return components[T::GetID()] != nullptr;
    }

    template<typename T>
    Ref<T> GetComponent() const {
        static_assert(std::is_base_of<IObjectComponent, T>::value, "T must be derived from IObjectComponent");

        return std::static_pointer_cast<T>(components[T::GetID()]);
    }
};