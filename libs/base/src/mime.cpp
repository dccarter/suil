//
// Created by Mpho Mbotho on 2020-10-13.
//

#include "suil/base/mime.hpp"

namespace suil {

    const char* mimetype(const String& filename) {
        const char *ext = strrchr(filename.data(), '.');
        static UnorderedMap<const char*, CaseInsensitive> MIME_TYPES = {
            {".html", "text/html"},
            {".css",  "text/css"},
            {".csv",  "text/csv"},
            {".txt",  "text/plain"},
            {".sgml", "text/sgml"},
            {".tsv",  "text/tab-separated-values"},
            // add compressed mime types
            {".bz",   "application/x-bzip"},
            {".bz2",  "application/x-bzip2"},
            {".gz",   "application/x-gzip"},
            {".tgz",  "application/x-tar"},
            {".tar",  "application/x-tar"},
            {".zip",  "application/zip, application/x-compressed-zip"},
            {".7z",   "application/zip, application/x-compressed-zip"},
            // add image mime types
            {".jpg",  "image/jpeg"},
            {".png",  "image/png"},
            {".svg",  "image/svg+xml"},
            {".gif",  "image/gif"},
            {".bmp",  "image/bmp"},
            {".tiff", "image/tiff"},
            {".ico",  "image/x-icon"},
            // add video mime types
            {".avi",  "video/avi"},
            {".mpeg", "video/mpeg"},
            {".mpg",  "video/mpeg"},
            {".mp4",  "video/mp4"},
            {".qt",   "video/quicktime"},
            // add audio mime types
            {".au",   "audio/basic"},
            {".midi", "audio/x-midi"},
            {".mp3",  "audio/mpeg"},
            {".ogg",  "audio/vorbis, application/ogg"},
            {".ra",   "audio/x-pn-realaudio, audio/vnd.rn-realaudio"},
            {".ram",  "audio/x-pn-realaudio, audio/vnd.rn-realaudio"},
            {".wav",  "audio/wav, audio/x-wav"},
            // Other common mime types
            {".json", "application/json"},
            {".js",   "application/javascript"},
            {".ttf",  "font/ttf"},
            {".xhtml","application/xhtml+xml"},
            {".xml",  "application/xml"}
        };

        if (ext != nullptr) {
            String tmp(ext);
            auto it = MIME_TYPES.find(tmp);
            if (it != MIME_TYPES.end())
                return it->second;
        }

        return "text/plain";
    }

}

