//
// Created by Mpho Mbotho on 2020-12-31.
//

#include "suil/http/server/fileserver.hpp"

#include <suil/base/datetime.hpp>
#include <suil/base/file.hpp>

#include <climits>

#include <sys/mman.h>

namespace suil::http::server {

    CachedFile::CachedFile()
    {
        flags = 0;
    }

    CachedFile::CachedFile(CachedFile&& o) noexcept
        : isGarbage{std::exchange(o.isGarbage, false)},
          isReloading{std::exchange(o.isReloading, false)},
          inFlight{std::exchange(o.inFlight, 0)},
          path{std::move(o.path)},
          data{std::exchange(o.data, nullptr)},
          len{std::exchange(o.len, 0)},
          size{std::exchange(o.size, 0)},
          refs{std::exchange(o.refs, 0)},
          lastModified{std::exchange(o.lastModified, -1)},
          lastAccessed{std::exchange(o.lastAccessed, -1)},
          fd{std::exchange(o.fd, -1)},
          reloadCond{std::move(o.reloadCond)}
    {
        flags = std::exchange(o.flags, 0);
    }

    CachedFile& CachedFile::operator=(CachedFile&& o) noexcept
    {
        if (this == &o) {
            return Ego;
        }

        isGarbage = std::exchange(o.isGarbage, false);
        isReloading = std::exchange(o.isReloading, false);
        inFlight = std::exchange(o.inFlight, 0);
        path = std::move(o.path);
        data = std::exchange(o.data, nullptr);
        len = std::exchange(o.len, 0);
        size = std::exchange(o.size, 0);
        refs = std::exchange(o.refs, 0);
        lastModified = std::exchange(o.lastModified, -1);
        lastAccessed = std::exchange(o.lastAccessed, -1);
        fd = std::exchange(o.fd, -1);
        reloadCond = std::move(o.reloadCond);
        flags = std::exchange(o.flags, 0);

        return Ego;
    }

    CachedFile::~CachedFile()
    {
        clear();
    }

    void CachedFile::clear()
    {
        if (data) {
            if (isMapped) {
                munmap(data, size);
            }
            else {
                ::free(data);
            }
            data = nullptr;
        }

        if (fd != -1) {
            auto tmp = fd;
            fd = -1;
            fdclear(tmp);
            ::close(tmp);
            reloadCond.notify();
        }

        isGarbage = false;
        isReloading = false;
        flags = 0;
        size = len = 0;
        lastModified = lastAccessed = 0;
    }

    void FileServer::init()
    {
        // add text mime types
        mime(".html", "text/html",
             opt(allowCaching, false),
             opt(allowSendFile, false));
        mime(".css", "text/css");
        mime(".csv", "text/csv");
        mime(".txt", "text/plain");
        mime(".sgml","text/sgml");
        mime(".tsv", "text/tab-separated-values");

        // add compressed mime types
        mime(".bz", "application/x-bzip",
             opt(allowCompress, false));
        mime(".bz2", "application/x-bzip2",
             opt(allowCompress, false));
        mime(".gz", "application/x-gzip",
             opt(allowCompress, false));
        mime(".tgz", "application/x-tar",
             opt(allowCompress, false));
        mime(".tar", "application/x-tar",
             opt(allowCompress, false));
        mime(".zip", "application/zip, application/x-compressed-zip",
             opt(allowCompress, false));
        mime(".7z", "application/zip, application/x-compressed-zip",
             opt(allowCompress, false));


        // add image mime types
        mime(".jpg", "image/jpeg");
        mime(".png", "image/png");
        mime(".svg", "image/svg+xml");
        mime(".gif", "image/gif");
        mime(".bmp", "image/bmp");
        mime(".tiff","image/tiff");
        mime(".ico", "image/x-icon");

        // add video mime types
        mime(".avi",  "video/avi");
        mime(".mpeg", "video/mpeg");
        mime(".mpg",  "video/mpeg");
        mime(".mp4",  "video/mp4");
        mime(".qt",   "video/quicktime");

        // add audio mime types
        mime(".au",  "audio/basic");
        mime(".midi","audio/x-midi");
        mime(".mp3", "audio/mpeg");
        mime(".ogg", "audio/vorbis, application/ogg");
        mime(".ra",   "audio/x-pn-realaudio, audio/vnd.rn-realaudio");
        mime(".ram",  "audio/x-pn-realaudio, audio/vnd.rn-realaudio");
        mime(".wav", "audio/wav, audio/x-wav");

        // Other common mime types
        mime(".json",  "application/json");
        mime(".map",   "application/json");
        mime(".js",    "application/javascript");
        mime(".ttf",   "font/ttf");
        mime(".xhtml", "application/xhtml+xml");
        mime(".xml",   "application/xml");

        char base[PATH_MAX];
        realpath(_config.root.data(), base);
        struct stat st{};
        if (stat(base, &st) != 0 || !S_ISDIR(st.st_mode)) {
            throw std::runtime_error(
                    "base dir: '" + std::string(base) + "' is not a valid directory");
        }

        size_t sz = strlen(base);
        if (base[sz-1] != '/') {
            base[sz] = '/';
            base[++sz] = '\0';
        }

        // this will dup over the base
        _wwwDir = String{base}.dup();
        _config.route = "/" SUIL_FILE_SERVER_ROUTE;
        // add some basic redirects
        alias("/", "index.html");
    }

    void FileServer::alias(const String& from, const String& to)
    {
        auto pos = to.rfind('.');
        if (pos == String::npos) {
            throw FileServerException("alias to file '", to, "' - file has unsupported format");
        }

        auto it = _redirects.find(from);
        if (it != _redirects.end()) {
            it->second = to.dup();
        }
        else {
            _redirects.emplace(from.dup(), to.dup());
        }
    }

    String FileServer::aliased(const String& path)
    {
        auto it = _redirects.find(path);
        if (it != _redirects.end()) {
            return it->second.peek();
        }
        return {};
    }

    void FileServer::handleRequest(const Request& req, Response& resp)
    {
        auto path = req.url().peek();
        auto pos = req.url().find('.');
        if (pos == String::npos) {
            // find path in the redirect lists
            path = aliased(req.url());
            if (!path) {
                // path does not exist
                throw HttpError(http::NotFound);
            }
            pos = path.rfind('.');
        }

        auto prefix = path.substr(0, pos);
        auto ext = path.substr(pos);

        auto it = _mimeTypes.find(ext);
        if (it == _mimeTypes.end()) {
            itrace("extension type (%s) not supported", ext());
            throw HttpError(http::NotFound);
        }

        auto& mm = it->second;
        auto sf = loadFile(path, mm);
        if (sf == _cachedFiles.end()) {
            itrace("requested static resource (" PRIs ") does not exist", _PRIs(path));
            // static file not found;
            throw HttpError(http::NotFound);
        }

        auto& cf = sf->second;
        if (mm.allowCaching) {
            // if file supports cache headers employ cache headers
            auto cc = req.header("If-Modified-Since");
            if (!cc.empty()) {
                time_t ifMod = Datetime(cc.data());
                if (ifMod >= cf.lastModified) {
                    // file was not modified
                    resp.end(http::NotModified);
                    return;
                }
            }

            cacheControl(req, resp, cf, mm);
        }

        if (req.getMethod() == Method::Head) {
            head(req, resp, cf, mm);
        }

        // get path from extension
        try {
            get(req, resp, cf, mm);
        } catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("'" PRIs "': %s", _PRIs(path), ex.message().c_str());
            resp.end(http::InternalError);
        }
    }

    void FileServer::get(const Request& req, Response& resp, CachedFile& cf, const MimeType& mm)
    {
        // set content type as configured on  mime
        resp.setContentType(mm.mime.peek());

        // prepare response
        prepareResponse(req, resp, cf, mm);
    }

    void FileServer::head(const Request& req, Response& resp, CachedFile& cf, const MimeType& mm)
    {
        if (mm.allowRange) {
            // let clients know that the server accepts ranges for current mime type
            resp.header("Accept-Ranges", "bytes");
        }
        else {
            // let clients know that the server doesn't accepts ranges for current mime type
            resp.header("Accept-Ranges", "none");
        }
    }

    void FileServer::prepareResponse(const Request& req, Response& resp, CachedFile& cf, const MimeType& mm)
    {
        if (mm.allowRange) {
            // let clients know that the server accepts ranges for current mime type
            resp.header("Accept-Ranges", "bytes");
        }
        else {
            // let clients know that the server doesn't accepts ranges for current mime type
            resp.header("Accept-Ranges", "none");
        }

        // check for range base Request support
        auto& range = req.header("Range");
        if (!range.empty() && mm.allowRange) {
            // prepare range based Request
            buildRangeResponse(req, resp, range, cf, mm);
            if (resp.isCompleted()) {
                return;
            }
        }
        else {
            // send the entire content
            addCachedFileChunk(resp, cf, cf.len, 0);
            resp.end(Status::Ok);
        }
    }

    void FileServer::cacheControl(
                const Request& req,
                Response& resp,
                const CachedFile& cf,
                const MimeType& mt)
    {
        auto lastMod = Datetime{cf.lastModified}(Datetime::HTTP_FMT);
        resp.header("Last-Modified", String{lastMod}.dup());
        if (mt.cacheExpires > 0) {
            Buffer b{64};
            b.appendf("public, max-age=%ld", mt.cacheExpires);
            resp.header("Cache-Control", String{b, true});
        }
    }

    bool FileServer::readFile(CachedFile& cf, const struct stat& st)
    {
        if (cf.fd < 0) {
            itrace("reloading a closed file not allowed");
            return false;
        }

        auto totalSize = size_t(st.st_size);
        if (size_t(st.st_size) > _config.mappedMin) {
            auto pageSize = getpagesize();
            totalSize += pageSize - (totalSize%pageSize);
            cf.data = mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, cf.fd, 0);
            if (cf.data == MAP_FAILED) {
                iwarn("mapping static resource %s {size %zu} failed: %s",
                          cf.path(), totalSize, errno_s);
                return false;
            }
            cf.isMapped = 1;
            cf.size = totalSize;

            return true;
        }

        // this code can yield the CPU so mark the resource as loading
        cf.isReloading = true;
        totalSize += 8;
        Buffer b{totalSize};
        size_t totalRead{0}, toRead{size_t(st.st_size)};
        do {
            auto nread = ::read(cf.fd, &b[totalRead], toRead-totalRead);
            if (nread < 0) {
                if (errno != EAGAIN) {
                    // reading file failed
                    iwarn("reading static resource %s failed: %s", cf.fd, errno_s);
                    break;
                }

                auto ev = fdwait(cf.fd, FDW_IN, _config.reloadFileTimeout);
                if ((ev & FDW_IN) != FDW_IN) {
                    // waiting for resource failed
                    iwarn("waiting static resource %s readability failed: %s", cf.fd, errno_s);
                    break;
                }
                continue;
            }
            totalRead += nread;
        } while (totalRead < toRead);

        if (totalRead == toRead) {
            b.seek(totalRead);
            cf.data = b.release();
            cf.size = totalSize;
            cf.isMapped = false;
        }
        cf.isReloading = false;

        return toRead == totalRead;
    }

    bool FileServer::reloadCachedFile(CachedFileIt& it, const struct stat& st)
    {
        auto& cf = it->second;
        if (!readFile(cf, st)) {
            // reading file failure
            auto cff = &cf;
            markCachedFileForRemoval(it);
            cff->reloadCond.notify();
            return false;
        }

        cf.reloadCond.notify();
        return true;
    }

    String FileServer::fileAbsolutePath(const String& rel) const
    {
        // we want to ensure that the file is within the base directory
        Buffer b(0);
        // append base, followed by file (notice rel(), ensure's const char* is used and size recomputed)
        b << _wwwDir << rel();
        char absolute[PATH_MAX];
        realpath((char *)b, absolute);
        String tmp{absolute, strlen(absolute), false};

        if (!tmp.startsWith(_wwwDir)) {
            // path violation
            idebug("requested path has back references: %s", rel());
            return {};
        }

        struct stat st{};
        if (stat(absolute, &st) != 0 || !S_ISREG(st.st_mode)) {
            // file does not exist
            idebug("Request path does not exist: %s", absolute);
            return {};
        }

        return tmp.dup();
    }

    String FileServer::relativePath(const CachedFile& cf)
    {
        return cf.path.substr(_wwwDir.size());
    }

    FileServer::CachedFileIt FileServer::loadFile(const String& rel, const MimeType& mt)
    {
        auto it = _cachedFiles.find(rel);
        if (it == _cachedFiles.end()) {
            auto path = fileAbsolutePath(rel);
            if (!path) {
                return it;
            }

            CachedFile cf;
            struct stat st{};
            stat(path.data(), &st);
            cf.fd = ::open(path.data(), O_RDONLY);
            if (cf.fd < 0) {
                iwarn("opening static resource %s failed: %s", path(), errno_s);
                return it;
            }

            cf.path         = std::move(path);
            cf.lastModified = time_t(st.st_mtim.tv_sec);
            cf.lastAccessed = time_t(st.st_atim.tv_sec);
            cf.len          = size_t(st.st_size);
            it  = addCachedFile(std::move(cf));

            if (_config.enableSendFile and mt.allowSendFile) {
                // Only enable sending if allowed for current file mime type
                it->second.useFd  = 1;
            }
            else if(!reloadCachedFile(it, st)) {
                itrace("reading cached resource '%s' failed: %s", it->second.path());
                ::close(it->second.fd);
                return _cachedFiles.end();
            }

            return it;
        }
        else {
            auto& cf = it->second;
            if (unlikely(cf.isReloading)) {
                // need to wait reload to be complete
                auto cfp = &cf;
                cfp->inFlight++;
                Sync sync;
                cfp->reloadCond.wait(sync);
                cfp->inFlight--;

                if (cfp->isGarbage) {
                    // at this point file was marked for removal so loading failed
                    return _cachedFiles.end();
                }

                // file was successfully reloaded
                return it;
            }

            struct  stat st{};
            if (::stat(cf.path(), &st) < 0) {
                // file might not exist anymore
                idebug("reloading static resource  %s failed: %s", cf.path(), errno_s);
                markCachedFileForRemoval(it);
                return _cachedFiles.end();
            }

            if (cf.lastModified == time_t(st.st_mtim.tv_sec)) {
                cf.lastAccessed = time_t(st.st_atim.tv_sec);
                // file not modified, no need to reload
                return it;
            }

            // if file was recently modified, reload
            cf.clear();
            cf.fd = ::open(cf.path(), O_RDONLY);
            if (cf.fd < 0) {
                iwarn("reloading static resource (%s) failed: %s", cf.path(), errno_s);
                markCachedFileForRemoval(it);
                return _cachedFiles.end();
            }

            cf.lastAccessed = time_t(st.st_atim.tv_sec);
            cf.lastModified = time_t(st.st_mtim.tv_sec);
            cf.len          = size_t(st.st_size);

            if (_config.enableSendFile and mt.allowSendFile) {
                // Only enable sending if allowed for current file mime type
                it->second.useFd  = 1;
            }
            else if (!reloadCachedFile(it, st)) {
                itrace("reloading static resource %s failed: %s", cf.path(), errno_s);
                ::close(cf.fd);
                return _cachedFiles.end();
            }

            return it;
        }
    }

    void FileServer::buildRangeResponse(
            const Request& req,
            Response& resp,
            const String& range,
            CachedFile& cf,
            const MimeType& mt)
    {
        std::vector<std::pair<size_t, size_t>> ranges;
        size_t start{0};
        auto pos = range.find('=');
        auto units = range.substr(start, pos);
        auto it = range.substr(pos);
        pos = it.empty()? String::npos: 0;
        for (; pos != String::npos; pos = it.find(',')) {
            it = it.substr(pos+1);
            pos = 0;
            while (isspace(it[pos])) pos++;

            size_t from{0}, to{0};
            from = std::strtoul(&it[pos], nullptr, 10);
            pos = it.find('-');
            if (pos != String::npos){
                it = it.substr(pos+1);
                if(!it.empty()) {
                    to = std::strtoul(&it[0], nullptr, 10) + 1;
                }
                else {
                    to = cf.len;
                }
            }
            else {
                it = it.substr(pos);
                to = cf.len;
            }
            itrace("partial content: %slu-%lu/%zu", from, to, cf.len);
            if (from > to or from > cf.len or to > cf.len) {
                itrace("requested range is out of bounds");
                resp.end(http::RequestRangeInvalid);
                return;
            }

            // add range to the list of ranges
            ranges.emplace_back(from, to-from);
        }

        if (ranges.empty()) {
            return;
        }

        if (ranges.size() == 1) {
            auto& [from, size] = ranges[0];
            addCachedFileChunk(resp, cf, size, from);

            // add range header
            Buffer b{64};
            b.appendf("bytes %lu-%lu/%lu", from, size-1, cf.len);
            resp.header("Content-Range", String{b, true});
        }
        else {
            // multiple ranges require partial content
            buildPartialContent(resp, ranges, cf, mt);
        }
    }

    void FileServer::buildPartialContent(
                Response& resp,
                const std::vector<std::pair<size_t, size_t>>& ranges,
                CachedFile& cf, const MimeType& mt)
    {

#undef  CRLF
#define CRLF "\r\n"
        auto boundary = suil::randbytes(8);
        bool first{true};
        for (auto& [from, size]: ranges) {
            Buffer hdr{128};
            hdr << (!first? CRLF "--" : "--") << boundary << CRLF;
            hdr << "Content-Type: " << mt.mime << CRLF;
            hdr.appendf("Content-Range: %lu-%lu/%lu", from, size-1, cf.len);
            hdr << CRLF CRLF;
            auto tmp = hdr.size();
            resp.chunk({hdr.release(), tmp, 0, [](void *ptr) { ::free(ptr); }});
            addCachedFileChunk(resp, cf, size, from);
            first = false;
        }
        Buffer term{boundary.size() + 6};
        term << "--" << boundary << "--";
        auto tmp = term.size();
        resp.chunk({term.release(), tmp, 0, [](void *ptr) { ::free(ptr); }});

        resp.setContentType(suil::catstr("multipart/byteranges; boundary=", boundary));
#undef  CRLF
    }

    FileServer::CachedFileIt FileServer::addCachedFile(CachedFile&& cf)
    {
        auto rel = relativePath(cf);
        if (cf.useFd) {
            // cached file is not really loaded in memory, just add to cache and move on
            return _cachedFiles.emplace(std::move(rel), std::move(cf)).first;
        }

        auto newSize = _totalMemCachedSize + cf.len;
        if (newSize < _config.maxCacheSize) {
            // still within bounds, update and move on
            _totalMemCachedSize = newSize;
            return _cachedFiles.emplace(std::move(rel), std::move(cf)).first;
        }

        // this function will ensure that at the end, first iterator is the least recently used
        auto leastRecentlyUsed = [](const CachedFileIt& a, const CachedFileIt& b) {
            auto& aa = a->second;
            auto& bb = b->second;
            return std::make_tuple(aa.lastAccessed, aa.inFlight) <
                   std::make_tuple(bb.lastAccessed, bb.inFlight);
        };

        std::set<CachedFileIt, decltype(leastRecentlyUsed)> cachedIts{leastRecentlyUsed};
        for (auto it = _cachedFiles.begin(); it != _cachedFiles.end(); it++) {
            cachedIts.emplace(it);
        }

        auto it = cachedIts.begin();
        while (newSize > _config.maxCacheSize and it != cachedIts.end()) {
            if ((*it)->second.useFd == 0) {
                // remove as much as we can, skip those that use file descriptor
                newSize -= (*it)->second.len;
                markCachedFileForRemoval(*it);
            }
            it++;
        }

        _totalMemCachedSize = newSize;
        return _cachedFiles.emplace(std::move(rel), std::move(cf)).first;
    }

    void FileServer::markCachedFileForRemoval(CachedFileIt it)
    {
        auto cf = std::move(it->second);
        _cachedFiles.erase(it);
        if (cf.useFd == 0) {
            _totalMemCachedSize -= cf.len;
        }

        if (cf.inFlight == 0) {
            // if the file is not in flight remove immediately
            cf.clear();
            return;
        }

        // Put file in garbage so that it will be collected by garbage collector
        cf.isGarbage = true;
        _garbageCache.push_back(std::move(cf));
        if (_isCollectingGarbage) {
            // might be waiting for garbage
            _gcCond.notify();
        }
        else {
            // not waiting, start collecting
            _isCollectingGarbage = true;
            go(garbageCollector(Ego));
        }
    }

    void FileServer::derefCachedFileChunk(CachedFile& cf)
    {
        // this function will be invoked after a chunk is used up
        cf.inFlight--;
        if (cf.isGarbage and cf.inFlight == 0) {
            // enable garbage collector
            _gcCond.notifyOne();
        }
    }

    void FileServer::addCachedFileChunk(Response& resp, CachedFile& cf, size_t size, off_t from)
    {
        cf.inFlight++;
        if (_config.enableSendFile and cf.useFd) {
            resp.chunk(net::Chunk{cf.fd, size, off_t(from), [this, cfp = &cf](int) {
                derefCachedFileChunk(*cfp);
            }});
        }
        else {
            resp.chunk(net::Chunk{cf.data, size, off_t(from), [this, cfp = &cf](void *) {
                derefCachedFileChunk(*cfp);
            }});
        }
    }

    void FileServer::garbageCollector(FileServer& S)
    {
        ltrace(&S, "garbage collector started");
        S._isCollectingGarbage = true;
        do {
            auto it = S._garbageCache.begin();
            while (it != S._garbageCache.end()) {
                if (it->inFlight == 0) {
                    // remove all cached files with 0 references
                    it->clear();
                    it = S._garbageCache.erase(it);
                }
                else {
                    it++;
                }
            }

            if (!S._garbageCache.empty()) {
                // wait for reference holders to release the reference
                Sync sync;
                S._gcCond.wait(sync);
            }
        } while (!S._garbageCache.empty());

        S._isCollectingGarbage = true;
        ltrace(&S, "garbage collector done");
    }
}