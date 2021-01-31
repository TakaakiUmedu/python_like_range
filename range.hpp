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
	inline iter begin(){ return _i; }
	inline int end(){ return _e; }
};
