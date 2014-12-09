// lexicalunit (c) 2012
//
// This code is released under the Artistic License 2.0.
// http://opensource.org/licenses/artistic-license-2.0

#ifndef LEXICALUNIT_DICT_H
#define LEXICALUNIT_DICT_H

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/swap.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/variant.hpp>
#include <string>
#include <utility>
#include <vector>

// todo: find_recursive(), count_recursive(), erase_recursive() methods?
// todo: find_if(), erase_if()/remove_if() methods? recursive versions too?
// todo: rearrange() method?
// todo: splice(), unique(), sort(), reverse() methods?
// todo: separate add(), modify(), and replace() methods?
// todo: key_iterators?: equal_range(), lower_bound(), upper_bound(), etc...
// todo: better coverage with unit tests
// todo: c++11: emplace_front(), emplace_back()
// todo: c++11: emplace(), emplace_hint()
// todo: c++11: key_iterator erase(key_const_iterator position);
// todo: c++11: key_iterator erase(key_const_iterator first, key_const_iterator last);

//! Finds the first type in the MPL Sequence to which T is convertible.
template<class Sequenece, class T>
struct find_convertible
: boost::mpl::deref<
	typename boost::mpl::find_if<
		Sequenece
		, boost::is_convertible<T, boost::mpl::_1>
	>::type
>
{ };

//! Inherits from true_type if T is convertible to any type in the MPL Sequence, otherwise inherits from false_type.
template<class Sequenece, class T>
struct has_convertible
: boost::mpl::not_<
	boost::is_same<
		typename boost::mpl::find_if<
			Sequenece
			, boost::is_convertible<T, boost::mpl::_1>
		>::type
		, typename boost::mpl::end<Sequenece>::type
	>
>
{ };

//! Inherits from true_type if T is a std::vector<...>, otherwise inherits from false_type.
template<class T>
struct is_vector
: boost::false_type
{ };

//! Inherits from true_type if T is a std::vector<...>, otherwise inherits from false_type.
template<class T, class A>
struct is_vector<std::vector<T, A> >
: boost::true_type
{ };

//! Inherits from true_type if T is supported directly by dict, otherwise inherits from false_type.
template<class>
struct dict_supports;

//! Inherits from true_type if T is can be implicitly supported directly by dict, otherwise inherits from false_type.
template<class>
struct dict_implicitly_supports;

//! Provides a Python-like dictionary type.
class dict
{
public:
	typedef std::string key_type; //!< Lookup type for this dictionary.
	typedef boost::variant<
		// order matters: preference first
		float
		, int
		, std::string
		, std::vector<int>
		, std::vector<float>
		, std::vector<std::string>
		, std::vector<bool>
		, boost::recursive_wrapper<dict>
		, boost::recursive_wrapper<std::vector<dict> >
		// , bool // problematic and unnecessary
	> mapped_type; //!< Limited supported types that can be stored in this dictionary.
	typedef mapped_type::types types; //!< MPL Sequence of supported types.
	typedef std::pair<key_type, mapped_type> value_type; //!< Value type stored by this dictionary.
	typedef value_type& reference; //!< value_type&.
	typedef const value_type& const_reference; //!< const value_type&.
	typedef value_type* pointer; //!< value_type*.
	typedef const value_type* const_pointer;  //!< const value_type*.

private:
	typedef boost::multi_index_container<
		value_type
		, boost::multi_index::indexed_by<
			boost::multi_index::sequenced<>
			, boost::multi_index::hashed_unique<boost::multi_index::member<value_type, key_type, &value_type::first> >
		>
	> storage_type;
	typedef storage_type::nth_index<0>::type sequenced_index_type;
	typedef storage_type::nth_index<1>::type key_index_type;

private:
	sequenced_index_type& sequenced_index()
	{
		return storage.get<0>();
	}

	const sequenced_index_type& sequenced_index() const
	{
		return storage.get<0>();
	}

	key_index_type& key_index()
	{
		return storage.get<1>();
	}

	const key_index_type& key_index() const
	{
		return storage.get<1>();
	}

	template<class T>
	typename boost::enable_if<dict_supports<T>, void>::type
	add_impl(const key_type& key, const T& value, const bool back)
	{
		if(count(key))
		{
			key_index().replace(key_index().find(key), value_type(key, value));
		}
		else
		{
			if(back)
				storage.push_back(value_type(key, value));
			else
				storage.push_front(value_type(key, value));
		}
	}

	template<class T>
	typename boost::enable_if<dict_implicitly_supports<T>, void>::type
	add_impl(const key_type& key, const T& value, const bool back);

	#ifndef BOOST_NO_STATIC_ASSERT
	template<class T>
	typename boost::disable_if<boost::mpl::or_<dict_supports<T>, dict_implicitly_supports<T> >, void>::type
	add_impl(const key_type& key, const T& value, const bool back)
	{
		static_assert(sizeof(T) == 0, "type is not supported");
	}
	#endif // BOOST_NO_STATIC_ASSERT

public:
	typedef storage_type::size_type size_type; //!< Unsigned integral type.
	typedef storage_type::difference_type difference_type; //!< Signed integer type.
	typedef key_index_type::key_equal key_equal; //!< Functor suitable for testing key_type equality.
	typedef key_index_type::hasher hasher; //!< Hashing function for key_type.
	typedef key_index_type::key_from_value key_from_value; //!< Functor suitable for extracting the key from a value_type.
	typedef sequenced_index_type::iterator iterator; //!< Sequential iterator, ordered by insertion.
	typedef sequenced_index_type::const_iterator const_iterator; //!< Sequential iterator, ordered by insertion.
	typedef sequenced_index_type::reverse_iterator reverse_iterator; //!< Reverse sequential iterator, ordered by insertion.
	typedef sequenced_index_type::const_reverse_iterator const_reverse_iterator; //!< Reverse sequential iterator, ordered by insertion.

public:
	//! Adds (or replaces if already existent) the given (key, value) pair to to this dictionary.
	//! Value may be implicitly converted to a supported type.
	template<class T>
	void add(const key_type& key, const T& value)
	{
		add_impl(key, value, true);
	}

	//! Adds (or replaces if already existent) the given (key, value) pair to the end of this dictionary.
	//! Value may be implicitly converted to a supported type.
	template<class T>
	void add_front(const key_type& key, const T& value)
	{
		add_impl(key, value, false);
	}

	//! Same as add().
	template<class T>
	void add_back(const key_type& key, const T& value)
	{
		add(key, value);
	}

	//! Gets the value associated with the given key out of this dictionary.
	//! Failure occurs if conversion to type T can not be preformed or if the given key does not exist.
	//! Returns true on success, false otherwise.
	template<class T>
	bool get(const key_type& key, T& value) const;

	//! Gets a dict value at the associated key if possible, otherwise returns an empty dict.
	dict get(const key_type& key) const;

	//! Same as get() but additionally supports recursively descending into sub-dictionaries by delimiting sub-keys with "::".
	template<class T>
	bool get_recursive(const key_type& key, T& value) const;

	//! Returns the first item from this dictionary.
	const_reference front() const
	{
		return sequenced_index().front();
	}

	//! Returns the last item from this dictionary.
	const_reference back() const
	{
		return sequenced_index().back();
	}

	//! Erases the first item from this dictionary.
	void pop_front()
	{
		storage.pop_front();
	}

	//! Erases the last item from this dictionary.
	void pop_back()
	{
		storage.pop_back();
	}

	//! Inserts the given value into this dictionary if it doesn't already exist.
	//! Returns iterator to the inserted value (or value that blocked insertion) and bool indicating success.
	template<class T>
	std::pair<iterator, bool> insert(const std::pair<key_type, T>& value);

	//! Inserts a range of values into this dictionary.
	template<class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		while(first != last)
			insert(*first++);
	}

	//! Returns an object suitable for extracting keys from values.
	key_from_value key_extractor() const
	{
		return key_index().key_extractor();
	}

	//! Returns an object suitable equality testing keys.
	key_equal key_eq() const
	{
		return key_index().key_eq();
	}

	//! Returns an object suitable for hashing keys.
	hasher hash_function() const
	{
		return key_index().hash_function();
	}

	//! Clears all items from this dictionary.
	void clear()
	{
		storage.clear();
	}

	//! Returns 1 if key exists in this dictionary, otherwise 0.
	//! Note that this method will not recursively look into sub-dictionaries.
	size_type count(const key_type& key) const
	{
		return key_index().count(key);
	}

	//! Returns the number of items in this dictionary.
	//! Note recursive counting in sub-dictionaries is not performed.
	size_type size() const
	{
		return storage.size();
	}

	//! Same as size() except that it will recursively descend into sub-dictionaries.
	size_type size_recursive() const;

	//! Searches for an value in this dictionary associated with the given key. If the given key isn't found, returns end().
	//! Note that recursive searching on sub-dictionaries is not performed.
	iterator find(const key_type &key)
	{
		using namespace std;
		sequenced_index_type& index = sequenced_index();
		return find_if(index.begin(), index.end()
			, boost::bind(key_equal(), key, boost::bind(key_extractor(), _1)));
	}

	//! Searches for an value in this dictionary associated with the given key. If the given key isn't found, returns end().
	//! Note that recursive searching on sub-dictionaries is not performed.
	const_iterator find(const key_type &key) const
	{
		using namespace std;
		const sequenced_index_type& index = sequenced_index();
		return find_if(index.begin(), index.end()
			, boost::bind(key_equal(), key, boost::bind(key_extractor(), _1)));
	}

	//! Inserts the element pointed to by i before position. If position == i, no operation is performed.
	void relocate(iterator position, iterator i)
	{
		sequenced_index().relocate(position, i);
	}

	//! The range of elements [first, last) is repositioned just before position.
	void relocate(iterator position, iterator first, iterator last)
	{
		sequenced_index().relocate(position, first, last);
	}

	//! True iff there are no values in this dictionary.
	bool empty() const
	{
		return storage.empty();
	}

	//! Returns the maximum number of values that may be stored in this dictionary.
	size_type max_size() const
	{
		return storage.max_size();
	}

	//! Erases the value pointer to by the given iterator from this dictionary.
	void erase(iterator pos)
	{
		sequenced_index().erase(pos);
	}

	//! Erases values matching the given key from this dictionary.
	//! Returns 1 if a value was erased, 0 otherwise.
	//! Note that this method does not recursively descend into sub-dictionaries.
	size_type erase(const key_type& key)
	{
		return key_index().erase(key);
	}

	//! Erases a range of values from this dictionary.
	//! Note that this method does not recursively descend into sub-dictionaries.
	void erase(iterator first, iterator last)
	{
		sequenced_index().erase(first, last);
	}

	//! Creates an empty dictionary.
	dict()
	{

	}

	//! Creates a dictionary from the range of given values.
	template<class InputIterator>
	dict(InputIterator first, InputIterator last)
	: storage(first, last)
	{

	}

	//! Creates a copy of the given dictionary.
	dict(const dict& other)
	: storage(other.storage)
	{

	}

	//! Replaces this dictionary with a copy of the given dictionary.
	dict& operator=(dict other)
	{
		swap(other);
		return *this;
	}

	//! Swaps this dictionary with another.
	void swap(dict& other) BOOST_NOEXCEPT
	{
		boost::swap(storage, other.storage);
	}

	//! Returns a sequential iterator to the beginning of the sequence.
	iterator begin()
	{
		return sequenced_index().begin();
	}

	//! Returns a sequential iterator to the beginning of the sequence.
	const_iterator begin() const
	{
		return sequenced_index().begin();
	}

	//! Returns a sequential iterator to one past the end of the sequence.
	iterator end()
	{
		return sequenced_index().end();
	}

	//! Returns a sequential iterator to one past the end of the sequence.
	const_iterator end() const
	{
		return sequenced_index().end();
	}

	//! Returns a reverse sequential iterator to the end of the sequence.
	reverse_iterator rbegin()
	{
		return sequenced_index().rbegin();
	}

	//! Returns a reverse sequential iterator to the end of the sequence.
	const_reverse_iterator rbegin() const
	{
		return sequenced_index().rbegin();
	}

	//! Returns a reverse sequential iterator to one past the beginning of the sequence.
	reverse_iterator rend()
	{
		return sequenced_index().rend();
	}

	//! Returns a reverse sequential iterator to one past the beginning of the sequence.
	const_reverse_iterator rend() const
	{
		return sequenced_index().rend();
	}

	//! Returns a sequential iterator to the beginning of the sequence.
	const_iterator cbegin() const
	{
		return sequenced_index().cbegin();
	}

	//! Returns a sequential iterator to one past the end of the sequence.
	const_iterator cend() const
	{
		return sequenced_index().cend();
	}

	//! Returns a reverse sequential iterator to the end of the sequence.
	const_reverse_iterator crbegin() const
	{
		return sequenced_index().crbegin();
	}

	//! Returns a reverse sequential iterator to one past the beginning of the sequence.
	const_reverse_iterator crend() const
	{
		return sequenced_index().crend();
	}

	//! Returns the hash load factor for this dictionary.
	float load_factor() const
	{
		return key_index().load_factor();
	}

	// Returns the maximum load factor for this dictionary.
	float max_load_factor() const
	{
		return key_index().max_load_factor();
	}

	// Sets the maximum load factor for this dictionary.
	void  max_load_factor(float z)
	{
		key_index().max_load_factor(z);
	}

	//! Rehashes the internal storage structure such that it does not exceed the maximum load factor and uses at least n buckets.
	void rehash(size_type n)
	{
		key_index().rehash(n);
	}

	//! Returns a std::string representation of this dictionary.
	std::string str() const;

	friend bool operator==(const dict& lhs, const dict& rhs);
	friend bool operator<(const dict& lhs, const dict& rhs);

private:
	storage_type storage;
};

template<class T>
struct dict_supports
: boost::mpl::or_<
	boost::mpl::contains<dict::types, T>
	, boost::is_convertible<T, std::string> > // support string literals
{ };

template<class T>
struct dict_implicitly_supports
: boost::mpl::and_<
	boost::mpl::not_<dict_supports<T> >
	, has_convertible<dict::types, T> > // better compile error
{ };

namespace std
{
	//! specializes the std::swap algorithm.
	template<>
	inline void swap(dict& lhs, dict& rhs)
	{
		lhs.swap(rhs);
	}
} // namespace std

namespace details
{
	template<class U, class T>
	U implicit_cast(const T& value)
	{
		return value;
	}

	template<class T>
	class get_visitor : public boost::static_visitor<bool>
	{
	public:
		explicit get_visitor(T& rvalue)
		: rvalue(rvalue)
		{

		}

		template<class U>
		typename boost::enable_if<boost::is_convertible<U, T>, bool>::type
		operator()(const U& value) const
		{
			rvalue = value;
			return true;
		}

		template<class U>
		typename boost::disable_if<boost::is_convertible<U, T>, bool>::type
		operator()(const U& value) const
		{
			return false;
		}

	private:
		T& rvalue;
	};

	template<>
	class get_visitor<std::string> : public boost::static_visitor<bool>
	{
	public:
		explicit get_visitor(std::string& rvalue)
		: rvalue(rvalue)
		{

		}

		template<class U>
		typename boost::disable_if<is_vector<U>, bool>::type
		operator()(const U& value) const
		{
			rvalue += boost::lexical_cast<std::string>(value);
			return true;
		}

		template<class U>
		typename boost::enable_if<is_vector<U>, bool>::type
		operator()(const U& value) const
		{
			rvalue += "[";
			for(typename U::const_iterator i = value.begin(), end = value.end(); i != end; ++i)
			{
				rvalue += boost::lexical_cast<std::string>(*i);
				if(std::distance(i, end) != 1)
					rvalue += ", ";
			}
			rvalue += "]";
			return true;
		}

	private:
		std::string& rvalue;
	};

	class size_visitor : public boost::static_visitor<void>
	{
	public:
		size_visitor(dict::size_type& count) : count(count) { }

		template<class T>
		void operator()(const T& value) const
		{
			++count;
		}

		void operator()(const dict& value) const
		{
			count += 1 + value.size_recursive();
		}

	private:
		dict::size_type& count;
	};
} // namespace details

inline std::ostream& operator<<(std::ostream& o, const dict& d)
{
	o << d.str();
	return o;
}

template<class T>
inline typename boost::enable_if<dict_implicitly_supports<T>, void>::type
dict::add_impl(const dict::key_type& key, const T& value, const bool back)
{
	add_impl(key, details::implicit_cast<typename find_convertible<types, T>::type>(value), back);
}

inline std::string dict::str() const
{
	std::string rvalue = "{";
	for(const_iterator i = this->begin(), end = this->end(); i != end; ++i)
	{
		std::string temp;
		boost::apply_visitor(details::get_visitor<std::string>(temp), i->second);
		rvalue += "'" +  i->first + "': " + temp;
		if(std::distance(i, end) != 1)
			rvalue += ", ";
	}
	rvalue += "}";
	return rvalue;
}

template<class T>
inline bool dict::get(const key_type& key, T& value) const
{
	const key_index_type& index = key_index();
	if(!index.count(key)) return false;
	return boost::apply_visitor(details::get_visitor<T>(value), index.find(key)->second);
}

inline dict dict::get(const key_type& key) const
{
	dict rvalue;
	get(key, rvalue);
	return rvalue;
}

template<class T>
inline std::pair<dict::iterator, bool> dict::insert(const std::pair<key_type, T>& value)
{
	iterator i = find(value.first);
	if(i == end())
		return std::make_pair(i, false);
	const bool rvalue = add(value.first, value.second);
	return std::make_pair(find(value.first), rvalue);
}

template<class T>
inline bool dict::get_recursive(const key_type& key, T& value) const
{
	// todo: can this be done more cleanly?
	std::vector<std::string> keys;
	std::string::size_type pos = key.find("::");
	std::string::size_type offset = 0;
	while(pos != std::string::npos)
	{
		keys.push_back(key.substr(offset, pos));
		offset += pos;
		pos = key.find("::", offset);
		offset += 2;
	}
	if(keys.empty())
		return get(key, value);

	std::vector<std::string>::const_iterator i = keys.begin();
	dict temp;
	if(!get(*i++, temp))
		return false;

	for(std::vector<std::string>::const_iterator end = keys.end() - 1; i != end; ++i)
		if(!temp.get(*i, temp))
			return false;

	return temp.get(keys.back(), value);
}

inline dict::size_type dict::size_recursive() const
{
	size_type count = 0;
	for(storage_type::const_iterator i = storage.begin(), end = storage.end(); i != end; ++i)
		boost::apply_visitor(details::size_visitor(count), i->second);
	return count;
}

inline bool operator==(const dict& lhs, const dict& rhs)
{
	return lhs.storage == rhs.storage;
}

inline bool operator!=(const dict& lhs, const dict& rhs)
{
	return !(lhs == rhs);
}

inline bool operator<(const dict& lhs, const dict& rhs)
{
	return lhs.storage < rhs.storage;
}

inline bool operator<=(const dict& lhs, const dict& rhs)
{
	return lhs < rhs || lhs == rhs;
}

inline bool operator>(const dict& lhs, const dict& rhs)
{
	return !(lhs <= rhs);
}

inline bool operator>=(const dict& lhs, const dict& rhs)
{
	return lhs > rhs || lhs == rhs;
}

#endif // LEXICALUNIT_DICT_H
