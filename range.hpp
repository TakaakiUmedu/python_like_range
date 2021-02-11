#include <tuple>
#include <functional>

namespace enumerable{
	using namespace std;

	template<typename T = int> class range{
		class iter{
			T _v, _d;
		public:
			inline iter(T v, T d): _v(v), _d(d){}
			inline iter operator ++(){ _v += _d; return *this; }
			inline iter operator ++(T){ const auto v = _v; _v += _d; return iter(v, _d); }
			inline operator T() const{ return _v; }
			inline T operator*() const{ return _v; }
			inline bool operator!=(T e) const{ return _d >= 0 ? _v < e : _v > e; }
		};
		const iter _i; const T _e;
	public:
		inline range(T s, T e, T d = 1): _i(s, d), _e(e){}
		inline range(T e): range(0, e){}
		inline iter begin() const{ return _i; }
		inline T end() const{ return _e; }
	};
	
	void hoge(){
		int a = 10;
		auto x = (tuple<int&>)make_tuple(ref(a));
	}
	
	template<typename E> using iter_b = decltype(declval<E>().begin());
	template<typename E> using iter_e = decltype(declval<E>().end());
	
	class default_type{ private: default_type(); };
	
	template<typename C, typename... R> class tuple_type_builder;
	template<typename C> class tuple_type_builder<C>{
	public:
		using type = tuple<default_type>;
	};
	template<typename C, typename... R> class tuple_type_builder{
	public:
		using type = decltype(tuple_cat(declval<tuple<default_type>>(), declval<typename tuple_type_builder<R...>::type>()));
	};
	template<typename... Cs> using tuple_type_from_containers = typename tuple_type_builder<Cs...>::type;
	
	template<typename V, typename I> static inline auto make_tuple_from_iter(I i){
		using T = decltype(*declval<I>());
		
		static_assert(!(!is_same<V, default_type>::value && is_reference<V>::value && is_lvalue_reference<T>::value && !is_const<V>::value && is_const<T>::value), "cannot make non const reference to const lvalue"); // error 1
		static_assert(!(!is_same<V, default_type>::value && is_reference<V>::value && !is_lvalue_reference<T>::value), "cannot make non reference for rvalue");
		
		if constexpr(is_same<V, default_type>::value){
			using P = typename remove_const<typename remove_reference<T>::type>::type;
			if constexpr(!is_lvalue_reference<T>::value || is_fundamental<P>::value){
				return (tuple<P>)make_tuple(*i);
			}else{
				return (tuple<const P&>)make_tuple(cref(*i));
			}
		}else{
			if constexpr(is_reference<V>::value){
				if constexpr(is_lvalue_reference<T>::value){
					if constexpr(is_const<V>::value){
						return (tuple<V>)make_tuple(cref(*i));
					}else{
						if constexpr(!is_const<T>::value){
							return (tuple<V>)make_tuple(ref(*i));
						}else{
							// error 1
						}
					}
				}else{
					// error 2
				}
			}else{
				return (tuple<V>)make_tuple(*i);
			}
		}
	}
	template<typename I1, typename I2> inline auto make_tuple_from_iters(I1& i1, I2& i2){
		return tuple_cat(make_tuple_from_iter(i1), make_tuple_from_iter(i2));
	}
	
	template<typename V, typename C, typename R = void> class zip_container;
	template<typename V, typename I, typename R = void> class zip_iter;
	
	template<typename V, typename I> class zip_iter<V, I>{
	public:
		I _i;
	public:
		zip_iter(I i): _i(i){}
		inline const auto operator*() const{ return make_tuple_from_iter<V, I>(_i); }
		inline void operator++(){ ++_i; }
		inline void operator++(int){ operator++(); }
		template<typename E> auto operator!=(const E& end) const{ return _i != end._i; }
	};
	
	template<typename V, typename I, typename R> class zip_iter: public zip_iter<V, I>{
	public:
		R _rest;
	public:
		zip_iter(I i, R rest): zip_iter<V, I>(i), _rest(rest){}
		inline const auto operator*() const{ return tuple_cat(make_tuple_from_iter<V, I>(this->_i), *_rest); }
		inline void operator++(){ zip_iter<V,I>::operator++(); ++_rest; }
		inline void operator++(int){ operator++(); }
		template<typename E> auto operator!=(const E& end) const{ return zip_iter<V, I>::operator!=(end) && _rest != end._rest; }
	};
	
	template<typename V, typename C> class zip_container<V, C>{
	public:
		C container;
	public:
		zip_container(C&& c): container(forward<C>(c)){}
		inline auto begin() { return zip_iter<V, decltype(container.begin())>(container.begin()); }
		inline auto end() { return zip_iter<V, decltype(container.end())>(container.end()); }
	};
	
	template<typename V, typename C, typename R> class zip_container: public zip_container<V, C>{
	public:
		R _rest;
	public:
		zip_container(C&& c, R rest): zip_container<V, C>(forward<C>(c)), _rest(rest){ }
		inline auto begin() { return zip_iter<V, decltype(this->container.begin()), decltype(_rest.begin())>(this->container.begin(), _rest.begin()); }
		inline auto end() { return zip_iter<V, decltype(this->container.end()), decltype(_rest.end())>(this->container.end(), _rest.end()); }
	};
	
	template<typename T, int index, typename C, typename... Cs> auto make_zip_container(C&& c, Cs&&... cs){
		using container_type = conditional_t<is_lvalue_reference<C>::value, C&, C>;
		if constexpr(sizeof...(Cs) == 0){
			return zip_container<typename tuple_element<index, T>::type, container_type>(forward<C>(c));
		}else{
			return zip_container<typename tuple_element<index, T>::type, container_type, decltype(make_zip_container<T, index + 1, Cs...>(forward<Cs>(cs)...))>(forward<C>(c), make_zip_container<T, index + 1, Cs...>(forward<Cs>(cs)...));
		}
	}
	
	template<typename T = tuple<default_type>, typename... Cs> auto zip(Cs&&... cs){
		using tuple_type = conditional_t<
			is_same<T, tuple<default_type>>::value,
			tuple_type_from_containers<Cs...>,
			T
		>;
		
		static_assert(tuple_size<tuple_type>::value == sizeof...(Cs), "number of tuple template parameter and arguments are not matched");
		return make_zip_container<tuple_type, 0, Cs...>(forward<Cs>(cs)...);
	}

	template<typename T = tuple<default_type>, typename... Cs> auto enumerate(Cs&&... cs){
		using tuple_type = conditional_t<
			is_same<T, tuple<default_type>>::value,
			tuple_type_from_containers<int, Cs...>,
			decltype(tuple_cat(make_tuple<int>(0), (T)declval<T>()))
		>;
		return zip<tuple_type>(range(numeric_limits<int>::max()), forward<Cs>(cs)...);
	}
	
/*
	enum { EE, EP, PE, PP };
	
	template<int K, typename E1, typename E2> class product_iter{
	protected:
		iter_b<E1> _i1i;
		iter_e<E1> _i1e;
		iter_b<E2> _i2b, _i2i;
		iter_e<E2> _i2e;
		bool not_ended = true;
	public:
		product_iter(iter_b<E1> i1b, iter_e<E1> i1e, iter_b<E2> i2b, iter_e<E2> i2e): _i1i(i1b), _i1e(i1e), _i2b(i2b), _i2i(_i2b), _i2e(i2e){}
		inline void operator++(){
			++_i2i;
			if(!(_i2i != _i2e)){
				_i2i = _i2b;
				++_i1i;
				if(!(_i1i != _i1e)){
					not_ended = false;
				}
			}
		}
		inline void operator++(int){ operator++(); }
		template<typename T> inline bool operator!=(const T&) const{ return not_ended; }
		inline auto operator*() const{
			if constexpr(K == EE) return make_tuple_from_iters(this->_i1i, this->_i2i);
			if constexpr(K == EP) return tuple_cat(make_tuple_from_iter(this->_i1i), *this->_i2i);
			if constexpr(K == PE) return tuple_cat(*this->_i1i, make_tuple_from_iter(this->_i2i));
			if constexpr(K == PP) return tuple_cat(*this->_i1i, *this->_i2i);
		}
	};

	template<typename E1, typename E2, typename I> class product_type{
		E1 _e1;
		E2 _e2;
	public:
		product_type(E1 e1, E2 e2): _e1(e1), _e2(e2){}
		inline auto begin() const{ return I(_e1.begin(), _e1.end(), _e2.begin(), _e2.end()); }
		inline int end() const{ return 0; }
	};
	
	template<typename E1, typename E2, typename E3, typename I> auto product(const product_type<E1, E2, I>&& p12,       E3&  e3){ return product_type<product_type<E1, E2, I>, E3&, product_iter<PE, product_type<E1, E2, I>, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I> auto product(const product_type<E1, E2, I>&& p12, const E3&& e3){ return product_type<product_type<E1, E2, I>, E3 , product_iter<PE, product_type<E1, E2, I>, E3>>(p12, e3); }
	template<typename E1, typename E2, typename E3, typename I> auto product(      E1&  e1, const product_type<E2, E3, I>&& p23){ return product_type<E1&, product_type<E2, E3, I>, product_iter<EP, E1, product_type<E2, E3, I>>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename I> auto product(const E1&& e1, const product_type<E2, E3, I>&& p23){ return product_type<E1 , product_type<E2, E3, I>, product_iter<EP, E1, product_type<E2, E3, I>>>(e1, p23); }
	template<typename E1, typename E2, typename E3, typename E4, typename I12, typename I34> auto product(const product_type<E1, E2, I12>&& p12, const product_type<E3, E4, I34>&& p34){ return product_type<product_type<E1, E2, I12>, product_type<E3, E4, I34>, product_iter<PP, product_type<E1, E2, I12>, product_type<E3, E4, I34>>>(p12, p34); }

	//the following does not work. I don't know why...
//	template<typename E1, typename E2, typename E3, typename I, typename P12 = product_type<E1, E2, I>> auto product(const P12&& p12,       E3&  e3){ return product_type<P12, E3&, product_iter_pe<P12, E3>>(p12, e3); }
//	template<typename E1, typename E2, typename E3, typename I, typename P12 = product_type<E1, E2, I>> auto product(const P12&& p12, const E3&& e3){ return product_type<P12, E3 , product_iter_pe<P12, E3>>(p12, e3); }
//	template<typename E1, typename E2, typename E3, typename I, typename P23 = product_type<E2, E3, I>> auto product(      E1&  e1, const P23&& p23){ return product_type<E1&, P23, product_iter_ep<E1, P23>>(e1, p23); }
//	template<typename E1, typename E2, typename E3, typename I, typename P23 = product_type<E2, E3, I>> auto product(const E1&& e1, const P23&& p23){ return product_type<E1 , P23, product_iter_ep<E1, P23>>(e1, p23); }
//	template<typename E1, typename E2, typename E3, typename E4, typename I12, typename I34, typename P12 = product_type<E1, E2, I12>, typename P34 = product_type<E3, E4, I34>> auto product(const P12&& p12, const P34&& p34){ return product_type<P12, P34, product_iter_pp<P12, P34>>(p12, p34); }

	
	template<typename E1, typename E2> inline auto product(E1&        e1, E2&        e2){ return product_type<E1&, E2&, product_iter<EE, E1, E2>>(     e1,       e2);  }
	template<typename E1, typename E2> inline auto product(E1&        e1, const E2&& e2){ return product_type<E1&, E2 , product_iter<EE, E1, E2>>(     e1,  move(e2)); }
	template<typename E1, typename E2> inline auto product(const E1&& e1, E2&        e2){ return product_type<E1 , E2&, product_iter<EE, E1, E2>>(move(e1),      e2);  }
	template<typename E1, typename E2> inline auto product(const E1&& e1, const E2&& e2){ return product_type<E1 , E2 , product_iter<EE, E1, E2>>(move(e1), move(e2)); }
	
	template<typename E1, typename E2, typename I1B = iter_b<E1>, typename I1E = iter_e<E1>, typename I2B = iter_b<E2>, typename I2E = iter_e<E2>> inline auto operator*(E1& e1, E2& e2){ return product(e1, e2); }
	template<typename E1, typename E2, typename I1B = iter_b<E1>, typename I1E = iter_e<E1>, typename I2B = iter_b<E2>, typename I2E = iter_e<E2>> inline auto operator*(E1& e1, const E2&& e2){ return product(e1, move(e2)); }
	template<typename E1, typename E2, typename I1B = iter_b<E1>, typename I1E = iter_e<E1>, typename I2B = iter_b<E2>, typename I2E = iter_e<E2>> inline auto operator*(const E1&& e1, E2& e2){ return product(move(e1), e2); }
	template<typename E1, typename E2, typename I1B = iter_b<E1>, typename I1E = iter_e<E1>, typename I2B = iter_b<E2>, typename I2E = iter_e<E2>> inline auto operator*(const E1&& e1, const E2&& e2){ return product(move(e1), move(e2)); }
*/
//	inline auto mdrange(int n){ return range(n); }
//	inline auto mdrange(const range&& r){ return r; }
//	template<typename ...Rs> inline auto mdrange(int n, Rs... rest){ return range(n) * mdrange(forward<Rs>(rest)...); }
//	template<typename ...Rs> inline auto mdrange(const range&& r, Rs... rest){ return r * mdrange(forward<Rs>(rest)...); }
}

using enumerable::range;
using enumerable::zip;
using enumerable::enumerate;
//using enumerable::product;
//using enumerable::operator*;
//using enumerable::mdrange;

