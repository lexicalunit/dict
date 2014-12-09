// lexicalunit (c) 2012
// 
// This code is released under the Artistic License 2.0.
// http://opensource.org/licenses/artistic-license-2.0

#include "dict.h"
#include <cassert>
#include <limits>

class int_visitor : public boost::static_visitor<void>
{
public:
	int_visitor(const int check)
	: check(check)
	{

	}

	// visiting an int
	void operator()(const int i) const
	{
		assert(check == i);
	}

	// visiting anything else
	template<class T>
	void operator()(const T&) const
	{
		assert(false);
	}

private:
	const int check;
};

int main()
{
	{
		// standard usage
		dict d;

		const int i_in = 5;
		const float f_in = 3.14;
		const bool b_in = true;
		const std::string s_in = "test";
		const int a_in[] = {1, 2, 3};
		const std::vector<int> v_in(a_in, a_in + sizeof(a_in) / sizeof(a_in[0]));
		
		d.add("int", i_in);
		d.add("float", f_in);
		d.add("bool", b_in);
		d.add("string", s_in);
		d.add("vector", v_in);

		int i_out = 0;
		float f_out = 0;
		bool b_out = false;
		std::string s_out;
		std::vector<int> v_out;

		assert(d.get("int", i_out));
		assert(d.get("float", f_out));
		assert(d.get("bool", b_out));
		assert(d.get("string", s_out));
		assert(d.get("vector", v_out));

		assert(i_in == i_out);
		assert(f_in == f_out);
		assert(b_in == b_out);
		assert(s_in == s_out);
		assert(v_in == v_out);

		dict child_in;
		const int child_value_in = 8;
		child_in.add("value", child_value_in);
		d.add("child", child_in);

		dict child_out;
		int child_value_out = 0;
		d.get("child", child_out);
		assert(child_out.get("value", child_value_out));
		assert(child_value_in == child_value_out);

		dict a, b, c;
		std::vector<dict> v;
		v.push_back(a);
		v.push_back(b);
		v.push_back(c);
		d.add("v", v);
		std::vector<dict> r;
		d.get_recursive("v", r);
		assert(v == r);

		d.add("a", 1);
		d.add("a", 2);
		int i;
		assert(d.get("a", i));
		assert(i == 2);
	}

	{
		// iterators and visitors
		dict d;
		d.add("a", 3);
		d.add("b", 2);
		d.add("c", 1);

		int check = 3;
		for(dict::iterator i = d.begin(), end = d.end(); i != end; ++i)
		{
			switch(check)
			{
				case 3: assert(i->first == "a"); break;
				case 2: assert(i->first == "b"); break;
				case 1: assert(i->first == "c"); break;
			}
			boost::apply_visitor(int_visitor(check), i->second); // using compile-time visitor
			assert(boost::get<int>(i->second) == check); // using run-time get
			--check;
		}
	}

	{
		// sub dictionaries
		dict a, b, c;
		a.add("v", 1);
		b.add("v", 2);
		c.add("v", 3);
		b.add("c", c);
		a.add("b", b);

		assert(a < b);
		assert(a < c);
		assert(b < c);

		dict b_out, c_out;
		assert(a.get_recursive("b", b_out));
		b_out.get("c", c_out);
		assert(b == b_out);
		assert(c == c_out);
		assert(a.size() == 2);

		int i;
		assert(a.get_recursive("b::c::v", i));
		assert(i == 3);

		// alternative to get_recursive
		i = 0;
		a.get("b").get("c").get("v", i);
		assert(i == 3);
	}

	{
		// implicit conversions
		dict d;
		d.add("fp", 92.1f);
		int k;
		assert(d.get("fp", k)); // got float as int
		assert(k == 92);

		d.add("ii", k);
		float fi;
		assert(d.get("ii", fi)); // got int as float
		assert(fi == 92);

		d.add("int", 1);
		bool b;
		assert(d.get("int", b)); // got int as bool
		assert(b);

		d.add("float", 3.14f);
		double lf;
		assert(d.get("float", lf)); // got float as double
		assert(std::abs(lf - 3.14) < std::numeric_limits<float>::epsilon());

		d.add("double", 6.92); // stored double as float
		assert(d.get("double", lf));
		assert(std::abs(lf - 6.92) < std::numeric_limits<float>::epsilon());

		d.add("string", "literal"); // stored literal as std::string
		std::string literal;
		assert(d.get("string", literal));
		assert(literal == "literal");

		long l = 327;
		d.add("long", l); // stored long as int
		long l_out;
		assert(d.get("long", l_out));
		assert(l_out == 327);
	}

	{
		// string conversions
		dict d;
		d.add("float", 1.2f);
		std::string s;
		d.get("float", s);
		assert(s.substr(0, 3) == "1.2");

		const int a_in[] = {1, 2, 3};
		const std::vector<int> v_in(a_in, a_in + sizeof(a_in) / sizeof(a_in[0]));
		d.add("vector", v_in);
		std::string sv;
		d.get("vector", sv);
		assert(sv == "[1, 2, 3]");
	}

	{
		// invalid access
		dict d, child;
		d.add("s", "s");
		double lf;
		assert(!d.get("s", lf)); // conversion fails 
		assert(!d.get("invalid", lf)); // lookup fails
		assert(!d.get("invalid", child)); // lookup fails
	}

	{
		// std::string representation of dict
		dict a, b, c;
		a.add("i", 1);
		a.add("f", "3.14");
		b.add("a", a);
		c.add("b", b);
		std::string s;
		assert(c.get("b", s));
		assert(s == "{'a': {'i': 1, 'f': 3.14}}");

		assert(c.size() == 1);
		assert(c.size_recursive() == 4);

		int x;
		assert(c.get_recursive("b::a::i", x));
		assert(x == 1);

		assert(c.str() == "{'b': {'a': {'i': 1, 'f': 3.14}}}");
	}

	{
		// various add() methods
		dict d;
		d.add_back("a", 1);
		d.add_front("b", 2);
		d.add_back("c", 3);
		dict::const_iterator i = d.begin();
		assert(i->first == "b");
		++i;
		assert(i->first == "a");
		++i;
		assert(i->first == "c");

		d.pop_front();
		d.pop_back();
		assert(d.front().first == "a");
		assert(d.back().first == "a");
	}

	{
		// compile errors
		// dict d;
		// d.add("foo", (int*)0); // no matching member function for type int*
	}
}
