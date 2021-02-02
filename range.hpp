#include <tuple>

namespace enumerable{
	using namespace std;

	template<typename I> inline auto make_tuple_from_iter(I& i){
		using T = decltype(*declval<I>());
		if constexpr(is_lvalue_reference<T>{}){
			return make_tuple<T&>(*i);
		}else{
			return make_tuple<T>(*i);
		}
	}
	template<typename I1, typename I2> inline auto make_tuple_from_iters(I1& i1, I2& i2){
		return tuple_cat(make_tuple_from_iter(i1), make_tuple_from_iter(i2));
	}
	
	class range{
		class iter{
			int _v, _d;
		public:
			inline iter(int v, int d): _v(v), _d(d){}
			inline operator ++(){ return _v += _d; }
			inline operator int() const{ return _v; }
			inline int operator*() const{ return _v; }
			inline int operator!=(int e) const{ return _d >= 0 ? _v < e : _v > e; }
		};
		const iter _i; const int _e;
	public:
		inline range(int s, int e, int d = 1): _i(s, d), _e(e){}
		inline range(int e): range(0, e){}
		inline iter begin() const{ return _i; }
		inline int end() const{ return _e; }
	};

	template<typename C, typename ...Cs> class zip_type;
	template<typename C> class zip_type<C>;
	template<typename I, typename ...Cs> class zip_iter;
	template<typename I> class zip_iter<I>;
	
	template<typename I> class zip_iter<I>{
	public:
		I _i;
	public:
		zip_iter(I i): _i(i){}
		inline auto operator*(){ return make_tuple_from_iter(_i); }
		inline void operator++(){ ++_i; }
		inline void operator++(int){ operator++(); }
		template<typename E> auto operator!=(const E& end){ return _i != end._i; }
	};
	
	template<typename I, typename ...Cs> class zip_iter: public zip_iter<I>{
	public:
		zip_iter<Cs...> _rest;
	public:
		zip_iter(I i, zip_iter<Cs...> rest): zip_iter<I>(i), _rest(rest){}
		inline auto operator*(){ return tuple_cat(make_tuple_from_iter(this->_i), *_rest); }
		inline void operator++(){ zip_iter<I>::operator++(); ++_rest; }
		inline void operator++(int){ operator++(); }
		template<typename E> auto operator!=(const E& end){ return zip_iter<I>::operator!=(end) && _rest != end._rest; }
	};
	
	template<typename C> class zip_type<C>{
	protected:
		C container;
	public:
		zip_type(C c): container(c){}
		inline auto begin() const{ return zip_iter<decltype(container.begin())>(this->container.begin()); }
		inline auto end() const{ return zip_iter<decltype(container.end())>(this->container.end()); }
	};
	
	template<typename C, typename ...Cs> class zip_type: public zip_type<C>{
	protected:
		zip_type<Cs...> _rest;
	public:
		zip_type(C c, zip_type<Cs...> rest): zip_type<C>(c), _rest(rest){ }
		inline auto begin() const{ return zip_iter(this->container.begin(), _rest.begin()); }
		inline auto end() const{ return zip_iter(this->container.end(), _rest.end()); }
	};

	template<typename C> zip_type<C&> zip(      C&  c);
	template<typename C> zip_type<C>  zip(const C&& c);
	template<typename C, typename ...Cs> zip_type<C&, Cs...> zip(      C&  c, Cs... cs);
	template<typename C, typename ...Cs> zip_type<C,  Cs...> zip(const C&& c, Cs... cs);
	
	template<typename C> zip_type<C&> zip(      C&  c){ return zip_type<C&>(c); }
	template<typename C> zip_type<C>  zip(const C&& c){ return zip_type<C>(move(c)); }
	template<typename C, typename ...Cs> zip_type<C&, Cs...> zip(      C&  c, Cs... cs){ return zip_type<C&, Cs...>(c, zip(forward<Cs>(cs)...)); }
	template<typename C, typename ...Cs> zip_type<C,  Cs...> zip(const C&& c, Cs... cs){ return zip_type<C, Cs...>(move(c), zip(forward<Cs>(cs)...)); }

	template<typename ...Cs> auto enumerate(Cs... cs){ return zip<range, Cs...>(range(numeric_limits<int>::max()), forward<Cs>(cs)...); }
	
	template<typename E1, typename E2> class product_iter{
	protected:
		using I1B = decltype(declval<E1>().begin());
		using I1E = decltype(declval<E1>().end());
		using I2B = decltype(declval<E2>().begin());
		using I2E = decltype(declval<E2>().end());
		I1B _i1i;
		I1E _i1e;
		I2B _i2b;
		I2B _i2i;
		I2E _i2e;
		bool not_ended = true;
	public:
		product_iter(I1B i1b, I1E i1e, I2B i2b, I2E i2e): _i1i(i1b), _i1e(i1e), _i2b(i2b), _i2i(_i2b), _i2e(i2e){}
		void operator++(){
			++_i2i;
			if(!(_i2i != _i2e)){
				_i2i = _i2b;
				++_i1i;
				if(!(_i1i != _i1e)){
					not_ended = false;
				}
			}
		}
		void operator++(int){ operator++(); }
		template<typename T> bool operator!=(const T&){ return not_ended; }
	};

	template<typename E1, typename E2> class product_iter_ee: public product_iter<E1, E2>{
	public:
		auto operator*(){ return make_tuple_from_iters(this->_i1i, this->_i2i); }
		product_iter_ee(typename product_iter<E1, E2>::I1B i1b, typename product_iter<E1, E2>::I1E i1e, typename product_iter<E1, E2>::I2B i2b, typename product_iter<E1, E2>::I2E i2e): product_iter<E1, E2>(i1b, i1e, i2b, i2e){}
	};

	template<typename E1, typename E2> class product_iter_ep: public product_iter<E1, E2>{
	public:
		auto operator*(){ return tuple_cat(make_tuple_from_iter(this->_i1i), *this->_i2i); }
		product_iter_ep(typename product_iter<E1, E2>::I1B i1b, typename product_iter<E1, E2>::I1E i1e, typename product_iter<E1, E2>::I2B i2b, typename product_iter<E1, E2>::I2E i2e): product_iter<E1, E2>(i1b, i1e, i2b, i2e){}
	};

	template<typename E1, typename E2> class product_iter_pe: public product_iter<E1, E2>{
	public:
		auto operator*(){ return tuple_cat(*this->_i1i, make_tuple_from_iter(this->_i2i)); }
		product_iter_pe(typename product_iter<E1, E2>::I1B i1b, typename product_iter<E1, E2>::I1E i1e, typename product_iter<E1, E2>::I2B i2b, typename product_iter<E1, E2>::I2E i2e): product_iter<E1, E2>(i1b, i1e, i2b, i2e){}
	};

	template<typename E1, typename E2> class product_iter_pp: public product_iter<E1, E2>{
	public:
		auto operator*(){ return tuple_cat(*this->_i1i, *this->_i2i); }
		product_iter_pp(typename product_iter<E1, E2>::I1B i1b, typename product_iter<E1, E2>::I1E i1e, typename product_iter<E1, E2>::I2B i2b, typename product_iter<E1, E2>::I2E i2e): product_iter<E1, E2>(i1b, i1e, i2b, i2e){}
	};

	template<typename E1, typename E2, typename I> class product_type{
		E1 _e1;
		E2 _e2;
	public:
		product_type(E1 e1, E2 e2): _e1(e1), _e2(e2){}
		auto begin() const{ return I(_e1.begin(), _e1.end(), _e2.begin(), _e2.end()); }
		int end() const{ return 0; }
	};
	
	template<typename E1, typename E2, typename E3, typename I> auto product(const product_type<E1, E2, I>&& p12,       E3&  e3){ return product_type<product_type<E1, E2, I>, E3&, product_iter_pe<product_type<E1, E2, I>, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I> auto product(const product_type<E1, E2, I>&& p12, const E3&& e3){ return product_type<product_type<E1, E2, I>, E3 , product_iter_pe<product_type<E1, E2, I>, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I> auto product(      E1&  e1, const product_type<E2, E3, I>&& p23){ return product_type<E1&, product_type<E2, E3, I>, product_iter_ep<E1, product_type<E2, E3, I>>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename I> auto product(const E1&& e1, const product_type<E2, E3, I>&& p23){ return product_type<E1 , product_type<E2, E3, I>, product_iter_ep<E1, product_type<E2, E3, I>>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename E4, typename I12, typename I34> auto product(const product_type<E1, E2, I12>&& p12, const product_type<E3, E4, I34>&& p34){ return product_type<product_type<E1, E2, I12>, product_type<E3, E4, I34>, product_iter_pp<product_type<E1, E2, I12>, product_type<E3, E4, I34>>>(p12, p34); }

/*	//the following does not work. I don't know why...
	template<typename E1, typename E2, typename E3, typename I, typename P12 = product_type<E1, E2, I>> auto product(const P12&& p12,       E3&  e3){ return product_type<P12, E3&, product_iter_pe<P12, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I, typename P12 = product_type<E1, E2, I>> auto product(const P12&& p12, const E3&& e3){ return product_type<P12, E3 , product_iter_pe<P12, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I, typename P23 = product_type<E2, E3, I>> auto product(      E1&  e1, const P23&& p23){ return product_type<E1&, P23, product_iter_ep<E1, P23>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename I, typename P23 = product_type<E2, E3, I>> auto product(const E1&& e1, const P23&& p23){ return product_type<E1 , P23, product_iter_ep<E1, P23>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename E4, typename I12, typename I34, typename P12 = product_type<E1, E2, I12>, typename P34 = product_type<E3, E4, I34>> auto product(const P12&& p12, const P34&& p34){ return product_type<P12, P34, product_iter_pp<P12, P34>>(p12, p34); }
*/
	
	template<typename E1, typename E2> auto product(E1&        e1, E2&        e2){ return product_type<E1&, E2&, product_iter_ee<E1, E2>>(     e1,       e2);  }
	template<typename E1, typename E2> auto product(E1&        e1, const E2&& e2){ return product_type<E1&, E2 , product_iter_ee<E1, E2>>(     e1,  move(e2)); }
	template<typename E1, typename E2> auto product(const E1&& e1, E2&        e2){ return product_type<E1 , E2&, product_iter_ee<E1, E2>>(move(e1),      e2);  }
	template<typename E1, typename E2> auto product(const E1&& e1, const E2&& e2){ return product_type<E1 , E2 , product_iter_ee<E1, E2>>(move(e1), move(e2)); }
	
	template<typename E1, typename E2, typename I1B = decltype(declval<E1>().begin()), typename I1E = decltype(declval<E1>().end()), typename I2B = decltype(declval<E2>().begin()), typename I2E = decltype(declval<E2>().end())> auto operator*(E1& e1, E2& e2){ return product(e1, e2); }
	template<typename E1, typename E2, typename I1B = decltype(declval<E1>().begin()), typename I1E = decltype(declval<E1>().end()), typename I2B = decltype(declval<E2>().begin()), typename I2E = decltype(declval<E2>().end())> auto operator*(E1& e1, const E2&& e2){ return product(e1, move(e2)); }
	template<typename E1, typename E2, typename I1B = decltype(declval<E1>().begin()), typename I1E = decltype(declval<E1>().end()), typename I2B = decltype(declval<E2>().begin()), typename I2E = decltype(declval<E2>().end())> auto operator*(const E1&& e1, E2& e2){ return product(move(e1), e2); }
	template<typename E1, typename E2, typename I1B = decltype(declval<E1>().begin()), typename I1E = decltype(declval<E1>().end()), typename I2B = decltype(declval<E2>().begin()), typename I2E = decltype(declval<E2>().end())> auto operator*(const E1&& e1, const E2&& e2){ return product(move(e1), move(e2)); }

	auto md_range(int n){ return range(n); }
	auto md_range(const range&& r){ return r; }
	template<typename ...Rs> auto md_range(int n, Rs... rest){ return range(n) * md_range(forward<Rs>(rest)...); }
	template<typename ...Rs> auto md_range(const range&& r, Rs... rest){ return r * md_range(forward<Rs>(rest)...); }
}

using enumerable::range;
using enumerable::zip;
using enumerable::enumerate;
using enumerable::product;
using enumerable::operator*;
using enumerable::md_range;

