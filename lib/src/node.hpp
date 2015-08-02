#pragma once

namespace plurals { namespace nodes {
	// symbols:
	class var : public expr {
	public:
		intmax_t eval(intmax_t n) const override { return n; }
	};

	class value : public expr {
		int m_val = 0;
	public:
		value() = default;
		value(int val) : m_val(val) {}
		value(value&&) = default;

		value(const value&) = delete;
		value& operator=(const value&) = delete;
		value& operator=(value&&) = delete;

		intmax_t eval(intmax_t) const override { return m_val; }
	};

	// unary-op

	class logical_not : public expr {
		std::unique_ptr<expr> m_arg1;
	public:
		logical_not() = default;
		explicit logical_not(std::unique_ptr<expr>&& arg1) : m_arg1(std::move(arg1)) {}
		logical_not(logical_not&&) = default;

		logical_not(const logical_not&) = delete;
		logical_not& operator=(const logical_not&) = delete;
		logical_not& operator=(logical_not&&) = delete;

		intmax_t eval(intmax_t n) const override { return !m_arg1->eval(n); }
	};

	// binary-ops

	class binary : public expr {
	protected:
		std::unique_ptr<expr> m_arg1;
		std::unique_ptr<expr> m_arg2;
	public:
		binary() = default;
		explicit binary(std::unique_ptr<expr>&& arg1, std::unique_ptr<expr>&& arg2) : m_arg1(std::move(arg1)), m_arg2(std::move(arg2)) {}
		binary(binary&&) = default;

		binary(const binary&) = delete;
		binary& operator=(const binary&) = delete;
		binary& operator=(binary&&) = delete;
	};

	class multiply : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) * m_arg2->eval(n); }
	};

	class divide : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override
		{
			auto right = m_arg2->eval(n);
			if (!right) throw false;
			return m_arg1->eval(n) / right;
		}
	};

	class modulo : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override
		{
			auto right = m_arg2->eval(n);
			if (!right) throw false;
			return m_arg1->eval(n) % right;
		}
	};

	class plus : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) + m_arg2->eval(n); }
	};

	class minus : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) - m_arg2->eval(n); }
	};

	class less_than : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) < m_arg2->eval(n); }
	};

	class greater_than : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) > m_arg2->eval(n); }
	};

	class less_than_or_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) <= m_arg2->eval(n); }
	};

	class greater_than_or_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) >= m_arg2->eval(n); }
	};

	class equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) == m_arg2->eval(n); }
	};

	class not_equal : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) != m_arg2->eval(n); }
	};

	class logical_and : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) && m_arg2->eval(n); }
	};

	class logical_or : public binary {
	public:
		using binary::binary;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) || m_arg2->eval(n); }
	};

	// ternary-op

	class ternary : public expr {
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
		ternary(ternary&&) = default;

		ternary(const ternary&) = delete;
		ternary& operator=(const ternary&) = delete;
		ternary& operator=(ternary&&) = delete;

		intmax_t eval(intmax_t n) const override { return m_arg1->eval(n) ? m_arg2->eval(n) : m_arg3->eval(n); }
	};
}}
