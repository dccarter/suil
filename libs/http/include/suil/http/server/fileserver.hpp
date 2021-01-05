//
// Created by Mpho Mbotho on 2020-12-30.
//

#ifndef SUIL_HTTP_SERVER_FILESERVER_HPP
#define SUIL_HTTP_SERVER_FILESERVER_HPP

#include <suil/http/common.hpp>
#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>

#include <suil/base/channel.hpp>
#include <suil/base/exception.hpp>

#include <list>
#include <unistd.h>

namespace suil::http::server {

    define_log_tag(FILE_SERVER);

    struct CachedFile {
        CachedFile();
        MOVE_CTOR(CachedFile) noexcept;
        MOVE_ASSIGN(CachedFile) noexcept;

        ~CachedFile();
        void clear();
        bool isGarbage{false};
        bool isReloading{false};
        int64 inFlight{0};
        String path{};
        void *data{nullptr};
        size_t len{0};
        size_t size{0};
        int64  refs{0};
        time_t lastModified{0};
        time_t lastAccessed{0};
        int fd{-1};
        union {
            struct {
                uint8 useFd: 1;
                uint8 isMapped: 1;
                uint8 _u6: 6;
            } __attribute__((packed));
            uint8 flags;
        };
        Conditional reloadCond{};
    private:
        DISABLE_COPY(CachedFile);
    };

    DECLARE_EXCEPTION(FileServerException);

    class FileServer : LOGGER(FILE_SERVER) {
    public:
        template <typename E, typename... Opts>
        FileServer(E& ep, Opts&&... opts)
        {
            if (!ep.apiBaseRoute()) {
                throw FileServerException("FileServer cannot be attached to an endpoint without an API base");
            }
            suil::applyConfig(_config, std::forward<Opts>(opts)...);

            init();

            ep(_config.route + "")
            (Method::Get, Method::Head)
            .attrs(opt(Static, true),
                   opt(Authorize, Auth{false}))
            ([this](const Request& req, Response& resp) {
                handleRequest(req, resp);
            });

            inotice("attached file server to http endpoint");
        }

        template <typename... Opts>
        void mime(const String& ext, const String& mm, Opts... opts)
        {
            auto it = _mimeTypes.find(ext);
            if (it == _mimeTypes.end()) {
                MimeType mt{mm.dup()};
                it = _mimeTypes.emplace(ext.dup(), std::move(mt)).first;
            }

            // apply specified configuration to mime type
            suil::applyConfig(it->second, std::forward<Opts>(opts)...);
            if (it->second.cacheExpires < 0) {
                // default to global expires
                it->second.cacheExpires = _config.cacheExpires;
            }
        }

        template<typename... Opts>
        void editMime(const String& ext, Opts&&... opts) {
            auto it = _mimeTypes.find(ext);
            if (it == _mimeTypes.end()) {
                throw FileServerException("mime '", ext, "' not registered");
            }

            // apply the settings
            suil::applyConfig(it->second, std::forward<Opts>(opts)...);

            if (it->second.cacheExpires < 0) {
                // if expires for mime was not modified, set to global expires
                it->second.cacheExpires = _config.cacheExpires;
            }
        }

        template<typename... Opts>
        void mime(const std::vector<String>& exts, Opts&&... opts) {
            for(auto ext : exts) {
                editMime(ext, opts...);
            }
        }

        void alias(const String& from, const String& to);

    private suil_ut:
        struct MimeType {
            String mime;
            int64 cacheExpires{-1};
            bool allowCompress{false};
            bool allowCaching{true};
            bool allowRange{true};
            bool allowSendFile{true};
        };
        using CachedFileIt = typename UnorderedMap<CachedFile>::iterator;

        void init();
        void handleRequest(const Request& req, Response& resp);
        void get(const Request& req, Response& resp, CachedFile& cf, const MimeType& mm);
        void head(const Request& req, Response& resp, CachedFile& cf, const MimeType& mm);
        String aliased(const String& path);
        CachedFileIt loadFile(const String& path, const MimeType& mt);
        String fileAbsolutePath(const String& rel) const;
        bool readFile(CachedFile& cf, const struct stat& st);
        bool reloadCachedFile(CachedFileIt& it, const struct stat& st);
        void cacheControl(const Request& req, Response& resp, const CachedFile& cf, const MimeType& mt);
        void prepareResponse(const Request& req, Response& resp, CachedFile& cf, const MimeType& mt);
        void buildRangeResponse(
                    const Request& req,
                    Response& resp,
                    const String& range,
                    CachedFile& cf,
                    const MimeType& mt);
        void buildPartialContent(
                Response& resp,
                const std::vector<std::pair<size_t, size_t>>& ranges,
                CachedFile& cf,
                const MimeType& mt);

        CachedFileIt addCachedFile(CachedFile&& cf);
        void markCachedFileForRemoval(CachedFileIt it);
        void derefCachedFileChunk(CachedFile& cf);
        void addCachedFileChunk(Response& resp, CachedFile& cf, size_t len, off_t offset);
        String relativePath(const CachedFile& cf);
        static void garbageCollector(FileServer& S);
        size_t _totalMemCachedSize{};
        String _wwwDir{};
        FileServerConfig _config{};
        UnorderedMap<String> _redirects{};
        UnorderedMap<MimeType> _mimeTypes{};
        UnorderedMap<CachedFile> _cachedFiles{};
        std::list<CachedFile> _garbageCache{};
        bool _isCollectingGarbage{false};
        Conditional _gcCond;
    };
}
#endif //SUIL_HTTP_SERVER_FILESERVER_HPP
