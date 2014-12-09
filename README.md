A proof-of-concept for a C++ dict class with Python-like features.

I used Boost.Variant as the value storage which offers a safe, generic, stack-based discriminated union container. Its interface includes both a run-time explicit value retrieval interface and a compile-time value visitation interface. As for the map part of the dict, I used Boost.MultiIndex instead of std::map because it offers the ability to store values using multiple indexes. The two indexes I used were a hashed index, for lookup by key, and a sequenced index, for lookup by insertion order.

### Neat features:
* Stores: float, int, std::string, std::vector&lt;int&gt;, std::vector&lt;float&gt;, std::vector&lt;std::string&gt;, std::vector&lt;bool&gt;, dict, std::vector&lt;dict&gt;.
* Templated add() and get() interface uses auto-magical type deduction.

        dict d;
        d.add("int", 1);
        d.add("string", "test");
        d.add("float", 3.14f);

        int i;
        std::string s;
        float f;
        d.get("int", i);
        d.get("string", s);
        d.get("float", f);

* add() and get() will work "as expected" even if you store a value as one type, but then try to get it as another type. As an extension to this, the add()/get() interface will even work with types not directly supported as long as they can be implicitly converted to or from a supported type.

        dict d;
        d.add("long", 88l); // implicitly converted to int
        d.add("double", 3.14); // implicitly converted to float
        int i;
        d.get("double", i); // implicitly converted from float

* Add to the front or back of the dict via add\_front() and add\_back(), also supports pop\_front() and pop\_back().
* Ability to arbitrarily relocate keys to a different position.
* A complete suite of iterator interface methods and support of the Boost.MultiIndex value visitation interface.
* Values can always be retrieved as a std::string, including vector or dict values (I added this just for fun). The string representation for vectors and dicts borrows from Python's syntax.

And a bunch of other little things. I also have a big todo list of other features that should totally be do-able but I just didn't get around to it. I learned a lot of cool things about Boost writing it. Anyway, enjoy!
