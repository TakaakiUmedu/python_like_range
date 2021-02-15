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
			inline iter operator ++(int){ const auto v = _v; _v += _d; return iter(v, _d); }
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
	
	class auto_t    final{ private: auto_t()    = delete; };
	class flatten_t final{ private: flatten_t() = delete; };
	
	namespace {
		template<typename E> using iter_b = decltype(declval<E>().begin());
		template<typename E> using iter_e = decltype(declval<E>().end());
		
		template<typename P, typename I> inline auto make_tuple_from_iter(I i){
			if constexpr(is_same_v<P, flatten_t>){
				return *i;
			}else{
				using A                  = decltype(*declval<I>());
				using A_non_ref          = remove_reference_t<A>;
				using A_naked            = remove_const_t<A_non_ref>;
				const bool A_const       = is_const_v<A_non_ref>;
				const bool A_ref         = is_lvalue_reference_v<A>;
				const bool A_fundamental = is_fundamental_v<A_naked>;
				
				using P_non_ref          = remove_reference_t<P>;
				using P_naked_tmp        = remove_const_t<P_non_ref>;
				const bool P_auto        = is_same_v<P_naked_tmp, auto_t>;
				const bool P_const_tmp   = is_const_v<P_non_ref>;
				const bool P_ref_tmp     = is_lvalue_reference_v<P>;
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
		}
		
		template<typename V, typename C, typename R = void> class zip_container;
		template<typename V, typename I, typename R = void> class zip_iter;
		
		template<typename V, typename I> class zip_iter<V, I>{
		public:
			I _i;
		public:
			zip_iter(I i): _i(i){}
			inline auto operator*() const{ return make_tuple_from_iter<V, I>(_i); }
			inline void operator++(){ ++_i; }
			inline void operator++(int){ operator++(); }
			template<typename E> inline auto operator!=(const E& end) const{ return _i != end._i; }
		};
		
		template<typename V, typename I, typename R> class zip_iter: public zip_iter<V, I>{
		public:
			R _rest;
		public:
			zip_iter(I i, R rest): zip_iter<V, I>(i), _rest(rest){}
			inline auto operator*() const{ return tuple_cat(make_tuple_from_iter<V, I>(this->_i), *_rest); }
			inline void operator++(){ zip_iter<V,I>::operator++(); ++_rest; }
			inline void operator++(int){ operator++(); }
			template<typename E> inline auto operator!=(const E& end) const{ return zip_iter<V, I>::operator!=(end) && _rest != end._rest; }
		};
		
		template<typename V, typename C> class zip_container<V, C>{
		public:
			C container;
		public:
			inline zip_container(C&& c): container(forward<C>(c)){}
			inline auto begin() { return zip_iter<V, decltype(container.begin())>(container.begin()); }
			inline auto end() { return zip_iter<V, decltype(container.end())>(container.end()); }
		};
		
		template<typename V, typename C, typename R> class zip_container: public zip_container<V, C>{
		public:
			R _rest;
		public:
			inline zip_container(C&& c, R rest): zip_container<V, C>(forward<C>(c)), _rest(rest){ }
			inline auto begin() { return zip_iter<V, decltype(this->container.begin()), decltype(_rest.begin())>(this->container.begin(), _rest.begin()); }
			inline auto end() { return zip_iter<V, decltype(this->container.end()), decltype(_rest.end())>(this->container.end(), _rest.end()); }
		};
		
		template<typename T, int index, typename C, typename... Cs> inline auto make_zip_container(C&& c, Cs&&... cs){
			using container_type = conditional_t<is_lvalue_reference_v<C>, C&, C>;
			if constexpr(sizeof...(Cs) == 0){
				return zip_container<tuple_element_t<index, T>, container_type>(forward<C>(c));
			}else{
				return zip_container<tuple_element_t<index, T>, container_type, decltype(make_zip_container<T, index + 1, Cs...>(forward<Cs>(cs)...))>(forward<C>(c), make_zip_container<T, index + 1, Cs...>(forward<Cs>(cs)...));
			}
		}
		
		template<size_t N, typename T, typename... Ts> class tuple_type_builder             { public: using type = typename tuple_type_builder<N - 1, T, T, Ts...>::type; };
		template<          typename T, typename... Ts> class tuple_type_builder<0, T, Ts...>{ public: using type = tuple<Ts...>; };
		
		template<size_t N, typename T> using make_tuple_type_from_arguments = conditional_t<
			is_same_v<remove_const_t<remove_reference_t<T>>, auto_t>,
			typename tuple_type_builder<N, T>::type,
			T
		>;
		
		template<size_t N, size_t O, typename T1, typename T2> class tuple_element_checker{
			public: static const bool value = tuple_element_checker<N - 1, O, T1, T2>::value && is_same_v<tuple_element_t<N + O, T1>, tuple_element_t<N, T2>>;
		};
		template<size_t O, typename T1, typename T2> class tuple_element_checker<0, O, T1, T2>{
			public: static const bool value = is_same_v<tuple_element_t<O, T1>, tuple_element_t<0, T2>>;
		};
		template<typename T1, typename T2> using tuple_check_head = tuple_element_checker<tuple_size_v<T2> - 1, 0, T1, T2>;
		template<typename T1, typename T2> using tuple_check_tail = tuple_element_checker<tuple_size_v<T2> - 1, tuple_size_v<T1> - tuple_size_v<T2>, T1, T2>;
	}

	template<typename T = auto_t, typename... Cs> inline auto zip(Cs&&... cs){
		using tuple_type = make_tuple_type_from_arguments<sizeof...(Cs), T>;
		
		static_assert(tuple_size_v<tuple_type> == sizeof...(Cs), "the numbers of tuple elements of parameter and arguments are not matched");
		return make_zip_container<tuple_type, 0, Cs...>(forward<Cs>(cs)...);
	}

	template<typename T = auto_t, typename... Cs> inline auto enumerate(Cs&&... cs){
		using tuple_type = decltype(tuple_cat(make_tuple<size_t>(0), declval<make_tuple_type_from_arguments<sizeof...(Cs), T>>()));
		
		return zip<tuple_type>(range<size_t>(numeric_limits<size_t>::max()), forward<Cs>(cs)...);
	}
	
	namespace{
		template<typename V1, typename V2, typename C1, typename C2> class product_iter{
		protected:
			iter_b<C1> _i1i;
			iter_e<C1> _i1e;
			iter_b<C2> _i2b, _i2i;
			iter_e<C2> _i2e;
			bool not_ended = true;
		public:
			inline product_iter(iter_b<C1> i1b, iter_e<C1> i1e, iter_b<C2> i2b, iter_e<C2> i2e): _i1i(i1b), _i1e(i1e), _i2b(i2b), _i2i(_i2b), _i2e(i2e){}
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
			inline auto operator*() const{ return tuple_cat(make_tuple_from_iter<V1>(this->_i1i), make_tuple_from_iter<V2>(this->_i2i)); }
		};
		
		class product_container_base{};
		
		template<typename C1, typename C2, typename I> class product_container: product_container_base{
			C1 _c1;
			C2 _c2;
		public:
			inline product_container(C1 c1, C2 c2): _c1(c1), _c2(c2){}
			inline auto begin() const{ return I(_c1.begin(), _c1.end(), _c2.begin(), _c2.end()); }
			inline int end() const{ return 0; }
		};
		
		template<typename T> class replace_tuple_head{};
		template<typename C1, typename C2, typename... Cs> class replace_tuple_head<tuple<C1, C2, Cs...>>{ public: using type = tuple<flatten_t, Cs...>; };
	}
	
	template<typename T = auto_t, typename C1, typename C2, typename... Cs> inline auto product(C1&& c1, C2&& c2, Cs&&... cs){
		using tuple_type = make_tuple_type_from_arguments<2 + sizeof...(Cs), T>;
		
		static_assert(tuple_size_v<tuple_type> == 2 + sizeof...(Cs), "the number of elements in requested tuple type is not matched with arguments");
		
		const bool c1_is_product = is_base_of_v<product_container_base, C1>;
		const bool c2_is_product = is_base_of_v<product_container_base, C2>;
		
		using type1_tmp = tuple_element_t<0, tuple_type>;
		using type2_tmp = tuple_element_t<1, tuple_type>;
		
		static_assert(!c1_is_product || is_same_v<type1_tmp, auto_t> || is_same_v<type1_tmp, flatten_t>, "type request for argument 1 is invalid, auto_t or flatten_t is required");
		static_assert(!c2_is_product || is_same_v<type2_tmp, auto_t> || is_same_v<type2_tmp, flatten_t>, "type request for argument 2 is invalid, auto_t or flatten_t is required");
		
		using type1 = conditional_t<conjunction_v<is_same<type1_tmp, auto_t>, bool_constant<c1_is_product>>, flatten_t, type1_tmp>;
		using type2 = conditional_t<conjunction_v<is_same<type2_tmp, auto_t>, bool_constant<c2_is_product>>, flatten_t, type2_tmp>;
		
		using container_type1 = conditional_t<is_lvalue_reference_v<C1>, C1&, C1>;
		using container_type2 = conditional_t<is_lvalue_reference_v<C2>, C2&, C2>;
		
		using iter_type = product_iter<type1, type2, container_type1, container_type2>;
		
		auto p = product_container<container_type1, container_type2, iter_type>(forward<C1>(c1), forward<C2>(c2));
		
		if constexpr(sizeof...(Cs) == 0){
			return p;
		}else{
			return product<typename replace_tuple_head<tuple_type>::type>(move(p), forward<Cs>(cs)...);
		}
	}
	
	template<typename C1, typename C2, typename I1B = iter_b<C1>, typename I1E = iter_e<C1>, typename I2B = iter_b<C2>, typename I2E = iter_e<C2>> inline auto operator*(C1&& c1, C2&& c2){ return product(forward<C1>(c1), forward<C2>(c2)); }
	
	template<typename T = size_t> inline auto mdrange(int n){ return range<T>(n); }
	template<typename T>          inline auto mdrange(range<T>&& r){ return r; }
	template<typename T = size_t, typename ...Rs> inline auto mdrange(int n, Rs&&... rest){ return forward<range<T>>(range<T>(n)) * mdrange(forward<Rs>(rest)...); }
	template<typename T, typename ...Rs>          inline auto mdrange(range<T>&& r, Rs&&... rest){ return forward<range<T>>(r) * mdrange(forward<Rs>(rest)...); }
}

