#pragma once

#include "AssetHandle.h"
#include "AssetMetadata.h"
#include "IAssetLoader.h"
#include "AssetEvents.h"
#include "Core/Result.h"
#include "Core/AsyncIO.h"
#include "Core/Event.h"
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <typeindex>
#include <atomic>
#include <future>
#include <queue>
#include <vector>
#include <any>
#include <functional>

// Forward declaration
namespace Sabora
{
    class EventDispatcher;
}

namespace Sabora
{
    /**
     * @brief Singleton manager for loading, caching, and managing assets.
     * 
     * AssetManager provides a comprehensive asset management system with:
     * - Type-safe asset loading via registered loaders
     * - Automatic caching (same path = same asset)
     * - Asynchronous loading with progress tracking
     * - Reference counting for automatic cleanup
     * - Hot reloading (automatic file watching)
     * - Event dispatching for asset lifecycle
     * 
     * Usage:
     * @code
     *   // Register a loader
     *   AssetManager::Get().RegisterLoader<Shader>(std::make_unique<ShaderLoader>());
     *   
     *   // Load asset asynchronously
     *   AssetHandle<Shader> shader = AssetManager::Get().LoadAsync<Shader>("Shaders/basic.glsl");
     *   
     *   // Check progress
     *   float progress = AssetManager::Get().GetLoadingProgress();
     *   
     *   // Use asset when loaded
     *   if (shader.IsLoaded()) {
     *       auto* shaderPtr = shader.Get();
     *   }
     * @endcode
     * 
     * @note All public methods are thread-safe.
     */
    class AssetManager
    {
    public:
        /**
         * @brief Get the singleton instance of AssetManager.
         * @return Reference to the AssetManager instance.
         */
        [[nodiscard]] static AssetManager& Get()
        {
            static AssetManager instance;
            return instance;
        }

        /**
         * @brief Initialize the AssetManager.
         * @param assetRoot Root directory for relative asset paths (optional).
         * @param eventDispatcher Event dispatcher for asset events (optional).
         * 
         * Should be called once during application initialization.
         */
        void Initialize(const std::filesystem::path& assetRoot = {}, class EventDispatcher* eventDispatcher = nullptr);

        /**
         * @brief Shutdown the AssetManager and clean up all assets.
         */
        void Shutdown();

        /**
         * @brief Set the root directory for relative asset paths.
         * @param assetRoot The root directory path.
         */
        void SetAssetRoot(const std::filesystem::path& assetRoot);

        /**
         * @brief Get the current asset root directory.
         * @return The asset root path.
         */
        [[nodiscard]] std::filesystem::path GetAssetRoot() const;

        /**
         * @brief Register a loader for a specific asset type.
         * @param loader Unique pointer to the loader implementation.
         * 
         * Each asset type needs a loader to be registered before assets
         * of that type can be loaded. Loaders are type-specific and handle
         * the actual file parsing and asset creation.
         * 
         * @note Replaces any existing loader for the same type.
         */
        template<typename T>
        void RegisterLoader(std::unique_ptr<IAssetLoader<T>> loader);

        /**
         * @brief Load an asset asynchronously.
         * @param path File path to the asset (relative to asset root or absolute).
         * @return AssetHandle to the asset (may not be loaded yet).
         * 
         * The asset will be loaded in the background. Use IsLoaded() on the
         * handle to check when it's ready, or subscribe to AssetLoadedEvent.
         * 
         * @note If the asset is already cached, returns handle to existing asset.
         * @note Paths are normalized and cached by normalized path.
         */
        template<typename T>
        [[nodiscard]] AssetHandle<T> LoadAsync(const std::filesystem::path& path);

        /**
         * @brief Load an asset synchronously (blocks until loaded).
         * @param path File path to the asset (relative to asset root or absolute).
         * @return AssetHandle to the asset, or invalid handle on failure.
         * 
         * This method blocks the calling thread until the asset is loaded.
         * Use LoadAsync() for non-blocking loading.
         * 
         * @note If the asset is already cached, returns immediately.
         */
        template<typename T>
        [[nodiscard]] AssetHandle<T> LoadSync(const std::filesystem::path& path);

        /**
         * @brief Check if an asset is loaded.
         * @param assetId The asset ID to check.
         * @return True if the asset is loaded and ready.
         */
        [[nodiscard]] bool IsAssetLoaded(uint64_t assetId) const;

        /**
         * @brief Get a pointer to the asset (internal use by AssetHandle).
         * @param handle The asset handle.
         * @return Pointer to the asset, or nullptr if not loaded.
         */
        template<typename T>
        [[nodiscard]] T* GetAsset(const AssetHandle<T>& handle) const;

        /**
         * @brief Get the loading progress for a specific asset.
         * @param handle The asset handle.
         * @return Progress from 0.0f (not started) to 1.0f (loaded).
         */
        template<typename T>
        [[nodiscard]] float GetAssetProgress(const AssetHandle<T>& handle) const;

        /**
         * @brief Get the overall loading progress for all assets.
         * @return Aggregate progress from 0.0f to 1.0f.
         * 
         * Calculates the average progress of all assets currently loading.
         */
        [[nodiscard]] float GetLoadingProgress() const;

        /**
         * @brief Get the number of assets currently loading.
         * @return Count of assets in Pending or Loading state.
         */
        [[nodiscard]] size_t GetLoadingAssetCount() const;

        /**
         * @brief Enable or disable hot reloading.
         * @param enable True to enable, false to disable.
         * 
         * When enabled, all loaded assets are automatically watched for
         * file changes and reloaded when modified.
         */
        void EnableHotReloading(bool enable);

        /**
         * @brief Check if hot reloading is enabled.
         * @return True if hot reloading is enabled.
         */
        [[nodiscard]] bool IsHotReloadingEnabled() const;

        /**
         * @brief Update the AssetManager (process loading queue, check for hot reloads).
         * 
         * Should be called once per frame from the main thread.
         */
        void Update();

        /**
         * @brief Unload assets with zero reference count.
         * 
         * Removes assets from the cache that are no longer referenced
         * by any AssetHandle. This frees memory for unused assets.
         */
        void UnloadUnusedAssets();

        /**
         * @brief Clear all cached assets (force unload everything).
         * 
         * @warning This will invalidate all existing AssetHandles.
         */
        void ClearCache();

        // Internal methods for AssetHandle reference counting
        void IncrementRefCount(uint64_t assetId);
        void DecrementRefCount(uint64_t assetId);

    private:
        AssetManager() = default;
        ~AssetManager() = default;

        // Non-copyable, non-movable
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        // Helper methods
        std::filesystem::path ResolvePath(const std::filesystem::path& path) const;
        uint64_t GetOrCreateAssetId(const std::filesystem::path& normalizedPath, std::type_index typeIndex);
        void ProcessLoadingQueue();
        void CheckHotReloads();
        void DispatchAssetLoadedEvent(uint64_t assetId, bool success, const std::string& errorMessage);
        void CheckAllAssetsLoaded();

        // Internal structure for async loading
        // Note: We use std::any to type-erase the future since we can't use std::unique_ptr<void>
        struct LoadingTask
        {
            uint64_t assetId = 0;
            std::filesystem::path filePath;
            std::type_index typeIndex = typeid(void);
            std::any future;  // Type-erased future - contains std::future<Result<std::unique_ptr<T>>>
            std::function<void(void*)> deleter;  // Function to delete the asset pointer
            
            // Default constructor
            LoadingTask() = default;
            
            // Constructor with parameters
            LoadingTask(uint64_t id, const std::filesystem::path& path, std::type_index type)
                : assetId(id)
                , filePath(path)
                , typeIndex(type)
            {
            }
        };

        // Thread-safe state
        mutable std::mutex m_Mutex;
        std::atomic<bool> m_Initialized{false};
        std::atomic<bool> m_HotReloadingEnabled{true};

        // Asset root directory
        std::filesystem::path m_AssetRoot;

        // Event dispatcher
        class EventDispatcher* m_EventDispatcher = nullptr;

        // Asset ID generation
        std::atomic<uint64_t> m_NextAssetId{1};

        // Loader registry (type_index -> loader)
        // Note: We use std::any to type-erase loaders since we can't use std::unique_ptr<void>
        std::unordered_map<std::type_index, std::any> m_Loaders;

        // Asset cache (normalized path -> AssetMetadata)
        std::unordered_map<std::filesystem::path, std::unique_ptr<AssetMetadata>> m_AssetCache;

        // Asset ID lookup (assetId -> AssetMetadata*)
        std::unordered_map<uint64_t, AssetMetadata*> m_AssetIdLookup;

        // Loading queue
        std::queue<LoadingTask> m_LoadingQueue;
        std::vector<LoadingTask> m_ActiveLoads;

        // Tracking for "all assets loaded" event
        std::atomic<size_t> m_PendingAssetCount{0};
        size_t m_LastPendingCount = 0;
    };

} // namespace Sabora

// Include template implementations
#include "AssetHandle.inl"
#include "AssetManager.inl"
