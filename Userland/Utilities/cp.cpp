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

#include "cp.h"
#include <LibCore/ArgsParser.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (pledge("stdio rpath wpath cpath fattr", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    bool link = false;
    bool recursion_allowed = false;
    bool verbose = false;
    Vector<const char*> sources;
    const char* destination = nullptr;

    Core::ArgsParser args_parser;
    args_parser.add_option(link, "Link files instead of copying", "link", 'l');
    args_parser.add_option(recursion_allowed, "Copy directories recursively", "recursive", 'R');
    args_parser.add_option(recursion_allowed, "Same as -R", nullptr, 'r');
    args_parser.add_option(verbose, "Verbose", "verbose", 'v');
    args_parser.add_positional_argument(sources, "Source file path", "source");
    args_parser.add_positional_argument(destination, "Destination file path", "destination");
    args_parser.parse(argc, argv);

    auto my_umask = umask(0);
    umask(my_umask);

    for (auto& source : sources) {
        bool ok = copy_file_or_directory(source, destination, recursion_allowed, link);
        if (!ok)
            return 1;
        if (verbose)
            printf("'%s' -> '%s'\n", source, destination);
    }
    return 0;
}
