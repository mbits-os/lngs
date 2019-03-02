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

#pragma once

#include <cstdint>

namespace lngs {
	// File:
	// LANG[ hdr....]....[last]
	//   - 'LANG' word immediately followed by ' hdr' section,
	//   - followed by other sections,
	//   - finished with 'last' sections
	//
	// Section:
	//  INDEX  OFFSET  SIZE    MEANING
	//  [0]         0      4   Section identifier
	//  [1]         4      4   Size of the rest of section - after that word - in words
	//  [2]         8  [1]*4   Section contents
	//
	// ' hdr' section:
	//  [2]         8      4   Version of the file syntax. This library will read any 1.x file and interpret it as 1.0 file
	//  [3]        12      4   Value of the strings@serial attribute from the strings definition file
	//
	// 'last' section:
	//  [1]         4      4   Always zero - there is no contents.
	//
	//  String sections ('attr', 'strs', 'keys'):
	//  [2]         8      4   Strings count
	//  [3]        12      4   Offset to the begining of the strings data, in words, counting from the begining of the section
	//  [4]        16 [2]*12   String keys
	//              0      4    - identifier of the key; for 'strs' and 'keys' it is @id from the strings definition file, for
	//                            'attr' it's one of attr_t
	//              4      4    - offset of the string counted from the beginning of the data, in bytes
	//              8      4    - length of the string in bytes, not counting the zero at the end
	//  [5]     [3]*4    ?*4   String data. Each string takes as much as [4][2] bytes, terminated by a zero byte. The data is word-aligned.

	struct section_header {
		uint32_t id;
		uint32_t ints;
	};

	inline namespace v1_0 {
		enum attr_t {
			ATTR_CULTURE,
			ATTR_LANGUAGE,
			ATTR_PLURALS
		};

		constexpr uint32_t version = 0x00000100u;

		enum tag_t : uint32_t {
			langtext_tag = 0x474E414Cu,
			hdrtext_tag  = 0x72646820u,
			attrtext_tag = 0x72747461u,
			strstext_tag = 0x73727473u,
			keystext_tag = 0x7379656Bu,
			lasttext_tag = 0x7473616Cu
		};

		struct file_header : section_header {
			uint32_t version;
			uint32_t serial;
		};

		struct string_header : section_header {
			uint32_t string_count;
			uint32_t string_offset; // in ints
		};

		struct string_key {
			uint32_t id = 0;
			uint32_t offset = 0;
			uint32_t length = 0;
		};
	}
}
