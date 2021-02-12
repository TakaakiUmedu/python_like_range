#include <tuple>
#include <functional>

namespace iterable{
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
	
	class auto_t{ private: auto_t(); };
	
	namespace{
		template<typename E> using iter_b = decltype(declval<E>().begin());
		template<typename E> using iter_e = decltype(declval<E>().end());
		
		template<typename P, typename I> static inline auto make_tuple_from_iter(I i){
			using A                  = decltype(*declval<I>());
			using A_non_ref          = typename remove_reference<A>::type;
			using A_naked            = typename remove_const<A_non_ref>::type;
			const bool A_const       = is_const<A_non_ref>::value;
			const bool A_ref         = is_lvalue_reference<A>::value;
			const bool A_fundamental = is_fundamental<A_naked>::value;
			
			using P_non_ref          = typename remove_reference<P>::type;
			using P_naked_tmp        = typename remove_const<P_non_ref>::type;
			const bool P_auto        = is_same<P_naked_tmp, auto_t>::value;
			const bool P_const_tmp   = is_const<P_non_ref>::value;
			const bool P_ref_tmp     = is_lvalue_reference<P>::value;
			const bool P_pure_auto   = P_auto && !P_const_tmp && !P_ref_tmp;
			
			using P_naked            = conditional_t<P_auto, A_naked, P_naked_tmp>;
			const bool P_const       = (P_const_tmp || (P_pure_auto && !A_fundamental && A_ref));
			const bool P_ref         = (P_ref_tmp   || (P_pure_auto && !A_fundamental && A_ref));
			
			static_assert(!(P_ref && !P_const && A_const), "cannot make non const reference to const lvalue");
			static_assert(!(P_ref && !A_ref), "cannot make reference for rvalue");
			
			if constexpr(P_ref){
				if constexpr(P_const){
					return (tuple<const P_naked&>)make_tuple(cref(*i));
				}else{
					return (tuple<P_naked&>)make_tuple(ref(*i));
				}
			}else{
				if constexpr(P_const){
					return (tuple<const P_naked>)make_tuple(*i);
				}else{
					return (tuple<P_naked>)make_tuple(*i);
				}
			}
		}
		template<typename... T> inline void dummy(T... x){} // I don't know how to avoid use dummy()
		
		template<typename TV, typename TI> class zip_iter{
			TI _i;
			template<typename TE, size_t N> inline bool comp(TE&& e){
				if constexpr(N < tuple_size_v<TI>){
					return get<N>(_i) != get<N>(e) && comp<TE, N + 1>(forward<TE>(e));
				}else{
					return true;
				}
			}
			template<size_t N> inline auto make_tuple(){
				auto tuple = make_tuple_from_iter<typename tuple_element<N, TV>::type>(get<N>(_i));
				if constexpr(N < tuple_size_v<TI> - 1){
					return tuple_cat(tuple, make_tuple<N + 1>());
				}else{
					return tuple;
				}
			}
		public:
			zip_iter(TI&& i): _i(i){}
			inline auto operator*(){ return make_tuple<0>(); }
			inline void operator++(){ apply([](auto&... i){ dummy(++i...); }, _i); } // I don't know how to remove dummy() call
			inline void operator++(int){ operator++(); }
			
			template<typename TE> inline bool operator!=(TE&& e){
				return comp<TE, 0>(forward<TE>(e));
			}
		
			operator TI(){ return *this; };
		};
		
		template<typename T, typename... Is> auto make_zip_iter(Is&&... is){
			return zip_iter<T, Is...>(forward<Is>(is)...);
		}
		
		template<typename TV, typename TC> class zip_container{
			TC _t;
		public:
			zip_container(TC&& t): _t(forward<TC>(t)){}
			auto begin(){ return apply([](auto&... c){ return make_zip_iter<TV>(make_tuple(c.begin()...)); }, _t); }
			auto end(){ return apply([](auto&... c){ return make_tuple(c.end()...); }, _t); }
			operator TC(){ return *this; };
		};
		
		template<size_t N, typename T, typename... Ts> class tuple_type_builder{              public: using type = typename tuple_type_builder<N - 1, T, T, Ts...>::type; };
		template<          typename T, typename... Ts> class tuple_type_builder<0, T, Ts...>{ public: using type = tuple<Ts...>; };
		
		template<typename T, typename... Cs> using make_tuple_type_from_arguments = conditional_t<
			is_same<typename remove_const<typename remove_reference<T>::type>::type, auto_t>::value,
			typename tuple_type_builder<sizeof...(Cs), T>::type,
			T
		>;	
	}
	template<typename T = auto_t, typename... Cs> auto zip(Cs&&... cs){
		using tuple_type = make_tuple_type_from_arguments<T, Cs...>;
		
		static_assert(tuple_size<tuple_type>::value == sizeof...(Cs), "the numbers of tuple elements of parameter and arguments are not matched");
		return zip_container<tuple_type, tuple<Cs...>>(forward_as_tuple<Cs...>(forward<Cs>(cs)...));
	}

	template<typename T = auto_t, typename... Cs> auto enumerate(Cs&&... cs){
		using tuple_type = decltype(tuple_cat(make_tuple<int>(0), declval<make_tuple_type_from_arguments<T, Cs...>>()));
		
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
/*
using iterable::range;
using iterable::zip;
using iterable::enumerate;
using iterable::auto_t;
*/
//using iterable::product;
//using iterable::operator*;
//using iterable::mdrange;

