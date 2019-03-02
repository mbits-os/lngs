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

namespace lngs::plurals::nodes {
	struct heap_only : expr {
		heap_only() = default;
		heap_only(heap_only&&) = delete;
		heap_only(const heap_only&) = delete;
		heap_only& operator=(const heap_only&) = delete;
		heap_only& operator=(heap_only&&) = delete;
	};

	// symbols:
	struct var : heap_only {
		intmax_t eval(intmax_t n, bool&) const noexcept override { return n; }
	};

	class value : public heap_only {
		int m_val = 0;
	public:
		value() = default;
		value(int val) : m_val(val) {}

		intmax_t eval(intmax_t, bool&) const noexcept override { return m_val; }
	};

	// unary-op

	class logical_not : public heap_only {
		std::unique_ptr<expr> m_arg1;
	public:
		logical_not() = default;
		explicit logical_not(std::unique_ptr<expr>&& arg1) : m_arg1(std::move(arg1)) {}

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto op = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			return !op;
		}
	};

	// binary-ops

	class binary : public heap_only {
	protected:
		std::unique_ptr<expr> m_arg1;
		std::unique_ptr<expr> m_arg2;
	public:
		explicit binary(std::unique_ptr<expr>&& arg1, std::unique_ptr<expr>&& arg2) : m_arg1(std::move(arg1)), m_arg2(std::move(arg2)) {}
	};

	class multiply : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left * right;
		}
	};

	class divide : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override
		{
			const auto right = m_arg2->eval(n, failed);
			if (!right)
				failed = true;
			if (failed)
				return 0;
			const auto left = m_arg1->eval(n, failed);
			return left / right;
		}
	};

	class modulo : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override
		{
			const auto right = m_arg2->eval(n, failed);
			if (!right)
				failed = true;
			if (failed)
				return 0;
			const auto left = m_arg1->eval(n, failed);
			return left % right;
		}
	};

	class plus : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left + right;
		}
	};

	class minus : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left - right;
		}
	};

	class less_than : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left < right;
		}
	};

	class greater_than : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left > right;
		}
	};

	class less_than_or_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left <= right;
		}
	};

	class greater_than_or_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left >= right;
		}
	};

	class equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left == right;
		}
	};

	class not_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left != right;
		}
	};

	class logical_and : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed || !left)
				return 0;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left && right;
		}
	};

	class logical_or : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			if (left)
				return 1;
			const auto right = m_arg2->eval(n, failed);
			if (failed)
				return 0;
			return left || right;
		}
	};

	// ternary-op

	class ternary : public heap_only {
		std::unique_ptr<expr> m_arg1;
		std::unique_ptr<expr> m_arg2;
		std::unique_ptr<expr> m_arg3;
	public:
		ternary() = default;
		explicit ternary(std::unique_ptr<expr>&& arg1, std::unique_ptr<expr>&& arg2, std::unique_ptr<expr>&& arg3)
			: m_arg1(std::move(arg1))
			, m_arg2(std::move(arg2))
			, m_arg3(std::move(arg3))
		{}

		intmax_t eval(intmax_t n, bool& failed) const noexcept override {
			const auto left = m_arg1->eval(n, failed);
			if (failed)
				return 0;
			const auto right = (left ? m_arg2 : m_arg3)->eval(n, failed);
			if (failed)
				return 0;
			return right;
		}
	};
}
