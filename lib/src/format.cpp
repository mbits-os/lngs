/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <format.hpp>
#include <cctype>

namespace str {
	namespace detail {
		std::string format(const char* fmt, const std::vector<std::string>& packed)
		{
			const char* c = fmt;
			const char* e = c ? c + strlen(c) : c;

			std::string o;
			o.reserve(e - c);

			while (c != e) {
				if (*c != '%') {
					o.push_back(*c);
				} else {
					++c; if (c == e) break;
					if (!std::isdigit((unsigned char)*c)) {
						o.push_back(*c);
					} else {
						size_t ndx = 0;
						while (c != e && std::isdigit((unsigned char)*c)) {
							ndx *= 10;
							ndx += *c - '0';
							++c;
						}

						if (ndx) {
							--ndx;
							if (packed.size() > ndx)
								o.append(packed[ndx]);
						}

						continue;
					}
				}
				++c;
			}

			return o;
		}
	}
}
