/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/LexicalPath.h>
#include <AK/String.h>
#include <AK/StringBuilder.h>
#include <LibCore/DirIterator.h>
#include <LibCore/File.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

bool copy_file_or_directory(String, String, bool, bool);
bool copy_file(String, String, const struct stat&, int);
bool copy_directory(String, String, bool);

static mode_t my_umask = 0;

/**
 * Copy a file or directory to a new location. Returns true if successful, false
 * otherwise. If there is an error, its description is output to stderr.
 *
 * Directories should only be copied if recursion_allowed is set.
 */
bool copy_file_or_directory(String src_path, String dst_path, bool recursion_allowed, bool link)
{
    int src_fd = open(src_path.characters(), O_RDONLY);
    if (src_fd < 0) {
        perror("open src");
        return false;
    }

    struct stat src_stat;
    int rc = fstat(src_fd, &src_stat);
    if (rc < 0) {
        perror("stat src");
        return false;
    }

    if (S_ISDIR(src_stat.st_mode)) {
        if (!recursion_allowed) {
            fprintf(stderr, "cp: -R not specified; omitting directory '%s'\n", src_path.characters());
            return false;
        }
        return copy_directory(src_path, dst_path, link);
    }
    if (link) {
        if (::link(src_path.characters(), dst_path.characters()) < 0) {
            perror("link");
            return false;
        }
        return true;
    }

    return copy_file(src_path, dst_path, src_stat, src_fd);
}

/**
 * Copy a source file to a destination file. Returns true if successful, false
 * otherwise. If there is an error, its description is output to stderr.
 *
 * To avoid repeated work, the source file's stat and file descriptor are required.
 */
bool copy_file(String src_path, String dst_path, const struct stat& src_stat, int src_fd)
{
    // NOTE: We don't copy the set-uid and set-gid bits.
    mode_t mode = (src_stat.st_mode & ~my_umask) & ~06000;

    int dst_fd = creat(dst_path.characters(), mode);
    if (dst_fd < 0) {
        if (errno != EISDIR) {
            perror("open dst");
            return false;
        }
        StringBuilder builder;
        builder.append(dst_path);
        builder.append('/');
        builder.append(LexicalPath(src_path).basename());
        dst_path = builder.to_string();
        dst_fd = creat(dst_path.characters(), 0666);
        if (dst_fd < 0) {
            perror("open dst");
            return false;
        }
    }

    if (src_stat.st_size > 0) {
        if (ftruncate(dst_fd, src_stat.st_size) < 0) {
            perror("cp: ftruncate");
            return false;
        }
    }

    for (;;) {
        char buffer[32768];
        ssize_t nread = read(src_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("read src");
            return false;
        }
        if (nread == 0)
            break;
        ssize_t remaining_to_write = nread;
        char* bufptr = buffer;
        while (remaining_to_write) {
            ssize_t nwritten = write(dst_fd, bufptr, remaining_to_write);
            if (nwritten < 0) {
                perror("write dst");
                return false;
            }
            assert(nwritten > 0);
            remaining_to_write -= nwritten;
            bufptr += nwritten;
        }
    }

    close(src_fd);
    close(dst_fd);
    return true;
}

/**
 * Copy the contents of a source directory into a destination directory.
 */
bool copy_directory(String src_path, String dst_path, bool link)
{
    int rc = mkdir(dst_path.characters(), 0755);
    if (rc < 0) {
        perror("cp: mkdir");
        return false;
    }

    String src_rp = Core::File::real_path_for(src_path);
    src_rp = String::format("%s/", src_rp.characters());
    String dst_rp = Core::File::real_path_for(dst_path);
    dst_rp = String::format("%s/", dst_rp.characters());

    if (!dst_rp.is_empty() && dst_rp.starts_with(src_rp)) {
        fprintf(stderr, "cp: Cannot copy %s into itself (%s)\n",
            src_path.characters(), dst_path.characters());
        return false;
    }

    Core::DirIterator di(src_path, Core::DirIterator::SkipDots);
    if (di.has_error()) {
        fprintf(stderr, "cp: DirIterator: %s\n", di.error_string());
        return false;
    }
    while (di.has_next()) {
        String filename = di.next_path();
        bool is_copied = copy_file_or_directory(
            String::format("%s/%s", src_path.characters(), filename.characters()),
            String::format("%s/%s", dst_path.characters(), filename.characters()),
            true, link);
        if (!is_copied) {
            return false;
        }
    }
    return true;
}
