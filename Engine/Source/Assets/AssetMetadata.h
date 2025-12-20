#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>

namespace Sabora
{
    /**
     * @brief Loading state for an asset.
     */
    enum class AssetLoadingState : uint8_t
    {
        Pending,   ///< Asset is queued for loading but not started
        Loading,   ///< Asset is currently being loaded
        Loaded,    ///< Asset has been successfully loaded
        Failed     ///< Asset failed to load (error occurred)
    };

    /**
     * @brief Internal metadata structure for tracking asset state.
     * 
     * AssetMetadata is used internally by AssetManager to track all
     * information about an asset including its state, reference count,
     * loading progress, and file information.
     * 
     * @note This is an internal structure - users should not access it directly.
     */
    struct AssetMetadata
    {
        /**
         * @brief Unique asset identifier.
         */
        uint64_t assetId = 0;

        /**
         * @brief Normalized file path to the asset.
         */
        std::filesystem::path filePath;

        /**
         * @brief Current loading state.
         */
        AssetLoadingState state = AssetLoadingState::Pending;

        /**
         * @brief Reference count (how many AssetHandles reference this asset).
         * 
         * @note Atomic for thread-safe access.
         */
        std::atomic<uint32_t> refCount{0};

        /**
         * @brief Loading progress (0.0f to 1.0f).
         * 
         * 0.0f = not started
         * 1.0f = fully loaded
         */
        std::atomic<float> progress{0.0f};

        /**
         * @brief Last file modification time (for hot reloading).
         */
        std::filesystem::file_time_type lastModifiedTime;

        /**
         * @brief Type index of the asset type (for type checking).
         */
        std::type_index typeIndex = typeid(void);

        /**
         * @brief Type-erased pointer to the loaded asset.
         * 
         * The actual asset is stored as std::unique_ptr<T> but we store
         * it as void* here for type erasure. AssetManager handles the casting.
         * 
         * @note The asset is owned by AssetManager and will be deleted
         *       using a deleter function when the asset is unloaded.
         */
        void* assetPtr = nullptr;
        
        /**
         * @brief Function to delete the asset pointer (type-specific deleter).
         * 
         * This is set when the asset is loaded and used to properly
         * delete the asset when it's unloaded.
         */
        std::function<void(void*)> deleter;

        /**
         * @brief File size in bytes (for progress tracking).
         */
        size_t fileSize = 0;

        /**
         * @brief Error message if loading failed.
         */
        std::string errorMessage;

        /**
         * @brief Whether this asset should be watched for hot reloading.
         */
        bool watchForHotReload = true;

        /**
         * @brief Default constructor.
         */
        AssetMetadata() = default;

        /**
         * @brief Construct metadata with initial values.
         */
        AssetMetadata(uint64_t id, const std::filesystem::path& path, std::type_index type)
            : assetId(id)
            , filePath(path)
            , state(AssetLoadingState::Pending)
            , refCount(0)
            , progress(0.0f)
            , typeIndex(type)
            , assetPtr(nullptr)
            , fileSize(0)
            , watchForHotReload(true)
        {
            // Get file modification time if file exists
            try
            {
                if (std::filesystem::exists(path))
                {
                    lastModifiedTime = std::filesystem::last_write_time(path);
                    fileSize = std::filesystem::file_size(path);
                }
            }
            catch (...)
            {
                // Ignore errors - file might not exist yet
            }
        }

        // Non-copyable, non-moveable (managed by AssetManager)
        // Cannot be moved because std::atomic is not moveable
        AssetMetadata(const AssetMetadata&) = delete;
        AssetMetadata& operator=(const AssetMetadata&) = delete;
        AssetMetadata(AssetMetadata&&) = delete;
        AssetMetadata& operator=(AssetMetadata&&) = delete;
    };

} // namespace Sabora
