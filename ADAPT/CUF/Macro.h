#ifndef CUF_MACRO_H
#define CUF_MACRO_H

//コンマ区切りを含む引数を一つにまとめるためのもの。
//HOGE(TIE(std::pair<double, double>))みたいにすれば、
//テンプレート引数中のコンマを無視してHOGEの第1引数にstd::pair<double, double>全てが押し込められる。
#define CUF_TIE_ARGS(...) __VA_ARGS__

#define CUF_REPEAT_FOR_10(first, x)\
x(0 + first) x(1 + first) x(2 + first) x(3 + first) x(4+ first) \
x(5 + first) x(6 + first) x(7 + first) x(8 + first) x(9 + first)

#define CUF_REPEAT_FOR_20(first, x)\
CUF_REPEAT_FOR_10(first, x) CUF_REPEAT_FOR_10(10 + first, x)

#define CUF_REPEAT_FOR_30(first, x)\
CUF_REPEAT_FOR_10(first, x) CUF_REPEAT_FOR_20(10 + first, x)

#define CUF_REPEAT_FOR_40(first, x)\
CUF_REPEAT_FOR_20(first, x) CUF_REPEAT_FOR_20(20 + first, x)

#define CUF_REPEAT_FOR_50(first, x)\
CUF_REPEAT_FOR_20(first, x) CUF_REPEAT_FOR_30(20 + first, x)

#define CUF_REPEAT_FOR_60(first, x)\
CUF_REPEAT_FOR_30(first, x) CUF_REPEAT_FOR_30(30 + first, x)

#define CUF_REPEAT_FOR_70(first, x)\
CUF_REPEAT_FOR_30(first, x) CUF_REPEAT_FOR_40(30 + first, x)

#define CUF_REPEAT_FOR_80(first, x)\
CUF_REPEAT_FOR_40(first, x) CUF_REPEAT_FOR_40(40 + first, x)

#define CUF_REPEAT_FOR_90(first, x)\
CUF_REPEAT_FOR_40(first, x) CUF_REPEAT_FOR_50(40 + first, x)

#define CUF_REPEAT_FOR_100(first, x)\
CUF_REPEAT_FOR_50(first, x) CUF_REPEAT_FOR_50(50 + first, x)

#define CUF_REPEAT_FOR(NoT, x) CUF_REPEAT_FOR_##NoT(0, x)

#endif
