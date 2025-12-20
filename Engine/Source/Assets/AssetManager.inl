#pragma once

#include "AssetManager.h"
#include "Core/EventManager.h"
#include <future>
#include <thread>
#include <utility>

namespace Sabora
{
    template<typename T>
    void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader<T>> loader)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        std::type_index typeIndex = typeid(T);
        
        // Store the loader (type-erased using std::any)
        // std::any requires copy-constructible types, so we convert unique_ptr to shared_ptr
        // This allows us to store the loader in std::any while maintaining proper ownership
        using LoaderPtrType = std::shared_ptr<IAssetLoader<T>>;
        
        // Convert unique_ptr to shared_ptr for storage in std::any
        // std::shared_ptr is copy-constructible and compatible with std::any
        LoaderPtrType sharedLoader = std::move(loader);
        m_Loaders[typeIndex] = sharedLoader;
    }

    template<typename T>
    AssetHandle<T> AssetManager::LoadAsync(const std::filesystem::path& path)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        // Resolve path (handle relative/absolute)
        std::filesystem::path resolvedPath = ResolvePath(path);
        std::filesystem::path normalizedPath = resolvedPath.lexically_normal();
        
        std::type_index typeIndex = typeid(T);
        
        // Check if already cached
        auto cacheIt = m_AssetCache.find(normalizedPath);
        if (cacheIt != m_AssetCache.end())
        {
            AssetMetadata* metadata = cacheIt->second.get();
            
            // Verify type matches
            if (metadata->typeIndex == typeIndex)
            {
                // Return handle to existing asset
                return AssetHandle<T>(metadata->assetId);
            }
        }
        
        // Get or create asset ID
        uint64_t assetId = GetOrCreateAssetId(normalizedPath, typeIndex);
        
        // Check if already loading
        AssetMetadata* metadata = m_AssetIdLookup[assetId];
        if (metadata->state == AssetLoadingState::Loading || metadata->state == AssetLoadingState::Loaded)
        {
            // Already loading or loaded, return handle
            return AssetHandle<T>(assetId);
        }
        
        // Queue for async loading
        metadata->state = AssetLoadingState::Pending;
        metadata->progress = 0.0f;
        
        // Create loading task
        LoadingTask task(assetId, normalizedPath, typeIndex);
        
        // Start async load
        auto loaderIt = m_Loaders.find(typeIndex);
        if (loaderIt == m_Loaders.end())
        {
            // No loader registered
            metadata->state = AssetLoadingState::Failed;
            metadata->errorMessage = "No loader registered for asset type: " + std::string(typeIndex.name());
            DispatchAssetLoadedEvent(assetId, false, metadata->errorMessage);
            return AssetHandle<T>(assetId);
        }
        
        // Get loader and start async load
        // Extract loader from std::any (stored as shared_ptr)
        std::any& loaderAny = loaderIt->second;
        IAssetLoader<T>* loader = std::any_cast<std::shared_ptr<IAssetLoader<T>>>(loaderAny).get();
        
        // Launch async load
        auto future = std::async(std::launch::async, [loader, normalizedPath, assetId, this]() -> Result<std::unique_ptr<T>> {
            // Update state to loading
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                AssetMetadata* meta = m_AssetIdLookup[assetId];
                if (meta)
                {
                    meta->state = AssetLoadingState::Loading;
                    meta->progress = 0.1f;  // Started loading
                }
            }
            
            // Load the asset
            Result<std::unique_ptr<T>> result = loader->Load(normalizedPath);
            
            // Update state and store asset
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                AssetMetadata* meta = m_AssetIdLookup[assetId];
                if (meta)
                {
                    if (result.IsSuccess())
                    {
                        meta->state = AssetLoadingState::Loaded;
                        meta->progress = 1.0f;
                        meta->assetPtr = result.Value().release();  // Transfer ownership
                        meta->deleter = [](void* ptr) {
                            if (ptr != nullptr)
                            {
                                delete static_cast<T*>(ptr);
                            }
                        };
                        meta->errorMessage.clear();
                    }
                    else
                    {
                        meta->state = AssetLoadingState::Failed;
                        meta->progress = 0.0f;
                        meta->errorMessage = result.GetError().ToString();
                    }
                }
            }
            
            return result;
        });
        
        // Store future with type information
        task.future = future;
        
        // Store deleter function for this type
        task.deleter = [](void* ptr) {
            if (ptr != nullptr)
            {
                delete static_cast<T*>(ptr);
            }
        };
        
        m_ActiveLoads.push_back(std::move(task));
        m_PendingAssetCount.fetch_add(1, std::memory_order_relaxed);
        
        return AssetHandle<T>(assetId);
    }

    template<typename T>
    AssetHandle<T> AssetManager::LoadSync(const std::filesystem::path& path)
    {
        // Load async first
        AssetHandle<T> handle = LoadAsync<T>(path);
        
        if (!handle.IsValid())
        {
            return handle;
        }
        
        // Wait for it to complete
        uint64_t assetId = handle.GetAssetId();
        
        // Wait for loading to complete
        std::unique_lock<std::mutex> lock(m_Mutex);
        
        // Wait for state to be Loaded or Failed
        // Since we can't easily wait on the type-erased future, we poll the state
        AssetMetadata* metadata = m_AssetIdLookup[assetId];
        if (metadata)
        {
            while (metadata->state == AssetLoadingState::Pending || metadata->state == AssetLoadingState::Loading)
            {
                // Wait a bit and check again
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                lock.lock();
            }
        }
        
        return handle;
    }

    template<typename T>
    T* AssetManager::GetAsset(const AssetHandle<T>& handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        uint64_t assetId = handle.GetAssetId();
        if (assetId == 0)
        {
            return nullptr;
        }
        
        auto it = m_AssetIdLookup.find(assetId);
        if (it == m_AssetIdLookup.end())
        {
            return nullptr;
        }
        
        AssetMetadata* metadata = it->second;
        if (metadata->state != AssetLoadingState::Loaded)
        {
            return nullptr;
        }
        
        // Verify type
        if (metadata->typeIndex != typeid(T))
        {
            return nullptr;
        }
        
        return static_cast<T*>(metadata->assetPtr);
    }

    template<typename T>
    float AssetManager::GetAssetProgress(const AssetHandle<T>& handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        uint64_t assetId = handle.GetAssetId();
        if (assetId == 0)
        {
            return 0.0f;
        }
        
        auto it = m_AssetIdLookup.find(assetId);
        if (it == m_AssetIdLookup.end())
        {
            return 0.0f;
        }
        
        return it->second->progress.load(std::memory_order_acquire);
    }

} // namespace Sabora
